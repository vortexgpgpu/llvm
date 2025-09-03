#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include <map>
#include <set>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "vortex-reconstruction"

//#define NDEBUG
#ifndef NDEBUG
#define LLVM_DEBUG(x)                                                          \
  do {                                                                         \
    x;                                                                         \
  } while (false)
#endif

class VortexReconstruction : public FunctionPass {
public:
  static char ID;
  VortexReconstruction();

  StringRef getPassName() const override;

  BasicBlock *cloneBlockFromNode(BasicBlock *A);
  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG(); // Some modifications, but preserves basic structure
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    FunctionPass::getAnalysisUsage(AU);
  }
};

namespace llvm {
void initializeVortexReconstructionPass(PassRegistry &);
FunctionPass *createVortexReconstructionPass() {
  return new VortexReconstruction();
}
} // End namespace llvm

INITIALIZE_PASS(VortexReconstruction, DEBUG_TYPE,
                "Fix function bitcasts for AMDGPU", false, false)

char VortexReconstruction::ID = 0;

StringRef VortexReconstruction::getPassName() const {
  return "Vortex Reconstruction Pass";
}

VortexReconstruction::VortexReconstruction() : FunctionPass(ID) {
  initializeVortexReconstructionPass(*PassRegistry::getPassRegistry());
}

BasicBlock *VortexReconstruction::cloneBlockFromNode(BasicBlock *A) {
  ValueToValueMapTy VMap;
  Twine Suffix("_cloned");
  Function *ParentFunc = A->getParent();
  BasicBlock *B = CloneBasicBlock(A, VMap, Suffix, ParentFunc);
  for (Instruction &I : *B) {
    RemapInstruction(&I, VMap, RF_IgnoreMissingLocals);
  }
  return B;
}

bool VortexReconstruction::runOnFunction(Function &F) {

  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  LLVM_DEBUG(dbgs() << "Reconstruction pass \n");
  LLVM_DEBUG(dbgs() << "Function name: " << F.getName() << "\n");
  //LLVM_DEBUG(dbgs() << "Function IR:\n");
  //LLVM_DEBUG(F.print(dbgs(), nullptr));

  //  Build Control Dependency Graph (CDGMap)
  std::map<BasicBlock *, std::set<BasicBlock *>> CDGMap;
  std::map<BasicBlock *, std::set<BasicBlock *>> CDGMap_reverse;

  // Generate CDG
  for (auto &BB : F) {
    auto *Term = BB.getTerminator();
    if (!Term || Term->getNumSuccessors() < 2)
      continue;

    auto BB_name = std::string(BB.getName().data());
    LLVM_DEBUG(dbgs() << "ReconstructionPass: start: " << BB_name << "\n");

    for (unsigned i = 0; i < Term->getNumSuccessors(); ++i) {
      BasicBlock *Succ = Term->getSuccessor(i);
      for (auto &Y : F) {
        if (&Y == &BB)
          continue;

        if (!PDT.dominates(Succ, &Y) && PDT.dominates(&Y, &BB)) {
          auto Succ_name = std::string(Succ->getName().data());
          auto Y_name = std::string(Y.getName().data());

          LLVM_DEBUG(dbgs()
                     << "ReconstructionPass: target BB: " << BB_name
                     << ", Succ: " << Succ_name << ", Y: " << Y_name << "\n");

          CDGMap[&BB].insert(Succ);
          CDGMap_reverse[Succ].insert(&BB);
        }
      }
    }
  }

  // Collect Loop Header Information
  std::set<BasicBlock *> LoopHeaders;
  std::set<BasicBlock *> LoopPreheaders;
  std::set<BasicBlock *> LoopExits;

  std::function<void(Loop *)> collectLoopBlocks = [&](Loop *L) {
    LoopHeaders.insert(L->getHeader());
    if (BasicBlock *PH = L->getLoopPreheader())
      LoopPreheaders.insert(PH);

    SmallVector<std::pair<BasicBlock *, BasicBlock *>, 4> ExitEdges;
    L->getExitEdges(ExitEdges);
    for (auto &Edge : ExitEdges) {
      BasicBlock *To = Edge.second;
      if (!L->contains(To))
        LoopExits.insert(To);
    }

    for (Loop *SubLoop : L->getSubLoops()) {
      collectLoopBlocks(SubLoop);
    }
  };

  for (Loop *TopLevelLoop : LI) {
    collectLoopBlocks(TopLevelLoop);
  }

  // Lambda Functions
  std::function<bool(BasicBlock *, BasicBlock *, std::set<BasicBlock *> &)>
      isReachableInCFG;
  isReachableInCFG = [&](BasicBlock *From, BasicBlock *To,
                         std::set<BasicBlock *> &Visited) -> bool {
    if (!From || Visited.count(From))
      return false;
    if (From == To)
      return true;
    Visited.insert(From);
    for (succ_iterator SI = succ_begin(From), SE = succ_end(From); SI != SE;
         ++SI) {
      if (isReachableInCFG(*SI, To, Visited))
        return true;
    }
    return false;
  };

  auto collectLinearChain =
      [](llvm::BasicBlock *B) -> std::vector<llvm::BasicBlock *> {
    std::vector<llvm::BasicBlock *> chain;
    while (B) {
      auto B_name = std::string(B->getName().data());
      LLVM_DEBUG(dbgs() << "collectLinearChain " << B_name << "\n");

      chain.push_back(B);
      if (llvm::succ_size(B) != 1)
        break;
      llvm::BasicBlock *Succ = *llvm::succ_begin(B);
      if (llvm::pred_size(Succ) != 1)
        break;
      B = Succ;
    }
    return chain;
  };

  auto replaceAndErasePhiFromBlock = [](llvm::BasicBlock *BB,
                                        llvm::BasicBlock *Pred) {
    for (auto It = BB->begin(); isa<PHINode>(It);) {
      PHINode *PN = cast<PHINode>(&*It++);
      Value *Incoming = PN->getIncomingValueForBlock(Pred);

      if (!Incoming)
        continue;

      PN->replaceAllUsesWith(Incoming);
      PN->eraseFromParent();
    }
  };

  LLVM_DEBUG(dbgs() << "ReconstructionPass: Check target node \n" << CDGMap_reverse.size()
                    << " nodes have CD preds\n");
  bool cloned = false;

  // Clone leaf nodes with multiple CD preds
  for (auto &pair : CDGMap_reverse) {
    BasicBlock *Node = pair.first;
    auto &Preds = pair.second;
        
    auto Node_name = std::string(Node->getName().data()); 

    LLVM_DEBUG(dbgs() << "ReconstructionPass: target Node: " << Node_name
                      << "\n");
    for( auto Pred : Preds ){
      auto Pred_name = std::string(Pred->getName().data());
      LLVM_DEBUG(dbgs() << "  CDG Pred: " << Pred_name << "\n");
    }

    if (Preds.size() < 2){
      LLVM_DEBUG(dbgs() << "ReconstructionPass: skip: less than 2 preds\n");
      continue;
    }

    // Check if Node is a CDG leaf
    if (CDGMap.find(Node) != CDGMap.end()) {
      LLVM_DEBUG(dbgs() << "ReconstructionPass: not leaf node\n");
      continue;
    }

    // Check if Node is Loop Header or Preheader
    if (LoopHeaders.find(Node) != LoopHeaders.end() ||
        LoopPreheaders.find(Node) != LoopPreheaders.end() ||
        LoopExits.find(Node) != LoopExits.end()) {
      LLVM_DEBUG(dbgs() << Node_name << " is a loop structures\n");
    }

    // BB is an exit-like block
    Instruction *T = Node->getTerminator();
    if (isa<ReturnInst>(T) || isa<UnreachableInst>(T) || isa<ResumeInst>(T)) {
      LLVM_DEBUG(dbgs() << "ReconstructionPass: exit-like block?\n");
      continue;
    }

    // For each valid predecessor, check CFG reachability
    int i = 0;
    auto chains = collectLinearChain(Node);
    if (chains.empty()) {
      LLVM_DEBUG(dbgs() << "ReconstructionPass: chain is empty\n");
      continue;
    }
    BasicBlock *firstPred;

    for (auto Pred : Preds) {
      auto Pred_name = std::string(Pred->getName().data());
      LLVM_DEBUG(dbgs() << "ReconstructionPass: Pred Node[" << i << "] "
                        << Pred_name << "\n");
      if (i++ == 0) {
        firstPred = Pred;
        continue;
      }

      std::set<BasicBlock *> Visited;
      if (!isReachableInCFG(Pred, Node, Visited)) {
        continue;
      }

      cloned = true;

      llvm::ValueToValueMapTy VMap;
      BasicBlock *PrevClone = nullptr;
      std::vector<BasicBlock *> ClonedBlocks;

      for (BasicBlock *BB : chains) {
        BasicBlock *Cloned = CloneBasicBlock(BB, VMap, ".clone", &F);
        VMap[BB] = Cloned;
        ClonedBlocks.push_back(Cloned);

        if (PrevClone) {
          Instruction *TI = PrevClone->getTerminator();
          for (unsigned i = 0; i < TI->getNumSuccessors(); ++i) {
            if (TI->getSuccessor(i) == BB) {
              TI->setSuccessor(i, Cloned);
            }
          }
        }
        replaceAndErasePhiFromBlock(Cloned, Pred);
        PrevClone = Cloned;
      }

      BasicBlock *FirstBB = chains.front();
      BasicBlock *FirstClone = cast<BasicBlock>(VMap[FirstBB]);
      Instruction *TI = Pred->getTerminator();
      for (unsigned i = 0; i < TI->getNumSuccessors(); ++i) {
        if (TI->getSuccessor(i) == FirstBB) {
          TI->setSuccessor(i, FirstClone);
        }
      }

      for (BasicBlock *ClonedBB : ClonedBlocks) {
        for (Instruction &I : *ClonedBB) {
          RemapInstruction(&I, VMap, RF_IgnoreMissingLocals);
        }
      }

      auto OrigTail = chains.back();
      for (BasicBlock *Succ : successors(OrigTail)) {
        for (Instruction &I : *Succ) {
          if (auto *PN = dyn_cast<PHINode>(&I)) {
            int Index = PN->getBasicBlockIndex(OrigTail);
            if (Index < 0)
              continue;

            Value *Incoming = PN->getIncomingValue(Index);
            if (VMap.count(Incoming)) {
              Incoming = VMap[Incoming];
            }

            PN->addIncoming(Incoming, PrevClone);
          }
        }
      }
    }

    for (auto BB : chains) {
      replaceAndErasePhiFromBlock(BB, firstPred);
    }
  }

  LLVM_DEBUG(dbgs() << "after Reconstruction pass \n");
  dbgs() << "ReconstructionPass: is cloned ? " << cloned << "\n";
  LLVM_DEBUG(dbgs() << "Function name: " << F.getName() << "\n");
  //LLVM_DEBUG(dbgs() << "Function IR:\n");
  //LLVM_DEBUG(F.print(dbgs(), nullptr));

  return cloned;
}
