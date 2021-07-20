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

  bool changed = false;

  for (auto I = df_begin(&F.getEntryBlock()),
            E = df_end(&F.getEntryBlock()); I != E; ++I) {
    BasicBlock *BB = *I;

    BranchInst *Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br) {
      continue;
    }

    // only process conditional branches
    if (Br->isUnconditional()) {
      dbgs() << "*** skip non conditional branch!\n" << *Br << "\n";
      continue;
    }

    // only process divergent branches
    if (DA_->isUniform(Br)) {
      dbgs() << "*** skip non divergent branch!\n" << *Br << "\n";
      continue;
    }
    
    // skip loop header
    Loop *L = LI_->getLoopFor(BB);
    if (L && L->getHeader() == BB) {
      dbgs() << "*** skip loop header branch!\n" << *Br << "\n";
      continue;
    }    
    
    auto succ0 = Br->getSuccessor(0);
    auto succ1 = Br->getSuccessor(1);
    auto ipdom = PDT_->findNearestCommonDominator(succ0, succ1);
    //dbgs() << "*** succs: BB#" << *succ0 << ", BB#" << *succ1 << "\n";
    //dbgs() << "*** ipdom: BB#" << *ipdom << "\n";

    IRBuilder<> irbuilder(Br);
    auto cond_i32 = irbuilder.CreateIntCast(Br->getCondition(), irbuilder.getInt32Ty(), false);
    CallInst::Create(split_func_, cond_i32, "", Br);

    // if ipdom is shared with another branch, 
    // insert new BB before it as unique ipdom
    if (joins_.count(ipdom)) {      
      auto &context = F.getContext();
      auto stub = BasicBlock::Create(context, "ipdom_stub", &F, ipdom);
      PDT_->addNewBlock(stub, ipdom);
      ipdom = stub;
    } else {
      joins_.insert(ipdom);
    }

    CallInst::Create(join_func_, "", ipdom->getFirstNonPHI());

    changed = true;
  }

  if (changed) {
    dbgs() << "*** final!\n" << F << "\n";
  }

  return true;
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