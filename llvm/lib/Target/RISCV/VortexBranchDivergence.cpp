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

using namespace vortex;
using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "vortex-branch-divergence"

#ifndef NDEBUG
#define LLVM_DEBUG(x) do {x;} while (false)
#endif

namespace vortex {

static BasicBlock* SplitBasicBlockBefore(BasicBlock* BB, BasicBlock::iterator I, const Twine &BBName) {
  assert(BB->getTerminator() &&
         "Can't use splitBasicBlockBefore on degenerate BB!");
  assert(I != BB->end() &&
         "Trying to get me to create degenerate basic block!");

  assert((!isa<PHINode>(*I) || BB->getSinglePredecessor()) &&
         "cannot split on multi incoming phis");

  BasicBlock *New = BasicBlock::Create(BB->getContext(), BBName, BB->getParent(), BB);
  // Save DebugLoc of split point before invalidating iterator.
  DebugLoc Loc = I->getDebugLoc();
  // Move all of the specified instructions from the original basic block into
  // the new basic block.
  New->getInstList().splice(New->end(), BB->getInstList(), BB->begin(), I);

  // Loop through all of the predecessors of the 'this' block (which will be the
  // predecessors of the New block), replace the specified successor 'this'
  // block to point at the New block and update any PHI nodes in 'this' block.
  // If there were PHI nodes in 'this' block, the PHI nodes are updated
  // to reflect that the incoming branches will be from the New block and not
  // from predecessors of the 'this' block.
  SmallVector<BasicBlock *, 32> preds(predecessors(BB));
  for (BasicBlock *Pred : preds) {
    Instruction *TI = Pred->getTerminator();
    TI->replaceSuccessorWith(BB, New);
    BB->replacePhiUsesWith(Pred, New);
  }
  // Add a branch instruction from  "New" to "this" Block.
  BranchInst *BI = BranchInst::Create(BB, New);
  BI->setDebugLoc(Loc);

  return New;
}

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
        dbgs() << "(" << BB->front() << ")";
      }
    }
    return str;
  }
};

///////////////////////////////////////////////////////////////////////////////

class VortexBranchDivergence : public FunctionPass {
private:

  LegacyDivergenceAnalysis *DA_;
  DominatorTree *DT_;
  PostDominatorTree *PDT_;
  LoopInfo *LI_;
  RegionInfo *RI_;

  std::vector<BasicBlock*> div_blocks_;
  std::vector<Loop*> loops_;

  NamePrinter namePrinter_;

  Function *tmask_func_;
  Function *pred_func_;
  Function *tmc_func_;
  Function *split_func_;
  Function *join_func_;

  void initialize(Module &M, const RISCVSubtarget &ST);

  void processBranches(LLVMContext* context, Function* function);

  void processLoops(LLVMContext* context, Function* function);

public:

  static char ID;

  VortexBranchDivergence();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;
};

}

///////////////////////////////////////////////////////////////////////////////

namespace llvm {

void initializeVortexBranchDivergencePass(PassRegistry &);

FunctionPass *createVortexBranchDivergencePass() {
  return new VortexBranchDivergence();
}

}

INITIALIZE_PASS_BEGIN(VortexBranchDivergence, "vortex-branch-divergence-3",
                      "Vortex Branch Divergence", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LegacyDivergenceAnalysis)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence, "vortex-branch-divergence-3",
                    "Vortex Branch Divergence", false, false)

namespace vortex {

char VortexBranchDivergence::ID = 1;

VortexBranchDivergence::VortexBranchDivergence() : FunctionPass(ID) {
  initializeVortexBranchDivergencePass(*PassRegistry::getPassRegistry());
}

StringRef VortexBranchDivergence::getPassName() const { 
  return "Vortex Handle Branch Divergence";
}

void VortexBranchDivergence::getAnalysisUsage(AnalysisUsage &AU) const {  
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequiredID(LoopSimplifyID);
  AU.addRequired<RegionInfoPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<LegacyDivergenceAnalysis>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

void VortexBranchDivergence::initialize(Module &M, const RISCVSubtarget &ST) {
  tmask_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmask);
  pred_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred);
  tmc_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmc);
  split_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split);
  join_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_join);  
}

bool VortexBranchDivergence::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling ***\n");

  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);  

  this->initialize(*F.getParent(), ST);

  auto &context = F.getContext();

  RI_ = &getAnalysis<RegionInfoPass>().getRegionInfo();
  LI_ = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DA_ = &getAnalysis<LegacyDivergenceAnalysis>();
  DT_ = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT_= &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  namePrinter_.init(&F);

  LLVM_DEBUG(dbgs() << "*** Region info:\n");
  LLVM_DEBUG(RI_->getTopLevelRegion()->dump());
  LLVM_DEBUG(dbgs() << "\n");

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
    if (DA_->isUniform(Br)) {
      LLVM_DEBUG(dbgs() << "*** skip uniform branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    auto ipdom = PDT_->findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
    if (ipdom == nullptr) {
      LLVM_DEBUG(dbgs() << "*** WARNING: block with no IPDOM:\n" << BB << "\n");
      continue;
    }

    auto loop = LI_->getLoopFor(BB);
    if (loop) {
      if (std::find(loops_.begin(), loops_.end(), loop) == loops_.end()) {
        // add new loop to the list
        LLVM_DEBUG(dbgs() << "*** divergent loop: " << namePrinter_.BBName(loop->getHeader()) << "\n");
        loops_.push_back(loop);
      }

      // check if the block is a nested branch
      if (loop->contains(ipdom)) {
        // add new branch to the list
        LLVM_DEBUG(dbgs() << "*** divergent branch: " << namePrinter_.BBName(BB) << "\n");
        div_blocks_.push_back(BB);
      }
    } else {
      // add new branch to the list
      LLVM_DEBUG(dbgs() << "*** divergent branch: " << namePrinter_.BBName(BB) << "\n");
      div_blocks_.push_back(BB);
    }
  }

  // apply transformation
  if (!loops_.empty() || !div_blocks_.empty()) {
    LLVM_DEBUG(dbgs() << "*** before changes!\n" << F << "\n");

    // process the loop
    // This should be done first such that loop analysis is not tempered
    if (!loops_.empty()) {
      this->processLoops(&context, &F);      
      loops_.clear();
      // update PDT
      PDT_->recalculate(F);
    }

    // process branches
    if (!div_blocks_.empty()) {
      this->processBranches(&context, &F);
      div_blocks_.clear();    
    }    

    LLVM_DEBUG(dbgs() << "*** after changes!\n" << F << "\n");
  }

  return true;
}

void VortexBranchDivergence::processLoops(LLVMContext* context, Function* function) {
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
    LLVM_DEBUG( dbgs() << "*** backup loop's tmask before preheader branch: " << namePrinter_.BBName(preheader) << "\n");
    auto tmask = CallInst::Create(tmask_func_, "tmask", preheader_br);
    
    // restore thread mask at loop exit blocks
    {
      SmallVector <BasicBlock *, 8> exiting_blocks;
      loop->getExitingBlocks(exiting_blocks); // blocks inside the loop going out

      for (auto exiting_block : exiting_blocks) {
        bool predicated = false;        
        auto branch = dyn_cast<BranchInst>(exiting_block->getTerminator());
        for (auto succ : branch->successors()) {
          // stub blocks insertion will add invalid exiting blocks to the loops
          // because we are not re-executing loop analysis.
          // we just eliminate those exiting blocks
          if (loop->contains(succ) 
           || stub_blocks.count(succ) != 0)
            continue;          
          if (!predicated) {
            predicated = true;
            // insert a predicate instruction to mask out threads that are exiting the loop            
            LLVM_DEBUG(dbgs() << "*** insert loop's predicate before exiting block: " << namePrinter_.BBName(exiting_block) << " -> " << namePrinter_.BBName(succ) <<  "\n");          
            IRBuilder<> ir_builder(branch);
            auto cond = branch->getCondition();
            auto not_cond = ir_builder.CreateNot(cond, namePrinter_.ValueName(cond) + ".not");
            auto not_cond_i32 = ir_builder.CreateIntCast(not_cond, ir_builder.getInt32Ty(), false, namePrinter_.ValueName(not_cond) + ".i32");
            CallInst::Create(pred_func_, not_cond_i32, "", branch);            
          }

          // restore thread mask before corresponding exit blocks
          auto stub = SplitBasicBlockBefore(succ, succ->getFirstInsertionPt(), "loop_exit_stub");
          stub_blocks.insert(stub);
          LLVM_DEBUG(dbgs() << "*** restore loop's tmask in stub '" << stub->getName() << "' before exit block: " << namePrinter_.BBName(succ) << "\n");  
          CallInst::Create(tmc_func_, tmask, "", &stub->back());            
        }
      }
    }
  }
}

void VortexBranchDivergence::processBranches(LLVMContext* context, Function* function) {  
  std::unordered_map<BasicBlock*, BasicBlock*> ipdoms;

  // pre-gather ipdoms for divergent branches
  for (auto BI = div_blocks_.rbegin(), BIE = div_blocks_.rend(); BI != BIE; ++BI) {
    auto block = *BI;
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);
    auto ipdom = PDT_->findNearestCommonDominator(branch->getSuccessor(0), branch->getSuccessor(1));
    assert(ipdom);
    ipdoms[block] = ipdom;
  }

  // traverse the list in reverse order
  for (auto BI = div_blocks_.rbegin(), BIE = div_blocks_.rend(); BI != BIE; ++BI) {
    auto block = *BI;
    auto ipdom = ipdoms[block];
    assert(ipdom);
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);

    auto region = RI_->getRegionFor(block);

    LLVM_DEBUG(dbgs() << "*** process branch " << namePrinter_.BBName(block) << ", region=" << region->getNameStr() << "\n");

    // insert split instruction before divergent branch
    LLVM_DEBUG(dbgs() << "*** insert split before " << namePrinter_.BBName(block) << "'s branch.\n");
    IRBuilder<> ir_builder(branch);
    auto cond = branch->getCondition();
    auto cond_i32 = ir_builder.CreateIntCast(cond, ir_builder.getInt32Ty(), false, namePrinter_.ValueName(cond) + ".i32");
    CallInst::Create(split_func_, cond_i32, "", branch);

    // insert a join stub block before    
    {
      SmallVector<BasicBlock *, 32> preds(predecessors(ipdom));
      LLVM_DEBUG(dbgs() << "*** number of predecessors of " << namePrinter_.ValueName(ipdom) << " = "<< preds.size() << "\n");  
    }
    auto stub = SplitBasicBlockBefore(ipdom, ipdom->getFirstInsertionPt(), "join_stub");
    LLVM_DEBUG(dbgs() << "*** insert join stub " << stub->getName() << " before " << namePrinter_.BBName(ipdom) << "\n");
    CallInst::Create(join_func_, "", &stub->back());
  }
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
      LLVM_DEBUG(dbgs() << "*** instruction: opcode=" << I.getOpcodeName() << ", name=" << I.getName() << "\n");
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
          auto gep = dyn_cast<ConstantExpr>(II->getOperand(1));
          auto gv  = dyn_cast<GlobalVariable>(gep->getOperand(0));
          auto cda = dyn_cast<ConstantDataArray>(gv->getInitializer());
          if (cda->getAsCString() == "vortex.uniform") {
            auto var = II->getOperand(0);
            if (auto AI = dyn_cast<AllocaInst>(var)) {              
              uv_annotations.insert(var);
              uv_nodes_.insert(var);
              LLVM_DEBUG(dbgs() << "*** uniform annotation: " << AI->getName() << "\n");
            } else
            if (auto CI = dyn_cast<CastInst>(var)) {
              auto var2 = CI->getOperand(0);
              uv_annotations.insert(var2);
              uv_nodes_.insert(var2);
              LLVM_DEBUG(dbgs() << "*** uniform annotation: " << var2->getName() << "\n");
            }
          } else
          if (cda->getAsCString() == "vortex.divergent") {
            auto var = II->getOperand(0);
            if (auto AI = dyn_cast<AllocaInst>(var)) {
              dv_annotations.insert(var);
              dv_nodes_.insert(var);
              LLVM_DEBUG(dbgs() << "*** divergent annotation: " << AI->getName() << "\n");
            } else
            if (auto CI = dyn_cast<CastInst>(var)) {
              auto var2 = CI->getOperand(0);
              dv_annotations.insert(var2);
              dv_nodes_.insert(var2);
              LLVM_DEBUG(dbgs() << "*** divergent annotation: " << var2->getName() << "\n");
            }
          }
        }
      }
    }
  }

  // Mark loads of divergent stores as divergent
  for (auto& BB : *function_) {
    for (auto& I : BB) {
      if (auto SI = dyn_cast<StoreInst>(&I)) {
        auto addr = SI->getPointerOperand();
        if (uv_annotations.count(addr) != 0) {
          auto value = SI->getValueOperand();
          if (auto CI = dyn_cast<CastInst>(value)) {
            auto src = CI->getOperand(0);
            uv_nodes_.insert(src);
          } else {
            uv_nodes_.insert(value);
          }
        } else
        if (dv_annotations.count(addr) != 0) {
          auto value = SI->getValueOperand();
          if (auto CI = dyn_cast<CastInst>(value)) {
            auto src = CI->getOperand(0);
            dv_nodes_.insert(src);
          } else {
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

} // vortex