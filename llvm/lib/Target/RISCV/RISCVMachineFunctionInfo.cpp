//=- RISCVMachineFunctionInfo.cpp - RISC-V machine function info --*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares RISCV-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#include "RISCVMachineFunctionInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace {
enum WarpGatherReg : unsigned { WGX31 = 1, WGX30 = 2, WGAll = WGX31 | WGX30 };
} // namespace

static unsigned getWarpGatherRegBit(StringRef Code) {
  if (Code.equals_insensitive("{x31}") || Code.equals_insensitive("{t6}"))
    return WGX31;
  if (Code.equals_insensitive("{x30}") || Code.equals_insensitive("{t5}"))
    return WGX30;
  return 0;
}

// The warp-gather registers an inline asm statement binds as a gather
// destination. WGATHER leaves the source lane's own value in place, so the
// intrinsics bind the destination read-write ("+r" on a register variable);
// an asm that only writes a pinned register writes the active lanes, as any
// instruction does.
static unsigned getWarpGatherDests(const InlineAsm &IA) {
  unsigned Outputs = 0, Inputs = 0;
  for (const InlineAsm::ConstraintInfo &CI : IA.ParseConstraints()) {
    if (CI.Type != InlineAsm::isOutput && CI.Type != InlineAsm::isInput)
      continue;
    unsigned Mask = 0;
    for (StringRef Code : CI.Codes)
      Mask |= getWarpGatherRegBit(Code);
    if (CI.Type == InlineAsm::isOutput) {
      Outputs |= Mask;
      if (CI.hasMatchingInput())
        Inputs |= Mask;
    } else {
      Inputs |= Mask;
    }
  }
  return Outputs & Inputs;
}

// The warp-gather registers a gather can write while lanes of this function
// are parked: those the function binds as a gather destination, and those of
// any code it can transfer control to while lanes stay parked. That is every
// function it calls, and, unless the module shows otherwise, code outside it:
// an external or indirect callee, and a callback a side-effecting inline asm
// statement can trap to (the ray-tracing wait runs any-hit shaders on a subset
// of the lanes).
static unsigned getWarpGatherRegMask(const Function &F) {
  unsigned Mask = 0;
  SmallPtrSet<const Function *, 8> Visited;
  SmallVector<const Function *, 8> Worklist = {&F};
  while (!Worklist.empty() && Mask != WGAll) {
    const Function *Fn = Worklist.pop_back_val();
    if (!Visited.insert(Fn).second)
      continue;
    for (const Instruction &I : instructions(*Fn)) {
      const auto *CB = dyn_cast<CallBase>(&I);
      if (!CB)
        continue;
      if (const auto *IA = dyn_cast<InlineAsm>(CB->getCalledOperand())) {
        unsigned Dests = getWarpGatherDests(*IA);
        Mask |= (Dests || !IA->hasSideEffects()) ? Dests : WGAll;
        continue;
      }
      const Function *Callee = CB->getCalledFunction();
      if (Callee && Callee->isIntrinsic())
        continue;
      if (!Callee || Callee->isDeclaration()) {
        Mask = WGAll;
        break;
      }
      Worklist.push_back(Callee);
    }
  }
  return Mask;
}

yaml::RISCVMachineFunctionInfo::RISCVMachineFunctionInfo(
    const llvm::RISCVMachineFunctionInfo &MFI)
    : VarArgsFrameIndex(MFI.getVarArgsFrameIndex()),
      VarArgsSaveSize(MFI.getVarArgsSaveSize()) {}

MachineFunctionInfo *RISCVMachineFunctionInfo::clone(
    BumpPtrAllocator &Allocator, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
    const {
  return DestMF.cloneInfo<RISCVMachineFunctionInfo>(*this);
}

RISCVMachineFunctionInfo::RISCVMachineFunctionInfo(const Function &F,
                                                   const RISCVSubtarget *STI) {

  // The default stack probe size is 4096 if the function has no
  // stack-probe-size attribute. This is a safe default because it is the
  // smallest possible guard page size.
  uint64_t ProbeSize = 4096;
  if (F.hasFnAttribute("stack-probe-size"))
    ProbeSize = F.getFnAttributeAsParsedInteger("stack-probe-size");
  else if (const auto *PS = mdconst::extract_or_null<ConstantInt>(
               F.getParent()->getModuleFlag("stack-probe-size")))
    ProbeSize = PS->getZExtValue();
  assert(int64_t(ProbeSize) > 0 && "Invalid stack probe size");

  // Round down to the stack alignment.
  uint64_t StackAlign =
      STI->getFrameLowering()->getTransientStackAlign().value();
  ProbeSize = std::max(StackAlign, alignDown(ProbeSize, StackAlign));
  StringRef ProbeKind;
  if (F.hasFnAttribute("probe-stack"))
    ProbeKind = F.getFnAttribute("probe-stack").getValueAsString();
  else if (const auto *PS = dyn_cast_or_null<MDString>(
               F.getParent()->getModuleFlag("probe-stack")))
    ProbeKind = PS->getString();
  if (ProbeKind.size()) {
    StackProbeSize = ProbeSize;
  }

  if (STI->hasVendorXVortex()) {
    unsigned Mask = getWarpGatherRegMask(F);
    if (Mask & WGX31)
      WarpGatherRegs.push_back(RISCV::X31);
    if (Mask & WGX30)
      WarpGatherRegs.push_back(RISCV::X30);
  }
}

void yaml::RISCVMachineFunctionInfo::mappingImpl(yaml::IO &YamlIO) {
  MappingTraits<RISCVMachineFunctionInfo>::mapping(YamlIO, *this);
}

void RISCVMachineFunctionInfo::initializeBaseYamlFields(
    const yaml::RISCVMachineFunctionInfo &YamlMFI) {
  VarArgsFrameIndex = YamlMFI.VarArgsFrameIndex;
  VarArgsSaveSize = YamlMFI.VarArgsSaveSize;
}

void RISCVMachineFunctionInfo::addSExt32Register(Register Reg) {
  SExt32Registers.push_back(Reg);
}

bool RISCVMachineFunctionInfo::isSExt32Register(Register Reg) const {
  return is_contained(SExt32Registers, Reg);
}
