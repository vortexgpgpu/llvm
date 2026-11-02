//===- VortexAliasAnalysis.cpp - Vortex address-space alias analysis ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// Address-space based alias analysis for the Vortex GPU.
//===----------------------------------------------------------------------===//

#include "VortexAliasAnalysis.h"
#include "RISCV.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-vortex-aa"

static cl::opt<unsigned> TraverseAddressSpacesLimit(
    "riscv-vortex-aa-traverse-limit", cl::Hidden,
    cl::desc("Depth limit for finding the address space of a generic pointer "
             "in the Vortex alias analysis"),
    cl::init(6));

AnalysisKey VortexAA::Key;

char VortexAAWrapperPass::ID = 0;

INITIALIZE_PASS(VortexAAWrapperPass, "riscv-vortex-aa",
                "Vortex address-space based alias analysis", false, true)

ImmutablePass *llvm::createVortexAAWrapperPass() {
  return new VortexAAWrapperPass();
}

VortexAAWrapperPass::VortexAAWrapperPass() : ImmutablePass(ID) {
  initializeVortexAAWrapperPassPass(*PassRegistry::getPassRegistry());
}

void VortexAAWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

// The first non-generic address space on the def chain of V. A pointer that
// belongs to two different spaces along an execution path is undefined.
static unsigned getAddressSpace(const Value *V, unsigned MaxLookup) {
  auto GetAS = [](const Value *V) -> unsigned {
    if (const auto *PTy = dyn_cast<PointerType>(V->getType()))
      return PTy->getAddressSpace();
    return VortexAS::Generic;
  };
  while (MaxLookup-- && GetAS(V) == VortexAS::Generic) {
    const Value *NewV = getUnderlyingObject(V, 1);
    if (NewV == V)
      break;
    V = NewV;
  }
  return GetAS(V);
}

static bool isDisjointSpace(unsigned AS) {
  return AS == VortexAS::Global || AS == VortexAS::Local;
}

AliasResult VortexAAResult::alias(const MemoryLocation &LocA,
                                  const MemoryLocation &LocB,
                                  AAQueryInfo &AAQI, const Instruction *) {
  unsigned ASA = getAddressSpace(LocA.Ptr, TraverseAddressSpacesLimit);
  unsigned ASB = getAddressSpace(LocB.Ptr, TraverseAddressSpacesLimit);
  if (isDisjointSpace(ASA) && isDisjointSpace(ASB) && ASA != ASB)
    return AliasResult::NoAlias;
  return AliasResult::MayAlias;
}
