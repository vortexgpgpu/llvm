//===- VortexAliasAnalysis.h - Vortex address-space alias analysis -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// Address-space based alias analysis for the Vortex GPU: global (1) and
/// local (3) memory are distinct, so pointers into them never alias. Generic
/// (0) pointers may point to either.
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_VORTEXALIASANALYSIS_H
#define LLVM_LIB_TARGET_RISCV_VORTEXALIASANALYSIS_H

#include "llvm/Analysis/AliasAnalysis.h"

namespace llvm {

class MemoryLocation;

class VortexAAResult : public AAResultBase {
public:
  VortexAAResult() = default;
  VortexAAResult(VortexAAResult &&Arg) : AAResultBase(std::move(Arg)) {}

  /// Stateless, so it stays valid across new pass manager invalidations.
  bool invalidate(Function &, const PreservedAnalyses &,
                  FunctionAnalysisManager::Invalidator &) {
    return false;
  }

  AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB,
                    AAQueryInfo &AAQI, const Instruction *CtxI = nullptr);
};

/// New pass manager analysis providing VortexAAResult.
class VortexAA : public AnalysisInfoMixin<VortexAA> {
  friend AnalysisInfoMixin<VortexAA>;

  static AnalysisKey Key;

public:
  using Result = VortexAAResult;

  VortexAAResult run(Function &F, AnalysisManager<Function> &AM) {
    return VortexAAResult();
  }
};

/// Legacy wrapper pass providing VortexAAResult.
class VortexAAWrapperPass : public ImmutablePass {
  std::unique_ptr<VortexAAResult> Result;

public:
  static char ID;

  VortexAAWrapperPass();

  VortexAAResult &getResult() { return *Result; }
  const VortexAAResult &getResult() const { return *Result; }

  bool doInitialization(Module &M) override {
    Result.reset(new VortexAAResult());
    return false;
  }

  bool doFinalization(Module &M) override {
    Result.reset();
    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

ImmutablePass *createVortexAAWrapperPass();
void initializeVortexAAWrapperPassPass(PassRegistry &);

} // end namespace llvm

#endif // LLVM_LIB_TARGET_RISCV_VORTEXALIASANALYSIS_H
