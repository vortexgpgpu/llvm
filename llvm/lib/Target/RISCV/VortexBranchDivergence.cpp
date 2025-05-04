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


#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"


#include "llvm/Analysis/UniformityAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/PostDominators.h"

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
#include "llvm/IR/Instructions.h"        // for CallBase, AllocaInst, LoadInst
#include "llvm/IR/DerivedTypes.h"        // for PointerType
#include "llvm/Analysis/ValueTracking.h" // for GetPointerBaseWithConstantOffset

#include <iostream>

using namespace vortex;
using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "vortex-branch-divergence"

#ifndef NDEBUG
#define LLVM_DEBUG(x) do {x;} while (false)
#endif

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

///////////////////////////////////////////////////////////////////////////////

struct VortexBranchDivergence0 : public FunctionPass {
private:
  UniformityInfo *UA_;

public:

  static char ID;

  VortexBranchDivergence0();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;
};

///////////////////////////////////////////////////////////////////////////////

class VortexBranchDivergence1 : public FunctionPass {
private:

  using StackEntry = std::pair<BasicBlock *, Value *>;
  using StackVector = SmallVector<StackEntry, 16>;

  int divergenceMode_;

  ReplaceSuccessor replaceSuccessor_;
  NamePrinter namePrinter_;

  std::vector<BasicBlock*> div_blocks_;
  DenseSet<BasicBlock*> div_blocks_set_;

  std::vector<Loop*> loops_;
  DenseSet<Loop*> loops_set_;

  UniformityInfo *UA_;
  DominatorTree *DT_;
  PostDominatorTree *PDT_;
  LoopInfo *LI_;
  RegionInfo *RI_;

  Type* SizeTTy_;

  Function *tmask_func_;
  Function *pred_func_;
  Function *pred_n_func_;
  Function *tmc_func_;
  Function *split_func_;
  Function *split_n_func_;
  Function *join_func_;
  Function *mov_func_;

  void initialize(Function &F, const RISCVSubtarget &ST);

  void processBranches(LLVMContext* context, Function* function);

  void processLoops(LLVMContext* context, Function* function);

  bool isUniform(Instruction *T);

public:

  static char ID;

  VortexBranchDivergence1(int divergenceMode = 0);

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;
};

///////////////////////////////////////////////////////////////////////////////

struct VortexBranchDivergence2 : public MachineFunctionPass {
private:
  int PassMode_;

public:
  static char ID;
  VortexBranchDivergence2(int PassMode);

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override;
};

}

///////////////////////////////////////////////////////////////////////////////

namespace llvm {

void initializeVortexBranchDivergence0Pass(PassRegistry &);
void initializeVortexBranchDivergence1Pass(PassRegistry &);

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

INITIALIZE_PASS_BEGIN(VortexBranchDivergence0, "vortex-branch-divergence-0",
                "Vortex Branch Divergence Pre-Processing", false, false)
INITIALIZE_PASS_DEPENDENCY(UniformityInfoWrapperPass)
INITIALIZE_PASS_END(VortexBranchDivergence0, "vortex-branch-divergence-0",
                    "Vortex Branch Divergence Pre-Processing", false, false)

INITIALIZE_PASS_BEGIN(VortexBranchDivergence1, "vortex-branch-divergence-1",
                      "Vortex Branch Divergence", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(UniformityInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence1, "vortex-branch-divergence-1",
                    "Vortex Branch Divergence", false, false)

INITIALIZE_PASS(VortexBranchDivergence2, "VortexBranchDivergence-2",
                "Vortex Branch Divergence Post-Processing", false, false)

namespace vortex {

char VortexBranchDivergence0::ID = 0;

StringRef VortexBranchDivergence0::getPassName() const {
  return "Vortex Unify Function Exit Nodes";
}

VortexBranchDivergence0::VortexBranchDivergence0() : FunctionPass(ID) {
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
  auto &Context = F.getContext();
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);

  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling Pass0 ***\n");

  LLVM_DEBUG(dbgs() << "*** before Pass0 changes!\n" << F << "\n");

  UA_ = &getAnalysis<UniformityInfoWrapperPass>().getUniformityInfo();

  bool changed = false;

  bool hasStdExtZicond = ST.hasStdExtZicond();

  if (!hasStdExtZicond) {
    // Lower Select instructions into standard if-then-else branches
    SmallVector <SelectInst*, 4> selects;

    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto SI = dyn_cast<SelectInst>(&*I)) {
        if (UA_->isUniform(SI))
          continue;
        selects.emplace_back(SI);
      }
    }

    for (auto SI : selects) {
      auto BB = SI->getParent();
      LLVM_DEBUG(dbgs() << "*** unswitching divergent select instruction: " << *SI << "\n");
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
    SmallVector <MinMaxIntrinsic*, 4> MMs;

    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto MMI = dyn_cast<MinMaxIntrinsic>(&*I)) {
        if (UA_->isUniform(MMI))
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
      LLVM_DEBUG(dbgs() << "*** unswitching divergent min/max instruction: " << *MMI << "\n");
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
    LLVM_DEBUG(dbgs() << "*** after Pass0 changes!\n" << F << "\n");
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
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<UniformityInfoWrapperPass>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

void VortexBranchDivergence1::initialize(Function &F, const RISCVSubtarget &ST) {
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

  RI_ = &getAnalysis<RegionInfoPass>().getRegionInfo();
  LI_ = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  UA_ = &getAnalysis<UniformityInfoWrapperPass>().getUniformityInfo();
  DT_ = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT_= &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  namePrinter_.init(&F);
  replaceSuccessor_.init(&F);
  div_blocks_.clear();
  div_blocks_set_.clear();
  loops_.clear();
  loops_set_.clear();
}

bool VortexBranchDivergence1::runOnFunction(Function &F) {
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);

  this->initialize(F, ST);

  auto &Context = F.getContext();

  LLVM_DEBUG(UA_->print(dbgs()));

  LLVM_DEBUG(dbgs() << "*** Region info:\n");
  LLVM_DEBUG(RI_->getTopLevelRegion()->dump());
  LLVM_DEBUG(dbgs() << "\n");

  bool changed = false;

  for (auto I = df_begin(&F.getEntryBlock()),
            E = df_end(&F.getEntryBlock()); I != E; ++I) {
    auto BB = *I;

    auto Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br)
      continue;

    // only process conditional branches
    if (Br->isUnconditional()) {
      LLVM_DEBUG(dbgs() << "*** skip non-conditional branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    // only process divergent branches
    if (this->isUniform(Br)) {
      LLVM_DEBUG(dbgs() << "*** skip uniform branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    auto loop = LI_->getLoopFor(BB);
    if (loop) {
      auto ipdom = PDT_->findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
      if (ipdom && loop->contains(ipdom)) {
        if (div_blocks_set_.insert(BB).second) {
          // add new branch to the list
          LLVM_DEBUG(dbgs() << "*** divergent branch: " << namePrinter_.BBName(BB) << "\n");
          div_blocks_.push_back(BB);
        }
      } else {
        if (loops_set_.insert(loop).second) {
          // add new loop to the list
          LLVM_DEBUG(dbgs() << "*** divergent loop: " << namePrinter_.BBName(loop->getHeader()) << "\n");
          loops_.push_back(loop);
        }
      }
    } else {
      auto ipdom = PDT_->findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
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
        LLVM_DEBUG(dbgs() << "*** divergent branch: " << namePrinter_.BBName(BB) << ", IPDOM=" << namePrinter_.BBName(ipdom) << "\n");
        div_blocks_.push_back(BB);
      }
    }
  }

  // apply transformation
  if (!loops_.empty() || !div_blocks_.empty()) {
    LLVM_DEBUG(dbgs() << "*** before changes!\n" << F << "\n");

    // process the loop
    // This should be done first such that loop analysis is not tempered
    if (!loops_.empty()) {
      this->processLoops(&Context, &F);
      loops_.clear();
      // update PDT
      PDT_->recalculate(F);
    }

    // process branches
    if (!div_blocks_.empty()) {
      this->processBranches(&Context, &F);
      div_blocks_.clear();
    }

    changed = true;

    LLVM_DEBUG(dbgs() << "*** after changes!\n" << F << "\n");
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

    LLVM_DEBUG(dbgs() << "*** process loop: " << namePrinter_.BBName(header) << "\n");

    // save current thread mask in preheader
    auto tmask = CallInst::Create(tmask_func_, "tmask", preheader_br);
    LLVM_DEBUG(dbgs() << "*** backup thread mask '" << namePrinter_.ValueName(tmask) << "' before loop preheader branch: " << namePrinter_.BBName(preheader) << "\n");

    // restore thread mask at loop exit blocks
    {
      SmallVector <BasicBlock *, 8> exiting_blocks;
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

          LLVM_DEBUG(dbgs() << "*** insert thread predicate '" << namePrinter_.ValueName(cond) << "' before exiting block: " << namePrinter_.BBName(exiting_block) << "\n");
          if (!loop->contains(succ0)) {
            CallInst::Create(pred_n_func_, {cond, tmask}, "", branch);
          } else {
            CallInst::Create(pred_func_, {cond, tmask}, "", branch);
          }
          LLVM_DEBUG(dbgs() << "*** after predicate change!\n" << function << "\n");

          // change branch condition
          auto cond_i1 = ir_builder.CreateICmpNE(cond, ConstantInt::get(SizeTTy_, 0), namePrinter_.ValueName(cond) + ".to.i1");
          branch->setCondition(cond_i1);
        }
      }
    }
  }
}

void VortexBranchDivergence1::processBranches(LLVMContext* context, Function* function) {
  std::unordered_map<BasicBlock*, BasicBlock*> ipdoms;

  // pre-gather ipdoms for divergent branches
  for (auto BI = div_blocks_.rbegin(), BIE = div_blocks_.rend(); BI != BIE; ++BI) {
    auto block = *BI;
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);
    auto ipdom = PDT_->findNearestCommonDominator(branch->getSuccessor(0), branch->getSuccessor(1));
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
#ifndef NDEBUG
    auto region = RI_->getRegionFor(block);
    LLVM_DEBUG(dbgs() << "*** process branch " << namePrinter_.BBName(block) << ", region=" << region->getNameStr() << "\n");
#endif
    // insert a mov instruction before split
    IRBuilder<> ir_builder(branch);
    auto cond_orig = branch->getCondition();
    auto cond_orig_i1 = ir_builder.CreateICmpNE(cond_orig, ConstantInt::get(cond_orig->getType(), 0), namePrinter_.ValueName(cond_orig) + ".to.i1");
    auto cond_orig_i32 = ir_builder.CreateIntCast(cond_orig_i1, SizeTTy_, false, namePrinter_.ValueName(cond_orig_i1) + ".to.i32");
    auto cond = CallInst::Create(mov_func_, cond_orig_i32, namePrinter_.ValueName(cond_orig_i32) + ".mov", branch);

    // insert split instruction before divergent branch
    LLVM_DEBUG(dbgs() << "*** insert split '" << namePrinter_.ValueName(cond) << "' before " << namePrinter_.BBName(block) << "'s branch.\n");
    auto stack_ptr = CallInst::Create(split_func_, cond, "", branch);

    // change branch condition
    auto cond_i1 = ir_builder.CreateICmpNE(cond, ConstantInt::get(SizeTTy_, 0), namePrinter_.ValueName(cond) + ".to.i1");
    branch->setCondition(cond_i1);

    // insert a join stub block before ipdom
    auto stub = BasicBlock::Create(*context, "join_stub", function, ipdom);
    LLVM_DEBUG(dbgs() << "*** insert join stub '" << stub->getName() << "' before " << namePrinter_.BBName(ipdom) << "\n");
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

bool VortexBranchDivergence1::isUniform(Instruction *I) {
  return UA_->isUniform(I)
      || (I->getMetadata("structurizecfg.uniform") != nullptr);
}

///////////////////////////////////////////////////////////////////////////////

VortexBranchDivergence2::VortexBranchDivergence2(int PassMode)
  : MachineFunctionPass(ID)
  , PassMode_(PassMode)
{}

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
  const auto &ST = MF.getSubtarget<RISCVSubtarget>();
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
            LLVM_DEBUG(dbgs() << "*** Vortex: cleanup removed branches!\n");
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
          LLVM_DEBUG(dbgs() << "*** Vortex: fixed predicate opcode!\n");
          Changed = true;
          continue;
        }
      }
    }
    break;
  }

  if (Changed) {
    LLVM_DEBUG(dbgs() << "*** after changes!\n" << MF.getName() << "\n");
    LLVM_DEBUG(MF.dump(););
  }

  return false;
}

StringRef VortexBranchDivergence2::getPassName() const {
  return "VortexBranchDivergence2";
}

char VortexBranchDivergence2::ID = 0;

///////////////////////////////////////////////////////////////////////////////

PreservedAnalyses UniformAnnotationPass::run(Function &F, FunctionAnalysisManager &AM) {
  bool changed = false;

  std::vector<std::pair<Instruction*, bool>> uniformInsts;

  // collect uniform IR annoations
  for (auto& BB : F) {
    for (auto& I : BB) {
      // process metadata-based annotations
      if (I.getMetadata("vortex.uniform") != nullptr) {
        LLVM_DEBUG(dbgs() << "*** found metadata annotation: " << I << "\n");
        uniformInsts.push_back({&I, false}); // false = metadata annotation
        continue;
      }
      // process intrinsic-based annotations
      if (auto II = dyn_cast<IntrinsicInst>(&I)) {
        if (II->getIntrinsicID() == Intrinsic::var_annotation) {
          auto gv  = dyn_cast<GlobalVariable>(II->getOperand(1));
          auto cda = dyn_cast<ConstantDataArray>(gv->getInitializer());
          if (cda->getAsCString() == "vortex.uniform") {
            auto AnnotatedValue = dyn_cast<AllocaInst>(II->getOperand(0));
            if (AnnotatedValue) {
              LLVM_DEBUG(dbgs() << "*** found var_annotation: " << *AnnotatedValue << "\n");
              uniformInsts.push_back({AnnotatedValue, true}); // true = var_annotation annotation
            }
            continue;
          }
        }
      }
    }
  }

  // process annotated values
  for (auto Instr : uniformInsts) {
    if (Instr.second) {
      // handle var_annotation annotations
      auto AnnotatedValue = reinterpret_cast<AllocaInst*>(Instr.first);
      std::vector<LoadInst*> loadsToReplace;
      StoreInst* Store = nullptr;

      // find all loads and stores to the annotated stack variable
      for (auto User : AnnotatedValue->users()) {
        if (auto LI = dyn_cast<LoadInst>(User)) {
          loadsToReplace.push_back(LI);
        }
        if (auto SI = dyn_cast<StoreInst>(User)) {
          Store = SI; // the annotation has been applied to the stack variable
        }
      }
      // insert riscv_vx_uniform intrinsic before all uses of collected loads
      if (Store != nullptr) {
        IRBuilder<> Builder(Store->getNextNode());
        auto LoadedValue = Builder.CreateLoad(AnnotatedValue->getAllocatedType(), AnnotatedValue, AnnotatedValue->getName() + ".loaded");
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

DivergenceTracker::DivergenceTracker(const Function &function)
  : function_(&function)
  , initialized_(false)
{}

// Return true if 'From' can reach 'To' in the *instruction‑level* CFG
// (i.e. successor = next inst in same BB, or first inst of successor BB).
bool canReachInst(const Instruction *From, const Instruction *To) {
  SmallPtrSet<const Instruction*, 8> visited;
  SmallVector<const Instruction*, 8> worklist{From};

  while (!worklist.empty()) {
    auto I = worklist.pop_back_val();
    if (I == To) return true;
    if (!visited.insert(I).second) continue; // Skip already visited

    // 1) Check next instruction in same block
    if (auto Next = I->getNextNode()) {
      worklist.push_back(Next);
    }
    // 2) Handle terminators (branch to successor blocks)
    else if (I->isTerminator()) {
      for (auto Succ : successors(I->getParent())) {
        if (!Succ->empty()) {
          worklist.push_back(&Succ->front());
        }
      }
    }
  }
  return false;
}

void DivergenceTracker::initialize() {
  LLVM_DEBUG(dbgs() << "*** DivergenceTracker::initialize(): " << function_->getName() << "\n");

  initialized_ = true;

  auto module = function_->getParent();

  // Mark all TLS globals as divergent
  for (auto& GV : module->globals()) {
    if (GV.isThreadLocal()) {
      if (dv_nodes_.insert(&GV).second) {
        LLVM_DEBUG(dbgs() << "*** divergent global variable: " << GV << "\n");
      }
    }
  }

  // Mark uniform intrinsic calls as uniform
  for (auto& BB : *function_) {
    for (auto& I : BB) {
      if (auto II = dyn_cast<IntrinsicInst>(&I)) {
        if (II->getIntrinsicID() == Intrinsic::riscv_vx_uniform) {
          LLVM_DEBUG(dbgs() << "*** uniform intrinsic variable: " << I << "\n");
          uv_nodes_.insert(&I);
        }
      }
    }
  }

  // Build our per‐alloca taint map:
  this->buildAllocaTaints();

  // Mark tainted loads as divergent
  for (auto &BB : *function_) {
    for (auto &I : BB) {
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        if (this->loadIsDivergent(LI))
          dv_nodes_.insert(LI);
      }
    }
  }
}

void DivergenceTracker::buildAllocaTaints() {
  const DataLayout &DL = function_->getParent()->getDataLayout();

  for (auto &BB : *function_) {
    for (auto &I : BB) {
      // Handle both CallInst and InvokeInst via CallBase
      if (auto *CB = dyn_cast<CallBase>(&I)) {
        // If this is an LLVM intrinsic, skip the purely‑metadata ones:
        if (auto *II = dyn_cast<IntrinsicInst>(CB)) {
          switch (II->getIntrinsicID()) {
            case Intrinsic::lifetime_start:
            case Intrinsic::lifetime_end:
            case Intrinsic::var_annotation:
            case Intrinsic::ptr_annotation:
            case Intrinsic::invariant_start:
            case Intrinsic::invariant_end:
            case Intrinsic::dbg_declare:
            case Intrinsic::dbg_value:
              continue;
            default:
              break;
          }
        }

        for (Value *ArgVal : CB->args()) {
          if (!ArgVal->getType()->isPointerTy())
            continue;

          int64_t offset = 0;
          Value *Current = ArgVal;
          bool HasValidOffset = true;

          // Trace GEPs and accumulate offset
          while (auto *GEP = dyn_cast<GEPOperator>(Current)) {
            APInt GEPOffset(DL.getIndexSizeInBits(GEP->getPointerAddressSpace()), 0);
            if (!GEP->accumulateConstantOffset(DL, GEPOffset)) {
              HasValidOffset = false;
              break;
            }
            offset += GEPOffset.getSExtValue();
            Current = GEP->getPointerOperand();
          }

          if (auto *AI = dyn_cast<AllocaInst>(Current)) {
            Type     *ElemTy    = AI->getAllocatedType();
            uint64_t  elemSize  = DL.getTypeAllocSize(ElemTy);
            uint64_t  taintSize = elemSize;
            if (!HasValidOffset) {
              // unknown offset ⇒ taint *entire* allocation
              if (auto *CI = dyn_cast<ConstantInt>(AI->getArraySize())) {
                taintSize = elemSize * CI->getZExtValue();
              }
              // for dynamic alloca: conservatively taint one element
              offset = 0;
            }
            taints_[AI].push_back({ &I, (uint64_t)offset, taintSize });
            LLVM_DEBUG(dbgs() << "*** Tainted Allocation " << *AI << ", offset [" << offset << ":" << (offset+taintSize) << "] via call: " << I << "\n");
          }
        }
      }
    }
  }
}

bool DivergenceTracker::loadIsDivergent(const LoadInst *LI) {
  const DataLayout &DL = function_->getParent()->getDataLayout();

  // get the pointer and peel off casts/GEP with offset
  auto Ptr = LI->getPointerOperand();
  int64_t loadOffset;
  auto basePtr = GetPointerBaseWithConstantOffset(Ptr, loadOffset, DL, true);
  auto AI = dyn_cast<AllocaInst>(basePtr);
  if (!AI)
    return false;

  uint64_t loadSize = DL.getTypeAllocSize(LI->getType());
  auto &Entries = taints_[AI];

  for (auto &E : Entries) {
    // 1) check byte‐range overlap
    if (loadOffset <  E.offset + E.size &&
        loadOffset + loadSize >  E.offset) {
      // 2) ensure the escaping call can actually reach this load
      if (canReachInst(E.callInst, const_cast<LoadInst*>(LI))) {
        LLVM_DEBUG(dbgs() << "*** Tained Divergent Load " << *LI << " overlaps taint ["<<E.offset<<","<<E.offset+E.size<<")" << " from call " << *E.callInst << "\n");
        return true;
      }
    }
  }
  return false;
}

bool DivergenceTracker::isSourceOfDivergence(const Value *V) {
  if (!initialized_) {
    this->initialize();
  }

  // check if node always uniform
  if (this->isAlwaysUniform(V))
    return false;

  // Mark annotated divergent variables
  if (dv_nodes_.count(V) != 0) {
    LLVM_DEBUG(dbgs() << "*** divergent annotated variable: " << *V << "\n");
    return true;
  }

  // Atomics are divergent because they are executed sequentially: when an
  // atomic operation refers to the same address in each thread, then each
  // thread after the first sees the value written by the previous thread as
  // original value.
  if (isa<AtomicRMWInst>(V)
   || isa<AtomicCmpXchgInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent atomic variable: " << *V << "\n");
    return true;
  }

  // We conservatively assume all function arguments to potentially be divergent
  if (isa<Argument>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent function argument: " << *V << "\n");
    return true;
  }

  // We conservatively assume function return values are divergent
  if (isa<CallInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent CallInst variable: " << *V << "\n");
    return true;
  }

  // We conservatively assume function return values are divergent
  if (isa<InvokeInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent InvokeInst variable: " << *V << "\n");
    return true;
  }

  // we are not certain about the rest!
  LLVM_DEBUG(dbgs() << "*** unknown divergent state for variable: " << *V << "\n");
  return false;
}

bool DivergenceTracker::isAlwaysUniform(const Value *V) {
  if (!initialized_) {
    this->initialize();
  }

  // Mark annotated uniform variables
  if (uv_nodes_.count(V) != 0) {
    LLVM_DEBUG(dbgs() << "*** uniform annotated variable: " << *V << "\n");
    return true;
  }

  // we not certain about the rest!
  LLVM_DEBUG(dbgs() << "*** unknown uniform state for variable: " << *V << "\n");
  return false;
}

} // vortex
