#include "VortexBranchDivergence.h"

#include "llvm/Support/Debug.h"
#include "RISCV.h"
#include "RISCVSubtarget.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/TargetPassConfig.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicsRISCV.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"        // for PointerType

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

#include "llvm/Analysis/UniformityAnalysis.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ValueTracking.h" // for GetPointerBaseWithConstantOffset

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#include "llvm/ADT/SCCIterator.h"

#include "llvm/IR/LegacyPassManager.h"

#include <iostream>

using namespace vortex;
using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "vortex-branch-divergence"

namespace vortex {

class NamePrinter {
private:
  std::unique_ptr<ModuleSlotTracker> MST_;

public:
  void init(Function* function) {
    auto module = function->getParent();
    MST_ = std::make_unique<ModuleSlotTracker>(module);
    MST_->incorporateFunction(*function);
  }

  std::string ValueName(llvm::Value* V) {
    std::string str("V.");
    if (V->hasName()) {
      str += std::string(V->getName().data(), V->getName().size());
    } else {
      auto slot = MST_->getLocalSlot(V);
      str += std::to_string(slot);
    }
    return str;
  }

  std::string BBName(llvm::BasicBlock* BB) {
    std::string str("BB.");
    if (BB->hasName()) {
      str += std::string(BB->getName().data(), BB->getName().size());
    } else {
      auto slot = MST_->getLocalSlot(&BB->front());
      if (slot > 0) {
        str += std::to_string(slot - 1);
      } else {
        str = "";
      }
    }
    return str;
  }
};

static void FindSuccessor(DenseSet<BasicBlock *>& visited,
                          BasicBlock* current,
                          BasicBlock* target,
                          std::vector<BasicBlock*>& out) {
  visited.insert(current);
  auto branch = dyn_cast<BranchInst>(current->getTerminator());
  if (!branch)
    return;
  for (auto succ : branch->successors()) {
    if (succ == target) {
      out.push_back(current);
    } else {
      if (visited.count(succ) == 0) {
        FindSuccessor(visited, succ, target, out);
      }
    }
  }
}

static void FindSuccessor(BasicBlock* start, BasicBlock* target, std::vector<BasicBlock*>& out) {
  DenseSet<BasicBlock *> visited;
  FindSuccessor(visited, start, target, out);
}

class ReplaceSuccessor {
private:
  DenseMap<std::pair<PHINode*, BasicBlock*>, PHINode*> phi_table_;
  NamePrinter namePrinter_;

public:

  void init(Function* function) {
    namePrinter_.init(function);
    phi_table_.clear();
  }

  bool replaceSuccessor(BasicBlock* BB, BasicBlock* oldSucc, BasicBlock* newSucc) {
    auto branch = dyn_cast<BranchInst>(BB->getTerminator());
    if (branch) {
      for (unsigned i = 0, n = branch->getNumSuccessors(); i < n; ++i) {
        auto succ = branch->getSuccessor(i);
        if (succ == oldSucc) {
          LLVM_DEBUG(dbgs() << "****** replace " << namePrinter_.BBName(BB) << ".succ[" << i << "]: " << namePrinter_.BBName(oldSucc) << " with " << namePrinter_.BBName(newSucc) << "\n");
          branch->setSuccessor(i, newSucc);
          this->replacePhiDefs(oldSucc, BB, newSucc);
          return true;
        }
      }
    }
    return false;
  }

  void replacePhiDefs(BasicBlock* block, BasicBlock* oldPred, BasicBlock* newPred) {
    // process all phi nodes in old successor
    for (auto II = block->begin(), IE = block->end(); II != IE; ++II) {
      PHINode *phi = dyn_cast<PHINode>(II);
      if (!phi)
        continue;

      for (unsigned op = 0, nOps = phi->getNumOperands(); op != nOps; ++op) {
        if (phi->getIncomingBlock(op) != oldPred)
          continue;

        PHINode* phi_stub;
        auto key = std::make_pair(phi, newPred);
        auto entry = phi_table_.find(key);
        if (entry != phi_table_.end()) {
          phi_stub = entry->second;
        } else {
          // create corresponding Phi node in new block
          phi_stub = PHINode::Create(phi->getType(), 1, phi->getName(), &newPred->front());
          phi_table_[key] = phi_stub;

          // add new phi to succesor's phi node
          phi->addIncoming(phi_stub, newPred);
        }

        // move phi's operand into new phi node
        Value *del_value = phi->removeIncomingValue(op);
        phi_stub->addIncoming(del_value, oldPred);
      }
    }
   }
};

static void InsertBasicBlock(const std::vector<BasicBlock*> BBs, BasicBlock* succBB, BasicBlock* newBB) {
  DenseMap<std::pair<PHINode*, BasicBlock*>, PHINode*> phi_table;
  for (auto BB : BBs) {
    auto TI = BB->getTerminator();
    TI->replaceSuccessorWith(succBB, newBB);
    for (auto& I : *succBB) {
      auto phi = dyn_cast<PHINode>(&I);
      if (!phi)
        continue;
      for (unsigned op = 0, n = phi->getNumOperands(); op != n; ++op) {
        if (phi->getIncomingBlock(op) != BB)
          continue;
        PHINode* phi_stub;
        auto key = std::make_pair(phi, newBB);
        auto entry = phi_table.find(key);
        if (entry != phi_table.end()) {
          phi_stub = entry->second;
        } else {
          // create corresponding Phi node in new block
          phi_stub = PHINode::Create(phi->getType(), 1, phi->getName(), &newBB->front());
          phi_table[key] = phi_stub;
          // add new phi to succesor's phi node
          phi->addIncoming(phi_stub, newBB);
        }
        // move phi's operand into new phi node
        auto value = phi->removeIncomingValue(op);
        phi_stub->addIncoming(value, BB);
      }
    }
  }
}

static BasicBlock* SplitBasicBlockBefore(BasicBlock* BB, BasicBlock::iterator I, const Twine &BBName) {
  assert(BB->getTerminator() &&
         "Can't use splitBasicBlockBefore on degenerate BB!");
  assert(I != BB->end() &&
         "Trying to get me to create degenerate basic block!");

  assert((!isa<PHINode>(*I) || BB->getSinglePredecessor()) &&
         "cannot split on multi incoming phis");

  auto New = BasicBlock::Create(BB->getContext(), BBName, BB->getParent(), BB);
  // Save DebugLoc of split point before invalidating iterator.
  auto Loc = I->getDebugLoc();
  // Move all of the specified instructions from the original basic block into
  // the new basic block.
  New->splice(New->end(), BB, I);

  // Loop through all of the predecessors of the 'this' block (which will be the
  // predecessors of the New block), replace the specified successor 'this'
  // block to point at the New block and update any PHI nodes in 'this' block.
  // If there were PHI nodes in 'this' block, the PHI nodes are updated
  // to reflect that the incoming branches will be from the New block and not
  // from predecessors of the 'this' block.
  SmallVector<BasicBlock *, 32> preds(predecessors(BB));
  for (auto Pred : preds) {
    auto TI = Pred->getTerminator();
    TI->replaceSuccessorWith(BB, New);
    BB->replacePhiUsesWith(Pred, New);
  }
  // Add a branch instruction from  "New" to "this" Block.
  auto BI = BranchInst::Create(BB, New);
  BI->setDebugLoc(Loc);

  return New;
}

static bool isUniformlyReached(BasicBlock &BB, UniformityInfo &UI) {
  SmallVector<BasicBlock*,8> Worklist(predecessors(&BB));
  SmallPtrSet<BasicBlock*,8> Visited;
  while (!Worklist.empty()) {
    auto CBB = Worklist.pop_back_val();
    if (UI.isDivergent(CBB->getTerminator()))
      return false;
    for (auto PBB : predecessors(CBB)) {
      if (Visited.insert(PBB).second) {
        Worklist.push_back(PBB);
      }
    }
  }
  return true;
}

static uint32_t findArgAlloca(Function &F, Argument &Arg, SmallPtrSet<Value*, 8> &Allocas) {
  uint32_t count = 0;
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto SI = dyn_cast<StoreInst>(&I)) {
        auto SIV = SI->getValueOperand();
        if (SIV == &Arg) {
          auto PO = SI->getPointerOperand();
          auto AI = llvm::findAllocaForValue(PO);
          if (AI) {
            count = Allocas.insert(AI).second ? (count + 1) : count;
          }
        }
      }
    }
  }
  return count;
}

static bool isDerivedFrom(Value *V, Value *Base, LoopInfo &LI) {
  SmallVector<const Value*, 8> Bases;
  llvm::getUnderlyingObjects(V, Bases, &LI, 0);
  return llvm::is_contained(Bases, Base);
}

static void addPassAndDeps(legacy::FunctionPassManager &FPM,
                           AnalysisID PassID,
                           SmallPtrSetImpl<AnalysisID> &alreadyAdded,
                           Pass *P = nullptr) {
  // Mark this pass as added first to prevent infinite recursion
  if (!alreadyAdded.insert(PassID).second) {
    if (P != nullptr) {
      LLVM_DEBUG(dbgs() << "VX: addPassAndDeps(): " << P->getPassName() << " (ID=" << PassID << ") already added!\n");
      std::abort();
    }
    return;
  }

  if (P == nullptr) {
    if (auto PI = PassRegistry::getPassRegistry()->getPassInfo(PassID)) {
      P = PI->createPass();
    } else {
      LLVM_DEBUG(dbgs() << "VX: addPassAndDeps(): pass not found: " << PassID << "!\n");
      std::abort();
    }
  }

  AnalysisUsage AU;
  P->getAnalysisUsage(AU);

  for (auto ReqID : AU.getRequiredSet()) {
    addPassAndDeps(FPM, ReqID, alreadyAdded);
  }

  for (auto ReqID : AU.getRequiredTransitiveSet()) {
    addPassAndDeps(FPM, ReqID, alreadyAdded);
  }

  //LLVM_DEBUG(dbgs() << "*** VX: addPassAndDeps(): added " << P->getPassName() << " (ID=" << PassID << ").\n");
  FPM.add(P);
}

static void addPassAndDeps(legacy::FunctionPassManager &FPM,
                           Pass *P,
                           SmallPtrSetImpl<AnalysisID> &alreadyAdded) {
  addPassAndDeps(FPM, P->getPassID(), alreadyAdded, P);
}

///////////////////////////////////////////////////////////////////////////////

struct VortexDivergenceAnalysis0 : public ModulePass {
public:

  static char ID;

  VortexDivergenceAnalysis0();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnModule(Module &M) override;
};

///////////////////////////////////////////////////////////////////////////////

struct VortexDivergenceAnalysis1 : public ModulePass {
public:

  static char ID;

  VortexDivergenceAnalysis1();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnModule(Module &M) override;
};

///////////////////////////////////////////////////////////////////////////////

struct VortexDivergenceArguments : public FunctionPass {
public:

  static char ID;

  VortexDivergenceArguments();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;

private:

  bool markLoadsAfterCall(CallBase *CB, AllocaInst *AI, DominatorTree &DT, dv_info_t *dv_info);

  bool isDivergentOutputPointer(Function &F, Argument &PtrArg,
    AliasAnalysis &AA, UniformityInfo &UA, LoopInfo &LI, TargetLibraryInfo &TLI);
};

///////////////////////////////////////////////////////////////////////////////

struct VortexBranchDivergence0 : public FunctionPass {
public:

  static char ID;

  VortexBranchDivergence0();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;
};

///////////////////////////////////////////////////////////////////////////////

class VortexBranchDivergence1 : public FunctionPass {
public:

  static char ID;

  VortexBranchDivergence1(int divergenceMode = 0);

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;

private:

  void initialize(Function &M);

  void processBranches(LLVMContext* context, Function* function, PostDominatorTree &PDT);

  void processLoops(LLVMContext* context, Function* function);

  using StackEntry = std::pair<BasicBlock *, Value *>;
  using StackVector = SmallVector<StackEntry, 16>;

  int divergenceMode_;

  ReplaceSuccessor replaceSuccessor_;
  NamePrinter namePrinter_;

  std::vector<BasicBlock*> div_blocks_;
  DenseSet<BasicBlock*> div_blocks_set_;

  std::vector<Loop*> loops_;
  DenseSet<Loop*> loops_set_;

  Type* SizeTTy_;

  Function *tmask_func_;
  Function *pred_func_;
  Function *pred_n_func_;
  Function *tmc_func_;
  Function *split_func_;
  Function *split_n_func_;
  Function *join_func_;
  Function *mov_func_;
};

///////////////////////////////////////////////////////////////////////////////

struct VortexBranchDivergence2 : public MachineFunctionPass {
public:
  static char ID;
  VortexBranchDivergence2(int PassMode);

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override;

private:
  int PassMode_;
};

}

///////////////////////////////////////////////////////////////////////////////

namespace llvm {

void initializeVortexDivergenceAnalysis0Pass(PassRegistry &);
void initializeVortexDivergenceAnalysis1Pass(PassRegistry &);
void initializeVortexDivergenceArgumentsPass(PassRegistry &);
void initializeVortexBranchDivergence0Pass(PassRegistry &);
void initializeVortexBranchDivergence1Pass(PassRegistry &);
void initializeVortexBranchDivergence2Pass(PassRegistry &);

ModulePass *createVortexDivergenceAnalysis0Pass() {
  return new VortexDivergenceAnalysis0();
}

ModulePass *createVortexDivergenceAnalysis1Pass() {
  return new VortexDivergenceAnalysis1();
}

FunctionPass *createVortexDivergenceArgumentsPass() {
  return new VortexDivergenceArguments();
}

FunctionPass *createVortexBranchDivergence0Pass() {
  return new VortexBranchDivergence0();
}

FunctionPass *createVortexBranchDivergence1Pass(int divergenceMode) {
  return new VortexBranchDivergence1(divergenceMode);
}

FunctionPass *createVortexBranchDivergence2Pass(int PassMode) {
  return new VortexBranchDivergence2(PassMode);
}

}

INITIALIZE_PASS_BEGIN(VortexDivergenceAnalysis0, "vortex-divergence-analysis-0",
                      "Vortex Divergence Analysis Prelogue", false, false)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexDivergenceAnalysis0, "vortex-divergence-analysis-0",
                    "Vortex Divergence Analysis Prelogue", false, false)

//--

INITIALIZE_PASS(VortexDivergenceAnalysis1, "vortex-divergence-analysis-1",
                "Vortex Divergence Analysis Epilogue", false, false)

//--

INITIALIZE_PASS_BEGIN(VortexDivergenceArguments, "vortex-divergence-arguments",
                      "Vortex Divergence Arguments", false, false)
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(UniformityInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(VortexDivergenceArguments, "vortex-divergence-arguments",
                    "Vortex Divergence Arguments", false, false)

//--

INITIALIZE_PASS_BEGIN(VortexBranchDivergence0, "vortex-branch-divergence-0",
                      "Vortex Branch Divergence Pre-Processing", false, false)
INITIALIZE_PASS_DEPENDENCY(UniformityInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence0, "vortex-branch-divergence-0",
                    "Vortex Branch Divergence Pre-Processing", false, false)

//--

INITIALIZE_PASS_BEGIN(VortexBranchDivergence1, "vortex-branch-divergence-1",
                      "Vortex Branch Divergence", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(UniformityInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence1, "vortex-branch-divergence-1",
                    "Vortex Branch Divergence", false, false)

//--

INITIALIZE_PASS(VortexBranchDivergence2, "VortexBranchDivergence-2",
                "Vortex Branch Divergence Post-Processing", false, false)

///////////////////////////////////////////////////////////////////////////////

namespace vortex {

char VortexDivergenceAnalysis0::ID = 0;

StringRef VortexDivergenceAnalysis0::getPassName() const {
  return "Vortex Divergence Analysis Prelogue";
}

VortexDivergenceAnalysis0::VortexDivergenceAnalysis0()
  : ModulePass(ID) {
  initializeVortexDivergenceAnalysis0Pass(*PassRegistry::getPassRegistry());
}

void VortexDivergenceAnalysis0::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<CallGraphWrapperPass>();
  AU.addRequired<TargetPassConfig>();
  ModulePass::getAnalysisUsage(AU);
}

bool VortexDivergenceAnalysis0::runOnModule(Module &M) {
  LLVM_DEBUG(dbgs() << "*** VX: VortexDivergenceAnalysis0::runOnModule(): " << M.getName() << "\n");

  TargetLibraryInfoImpl TLII(Triple(M.getTargetTriple()));
  auto &TPC = getAnalysis<TargetPassConfig>();
  auto &TM = TPC.getTM<LLVMTargetMachine>();

  // build legacy pass manager
  legacy::FunctionPassManager FPM(&M);

  SmallPtrSet<AnalysisID, 32> alreadyAdded;

  auto TTIWP = createTargetTransformInfoWrapperPass(TM.getTargetIRAnalysis());
  addPassAndDeps(FPM, TTIWP, alreadyAdded);

  auto PC = TM.createPassConfig(FPM);
  addPassAndDeps(FPM, PC, alreadyAdded);

  auto TLIWP = new TargetLibraryInfoWrapperPass(TLII);
  addPassAndDeps(FPM, TLIWP, alreadyAdded);

  auto VDAP = new VortexDivergenceArguments;
  addPassAndDeps(FPM, VDAP, alreadyAdded);

  FPM.doInitialization();

  // clear divergence info
  DivergenceInfo::clear(&M);

  // build function call graph
  SmallVector<Function*, 32> funcs;
  auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  for (auto Icg = scc_begin(&CG), Ecg = scc_end(&CG); Icg != Ecg; ++Icg) {
    for (auto &CGN : *Icg) {
      auto F = CGN->getFunction();
      if (!F || F->isDeclaration())
        continue;
      funcs.push_back(F);
    }
  }

  // traverse call graph in reverse post-order
  // execute analysis passes on each function until convergence
  bool changed;
  do {
    changed = false;
    for (auto it = funcs.rbegin(), ie = funcs.rend(); it != ie; ++it) {
      changed |= FPM.run(**it);
    }
  } while (changed);

  FPM.doFinalization();

  return false;
}

///////////////////////////////////////////////////////////////////////////////

char VortexDivergenceAnalysis1::ID = 0;

StringRef VortexDivergenceAnalysis1::getPassName() const {
  return "Vortex Divergence Analysis Epilogue";
}

VortexDivergenceAnalysis1::VortexDivergenceAnalysis1()
  : ModulePass(ID) {
  initializeVortexDivergenceAnalysis1Pass(*PassRegistry::getPassRegistry());
}

void VortexDivergenceAnalysis1::getAnalysisUsage(AnalysisUsage &AU) const {
  ModulePass::getAnalysisUsage(AU);
}

bool VortexDivergenceAnalysis1::runOnModule(Module &M) {
  LLVM_DEBUG(dbgs() << "*** VX: VortexDivergenceAnalysis1::runOnModule(): " << M.getName() << "\n");
  DivergenceInfo::clear(&M);
  return false;
}

///////////////////////////////////////////////////////////////////////////////

char VortexDivergenceArguments::ID = 0;

StringRef VortexDivergenceArguments::getPassName() const {
  return "Vortex Divergence Arguments";
}

VortexDivergenceArguments::VortexDivergenceArguments()
  : FunctionPass(ID) {
  initializeVortexDivergenceArgumentsPass(*PassRegistry::getPassRegistry());
}

void VortexDivergenceArguments::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AAResultsWrapperPass>();
  AU.addRequired<UniformityInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  FunctionPass::getAnalysisUsage(AU);
}

bool VortexDivergenceArguments::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "*** VX: VortexDivergenceArguments::runOnFunction(): " << F.getName() << "\n");

  auto dv_info = DivergenceInfo::get(&F);
  auto &AA = getAnalysis<AAResultsWrapperPass>().getAAResults();
  auto &UA = getAnalysis<UniformityInfoWrapperPass>().getUniformityInfo();
  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &TLI = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(F);

  bool changed = false;

  // analyze uniformity of function call arguments
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto CB = dyn_cast<CallBase>(&I)) {
        for (unsigned ai = 0, an = CB->arg_size(); ai < an; ++ai) {
          auto value = CB->getArgOperand(ai);
          bool isPtrArgDivergent = false;
          bool isPtrArg = value->getType()->isPointerTy();
          if (isPtrArg) {
            auto PtrLoc = MemoryLocation(value, LocationSize::beforeOrAfterPointer());
            if (CB->mayWriteToMemory()
             && AA.getModRefInfo(CB, PtrLoc) >= ModRefInfo::Mod) {
              isPtrArgDivergent = true; // assume divergent
            }
          }
          if (auto Callee = CB->getCalledFunction()) {
            if (Callee->hasLocalLinkage()) {
              auto arg = std::next(Callee->arg_begin(), ai);
              bool isDivergent = UA.isDivergent(value);
              LLVM_DEBUG(dbgs() << "*** VX: " << (isDivergent ? "divergent" : "uniform") << " function call argument" << ai << "' in '" << F.getName() << "': " << I << "\n");
              changed |= DivergenceInfo::setUniformInArg(Callee, arg, !isDivergent);
              if (isPtrArgDivergent) {
                // check if prioor analysis has already evaluated this pointer
                isPtrArgDivergent = !DivergenceInfo::isUniformOutArg(Callee, arg);
                LLVM_DEBUG(dbgs() << "*** VX: isUniformOutArg() - " << (isPtrArgDivergent ? "divergent" : "uniform") << " function call pointer argument" << ai << "' in '" << F.getName() << "': " << I << "\n");
              }
            }
          }
          if (isPtrArg) {
            // function call arguments that are pointers could be written
            // by the caller function witha divergent value, making all loads
            // from that pointer divergent.
            LLVM_DEBUG(dbgs() << "*** VX: " << (isPtrArgDivergent ? "divergent" : "uniform") << " function call pointer argument" << ai << "' in '" << F.getName() << "': " << I << "\n");
            if (isPtrArgDivergent) {
              auto AI = llvm::findAllocaForValue(value);
              if (AI) {
                changed |= this->markLoadsAfterCall(CB, AI, DT, dv_info);
              }
            }
          }
        }
      }
    }
  }

  // analyze function pointer arguments
  for (auto &Arg : F.args()) {
    if (!Arg.getType()->isPointerTy())
      continue; // skip non-pointers
    bool isDivergent = this->isDivergentOutputPointer(F, Arg, AA, UA, LI, TLI);
    changed |= dv_info->setUniformOutArg(&Arg, !isDivergent);
  }

  // analyze return instructions
  bool is_ret_divergent = false;
  for (auto &BB : F) {
    if (auto RI = dyn_cast<ReturnInst>(BB.getTerminator())) {
      auto RetVal = RI->getReturnValue();
      if (RetVal) {
        bool isDivergent = UA.isDivergent(RetVal);
        LLVM_DEBUG(dbgs() << "*** VX: " << (isDivergent ? "divergent" : "uniform") << " returned value '" << RetVal->getName() << "' in '" << F.getName() << "': " << *RI << "\n");
        if (isDivergent) {
          is_ret_divergent = true;
        }
      }
    }
  }
  changed |= dv_info->setUniformRet(!is_ret_divergent);

  return changed;
}

// Scan forward in the CFG, marking only loads from AI as divergent
bool VortexDivergenceArguments::markLoadsAfterCall(CallBase *CB, AllocaInst *AI, DominatorTree &DT, dv_info_t *dv_info) {
  bool changed = false;
  auto F = CB->getFunction();
  for (Instruction &I : instructions(*F)) {
    if (auto *LI = dyn_cast<LoadInst>(&I)) {
      // Check if CB dominates this load
      if (!DT.dominates(CB, LI))
        continue;
      // Check that the load really comes from AI
      auto LIP = LI->getPointerOperand();
      auto LAI = llvm::findAllocaForValue(LIP);
      if (LAI == AI) {
        // Mark this load as divergent
        changed |= dv_info->setDivergentInstr(LI);
      }
    }
  }
  return changed;
}

bool VortexDivergenceArguments::isDivergentOutputPointer(
    Function &F,
    Argument &PtrArg,
    AliasAnalysis &AA,
    UniformityInfo &UA,
    LoopInfo &LI,
    TargetLibraryInfo &TLI) {
  assert(PtrArg.getType()->isPointerTy());

  SmallPtrSet<Value*, 8> ArgAllocas;
  ArgAllocas.insert(&PtrArg);
  findArgAlloca(F, PtrArg, ArgAllocas);

  for (auto &BB : F) {
    for (auto &I : BB) {
      // Check for calls that might modify the pointer
      if (auto CB = dyn_cast<CallBase>(&I)) {
        for (unsigned ai = 0, an = CB->arg_size(); ai < an; ++ai) {
          auto ArgV = CB->getArgOperand(ai);
          if (!ArgV->getType()->isPointerTy())
            continue;
          for (auto V : ArgAllocas) {
             // first check if is this call escapes pointer V
            if (isDerivedFrom(ArgV, V, LI)) {
              bool NoCapture = false;
              if (auto Callee = CB->getCalledFunction()) {
                if (ai < Callee->arg_size()) {
                  auto CalleeArg = Callee->getArg(ai);
                  NoCapture = CalleeArg->hasNoCaptureAttr();
                }
              }
              if (!NoCapture) {
                LLVM_DEBUG(dbgs() << "*** VX: divergent pointer argument '" << PtrArg.getName() << "' in '" << F.getName() << "' escaped by call: " << *CB << "\n");
                return true;
              }
            }
            if (CB->mayWriteToMemory()) {
              auto VLoc = MemoryLocation(V, LocationSize::beforeOrAfterPointer());
              if (AA.getModRefInfo(CB, VLoc) >= ModRefInfo::Mod) {
                // check if a call argunment aliases PtrArg
                auto ML = MemoryLocation::getForArgument(CB, ai, TLI);
                if (AA.alias(ML, VLoc) != AliasResult::NoAlias) {
                  // check if the argument was anlayzed as uniform
                  if (auto Callee = CB->getCalledFunction()) {
                    auto arg = std::next(Callee->arg_begin(), ai);
                    if (DivergenceInfo::isUniformOutArg(Callee, arg))
                      continue; // uniform, move next
                  }
                  LLVM_DEBUG(dbgs() << "*** VX: divergent pointer argument '" << PtrArg.getName() << "' in '" << F.getName() << "' modified by call: " << *CB << "\n");
                  return true; // assume divergent
                }
              }
            }
          }
        }
      } else // Check for stores to the pointer
      if (auto SI = dyn_cast<StoreInst>(&I)) {
        for (auto ArgV : ArgAllocas) {
          auto SV = SI->getValueOperand();
          auto SLoc = MemoryLocation::get(SI);
          {
            // first check is this store escapes the pointer.
            // i.e. the value stored derived from the pointer argument.
            if (isDerivedFrom(SV, ArgV, LI)) {
              // clear out stores into known slots
              bool StoredIntoKnownSlot = false;
              for (auto V : ArgAllocas) {
                MemoryLocation VLoc(V, LocationSize::beforeOrAfterPointer());
                if (AA.alias(SLoc, VLoc) != AliasResult::NoAlias) {
                  StoredIntoKnownSlot = true;
                  break;
                }
              }
              if (!StoredIntoKnownSlot) {
                LLVM_DEBUG(dbgs() << "*** VX: divergent pointer argument '" << PtrArg.getName() << "' in '" << F.getName() << "' escaped by store: " << *SI << "\n");
                return true; // assume divergent
              }
            }
          }

          {
            // Check for divergent store or divergent value.
            // i.e. the destination address overlaps with the pointer argument,
            // and the value stored is divergent or the store is happening in a divergent basic block.
            auto VLoc = MemoryLocation(ArgV, LocationSize::beforeOrAfterPointer());
            if (AA.alias(SLoc, VLoc) != AliasResult::NoAlias) {
              if (UA.isDivergent(SV)
              || !isUniformlyReached(*SI->getParent(), UA)) {
                LLVM_DEBUG(dbgs() << "*** VX: divergent pointer argument '" << PtrArg.getName() << "' in '" << F.getName() << "' written by store: " << *SI << "\n");
                return true; // assume divergent
              }
            }
          }
        }
      }
    }
  }

  LLVM_DEBUG(dbgs() << "*** VX: uniform pointer argument '" << PtrArg.getName() << "' in '" << F.getName() << "'\n");
  return false;
}

///////////////////////////////////////////////////////////////////////////////

char VortexBranchDivergence0::ID = 0;

StringRef VortexBranchDivergence0::getPassName() const {
  return "Vortex Branch Divergence Pre-Processing";
}

VortexBranchDivergence0::VortexBranchDivergence0()
  : FunctionPass(ID) {
  initializeVortexBranchDivergence0Pass(*PassRegistry::getPassRegistry());
}

void VortexBranchDivergence0::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreservedID(BreakCriticalEdgesID);
  AU.addPreservedID(LowerSwitchID);
  AU.addRequired<UniformityInfoWrapperPass>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

bool VortexBranchDivergence0::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "*** VX: VortexBranchDivergence0::runOnFunction(): " << F.getName() << "\n");

  auto &Context = F.getContext();
  auto &TPC = getAnalysis<TargetPassConfig>();
  auto &TM = TPC.getTM<LLVMTargetMachine>();
  auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  auto &UA = getAnalysis<UniformityInfoWrapperPass>().getUniformityInfo();

  LLVM_DEBUG(dbgs() << "*** VX: before changes!\n" << F << "\n");

  bool changed = false;

  bool hasStdExtZicond = ST.hasStdExtZicond();

  if (!hasStdExtZicond) {
    // Lower Select instructions into standard if-then-else branches
    SmallVector<SelectInst*, 4> selects;

    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto SI = dyn_cast<SelectInst>(&*I)) {
        if (UA.isUniform(SI))
          continue;
        selects.emplace_back(SI);
      }
    }

    for (auto SI : selects) {
      auto BB = SI->getParent();
      LLVM_DEBUG(dbgs() << "*** VX: unswitching divergent select instruction: " << *SI << "\n");
      SplitBlockAndInsertIfThen(SI->getCondition(), SI, false);
      auto CondBr = cast<BranchInst>(BB->getTerminator());
      auto ThenBB = CondBr->getSuccessor(0);
      auto Phi = PHINode::Create(SI->getType(), 2, "unswitched.select", SI);
      Phi->addIncoming(SI->getTrueValue(), ThenBB);
      Phi->addIncoming(SI->getFalseValue(), BB);
      SI->replaceAllUsesWith(Phi);
      SI->eraseFromParent();
      changed = true;
    }
  }

  if (!hasStdExtZicond) {
    // Lower Min/Max intrinsics into standard if-then-else branches
    SmallVector<MinMaxIntrinsic*, 4> MMs;

    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto MMI = dyn_cast<MinMaxIntrinsic>(&*I)) {
        if (UA.isUniform(MMI))
          continue;
        auto ID = MMI->getIntrinsicID();
        if (ID == Intrinsic::smin
         || ID == Intrinsic::smax
         || ID == Intrinsic::umin
         || ID == Intrinsic::umax) {
          MMs.emplace_back(MMI);
        }
      }
    }

    for (auto MMI : MMs) {
      LLVM_DEBUG(dbgs() << "*** VX: unswitching divergent min/max instruction: " << *MMI << "\n");
      auto BB = MMI->getParent();
      auto ID = MMI->getIntrinsicID();
      auto LHS = MMI->getArgOperand(0);
      auto RHS = MMI->getArgOperand(1);
      IRBuilder<> Builder(MMI);
      auto Cond = (ID == Intrinsic::smin || ID == Intrinsic::smax)
                  ? Builder.CreateICmpSLT(LHS, RHS)
                  : Builder.CreateICmpULT(LHS, RHS);
      SplitBlockAndInsertIfThen(Cond, MMI, false);
      auto CondBr = cast<BranchInst>(BB->getTerminator());
      auto ThenBB = CondBr->getSuccessor(0);
      auto Phi = PHINode::Create(MMI->getType(), 2, "unswitched.minmax", MMI);
      if (ID == Intrinsic::smin || ID == Intrinsic::umin) {
        Phi->addIncoming(LHS, ThenBB);
        Phi->addIncoming(RHS, BB);
      } else {
        Phi->addIncoming(RHS, ThenBB);
        Phi->addIncoming(LHS, BB);
      }
      MMI->replaceAllUsesWith(Phi);
      MMI->eraseFromParent();
      changed = true;
    }
  }

  {
    std::vector<BasicBlock*> ReturningBlocks;
    std::vector<BasicBlock*> UnreachableBlocks;

    for (auto& BB : F) {
      if (isa<ReturnInst>(BB.getTerminator()))
        ReturningBlocks.push_back(&BB);
      else if (isa<UnreachableInst>(BB.getTerminator()))
        UnreachableBlocks.push_back(&BB);
    }

    //
    // Handle return blocks
    //
    BasicBlock* ReturnBlock = nullptr;
    if (ReturningBlocks.empty()) {
      ReturnBlock = nullptr;
    } else if (ReturningBlocks.size() == 1) {
      ReturnBlock = ReturningBlocks.front();
    } else {
      // Otherwise, fold all returns into a single exit block.
      // We need to insert a new basic block into the function, add PHI
      // nodes (if the function returns values), and convert all of the return
      // instructions into unconditional branches.
      BasicBlock *NewRetBlock = BasicBlock::Create(Context, "UnifiedReturnBlock", &F);

      PHINode *PN = nullptr;
      if (F.getReturnType()->isVoidTy()) {
        ReturnInst::Create(Context, nullptr, NewRetBlock);
      } else {
        // If the function doesn't return void... add a PHI node to the block...
        PN = PHINode::Create(F.getReturnType(), ReturningBlocks.size(), "UnifiedRetVal");
        PN->insertInto(NewRetBlock, NewRetBlock->end());
        ReturnInst::Create(Context, PN, NewRetBlock);
      }

      // Loop over all of the blocks, replacing the return instruction with an
      // unconditional branch.
      for (auto BB : ReturningBlocks) {
        // Add an incoming element to the PHI node for every return instruction that
        // is merging into this new block...
        if (PN)
          PN->addIncoming(BB->getTerminator()->getOperand(0), BB);

        BB->back().eraseFromParent();;  // Remove the return insn
        BranchInst::Create(NewRetBlock, BB);
      }
      ReturnBlock = NewRetBlock;
      changed = true;
    }

    //
    // Handle unreacheable blocks
    //
    BasicBlock* UnreachableBlock = nullptr;
    if (UnreachableBlocks.empty()) {
      UnreachableBlock = nullptr;
    } else if (UnreachableBlocks.size() == 1) {
      UnreachableBlock = UnreachableBlocks.front();
    } else {
      UnreachableBlock = BasicBlock::Create(Context, "UnifiedUnreachableBlock", &F);
      new UnreachableInst(Context, UnreachableBlock);
      for (BasicBlock *BB : UnreachableBlocks) {
        BB->back().eraseFromParent();  // Remove the unreachable inst.
        auto Br = BranchInst::Create(UnreachableBlock, BB);
        Br->setMetadata("Unreachable", MDNode::get(Context, std::nullopt));
      }
      changed = true;
    }

    // Ensure single exit block
    if (UnreachableBlock && ReturnBlock) {

      auto NewRetBlock = BasicBlock::Create(Context, "UnifiedReturnAndUnreachableBlock", &F);
      auto RetType = F.getReturnType();
      PHINode* PN = nullptr;

      if (!RetType->isVoidTy()) {
        // Need to insert PhI node to merge return values from incoming blocks
        PN = PHINode::Create(RetType, ReturningBlocks.size(), "UnifiedReturnAndUnreachableVal");
        PN->insertInto(NewRetBlock, NewRetBlock->end());

        auto DummyRetValue = llvm::Constant::getNullValue(RetType);
        PN->addIncoming(DummyRetValue, UnreachableBlock);

        PN->addIncoming(ReturnBlock->getTerminator()->getOperand(0), ReturnBlock);
      }

      ReturnInst::Create(Context, PN, NewRetBlock);

      UnreachableBlock->back().eraseFromParent();
      auto Br = BranchInst::Create(NewRetBlock, UnreachableBlock);
      Br->setMetadata("Unreachable", llvm::MDNode::get(Context, std::nullopt));

      ReturnBlock->back().eraseFromParent();
      BranchInst::Create(NewRetBlock, ReturnBlock);

      ReturnBlock = NewRetBlock;
      changed = true;
    }
  }

  if (changed) {
    LLVM_DEBUG(dbgs() << "*** VX: after changes!\n" << F << "\n");
  }

  return changed;
}

///////////////////////////////////////////////////////////////////////////////

char VortexBranchDivergence1::ID = 0;

VortexBranchDivergence1::VortexBranchDivergence1(int divergenceMode)
  : FunctionPass(ID)
  , divergenceMode_(divergenceMode) {
  initializeVortexBranchDivergence1Pass(*PassRegistry::getPassRegistry());
}

StringRef VortexBranchDivergence1::getPassName() const {
  return "Vortex Branch Divergence";
}

void VortexBranchDivergence1::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<RegionInfoPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<UniformityInfoWrapperPass>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

void VortexBranchDivergence1::initialize(Function &F) {
  auto& M = *F.getParent();
  auto& Context = M.getContext();

  auto sizeTSize = M.getDataLayout().getPointerSizeInBits();
  switch (sizeTSize) {
  case 128: SizeTTy_ = llvm::Type::getInt128Ty(Context); break;
  case 64:  SizeTTy_ = llvm::Type::getInt64Ty(Context); break;
  case 32:  SizeTTy_ = llvm::Type::getInt32Ty(Context); break;
  case 16:  SizeTTy_ = llvm::Type::getInt16Ty(Context); break;
  case 8:   SizeTTy_ = llvm::Type::getInt8Ty(Context); break;
  default:
    LLVM_DEBUG(dbgs() << "Error: invalid pointer size: " << sizeTSize << "\n");
  }

  if (sizeTSize == 64) {
    tmask_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmask_i64);
    pred_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred_i64);
    pred_n_func_= Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred_n_i64);
    tmc_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmc_i64);
    split_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split_i64);
    split_n_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split_n_i64);
    join_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_join_i64);
    mov_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_mov_i64);
  } else {
    assert(sizeTSize == 32);
    tmask_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmask_i32);
    pred_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred_i32);
    pred_n_func_= Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred_n_i32);
    tmc_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmc_i32);
    split_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split_i32);
    split_n_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split_n_i32);
    join_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_join_i32);
    mov_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_mov_i32);
  }

  namePrinter_.init(&F);
  replaceSuccessor_.init(&F);

  div_blocks_.clear();
  div_blocks_set_.clear();
  loops_.clear();
  loops_set_.clear();
}

bool VortexBranchDivergence1::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "*** VX: VortexBranchDivergence1::runOnFunction(): " << F.getName() << "\n");

  this->initialize(F);

  auto &UA = getAnalysis<UniformityInfoWrapperPass>().getUniformityInfo();
  auto &RI = getAnalysis<RegionInfoPass>().getRegionInfo();
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &PDT= getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  //--

  auto &Context = F.getContext();

  LLVM_DEBUG(UA.print(dbgs()));
  LLVM_DEBUG(dbgs() << "*** VX: Region info:\n");
  LLVM_DEBUG(RI.getTopLevelRegion()->dump());
  LLVM_DEBUG(dbgs() << "\n");

  bool changed = false;

  for (auto I = df_begin(&F.getEntryBlock()),
            E = df_end(&F.getEntryBlock()); I != E; ++I) {
    auto BB = *I;
    auto Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (Br == nullptr)
      continue;

    // only process conditional branches
    if (Br->isUnconditional()) {
      LLVM_DEBUG(dbgs() << "*** VX: skip non-conditional branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    // only process divergent branches
    if (UA.isUniform(Br)
     || (Br->getMetadata("structurizecfg.uniform") != nullptr)) {
      LLVM_DEBUG(dbgs() << "*** VX: skip uniform branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    auto loop = LI.getLoopFor(BB);
    if (loop != nullptr) {
      auto ipdom = PDT.findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
      if (ipdom && loop->contains(ipdom)) {
        if (div_blocks_set_.insert(BB).second) {
          // add new branch to the list
          LLVM_DEBUG(dbgs() << "*** VX: divergent branch: " << namePrinter_.BBName(BB) << "\n");
          div_blocks_.push_back(BB);
        }
      } else {
        if (loops_set_.insert(loop).second) {
          // add new loop to the list
          LLVM_DEBUG(dbgs() << "*** VX: divergent loop: " << namePrinter_.BBName(loop->getHeader()) << "\n");
          loops_.push_back(loop);
        }
      }
    } else {
      auto ipdom = PDT.findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
      if (ipdom == nullptr) {
        llvm::errs() << "Warning: divergent branch with no IPDOM: " << namePrinter_.BBName(BB) << " --- skipping.\n";
        continue;
      }
      bool has_unreacheable = false;
      for (auto succ : Br->successors()) {
        if (succ->back().getMetadata("Unreachable") != nullptr) {
          has_unreacheable = true;
          break;
        }
      }
      if (has_unreacheable) {
        llvm::errs() << "Warning: divergent branch with unreachable IPDOM: " << namePrinter_.BBName(BB) << " --- skipping.\n";
        continue;
      }
      if (div_blocks_set_.insert(BB).second) {
        // add new branch to the list
        LLVM_DEBUG(dbgs() << "*** VX: divergent branch: " << namePrinter_.BBName(BB) << ", IPDOM=" << namePrinter_.BBName(ipdom) << "\n");
        div_blocks_.push_back(BB);
      }
    }
  }

  // apply transformation
  if (!loops_.empty() || !div_blocks_.empty()) {
    LLVM_DEBUG(dbgs() << "*** VX: before changes!\n" << F << "\n");

    // process the loop
    // This should be done first such that loop analysis is not tempered
    if (!loops_.empty()) {
      this->processLoops(&Context, &F);
      loops_.clear();
      // update PDT
      PDT.recalculate(F);
    }

    // process branches
    if (!div_blocks_.empty()) {
      this->processBranches(&Context, &F, PDT);
      div_blocks_.clear();
    }

    changed = true;

    LLVM_DEBUG(dbgs() << "*** VX: after changes!\n" << F << "\n");
  }

  // remove uniform intrinsics
  for (auto iter = inst_begin(F), iterE = inst_end(F); iter != iterE;) {
    auto& I = *iter++;
    if (auto II = dyn_cast<IntrinsicInst>(&I)) {
      if (II->getIntrinsicID() == Intrinsic::riscv_vx_uniform) {
        auto src = II->getOperand(0);
        II->replaceAllUsesWith(src);
        II->eraseFromParent();
      }
    }
  }

  return changed;
}

void VortexBranchDivergence1::processLoops(LLVMContext* context, Function* function) {
  DenseSet<const BasicBlock *> stub_blocks;

  // traverse the list in reverse order
  for (auto it = loops_.rbegin(), ite = loops_.rend(); it != ite; ++it) {
    auto loop = *it;
    auto header = loop->getHeader();
    assert(header);

    auto preheader = loop->getLoopPreheader();
    assert(preheader);

    auto preheader_term = preheader->getTerminator();
    assert(preheader_term);

    auto preheader_br = dyn_cast<BranchInst>(preheader_term);
    assert(preheader_br);

    LLVM_DEBUG(dbgs() << "*** VX: process loop: " << namePrinter_.BBName(header) << "\n");

    // save current thread mask in preheader
    auto tmask = CallInst::Create(tmask_func_, "tmask", preheader_br);
    LLVM_DEBUG(dbgs() << "*** VX: backup thread mask '" << namePrinter_.ValueName(tmask) << "' before loop preheader branch: " << namePrinter_.BBName(preheader) << "\n");

    // restore thread mask at loop exit blocks
    {
      SmallVector<BasicBlock *, 8> exiting_blocks;
      loop->getExitingBlocks(exiting_blocks); // blocks inside the loop going out

      for (auto exiting_block : exiting_blocks) {
        int exit_edges = 0;
        auto branch = dyn_cast<BranchInst>(exiting_block->getTerminator());
        for (auto succ : branch->successors()) {
          // stub blocks insertion will generate invalid exiting blocks.
          // we just need exclude those new blocks.
          if (loop->contains(succ)
           || stub_blocks.count(succ) != 0)
            continue;

          if (branch->isUnconditional())
            continue;

          // ensure only one exit edge
          assert(exit_edges == 0);
          ++exit_edges;

          // insert a predicate instruction to mask out threads that are exiting the loop

          IRBuilder<> ir_builder(branch);
          auto succ0 = branch->getSuccessor(0);
          auto cond_orig = branch->getCondition();
          auto cond_orig_i1 = ir_builder.CreateICmpNE(cond_orig, ConstantInt::get(cond_orig->getType(), 0), namePrinter_.ValueName(cond_orig) + ".to.i1");
          auto cond_orig_i32 = ir_builder.CreateIntCast(cond_orig_i1, SizeTTy_, false, namePrinter_.ValueName(cond_orig_i1) + ".to.i32");

          // insert a custom mov instruction to prevent branch condition from being optimized away during codegen
          auto cond = CallInst::Create(mov_func_, cond_orig_i32, namePrinter_.ValueName(cond_orig_i32) + ".mov", branch);

          LLVM_DEBUG(dbgs() << "*** VX: insert thread predicate '" << namePrinter_.ValueName(cond) << "' before exiting block: " << namePrinter_.BBName(exiting_block) << "\n");
          if (!loop->contains(succ0)) {
            CallInst::Create(pred_n_func_, {cond, tmask}, "", branch);
          } else {
            CallInst::Create(pred_func_, {cond, tmask}, "", branch);
          }
          LLVM_DEBUG(dbgs() << "*** VX: after predicate change!\n" << function << "\n");

          // change branch condition
          auto cond_i1 = ir_builder.CreateICmpNE(cond, ConstantInt::get(SizeTTy_, 0), namePrinter_.ValueName(cond) + ".to.i1");
          branch->setCondition(cond_i1);
        }
      }
    }
  }
}

void VortexBranchDivergence1::processBranches(LLVMContext* context, Function* function, PostDominatorTree &PDT) {
  std::unordered_map<BasicBlock*, BasicBlock*> ipdoms;

  // pre-gather ipdoms for divergent branches
  for (auto BI = div_blocks_.rbegin(), BIE = div_blocks_.rend(); BI != BIE; ++BI) {
    auto block = *BI;
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);
    auto ipdom = PDT.findNearestCommonDominator(branch->getSuccessor(0), branch->getSuccessor(1));
    if (ipdom == nullptr) {
      llvm::errs() << "error: divergent branch with no IPDOM: " << namePrinter_.BBName(block) << "\n";
      std::abort();
    }
    ipdoms[block] = ipdom;
  }

  // traverse the list in reverse order
  for (auto BI = div_blocks_.rbegin(), BIE = div_blocks_.rend(); BI != BIE; ++BI) {
    auto block = *BI;
    auto ipdom = ipdoms[block];
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);
    // insert a mov instruction before split
    IRBuilder<> ir_builder(branch);
    auto cond_orig = branch->getCondition();
    auto cond_orig_i1 = ir_builder.CreateICmpNE(cond_orig, ConstantInt::get(cond_orig->getType(), 0), namePrinter_.ValueName(cond_orig) + ".to.i1");
    auto cond_orig_i32 = ir_builder.CreateIntCast(cond_orig_i1, SizeTTy_, false, namePrinter_.ValueName(cond_orig_i1) + ".to.i32");
    auto cond = CallInst::Create(mov_func_, cond_orig_i32, namePrinter_.ValueName(cond_orig_i32) + ".mov", branch);

    // insert split instruction before divergent branch
    LLVM_DEBUG(dbgs() << "*** VX: insert split '" << namePrinter_.ValueName(cond) << "' before " << namePrinter_.BBName(block) << "'s branch.\n");
    auto stack_ptr = CallInst::Create(split_func_, cond, "", branch);

    // change branch condition
    auto cond_i1 = ir_builder.CreateICmpNE(cond, ConstantInt::get(SizeTTy_, 0), namePrinter_.ValueName(cond) + ".to.i1");
    branch->setCondition(cond_i1);

    // insert a join stub block before ipdom
    auto stub = BasicBlock::Create(*context, "join_stub", function, ipdom);
    LLVM_DEBUG(dbgs() << "*** VX: insert join stub '" << stub->getName() << "' before " << namePrinter_.BBName(ipdom) << "\n");
    auto stub_br = BranchInst::Create(ipdom, stub);
    CallInst::Create(join_func_, stack_ptr, "", stub_br);
    std::vector<BasicBlock*> preds;
    FindSuccessor(block, ipdom, preds);
    for (auto pred : preds) {
      bool found = replaceSuccessor_.replaceSuccessor(pred, ipdom, stub);
      if (!found) {
        std::abort();
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

char VortexBranchDivergence2::ID = 0;

StringRef VortexBranchDivergence2::getPassName() const {
  return "VortexBranchDivergence2Pass";
}

VortexBranchDivergence2::VortexBranchDivergence2(int PassMode)
  : MachineFunctionPass(ID)
  , PassMode_(PassMode) {
  initializeVortexBranchDivergence2Pass(*PassRegistry::getPassRegistry());
}

static bool FindNextJoin(MachineBasicBlock::iterator* out,
                         const MachineBasicBlock::iterator& start,
                         const MachineBasicBlock& curMBB) {
  for (auto it = start; it != curMBB.end(); ++it) {
    if (it->getOpcode() == RISCV::VX_JOIN) {
      *out = it;
      return true;
    }
  }
  if (curMBB.succ_size() == 1) {
    auto succMBB = *curMBB.succ_begin();
    return FindNextJoin(out, succMBB->begin(), *succMBB);
  }
  return false;
}

bool VortexBranchDivergence2::runOnMachineFunction(MachineFunction &MF) {
  auto &ST = MF.getSubtarget<RISCVSubtarget>();
  auto TII = ST.getInstrInfo();
  auto& MRI = MF.getRegInfo();

  bool Changed = false;

  switch (PassMode_) {
  case 0:
    for (auto& MBB : MF) {
      for (auto _MII = MBB.instr_begin(), MIIEnd = MBB.instr_end(); _MII != MIIEnd;) {
        auto MII = _MII++;
        auto& MI = *MII;
        if (MI.getOpcode() == RISCV::VX_MOV) {
          auto DestReg = MI.getOperand(0).getReg();
          auto SrcReg = MI.getOperand(1).getReg();
          MRI.replaceRegWith(DestReg, SrcReg);
          MI.eraseFromParent();
          Changed = true;
        }
      }
    }
    break;

  case 1:
    for (auto& MBB : MF) {
      for (auto _MII = MBB.instr_begin(), MIIEnd = MBB.instr_end(); _MII != MIIEnd;) {
        auto MII = _MII++;
        auto& MI = *MII;
        if (!(MI.getOpcode() == RISCV::VX_SPLIT
          || MI.getOpcode() == RISCV::VX_SPLIT_N))
          continue;

        // find the corresponding branch instruction
        auto MII_br = MII;
        for (;MII_br != MIIEnd; ++MII_br) {
          if (MII_br->isBranch())
            break;
        }

        if (MII_br == MIIEnd
         || MII_br->getOpcode() == RISCV::PseudoBR) {
          // if a join instruction is found in same or proceeding fallthrough blocks,
          // that means the protected branch was removed during optimization passes
          // we can safely remove the left-out split and join instructions
          MachineBasicBlock::iterator MII_join;
          if (FindNextJoin(&MII_join, std::next(MII), MBB)) {
            if (_MII == MII_join) {
              ++_MII;
            }
            MII_join->eraseFromParent();
            MI.eraseFromParent();
            LLVM_DEBUG(dbgs() << "*** VX: Vortex: cleanup removed branches!\n");
            Changed = true;
            continue;
          }

          llvm::errs() << "error: missing divergent branch!\n" << MBB << "\n";
          std::abort();
        }

        // ensure Branch BEQ/BNE xi, x0
        if (!(MII_br->getOpcode() == RISCV::BEQ
          || MII_br->getOpcode() == RISCV::BNE)
        || !MII_br->getOperand(0).isReg()
        || !MII_br->getOperand(1).isReg()
        || MII_br->getOperand(1).getReg() != RISCV::X0) {
          llvm::errs() << "error: unsupported divergent branch!\n" << MBB << "\n";
          std::abort();
        }

        // ensure branch opcode match
        if (MII_br->getOpcode() == RISCV::BEQ) {
          switch (MI.getOpcode()) {
          case RISCV::VX_SPLIT:
            MI.setDesc(TII->get(RISCV::VX_SPLIT_N));
            break;
          case RISCV::VX_SPLIT_N:
            MI.setDesc(TII->get(RISCV::VX_SPLIT));
            break;
          }
          LLVM_DEBUG(dbgs() << "*** VX: Vortex: fixed predicate opcode!\n");
          Changed = true;
          continue;
        }
      }
    }
    break;
  }

  if (Changed) {
    LLVM_DEBUG(dbgs() << "*** VX: after changes!\n" << MF.getName() << "\n");
    LLVM_DEBUG(MF.dump(););
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////

// This pass collects uniform IR annotations and replaces them with
// riscv_vx_uniform intrinsic calls. It processes both metadata-based
// annotations (e.g., "vortex.uniform") and intrinsic-based annotations
// (e.g., "var_annotation" with "vortex.uniform" value).
//
// WARNING: This pass should be executed as early as possible in the pipeline,
// before any optimizations that might remove the annotated instructions.
PreservedAnalyses UniformAnnotationPass::run(Function &F, FunctionAnalysisManager &AM) {
  bool changed = false;

  std::vector<std::pair<Instruction*, bool>> uniformInsts;

  // collect uniform IR annoations
  for (auto& BB : F) {
    for (auto& I : BB) {
      // process metadata-based annotations
      if (I.getMetadata("vortex.uniform") != nullptr) {
        LLVM_DEBUG(dbgs() << "*** VX: found metadata annotation: " << I << "\n");
        uniformInsts.push_back({&I, false}); // false = metadata annotation
      } else
      // process intrinsic-based annotations
      if (auto II = dyn_cast<IntrinsicInst>(&I)) {
        if (II->getIntrinsicID() == Intrinsic::var_annotation) {
          auto gv  = dyn_cast<GlobalVariable>(II->getOperand(1));
          auto cda = dyn_cast<ConstantDataArray>(gv->getInitializer());
          if (cda->getAsCString() == "vortex.uniform") {
            auto AnnotatedAlloca = dyn_cast<AllocaInst>(II->getOperand(0));
            if (AnnotatedAlloca) {
              LLVM_DEBUG(dbgs() << "*** VX: found var_annotation: " << *AnnotatedAlloca << "\n");
              uniformInsts.push_back({AnnotatedAlloca, true}); // true = var_annotation annotation
            }
          }
        }
      }
    }
  }

  // process annotated values
  for (auto Instr : uniformInsts) {
    if (Instr.second) {
      // handle var_annotation annotations
      auto AnnotatedAlloca = reinterpret_cast<AllocaInst*>(Instr.first);
      std::vector<LoadInst*> loadsToReplace;
      StoreInst* Store = nullptr;

      // find all loads and stores to the annotated stack variable
      for (auto User : AnnotatedAlloca->users()) {
        if (auto LI = dyn_cast<LoadInst>(User)) {
          loadsToReplace.push_back(LI);
        }
        if (auto SI = dyn_cast<StoreInst>(User)) {
          Store = SI; // the annotation has been applied to the stack variable
        }
      }
      // insert riscv_vx_uniform intrinsic before all uses of collected loads
      if (Store != nullptr) {
        IRBuilder<> Builder(Store->getNextNode()); // insert after the store
        auto LoadedValue = Builder.CreateLoad(AnnotatedAlloca->getAllocatedType(), AnnotatedAlloca, AnnotatedAlloca->getName() + ".loaded");
        auto ValueType = LoadedValue->getType();
        auto IntrinsicFunc = Intrinsic::getDeclaration(F.getParent(), Intrinsic::riscv_vx_uniform, {ValueType, ValueType});
        auto CallInst = Builder.CreateCall(IntrinsicFunc, {LoadedValue}, LoadedValue->getName() + ".uniform");
        for (auto LI : loadsToReplace) {
          LI->replaceAllUsesWith(CallInst);
          LI->eraseFromParent();
        }
        changed = true;
      }
    } else {
      // insert riscv_vx_uniform intrinsic before all uses of annotated instruction
      auto I = Instr.first;
      IRBuilder<> Builder(I->getNextNode());
      auto ValueType = I->getType();
      auto IntrinsicFunc = Intrinsic::getDeclaration(F.getParent(), Intrinsic::riscv_vx_uniform, {ValueType, ValueType});
      auto CallInst = Builder.CreateCall(IntrinsicFunc, {llvm::UndefValue::get(ValueType)}, I->getName() + ".uniform");
      I->replaceAllUsesWith(CallInst);
      CallInst->setArgOperand(0, I);
      changed = true;
    }
  }

  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

///////////////////////////////////////////////////////////////////////////////

llvm::DenseMap<const llvm::Function*, dv_info_t> DivergenceInfo::dv_infos_;
llvm::sys::Mutex DivergenceInfo::mutex_;

dv_info_t* DivergenceInfo::get(const llvm::Function* F) {
  llvm::sys::ScopedLock lock(mutex_);
  auto &x = dv_infos_[F];
  return &x;
}

bool DivergenceInfo::setUniformInArg(const llvm::Function* F, const llvm::Argument* Arg, bool is_uniform) {
  llvm::sys::ScopedLock lock(mutex_);
  return dv_infos_[F].setUniformInArg(Arg, is_uniform);
}

bool DivergenceInfo::isUniformRet(const llvm::Function* F) {
  llvm::sys::ScopedLock lock(mutex_);
  auto it = dv_infos_.find(F);
  if (it != dv_infos_.end()) {
    return it->second.isUniformRet();
  }
  return false; // conservative
}

bool DivergenceInfo::isUniformOutArg(const llvm::Function* F, const llvm::Argument* Arg) {
  llvm::sys::ScopedLock lock(mutex_);
  auto it = dv_infos_.find(F);
  if (it != dv_infos_.end()) {
    return it->second.isUniformOutArg(Arg);
  }
  return false; // conservative
}

void DivergenceInfo::clear(const llvm::Module* M) {
  llvm::sys::ScopedLock lock(mutex_);
  // Clear all divergence info for the module
  for (auto it = dv_infos_.begin(); it != dv_infos_.end();) {
    auto itc = it++;
    if (itc->first->getParent() == M) {
      dv_infos_.erase(itc);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

DivergenceTracker::DivergenceTracker(const Function &F)
  : function_(&F)
  , initialized_(false) {
  // Note: defer initialization until use because this constructor is also invoked,
  // when divergence analysis is not needed as part of the RISCVTTIImpl's contruction.
}

void DivergenceTracker::initialize() {
  LLVM_DEBUG(dbgs() << "*** VX: DivergenceTracker::initialize(): " << function_->getName() << "\n");
  dv_info_ = DivergenceInfo::get(function_);
  initialized_ = true;
}

bool DivergenceTracker::isSourceOfDivergence(const Value *V) {
  if (!initialized_) {
    this->initialize();
  }

  if (dv_info_->isDivergentInstr(V)) {
    LLVM_DEBUG(dbgs() << "*** VX: divergent=true for variable: " << *V << "\n");
    return true; // divergent
  }

  // We conservatively assume all function arguments to potentially be divergent
  // except when they are marked as uniform.
  if (auto Arg = dyn_cast<Argument>(V)) {
    if (dv_info_->isUniformInArg(Arg)) {
      LLVM_DEBUG(dbgs() << "*** VX: divergent=false for argument: " << *V << "\n");
      return false; // uniform
    } else {
      LLVM_DEBUG(dbgs() << "*** VX: divergent=true for argument: " << *V << "\n");
      return true; // divergent
    }
  }

  // Atomics are divergent because they are executed sequentially: when an
  // atomic operation refers to the same address in each thread, then each
  // thread after the first sees the value written by the previous thread as
  // original value.
  if (isa<AtomicRMWInst>(V)
   || isa<AtomicCmpXchgInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** VX: divergent=true for atomic variable: " << *V << "\n");
    return true;
  }

  // We conservatively assume function return values are divergent
  // if they are not marked as uniform in isAlwaysUniform
  // this list also include intrinsics like "Intrinsic::threadlocal_address"
  // used for accessing TLS variables.
  if (isa<CallBase>(V)) {
    LLVM_DEBUG(dbgs() << "*** VX: divergent=true for function call: " << *V << "\n");
    return true; // assume divergent
  }

  // we are not certain about the rest!
  LLVM_DEBUG(dbgs() << "*** VX: divergent=false for variable: " << *V << "\n");
  return false;
}

bool DivergenceTracker::isAlwaysUniform(const Value *V) {
  if (!initialized_) {
    this->initialize();
  }
 
  // SJ: testing
  int gVortexBranchDivergenceOptLevel = 8;
  if (std::getenv("VORTEX_DIVERGENCE_OPT_LEVEL") != nullptr)
    gVortexBranchDivergenceOptLevel =
      std::stoi(std::string(std::getenv("VORTEX_DIVERGENCE_OPT_LEVEL")));
  if ( gVortexBranchDivergenceOptLevel >= 0 && gVortexBranchDivergenceOptLevel < 3)
  {
    LLVM_DEBUG(dbgs() << "*** VX: Skip isAlwaysUniform func of divergence tracker (opt level 0-2)\n");
    return false;
  }

  if (auto CB = dyn_cast<CallBase>(V)) {
    if (auto II = dyn_cast<IntrinsicInst>(CB)) {
      if (II->getIntrinsicID() == Intrinsic::riscv_vx_uniform) {
        LLVM_DEBUG(dbgs() << "*** VX: uniform=true for intrinsic annotation: " << *V << "\n");
        return true;
      }
    } else
    if (CB->isInlineAsm()) {
      // All machine CSRs and some special user CSRs are always uniform
      auto IA = cast<InlineAsm>(CB->getCalledOperand());
      StringRef AsmStr = IA->getAsmString();
      if (!(AsmStr.contains('\n') || AsmStr.contains('\n'))) { // skip multi-line asm
        AsmStr = AsmStr.ltrim(); // skip leading spaces
        if ((AsmStr.starts_with("csrr ")
          || AsmStr.starts_with("csrw ")
          || AsmStr.starts_with("csrwi ")
          || AsmStr.starts_with("csrs ")
          || AsmStr.starts_with("csrsi ")
          || AsmStr.starts_with("csrc ")
          || AsmStr.starts_with("csrci ")
          || AsmStr.starts_with("csrrw ")
          || AsmStr.starts_with("csrrwi ")
          || AsmStr.starts_with("csrrs ")
          || AsmStr.starts_with("csrrsi ")
          || AsmStr.starts_with("csrrc ")
          || AsmStr.starts_with("csrrci "))) {
          auto Addr = CB->getArgOperand(0);
          if (auto *C = dyn_cast<ConstantInt>(Addr)) {
            uint64_t Addr = C->getZExtValue();
            // extract RISC-V CSR privileged level
            uint32_t level = (Addr >> 8) & 0x3;
            // Machine CSRs are always uniform
            if (level == 0x3) {
              LLVM_DEBUG(dbgs() << "*** VX: uniform=true for machine CSR: Add=" << format_hex(Addr, 0) << ", " << *V << "\n");
              return true;
            }
            if (Addr == 0xCC1    // warp_id
             || Addr == 0xCC2    // core_id
             || Addr == 0xCC3    // active warps
             || Addr == 0xCC4) { // active threads
              // special user CSRs are also uniform
              LLVM_DEBUG(dbgs() << "*** VX: uniform=true for special user CSR: Add=" << format_hex(Addr, 0) << ", " << *V << "\n");
              return true;
            }
          }
        }
      }
      LLVM_DEBUG(dbgs() << "*** VX: uniform=false for inline assembly: " << *V << "\n");
      return false; // unknown
    } else {
      if (auto Callee = CB->getCalledFunction()) {
        // We conservatively assume function return values are divergent
        // except when they are marked as uniform.
        bool is_uniform = DivergenceInfo::isUniformRet(Callee);
        if (is_uniform) {
          LLVM_DEBUG(dbgs() << "*** VX: uniform=true for function Call: " << *V << "\n");
          return true; // uniform
        }
        LLVM_DEBUG(dbgs() << "*** VX: uniform=false for function Call: " << *V << "\n");
        return false; // unknown
      } else {
        LLVM_DEBUG(dbgs() << "*** VX: uniform=false for indirect Call: " << *V << "\n");
        return false; // unknown
      }
    }
  }
  // we not certain about the rest!
  LLVM_DEBUG(dbgs() << "*** VX: uniform=false for variable: " << *V << "\n");
  return false;
}

} // vortex
