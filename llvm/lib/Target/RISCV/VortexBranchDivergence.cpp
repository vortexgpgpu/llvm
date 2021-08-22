#include "VortexBranchDivergence.h"

#include "RISCV.h"
#include "RISCVSubtarget.h"
#include "llvm/IR/IntrinsicsRISCV.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Support/Debug.h"

/*
#include "HARPTargetMachine.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ADT/StringExtras.h"

#include "llvm/IR/Dominators.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <vector>
#include <queue>*/

using namespace vortex;
using namespace llvm;

// CSR thread Id
#define CSR_WTID 0xCC0

#define DEBUG_TYPE "vortex-branch-divergence"

namespace llvm {

void initializeVortexBranchDivergencePass(PassRegistry &);

FunctionPass *createVortexBranchDivergencePass() {
  return new VortexBranchDivergence();
}

}

INITIALIZE_PASS_BEGIN(VortexBranchDivergence, DEBUG_TYPE,
                      "Vortex Branch Divergence Handling", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LegacyDivergenceAnalysis)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence, DEBUG_TYPE,
                    "Vortex Branch Divergence Handling", false, false)

///////////////////////////////////////////////////////////////////////////////

namespace vortex {

char VortexBranchDivergence::ID = 0;

VortexBranchDivergence::VortexBranchDivergence() : FunctionPass(ID) {
  initializeVortexBranchDivergencePass(*PassRegistry::getPassRegistry());
}

StringRef VortexBranchDivergence::getPassName() const { 
  return "Vortex Handle Branch Divergence";
}

void VortexBranchDivergence::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
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
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();

  // Check if the Vortex extension is enabled
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  if (!ST.hasExtVortex())
    return false;

  dbgs() << "*** Vortex Divergent Branch Annotation ***\n";

  LI_ = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DA_ = &getAnalysis<LegacyDivergenceAnalysis>();
  DT_ = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT_= &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree(); 

  this->initialize(*F.getParent(), ST); 

  dbgs() << "*** before changes!\n" << F << "\n";

  bool changed = false;

  for (auto I = df_begin(&F.getEntryBlock()),
            E = df_end(&F.getEntryBlock()); I != E; ++I) {
    auto BB = *I;

    auto Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br) {
      dbgs() << "*** skip no branch: " << BB->getName() << "\n";
      continue;
    }

    // only process conditional branches
    if (Br->isUnconditional()) {
      dbgs() << "*** skip non conditional branch: " << BB->getName() << "\n";
      continue;
    }

    // only process divergent branches
    if (DA_->isUniform(Br)) {
      dbgs() << "*** skip non divergent branch: " << BB->getName() << "\n";
      continue;
    }
    
    auto L = LI_->getLoopFor(BB);
    if (L && L->isLoopLatch(BB)) {
      if ( std::find(loops_.begin(), loops_.end(), L) == loops_.end())
        loops_.push_back(L);
      dbgs() << "*** divergent loop latch: " << BB->getName() << ", loop=" << L->getHeader()->getName() << "\n";
      continue;
    }

    auto succ0 = Br->getSuccessor(0);
    auto succ1 = Br->getSuccessor(1);
    auto ipdom = PDT_->findNearestCommonDominator(succ0, succ1);

    splits_.push_back({BB, ipdom});

    dbgs() << "*** divergent branch: " << BB->getName() << ", ipdom=" << ipdom->getName() << "\n";

    changed = true;
  }

  if (changed) {
    // apply transformation
    auto &context = F.getContext();    
    this->processSplitJoins(&context, &F);
    this->processLoops(&context, &F);
  }

  if (changed) {
    dbgs() << "*** after changes!\n" << F << "\n";
  }

  splits_.clear();
  loops_.clear();
  stub_blocks_.clear();
  
  return true;
}

void VortexBranchDivergence::processSplitJoins(LLVMContext* context, Function* function) {
  // traverse the list in reverse to handle nested splits first
  for (auto it = splits_.rbegin(), ite = splits_.rend(); it != ite; ++it) {
    auto block = it->first;
    auto ipdom = it->second;

    dbgs() << "*** insert split/join: " << block->getName() << "\n";
  
    auto branch = dyn_cast<BranchInst>(block->getTerminator());

    // insert splits before divergent branch    
    IRBuilder<> ir_builder(branch);
    auto cond = branch->getCondition();
    auto cond_i32 = ir_builder.CreateIntCast(cond, ir_builder.getInt32Ty(), false, cond->getName() + ".i32");
    CallInst::Create(split_func_, cond_i32, "", branch);

    // insert join before ipdom
    auto stub = BasicBlock::Create(*context, "join_stub", function, ipdom);
    auto stub_br = BranchInst::Create(ipdom, stub);
    CallInst::Create(join_func_, "", stub_br);

    this->recurseReplaceSuccessor(block, ipdom, stub);
    
    PDT_->addNewBlock(stub, ipdom);
  }
}

void VortexBranchDivergence::processLoops(LLVMContext* context, Function* function) {
  // traverse the list in reverse to handle nested loops first
   for (auto it = loops_.rbegin(), ite = loops_.rend(); it != ite; ++it) {
    auto loop = *it;

    auto header = loop->getHeader();

    auto preheader = loop->getLoopPreheader();
    assert(preheader);

    auto preheader_br = dyn_cast<BranchInst>(preheader->getTerminator());
    assert(preheader_br);

    dbgs() << "*** process loop: " << header->getName() << "\n";

    BasicBlock* exit_block = NULL;
    {
      SmallVector <BasicBlock *, 8> exit_blocks;
      loop->getUniqueExitBlocks(exit_blocks);
      for (auto block : exit_blocks) {
        if (exit_block == NULL) {
          exit_block = block;
        } else {
          exit_block = PDT_->findNearestCommonDominator(exit_block, block);
        }
      }
      assert(exit_block);
      dbgs() << "****** common loop exit_block: " << exit_block->getName() << "\n";
   }

    // save current thread mask in preheader
    auto tmask = CallInst::Create(tmask_func_, "tmask", preheader_br);

    // restore thread mask before exit block
    auto loop_exit_stub = BasicBlock::Create(*context, "loop_exit_stub", function, exit_block);
    {      
      auto loop_exit_stub_br = BranchInst::Create(exit_block, loop_exit_stub);
      PDT_->addNewBlock(loop_exit_stub, exit_block);      
      CallInst::Create(tmc_func_, tmask, "", loop_exit_stub_br);            
    }

    // set predicate mask for each backedge
    {
      SmallVector <BasicBlock *, 8> latches;
      loop->getLoopLatches(latches);
      for (auto latch : latches) {
        auto latch_br = dyn_cast<BranchInst>(latch->getTerminator());
        dbgs() << "****** update loop latch: " << latch->getName() << "\n";
        if (latch_br && !DA_->isUniform(latch_br)) {
          IRBuilder<> ir_builder(latch_br);
          auto cond = latch_br->getCondition();
          auto not_cond = ir_builder.CreateNot(cond, cond->getName() + ".not");
          auto not_cond_i32 = ir_builder.CreateIntCast(not_cond, ir_builder.getInt32Ty(), false, not_cond->getName() + ".i32");
          CallInst::Create(pred_func_, not_cond_i32, "", latch_br);
          this->recurseReplaceSuccessor(latch, exit_block, loop_exit_stub);
        }
      }
    }
  }
}

void VortexBranchDivergence::recurseReplaceSuccessor(BasicBlock* start, 
                                                     BasicBlock* oldBB, 
                                                     BasicBlock* newBB) {
                    
  stub_blocks_.insert(newBB);
  DenseSet<const BasicBlock *> visited(stub_blocks_);
  this->recurseReplaceSuccessor(visited, start, oldBB, newBB);
}

void VortexBranchDivergence::recurseReplaceSuccessor(DenseSet<const BasicBlock *>& visited, 
                                                     BasicBlock* start, 
                                                     BasicBlock* oldBB, 
                                                     BasicBlock* newBB) {
  visited.insert(start);
  auto branch = dyn_cast<BranchInst>(start->getTerminator());
  if (branch) {
    for (unsigned i = 0, n = branch->getNumSuccessors(); i < n; ++i) {
      auto succ = branch->getSuccessor(i);
      if (succ == oldBB) {
        dbgs() << "****** replace " << start->getName() << ".succ" << i << ": " << oldBB->getName() << " with " << newBB->getName() << "\n";
        branch->setSuccessor(i, newBB);
      } else {        
        if (visited.count(succ) == 0) {          
          this->recurseReplaceSuccessor(visited, succ, oldBB, newBB);
        }      
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

bool isSourceOfDivergenceHandler::eval(const Value *V) {  
  if (isa<AtomicRMWInst>(V) 
   || isa<AtomicCmpXchgInst>(V)) {
    // Atomics are divergent because they are executed sequentially: when an
    // atomic operation refers to the same address in each thread, then each
    // thread after the first sees the value written by the previous thread as
    // original value.
    return true;  
  } else 
  if (auto CI = dyn_cast<CallInst>(V)) {
    // CSR instruction WTID generates a thread divergent value
    if (!CI->isInlineAsm())
      return false;

    auto CV = CI->getCalledValue();
    if (const InlineAsm *IA = dyn_cast<InlineAsm>(CV)) {
      auto& asm_str = IA->getAsmString();
      if (asm_str.substr(0, 4) == "csrr") {
        auto AO = CI->getArgOperand(0);
        if (const ConstantInt *C = dyn_cast<ConstantInt>(AO)) {
          if (C->getValue() == CSR_WTID) {
            dbgs() << "*** CSR_WTID read: ";
            V->print(dbgs());
            dbgs() << "\n";
            DV_.insert(V);
            return true;
          }
        }        
      }
    }
    
    return false;
  } else 
  if (auto ST = dyn_cast<StoreInst>(V)) {        
    // track stores for divergent stack values
    auto addr = ST->getPointerOperand();
    if (dyn_cast<AllocaInst>(addr) != NULL) {
      auto value = ST->getValueOperand();
      if (DV_.count(value)) {
        DV_.insert(addr);
      }
    }
  } else  
  if (auto LD = dyn_cast<LoadInst>(V)) {        
    // loads from divergent stack values are divergent
    auto addr = LD->getPointerOperand();
    if (dyn_cast<AllocaInst>(addr) != NULL) {
      if (DV_.count(addr)) {
        DV_.insert(V);
        return true;
      }
    }
  }

  return false;
}

}