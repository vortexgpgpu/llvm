#include "RISCV.h"
#include "RISCVSubtarget.h"

//#include "llvm-c/Core.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsRISCV.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"

#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "RISCV.h"
#include "RISCVSubtarget.h"
#include "llvm/InitializePasses.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/ADT/APInt.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace llvm;
// using namespace vortex;

#define DEBUG_TYPE "vortex-intrinsic-lowering"

void printIR(llvm::Module *module_) {
  std::string module_str;
  llvm::raw_string_ostream ostream{module_str};
  module_->print(ostream, nullptr, false);
  std::cout << module_str << std::endl;
}

class VortexIntrinsicFuncLowering final : public ModulePass {

  bool runOnModule(Module &M) override;
  bool Modified;

  public:
  static char ID;
  VortexIntrinsicFuncLowering();
};

namespace llvm {

  void initializeVortexIntrinsicFuncLoweringPass(PassRegistry &);
  ModulePass *createVortexIntrinsicFuncLoweringPass() {
    return new VortexIntrinsicFuncLowering();
  }
} // End namespace llvm

INITIALIZE_PASS(VortexIntrinsicFuncLowering, DEBUG_TYPE,
    "Fix function bitcasts for AMDGPU", false, false)

char VortexIntrinsicFuncLowering::ID = 0;

VortexIntrinsicFuncLowering::VortexIntrinsicFuncLowering() : ModulePass(ID) {
  initializeVortexIntrinsicFuncLoweringPass(*PassRegistry::getPassRegistry());
}

int CheckFTarget(std::vector<StringRef> FTargets, StringRef fname) {
  for (size_t i = 0; i < FTargets.size(); i++) {
    if (FTargets[i].equals(fname)) {
      return (i);
    }
  }
  return -1;
}

bool VortexIntrinsicFuncLowering::runOnModule(Module &M) {
  Modified = false;
  std::cerr << "VORTEX Intrinsic Func pass " << std::endl;

  std::set<llvm::Function *> DeclToRemove;
  std::set<llvm::Instruction *> CallToRemove;
  std::set<llvm::Instruction *> vxBarCallToRemove;
  std::vector<StringRef> FTargets = {
    "vx_barrier",
    "vx_num_threads", "vx_num_warps", "vx_num_cores", "vx_num_clusters",
    "vx_thread_id", "vx_warp_id", "vx_core_id", "vx_cluster_id",
    "vx_thread_mask", "vx_tmc"};

  Type* SizeTTy_;
  auto& Context = M.getContext();

  auto sizeTSize = M.getDataLayout().getPointerSizeInBits();
  switch (sizeTSize) {
    case 128: SizeTTy_ = llvm::Type::getInt128Ty(Context); break;
    case 64:  SizeTTy_ = llvm::Type::getInt64Ty(Context); break;
    case 32:  SizeTTy_ = llvm::Type::getInt32Ty(Context); break;
    case 16:  SizeTTy_ = llvm::Type::getInt16Ty(Context); break;
    case 8:   SizeTTy_ = llvm::Type::getInt8Ty(Context); break;
    default:
              SizeTTy_ = llvm::Type::getInt32Ty(Context); break;
  }

  Function *bar_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_bar, {SizeTTy_, SizeTTy_});
  Function *tid_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tid, {SizeTTy_});
  Function *wid_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_wid, {SizeTTy_});
  Function *cid_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_cid, {SizeTTy_});
  Function *gid_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_gid, {SizeTTy_});

  Function *nt_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_nt, {SizeTTy_});
  Function *nw_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_nw, {SizeTTy_});
  Function *nc_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_nc, {SizeTTy_});
  Function *ng_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_ng, {SizeTTy_});
  Function* tmask_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmask, {llvm::Type::getInt32Ty(Context)});
  Function *tmc_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmc, {SizeTTy_});

  // Find tharget vx intrinsic
  for (llvm::Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    llvm::Function *F = &*I;
    if (F->isDeclaration()) {
      int check = CheckFTarget(FTargets, F->getName());
      if (check != -1)
        DeclToRemove.insert(F);
      continue;
    }
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
      for (BasicBlock::iterator BI = I->begin(), BE = I->end(); BI != BE;
          ++BI) {
        Instruction *Instr = dyn_cast<Instruction>(BI);
        if (!llvm::isa<CallInst>(Instr))
          continue;
        CallInst *CallInstr = dyn_cast<CallInst>(Instr);
        Function *Callee = CallInstr->getCalledFunction();
        if (Callee == nullptr)
          continue;

        int check = CheckFTarget(FTargets, Callee->getName());

        if (check == 0) {
          vxBarCallToRemove.insert(Instr);

        } else if (check == 1) {
          auto ntinst = CallInst::Create(nt_func_, "nt", Instr);
          Instr->replaceAllUsesWith(ntinst);
          CallToRemove.insert(Instr);

        } else if (check == 2) {
          auto nwinst = CallInst::Create(nw_func_, "nw", Instr);
          Instr->replaceAllUsesWith(nwinst);
          CallToRemove.insert(Instr);

        } else if (check == 3) {
          auto ncinst = CallInst::Create(nc_func_, "nc", Instr);
          Instr->replaceAllUsesWith(ncinst);
          CallToRemove.insert(Instr);

        } else if (check == 4) {
          auto nginst = CallInst::Create(ng_func_, "nw", Instr);
          Instr->replaceAllUsesWith(nginst);
          CallToRemove.insert(Instr);

        } else if (check == 5) {
          auto tidinst = CallInst::Create(tid_func_, "tid", Instr);
          Instr->replaceAllUsesWith(tidinst);
          CallToRemove.insert(Instr);

        } else if (check == 6) {
          auto widinst = CallInst::Create(wid_func_, "wid", Instr);
          Instr->replaceAllUsesWith(widinst);
          CallToRemove.insert(Instr);

        } else if (check == 7) {
          auto cidinst = CallInst::Create(cid_func_, "cid", Instr);
          Instr->replaceAllUsesWith(cidinst);
          CallToRemove.insert(Instr);

        }  else if (check == 8) {
          auto gidinst = CallInst::Create(gid_func_, "gid", Instr);
          Instr->replaceAllUsesWith(gidinst);
          CallToRemove.insert(Instr);
        } else if (check == 9) {
          auto tmaskinst = CallInst::Create(tmask_func_, "tmask", Instr);
          Instr->replaceAllUsesWith(tmaskinst);
          CallToRemove.insert(Instr);

        } else if (check == 10) {
          CallInst *Callinst = dyn_cast<CallInst>(Instr);
          auto tmask = Callinst->getArgOperand(0);
          auto tmcinst = CallInst::Create(tmc_func_, {tmask}, "", Instr);
          Instr->replaceAllUsesWith(tmcinst);
          CallToRemove.insert(Instr);
        }
      } // end of BB loop
    }   // end of F loop
  }     // end of M loop

  // Insert vx_barrier(barCnt, warp_size)
  if (!vxBarCallToRemove.empty()) {
    int barCnt = 1;
    for (auto B : vxBarCallToRemove) {

      CallInst *Callinst = dyn_cast<CallInst>(B);
      LLVMContext &context = M.getContext();
      auto barID =
        llvm::ConstantInt::get(context, llvm::APInt(32, (barCnt++), false));
      auto barCnt = Callinst->getArgOperand(1);
      // auto barCnt = llvm::ConstantInt::get(context, llvm::APInt(32, 4,
      // false));
      auto barinst = CallInst::Create(bar_func_, {barID, barCnt}, "", B);
      B->replaceAllUsesWith(barinst);
    }
    Modified = true;
  }

  for (auto B : vxBarCallToRemove) {
    B->eraseFromParent();
  }

  if (!CallToRemove.empty())
    Modified = true;

  for (auto B : CallToRemove) {
    B->eraseFromParent();
  }

  for (auto F : DeclToRemove) {
    F->eraseFromParent();
  }
  printIR(&M);
  return Modified;
}
