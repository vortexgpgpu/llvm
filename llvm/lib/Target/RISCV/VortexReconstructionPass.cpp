//===-- VortexReconstructionPass.cpp - Vortex CFG reconstruction ---------===//
//
// Part of the Volt compiler (Vortex branch divergence optimizations).
//
// Ported from the Volt LLVM 18 branch (vortexgpgpu/llvm `volt`, "reconstruction
// pass") to LLVM 20 / `vortex_3.x`. Enabled by -vortex-branch-opt >= 9.
//
// The pass builds the control-dependence graph (CDG) of a function from its
// post-dominator tree. A block that is a CDG leaf, is control dependent on two
// or more branches and is not exit-like has its linear block chain cloned per
// additional control-dependence predecessor. Each clone is then reached from a
// single branch, which lets the structurizer and the Vortex split/join
// insertion handle the region without merging unrelated divergent paths.
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include <functional>
#include <map>
#include <set>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "vortex-reconstruction"

namespace {

class VortexReconstruction : public FunctionPass {
public:
  static char ID;
  VortexReconstruction() : FunctionPass(ID) {
    initializeVortexReconstructionPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override {
    return "Vortex Reconstruction Pass";
  }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    // The pass clones blocks and rewires branches: it does not preserve the CFG.
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    FunctionPass::getAnalysisUsage(AU);
  }
};

} // end anonymous namespace

char VortexReconstruction::ID = 0;

INITIALIZE_PASS_BEGIN(VortexReconstruction, DEBUG_TYPE,
                      "Vortex Reconstruction Pass", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_END(VortexReconstruction, DEBUG_TYPE,
                    "Vortex Reconstruction Pass", false, false)

namespace llvm {
FunctionPass *createVortexReconstructionPass() {
  return new VortexReconstruction();
}
} // end namespace llvm

bool VortexReconstruction::runOnFunction(Function &F) {
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  LLVM_DEBUG(dbgs() << "Reconstruction pass: " << F.getName() << "\n";
             F.print(dbgs(), nullptr));

  // Control dependence graph: CDGMap[branch] = successors the branch decides,
  // CDGMapReverse[succ] = branches the successor is control dependent on.
  std::map<BasicBlock *, std::set<BasicBlock *>> CDGMap;
  std::map<BasicBlock *, std::set<BasicBlock *>> CDGMapReverse;

  for (auto &BB : F) {
    auto *Term = BB.getTerminator();
    if (!Term || Term->getNumSuccessors() < 2)
      continue;
    for (unsigned I = 0, E = Term->getNumSuccessors(); I < E; ++I) {
      BasicBlock *Succ = Term->getSuccessor(I);
      for (auto &Y : F) {
        if (&Y == &BB)
          continue;
        if (!PDT.dominates(Succ, &Y) && PDT.dominates(&Y, &BB)) {
          LLVM_DEBUG(dbgs() << "  CD edge: " << BB.getName() << " -> "
                            << Succ->getName() << " (Y: " << Y.getName()
                            << ")\n");
          CDGMap[&BB].insert(Succ);
          CDGMapReverse[Succ].insert(&BB);
        }
      }
    }
  }

  // Loop structure: chains that touch it are not cloned (see below).
  std::set<BasicBlock *> LoopHeaders, LoopPreheaders, LoopExits;
  std::function<void(Loop *)> CollectLoopBlocks = [&](Loop *L) {
    LoopHeaders.insert(L->getHeader());
    if (BasicBlock *PH = L->getLoopPreheader())
      LoopPreheaders.insert(PH);
    SmallVector<std::pair<BasicBlock *, BasicBlock *>, 4> ExitEdges;
    L->getExitEdges(ExitEdges);
    for (auto &Edge : ExitEdges)
      if (!L->contains(Edge.second))
        LoopExits.insert(Edge.second);
    for (Loop *SubLoop : L->getSubLoops())
      CollectLoopBlocks(SubLoop);
  };
  for (Loop *TopLevelLoop : LI)
    CollectLoopBlocks(TopLevelLoop);

  std::function<bool(BasicBlock *, BasicBlock *, std::set<BasicBlock *> &)>
      IsReachableInCFG = [&](BasicBlock *From, BasicBlock *To,
                             std::set<BasicBlock *> &Visited) -> bool {
    if (!From || Visited.count(From))
      return false;
    if (From == To)
      return true;
    Visited.insert(From);
    for (BasicBlock *Succ : successors(From))
      if (IsReachableInCFG(Succ, To, Visited))
        return true;
    return false;
  };

  // Single-entry/single-exit chain starting at B.
  auto CollectLinearChain = [](BasicBlock *B) {
    std::vector<BasicBlock *> Chain;
    while (B) {
      Chain.push_back(B);
      if (succ_size(B) != 1)
        break;
      BasicBlock *Succ = *succ_begin(B);
      if (pred_size(Succ) != 1)
        break;
      B = Succ;
    }
    return Chain;
  };

  // The first block of a clone has Pred as its only predecessor: replace its
  // PHIs by the value incoming from Pred. PHIs without a Pred entry are kept.
  auto ReplaceAndErasePhiFromBlock = [](BasicBlock *BB, BasicBlock *Pred) {
    for (auto It = BB->begin(); It != BB->end();) {
      auto *PN = dyn_cast<PHINode>(&*It++);
      if (!PN)
        break;
      int Idx = PN->getBasicBlockIndex(Pred);
      if (Idx < 0)
        continue;
      PN->replaceAllUsesWith(PN->getIncomingValue(Idx));
      PN->eraseFromParent();
    }
  };

  bool Cloned = false;

  // Clone CDG leaf nodes that have several control-dependence predecessors.
  for (auto &Pair : CDGMapReverse) {
    BasicBlock *Node = Pair.first;
    auto &Preds = Pair.second;

    LLVM_DEBUG(dbgs() << "  node " << Node->getName() << ": " << Preds.size()
                      << " CD preds\n");
    if (Preds.size() < 2)
      continue;
    if (CDGMap.count(Node)) // not a CDG leaf
      continue;
    Instruction *T = Node->getTerminator();
    if (isa<ReturnInst>(T) || isa<UnreachableInst>(T) || isa<ResumeInst>(T))
      continue; // exit-like block

    std::vector<BasicBlock *> Chain = CollectLinearChain(Node);
    if (Chain.empty())
      continue;

    // Keep loops single-entry: a chain containing a loop header, preheader or
    // exit block, or flowing into a loop header, is left alone. Cloning it
    // would add a second entry edge into the loop, which the Vortex
    // split/join insertion does not handle (CuPBoP hotspot hung at level 9).
    // (The Volt LLVM 18 version only reported these blocks.)
    bool TouchesLoop = false;
    for (BasicBlock *BB : Chain)
      if (LoopHeaders.count(BB) || LoopPreheaders.count(BB) || LoopExits.count(BB))
        TouchesLoop = true;
    for (BasicBlock *Succ : successors(Chain.back()))
      if (LoopHeaders.count(Succ))
        TouchesLoop = true;
    if (TouchesLoop) {
      LLVM_DEBUG(dbgs() << "  " << Node->getName()
                        << ": chain touches a loop structure, skipped\n");
      continue;
    }

    std::set<BasicBlock *> ChainSet(Chain.begin(), Chain.end());
    BasicBlock *FirstPred = nullptr;
    for (BasicBlock *Pred : Preds) {
      if (!FirstPred) {
        FirstPred = Pred; // the original chain stays with the first predecessor
        continue;
      }
      std::set<BasicBlock *> Visited;
      if (!IsReachableInCFG(Pred, Node, Visited))
        continue;

      Cloned = true;
      ValueToValueMapTy VMap;
      BasicBlock *PrevClone = nullptr;
      std::vector<BasicBlock *> ClonedBlocks;

      for (BasicBlock *BB : Chain) {
        BasicBlock *Clone = CloneBasicBlock(BB, VMap, ".clone", &F);
        VMap[BB] = Clone;
        ClonedBlocks.push_back(Clone);
        if (PrevClone) {
          Instruction *TI = PrevClone->getTerminator();
          for (unsigned I = 0, E = TI->getNumSuccessors(); I < E; ++I)
            if (TI->getSuccessor(I) == BB)
              TI->setSuccessor(I, Clone);
        }
        if (BB == Chain.front())
          ReplaceAndErasePhiFromBlock(Clone, Pred);
        PrevClone = Clone;
      }

      BasicBlock *FirstClone = cast<BasicBlock>(VMap[Chain.front()]);
      Instruction *TI = Pred->getTerminator();
      for (unsigned I = 0, E = TI->getNumSuccessors(); I < E; ++I)
        if (TI->getSuccessor(I) == Chain.front())
          TI->setSuccessor(I, FirstClone);
      // Pred no longer reaches the original chain: drop only its PHI entries.
      // The original block keeps every other predecessor (FirstPred and the
      // non-control-dependent ones), so its PHIs must stay. (The Volt LLVM 18
      // version replaced them by FirstPred's value, which is wrong as soon as
      // another predecessor remains, e.g. srad/heartwall at -vortex-branch-opt=9.)
      for (PHINode &PN : Chain.front()->phis())
        while (PN.getBasicBlockIndex(Pred) >= 0)
          PN.removeIncomingValue(Pred, /*DeletePHIIfEmpty=*/false);

      for (BasicBlock *ClonedBB : ClonedBlocks)
        for (Instruction &I : *ClonedBB)
          RemapInstruction(&I, VMap, RF_IgnoreMissingLocals);

      // The tail clone is a new predecessor of the chain's successors.
      BasicBlock *OrigTail = Chain.back();
      for (BasicBlock *Succ : successors(OrigTail)) {
        for (PHINode &PN : Succ->phis()) {
          int Idx = PN.getBasicBlockIndex(OrigTail);
          if (Idx < 0)
            continue;
          Value *Incoming = PN.getIncomingValue(Idx);
          if (VMap.count(Incoming))
            Incoming = VMap[Incoming];
          PN.addIncoming(Incoming, PrevClone);
        }
      }
      // (after the PHI update above, so PHIs created by SSAUpdater are complete)
      // Values defined in the chain now have a second definition in the clone:
      // rewrite their uses outside the chain with SSAUpdater (PHIs where needed).
      for (BasicBlock *BB : Chain) {
        for (Instruction &Inst : *BB) {
          auto It = VMap.find(&Inst);
          if (It == VMap.end() || Inst.use_empty())
            continue;
          auto *CloneInst = cast<Instruction>(It->second);
          SmallVector<Use *, 8> OutsideUses;
          for (Use &U : Inst.uses()) {
            auto *UI = cast<Instruction>(U.getUser());
            BasicBlock *UseBB = UI->getParent();
            if (auto *PN = dyn_cast<PHINode>(UI))
              UseBB = PN->getIncomingBlock(U);
            if (!ChainSet.count(UseBB))
              OutsideUses.push_back(&U);
          }
          if (OutsideUses.empty())
            continue;
          SSAUpdater SSA;
          SSA.Initialize(Inst.getType(), Inst.getName());
          SSA.AddAvailableValue(BB, &Inst);
          SSA.AddAvailableValue(CloneInst->getParent(), CloneInst);
          for (Use *U : OutsideUses)
            SSA.RewriteUse(*U);
        }
      }

    }

    // Simplify the original first block's PHIs that are left with one entry.
    for (auto It = Chain.front()->begin(); It != Chain.front()->end();) {
      auto *PN = dyn_cast<PHINode>(&*It++);
      if (!PN)
        break;
      if (PN->getNumIncomingValues() == 1) {
        PN->replaceAllUsesWith(PN->getIncomingValue(0));
        PN->eraseFromParent();
      }
    }
  }

  LLVM_DEBUG(dbgs() << "Reconstruction pass: cloned=" << Cloned << "\n";
             F.print(dbgs(), nullptr));
  return Cloned;
}
