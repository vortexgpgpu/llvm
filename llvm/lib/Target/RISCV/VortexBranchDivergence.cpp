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

#include "llvm/Analysis/LegacyDivergenceAnalysis.h"
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

#include <iostream>

using namespace vortex;
using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "vortex-branch-divergence"

#ifndef NDEBUG
#define LLVM_DEBUG(x) do {x;} while (false)
#endif

//#define USE_MI

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
  LegacyDivergenceAnalysis *DA_;

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

  LegacyDivergenceAnalysis *DA_;
  DominatorTree *DT_;
  PostDominatorTree *PDT_;
  LoopInfo *LI_;
  RegionInfo *RI_;

  Type* SizeTTy_;

  Function *tmask_func_;
  Function *pred_func_;
  Function *tmc_func_;
  Function *split_func_;
  Function *join_func_;

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
  std::string PassName_;

public: 
  static char ID; 
  VortexBranchDivergence2(const char* PassName);

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

FunctionPass *createVortexBranchDivergence2Pass(const char* PassName) {
  return new VortexBranchDivergence2(PassName);
}

}

INITIALIZE_PASS_BEGIN(VortexBranchDivergence0, "vortex-branch-divergence-0",
                "Vortex Branch Divergence Pre-Pass", false, false)
INITIALIZE_PASS_DEPENDENCY(LegacyDivergenceAnalysis)
INITIALIZE_PASS_END(VortexBranchDivergence0, "vortex-branch-divergence-0",
                    "Vortex Branch Divergence Pre-Pass", false, false)

INITIALIZE_PASS_BEGIN(VortexBranchDivergence1, "vortex-branch-divergence-1",
                      "Vortex Branch Divergence", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LegacyDivergenceAnalysis)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence1, "vortex-branch-divergence-1",
                    "Vortex Branch Divergence", false, false)

INITIALIZE_PASS(VortexBranchDivergence2, "VortexBranchDivergence-2",
                "Vortex Branch Divergence Post-Pass", false, false)

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
  AU.addRequired<LegacyDivergenceAnalysis>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

bool VortexBranchDivergence0::runOnFunction(Function &F) {
  // Check if the Vortex extension is enabled
  auto &Context = F.getContext();
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();  
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  if (!ST.hasExtVortex())
    return false;

  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling Pass0 ***\n");

  LLVM_DEBUG(dbgs() << "*** before Pass0 changes!\n" << F << "\n");

  DA_ = &getAnalysis<LegacyDivergenceAnalysis>();

  bool changed = false;  

  {
    // Lower Select instructions into standard if-then-else branches  
    SmallVector <SelectInst*, 4> selects; 
    
    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto SI = dyn_cast<SelectInst>(&*I)) {
        if (DA_->isUniform(SI))
          continue;
        LLVM_DEBUG(dbgs() << "*** unswitching divergent select instruction: " << *SI << "\n");
        selects.emplace_back(SI);
      }
    }

    for (auto SI : selects) {
      auto BB = SI->getParent();
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

  {
    // Lower Min/Max intrinsics into standard if-then-else branches  
    SmallVector <MinMaxIntrinsic*, 4> MMs; 
    
    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto MMI = dyn_cast<MinMaxIntrinsic>(&*I)) {
        if (DA_->isUniform(MMI))
          continue;
        LLVM_DEBUG(dbgs() << "*** unswitching divergent min/max instruction: " << *MMI << "\n");
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
  return "Vortex Handle Branch Divergence";
}

void VortexBranchDivergence1::getAnalysisUsage(AnalysisUsage &AU) const {  
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<RegionInfoPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<LegacyDivergenceAnalysis>();
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

  tmask_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmask, {SizeTTy_});
  pred_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred, {SizeTTy_, SizeTTy_});
  tmc_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmc, {SizeTTy_});
  split_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split, {SizeTTy_, SizeTTy_});
  join_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_join, {SizeTTy_}); 

  RI_ = &getAnalysis<RegionInfoPass>().getRegionInfo();
  LI_ = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DA_ = &getAnalysis<LegacyDivergenceAnalysis>();
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
  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling ***\n");

  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);  

  this->initialize(F, ST);

  auto &Context = F.getContext();

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

  return changed;
}

void VortexBranchDivergence1::processLoops(LLVMContext* context, Function* function) {
  DenseSet<const BasicBlock *> stub_blocks;

  // traverse the list in reverse order
  for (auto it = loops_.rbegin(), ite = loops_.rend(); it != ite; ++it) {
    auto loop = *it;
    auto header = loop->getHeader();

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
        int exit_edges = false;        
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
          auto cond = branch->getCondition();
          auto not_cond = ir_builder.CreateNot(cond, namePrinter_.ValueName(cond) + ".not");
          auto not_cond_i32 = ir_builder.CreateIntCast(not_cond, SizeTTy_, false, namePrinter_.ValueName(not_cond) + ".i32");
          LLVM_DEBUG(dbgs() << "*** insert thread predicate '" << namePrinter_.ValueName(not_cond_i32) << "' before exiting block: " << namePrinter_.BBName(exiting_block) << "\n");          
          CallInst::Create(pred_func_, {not_cond_i32, tmask}, "", branch);
          
          /*// restore thread mask before corresponding exit blocks          
          auto stub = BasicBlock::Create(*context, "loop_exit_stub", function, succ);
          LLVM_DEBUG(dbgs() << "*** restore thread mask in stub '" << stub->getName() << "' before exit block: " << namePrinter_.BBName(succ) << "\n");
          stub_blocks.insert(stub);
          auto stub_br = BranchInst::Create(succ, stub);          
          bool found = replaceSuccessor_.replaceSuccessor(exiting_block, succ, stub);
          if (!found) {
            std::abort();
          }
          CallInst::Create(tmc_func_, tmask, "", stub_br);*/
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
    // insert split instruction before divergent branch      
    IRBuilder<> ir_builder(branch);
    auto cond = branch->getCondition();
    auto cond_i32 = ir_builder.CreateIntCast(cond, SizeTTy_, false, namePrinter_.ValueName(cond) + ".i32");
    LLVM_DEBUG(dbgs() << "*** insert split '" << namePrinter_.ValueName(cond_i32) << "' before " << namePrinter_.BBName(block) << "'s branch.\n");
    auto stack_ptr = CallInst::Create(split_func_, cond_i32, "", branch);

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
  return DA_->isUniform(I)
      || (I->getMetadata("structurizecfg.uniform") != nullptr);
}

///////////////////////////////////////////////////////////////////////////////

DivergenceTracker::DivergenceTracker(const Function &function) 
  : function_(&function)
  , initialized_(false) 
{}

void DivergenceTracker::initialize() {
  LLVM_DEBUG(dbgs() << "*** DivergenceTracker::initialize(): " << function_->getName() << "\n");

  initialized_ = true;

  DenseSet<const Value *> dv_annotations;
  DenseSet<const Value *> uv_annotations;

  auto module = function_->getParent();

  // Mark all TLS globals as divergent
  for (auto& GV : module->getGlobalList()) {
    if (GV.isThreadLocal()) {
      if (dv_nodes_.insert(&GV).second) {
        LLVM_DEBUG(dbgs() << "*** divergent global variable: " << GV.getName() << "\n");
      }
    }
  }

  for (auto& BB : *function_) {
    for (auto& I : BB) {      
      //LLVM_DEBUG(dbgs() << "*** instruction: opcode=" << I.getOpcodeName() << ", name=" << I.getName() << "\n");
      if (I.getMetadata("vortex.uniform") != NULL) {
        uv_annotations.insert(&I);
        uv_nodes_.insert(&I);
        LLVM_DEBUG(dbgs() << "*** uniform annotation: " << I.getName() << "\n");
      } else
      if (I.getMetadata("vortex.divergent") != NULL) {
        dv_annotations.insert(&I);
        dv_nodes_.insert(&I);
        LLVM_DEBUG(dbgs() << "*** divergent annotation: " << I.getName() << "\n");
      } else
      if (auto II = dyn_cast<llvm::IntrinsicInst>(&I)) {
        if (II->getIntrinsicID() == llvm::Intrinsic::var_annotation) {
          auto gv  = dyn_cast<GlobalVariable>(II->getOperand(1));
          auto cda = dyn_cast<ConstantDataArray>(gv->getInitializer());
          if (cda->getAsCString() == "vortex.uniform") {
            Value* var_src = nullptr;
            auto var = II->getOperand(0);            
            if (auto AI = dyn_cast<AllocaInst>(var)) {
              var_src = AI;               
              LLVM_DEBUG(dbgs() << "*** uniform annotation: " << AI->getName() << ".src(" << var_src << ")\n");        
            } else
            if (auto CI = dyn_cast<CastInst>(var)) {
              var_src = CI->getOperand(0);
              LLVM_DEBUG(dbgs() << "*** uniform annotation: " << CI->getName() << ".src(" << var_src << ")\n");     
            }
            uv_annotations.insert(var_src);
            uv_nodes_.insert(var_src);            
          } else
          if (cda->getAsCString() == "vortex.divergent") {
            Value* var_src = nullptr;
            auto var = II->getOperand(0);            
            if (auto AI = dyn_cast<AllocaInst>(var)) {
              var_src = AI;
              LLVM_DEBUG(dbgs() << "*** uniform annotation: " << AI->getName() << ".src(" << var_src << "\n");                           
            } else
            if (auto CI = dyn_cast<CastInst>(var)) {
              var_src = CI->getOperand(0);
              LLVM_DEBUG(dbgs() << "*** uniform annotation: " << CI->getName() << ".src(" << var_src << "\n");                          
            }
            dv_annotations.insert(var_src);
            dv_nodes_.insert(var_src);
          }
        }
      }
    }
  }

  // Mark the value of divergent stores as divergent
  for (auto& BB : *function_) {
    for (auto& I : BB) {
      if (auto GE = dyn_cast<GetElementPtrInst>(&I)) {
        auto addr = GE->getPointerOperand();
        if (uv_annotations.count(addr) != 0) {
          LLVM_DEBUG(dbgs() << "*** uniform annotation: " << GE->getName() << "\n");
          uv_nodes_.insert(GE);
        } else
        if (dv_annotations.count(addr) != 0) {
          LLVM_DEBUG(dbgs() << "*** divergent annotation: " << GE->getName() << "\n");
          dv_nodes_.insert(GE);
        }
      } else
      if (auto SI = dyn_cast<StoreInst>(&I)) {
        auto addr = SI->getPointerOperand();
        if (uv_annotations.count(addr) != 0) {
          auto value = SI->getValueOperand();
          if (auto CI = dyn_cast<CastInst>(value)) {
            LLVM_DEBUG(dbgs() << "*** uniform annotation: " << CI->getName() << ".src\n");
            auto src = CI->getOperand(0);
            uv_nodes_.insert(src);
          } else {
            LLVM_DEBUG(dbgs() << "*** uniform annotation: " << SI->getName() << ".value\n");
            uv_nodes_.insert(value);
          }
        } else
        if (dv_annotations.count(addr) != 0) {
          auto value = SI->getValueOperand();
          if (auto CI = dyn_cast<CastInst>(value)) {
            LLVM_DEBUG(dbgs() << "*** divergent annotation: " << CI->getName() << ".src\n");
            auto src = CI->getOperand(0);
            dv_nodes_.insert(src);
          } else {
            LLVM_DEBUG(dbgs() << "*** divergent annotation: " << SI->getName() << ".value\n");
            dv_nodes_.insert(value);
          }
        }
      }
    }
  }
}

bool DivergenceTracker::eval(const Value *V) {
  if (!initialized_) {
    this->initialize();    
  }

  // Mark variable as uniform is specified via aannotation
  if (uv_nodes_.count(V) != 0) {
    LLVM_DEBUG(dbgs() << "*** uniform annotated variable: " << V->getName() << "\n");
    return false;
  }

  // Mark variable with divergent is detected as TLS
  if (dv_nodes_.count(V) != 0) {
    LLVM_DEBUG(dbgs() << "*** divergent annotated variable: " << V->getName() << "\n");
    return true;
  }

  // We conservatively assume all function arguments to potentially be divergent
  if (isa<Argument>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent function argument: " << V->getName() << "\n");
    return true;
  }

  // We conservatively assume function return values are divergent
  if (isa<CallInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent return variable: " << V->getName() << "\n");
    return true;
  }

  // Atomics are divergent because they are executed sequentially: when an
  // atomic operation refers to the same address in each thread, then each
  // thread after the first sees the value written by the previous thread as
  // original value.
  if (isa<AtomicRMWInst>(V)
   || isa<AtomicCmpXchgInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent atomic variable: " << V->getName() << "\n");
    return true;
  }

  // Mark loads from divergent addresses as divergent
  if (auto LD = dyn_cast<LoadInst>(V)) {
    auto addr = LD->getPointerOperand();
    if (dv_nodes_.count(addr) != 0) {
      LLVM_DEBUG(dbgs() << "*** divergent load variable: " << V->getName() << "\n");
      return true;
    }
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////

VortexBranchDivergence2::VortexBranchDivergence2(const char* PassName) 
  : MachineFunctionPass(ID)
  , PassName_(PassName)
{}

bool VortexBranchDivergence2::runOnMachineFunction(MachineFunction &MF) {
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  bool Changed = false;

#ifdef USE_MI

  for (auto &MBB : MF) {      
    MachineInstr *predInstr = nullptr;
    int distance = 0;
    for (auto &MI : MBB) {
      if (MI.getOpcode() == RISCV::VX_PRED) {
        if (predInstr) {          
          LLVM_DEBUG(dbgs() << "*** duplicate VX_PRED! distance=" << distance << "\n");
          predInstr->removeFromParent();
        }
        predInstr = &MI;
        distance = 0;
      }
      if (predInstr && MI.isBranch()) {
        if (distance != 1) {
          LLVM_DEBUG(dbgs() << "*** moving VX_PRED! distance=" << distance << "\n");
          predInstr->removeFromParent();
          MBB.insert(MI.getIterator(), predInstr);
        } else {
          LLVM_DEBUG(dbgs() << "*** correct VX_PRED distance=" << distance << "\n");
        }
        predInstr = nullptr;
      }
      ++distance;
      
      /*if (!MI.isBranch())
        continue;
      if (!MI.isConditionalBranch())
        continue;
      auto &Cond = MI.getOperand(0);
      if (!Cond.isReg())
        continue;
      auto BB = MBB.getBasicBlock();
      if (!BB)
        continue;
      auto Term = BB->getTerminator();
      if (Term->getMetadata("Predicate") == nullptr)
        continue;
      LLVM_DEBUG(dbgs() << "*** found Predicate! in " << PassName_ << "\n");
      auto &DL = MI.getDebugLoc();
      auto NotCondReg = MF.getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);
      auto CondReg = Cond.getReg();
      BuildMI(MBB, MI, DL, TII->get(RISCV::SLTIU), NotCondReg)
        .addReg(CondReg)
        .addImm(1);
      BuildMI(MBB, MI, DL, TII->get(RISCV::VX_PRED))
        .addReg(NotCondReg);*/

      Changed = true;
    }
    if (predInstr) {
      LLVM_DEBUG(dbgs() << "*** unused VX_PRED! distance=" << distance << "\n");
      predInstr->removeFromParent();      
    }
  }

  /*for (auto &MBB : MF) {      
    MachineInstr *splitInstr = nullptr;
    int distance = 0;
    for (auto &MI : MBB) {
      if (MI.getOpcode() == RISCV::VX_SPLIT) {
        splitInstr = &MI;
        distance = 0;
      }
      if (splitInstr && MI.isBranch()) {
        assert(MI.isConditionalBranch());
        auto &Cond = MI.getOperand(0);
        assert(Cond.isReg());        
        if (distance != 1) {
          LLVM_DEBUG(dbgs() << "*** moving VX_SPLIT! distance=" << distance << "\n");
          splitInstr->removeFromParent();
          MBB.insert(MI.getIterator(), splitInstr);
        } else {
          LLVM_DEBUG(dbgs() << "*** correct VX_SPLIT distance=" << distance << "\n");
        }
        //auto CondReg = Cond.getReg();
        //splitInstr->getOperand(0).setReg(CondReg);
        splitInstr = nullptr;
      }
      ++distance;
      Changed = true;
    }
  }*/

#endif

  return Changed;
}

StringRef VortexBranchDivergence2::getPassName() const { 
  return "VortexBranchDivergence2"; 
}

char VortexBranchDivergence2::ID = 0;

} // vortex
