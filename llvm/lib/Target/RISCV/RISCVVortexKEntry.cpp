//===- RISCVVortexKEntry.cpp - Vortex kernel-entry alias emitter ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Emits, for every function carrying the `vortex.kernel` annotation, an
// external symbol alias named `__vx_kentry_<function>` pointing at the
// function. The Vortex packaging tool enumerates these symbols to build the
// kernel table, and the device launches each kernel by the resolved address.
// The kernel body is kept against --gc-sections by the `retain` attribute the
// `__kernel` macro applies (the device dispatches by address, so there is no
// static reference to the body); the alias symbol rides along in that section.
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-vortex-kentry"
#define PASS_NAME "RISC-V Vortex kernel-entry alias emitter"

namespace {

class RISCVVortexKEntry : public ModulePass {
public:
  static char ID;

  RISCVVortexKEntry() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;

  StringRef getPassName() const override { return PASS_NAME; }
};

} // end anonymous namespace

bool RISCVVortexKEntry::runOnModule(Module &M) {
  const GlobalVariable *GA = M.getGlobalVariable("llvm.global.annotations");
  if (!GA || !GA->hasInitializer())
    return false;
  const auto *CA = dyn_cast<ConstantArray>(GA->getInitializer());
  if (!CA)
    return false;

  bool Changed = false;
  for (const Value *Op : CA->operands()) {
    const auto *CS = dyn_cast<ConstantStruct>(Op);
    if (!CS || CS->getNumOperands() < 2)
      continue;
    auto *F = dyn_cast<Function>(CS->getOperand(0)->stripPointerCasts());
    if (!F)
      continue;
    const auto *StrGV =
        dyn_cast<GlobalVariable>(CS->getOperand(1)->stripPointerCasts());
    if (!StrGV || !StrGV->hasInitializer())
      continue;
    const auto *Str = dyn_cast<ConstantDataArray>(StrGV->getInitializer());
    if (!Str || !Str->isCString() || Str->getAsCString() != "vortex.kernel")
      continue;

    std::string AliasName = ("__vx_kentry_" + F->getName()).str();
    if (M.getNamedValue(AliasName))
      continue; // already emitted

    GlobalAlias::create(F->getValueType(), F->getAddressSpace(),
                        GlobalValue::ExternalLinkage, AliasName, F, &M);
    // The kernel body carries the `retain` attribute, so --gc-sections keeps
    // its section (SHF_GNU_RETAIN); the alias symbol rides along inside it.
    Changed = true;
  }
  return Changed;
}

INITIALIZE_PASS(RISCVVortexKEntry, DEBUG_TYPE, PASS_NAME, false, false)

char RISCVVortexKEntry::ID = 0;

ModulePass *llvm::createRISCVVortexKEntryPass() {
  return new RISCVVortexKEntry();
}
