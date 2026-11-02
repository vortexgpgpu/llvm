//===-- RISCVVortexWideLoadSplit.cpp - Split XVortex wide loads -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// vx_flw.d/.q define all words of a register group at once. When its words
// are read at different points, the merged load keeps every word live from
// the first read on, where scalar loads would each be live only from their
// own read: a quad of a 4x4 register tile's A column, read one word per k
// step, holds 4 FP registers across the whole k loop instead of 1. Where that
// pushes a block's FP register pressure beyond the register file, the
// scheduler cannot help (the group is one def), and the allocator spills.
//
// The macro-op is exactly its flw uops, so it can be split back at no cost in
// correctness. This pass runs on the scheduled code, before register
// allocation, on blocks whose FP pressure exceeds the register file. It
// models each wide load split into per-word flw placed at the word's first
// read, and greedily splits the loads that remove the most excess pressure,
// until none is left or no split helps. Wide loads in blocks that fit stay
// wide. Running after the scheduler keeps the word loads where they were put:
// before it, load clustering pulls them back together.
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "RISCVInstrInfo.h"
#include "RISCVSubtarget.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-vortex-wide-load-split"
#define PASS_NAME "RISC-V Vortex wide load split"

static cl::opt<bool> EnableWideLoadSplit(
    "riscv-vortex-split-wide-loads",
    cl::desc("Split XVortex wide loads that push FP register pressure beyond "
             "the register file"),
    cl::init(true), cl::Hidden);

namespace {

// One vx_flw.d/.q whose words are read only through sub-registers, in its own
// block.
struct WideLoad {
  MachineInstr *MI;
  Register Dst;
  unsigned Def;                          // position of MI
  unsigned Last;                         // position of the last read
  unsigned Sink;                         // latest position a word load may
                                         // move to
  SmallVector<std::pair<unsigned, unsigned>, 4> Words; // first/last read
  SmallVector<SmallVector<MachineOperand *, 4>, 4> Uses; // per word
  bool Split = false;
};

class RISCVVortexWideLoadSplit : public MachineFunctionPass {
public:
  static char ID;
  RISCVVortexWideLoadSplit() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;
  StringRef getPassName() const override { return PASS_NAME; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<LiveIntervalsWrapperPass>();
    AU.addPreserved<LiveIntervalsWrapperPass>();
    AU.addRequired<SlotIndexesWrapperPass>();
    AU.addPreserved<SlotIndexesWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

private:
  const RISCVInstrInfo *TII = nullptr;
  const TargetRegisterInfo *TRI = nullptr;
  MachineRegisterInfo *MRI = nullptr;
  LiveIntervals *LIS = nullptr;
  unsigned FPSet = 0;
  unsigned FPLimit = 0;

  unsigned fpWeight(Register Reg) const;
  bool processBlock(MachineBasicBlock &MBB);
  bool collect(MachineInstr &MI, ArrayRef<MachineInstr *> Order,
               const DenseMap<const MachineInstr *, unsigned> &Pos,
               WideLoad &W) const;
  void split(WideLoad &W, ArrayRef<MachineInstr *> Order);
};

} // end anonymous namespace

char RISCVVortexWideLoadSplit::ID = 0;

INITIALIZE_PASS_BEGIN(RISCVVortexWideLoadSplit, DEBUG_TYPE, PASS_NAME, false,
                      false)
INITIALIZE_PASS_DEPENDENCY(LiveIntervalsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(SlotIndexesWrapperPass)
INITIALIZE_PASS_END(RISCVVortexWideLoadSplit, DEBUG_TYPE, PASS_NAME, false,
                    false)

char &llvm::RISCVVortexWideLoadSplitID = RISCVVortexWideLoadSplit::ID;

// The weight of Reg in the FP pressure set, as the scheduler counts it.
unsigned RISCVVortexWideLoadSplit::fpWeight(Register Reg) const {
  for (PSetIterator PSet = MRI->getPressureSets(Reg); PSet.isValid(); ++PSet)
    if (*PSet == FPSet)
      return PSet.getWeight();
  return 0;
}

static unsigned getWordCount(unsigned Opcode) {
  switch (Opcode) {
  case RISCV::VX_FLW_D:
    return 2;
  case RISCV::VX_FLW_Q:
    return 4;
  default:
    return 0;
  }
}

bool RISCVVortexWideLoadSplit::collect(
    MachineInstr &MI, ArrayRef<MachineInstr *> Order,
    const DenseMap<const MachineInstr *, unsigned> &Pos, WideLoad &W) const {
  unsigned NumWords = getWordCount(MI.getOpcode());
  if (!NumWords || !MI.getOperand(1).isReg() ||
      !MI.getOperand(1).getReg().isVirtual() || !MI.hasOneMemOperand())
    return false;
  Register Dst = MI.getOperand(0).getReg();
  if (!Dst.isVirtual() || MI.getOperand(0).getSubReg() ||
      !MRI->hasOneDef(Dst))
    return false;

  W.MI = &MI;
  W.Dst = Dst;
  W.Def = Pos.lookup(&MI);
  W.Last = W.Def;
  W.Words.assign(NumWords, {~0u, 0u});
  W.Uses.assign(NumWords, {});
  unsigned GroupBits = TRI->getRegSizeInBits(*MRI->getRegClass(Dst));
  for (MachineOperand &MO : MRI->use_nodbg_operands(Dst)) {
    MachineInstr *UseMI = MO.getParent();
    auto It = Pos.find(UseMI);
    unsigned Sub = MO.getSubReg();
    if (It == Pos.end() || !Sub || MO.isDef() ||
        TRI->getSubRegIdxSize(Sub) != GroupBits / NumWords)
      return false;
    unsigned Word = TRI->getSubRegIdxOffset(Sub) / (GroupBits / NumWords);
    auto &[First, Last] = W.Words[Word];
    First = std::min(First, It->second);
    Last = std::max(Last, It->second);
    W.Last = std::max(W.Last, It->second);
    W.Uses[Word].push_back(&MO);
  }

  // A word's load may move down to its first read unless a store, call or
  // redefinition of the base lies in between.
  Register Base = MI.getOperand(1).getReg();
  W.Sink = W.Def;
  while (W.Sink + 1 < Order.size()) {
    const MachineInstr &Next = *Order[W.Sink + 1];
    if (Next.mayStore() || Next.isCall() || Next.hasUnmodeledSideEffects() ||
        Next.hasOrderedMemoryRef() || Next.modifiesRegister(Base, TRI))
      break;
    ++W.Sink;
  }
  return true;
}

// The position after which word Word is live once W is split.
static unsigned getWordStart(const WideLoad &W, unsigned Word) {
  unsigned First = W.Words[Word].first;
  return First <= W.Sink ? First - 1 : W.Def;
}

// Replace W.MI by one flw per word that is read, each just before its first
// read when it may move there.
void RISCVVortexWideLoadSplit::split(WideLoad &W,
                                     ArrayRef<MachineInstr *> Order) {
  MachineInstr &MI = *W.MI;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  Register Base = MI.getOperand(1).getReg();
  int64_t Imm = MI.getOperand(2).getImm();
  MachineMemOperand *MMO = *MI.memoperands_begin();

  for (unsigned Word = 0, E = W.Words.size(); Word != E; ++Word) {
    if (W.Uses[Word].empty())
      continue;
    Register Reg = MRI->createVirtualRegister(&RISCV::FPR32RegClass);
    MachineInstr *Before = &MI;
    if (W.Words[Word].first <= W.Sink)
      Before = Order[W.Words[Word].first];
    MachineInstr *Load =
        BuildMI(MBB, Before, MI.getDebugLoc(), TII->get(RISCV::FLW), Reg)
            .addReg(Base)
            .addImm(Imm + 4 * Word)
            .addMemOperand(MF.getMachineMemOperand(MMO, 4 * Word, 4));
    LIS->InsertMachineInstrInMaps(*Load);
    for (MachineOperand *MO : W.Uses[Word]) {
      MO->setReg(Reg);
      MO->setSubReg(0);
      MO->setIsKill(false);
    }
    LIS->createAndComputeVirtRegInterval(Reg);
  }

  for (MachineOperand &MO : make_early_inc_range(MRI->reg_operands(W.Dst))) {
    assert(MO.isDebug() || MO.getParent() == &MI);
    if (MO.isDebug())
      MO.getParent()->setDebugValueUndef();
  }
  LIS->RemoveMachineInstrFromMaps(MI);
  MI.eraseFromParent();
  LIS->removeInterval(W.Dst);
  LIS->removeInterval(Base);
  LIS->createAndComputeVirtRegInterval(Base);
}

bool RISCVVortexWideLoadSplit::processBlock(MachineBasicBlock &MBB) {
  SmallVector<MachineInstr *, 128> Order;
  DenseMap<const MachineInstr *, unsigned> Pos;
  SmallVector<WideLoad, 8> Loads;
  for (MachineInstr &MI : MBB) {
    if (MI.isDebugInstr())
      continue;
    Pos[&MI] = Order.size();
    Order.push_back(&MI);
  }
  for (MachineInstr *MI : Order) {
    WideLoad W;
    if (collect(*MI, Order, Pos, W))
      Loads.push_back(std::move(W));
  }
  if (Loads.empty())
    return false;

  // FP pressure after each instruction, from the live intervals.
  unsigned N = Order.size();
  SmallVector<int, 128> Pressure(N, 0);
  SlotIndex Start = LIS->getMBBStartIdx(&MBB), End = LIS->getMBBEndIdx(&MBB);
  for (unsigned I = 0, E = MRI->getNumVirtRegs(); I != E; ++I) {
    Register Reg = Register::index2VirtReg(I);
    if (!LIS->hasInterval(Reg))
      continue;
    unsigned Weight = fpWeight(Reg);
    if (!Weight)
      continue;
    const LiveInterval &LI = LIS->getInterval(Reg);
    if (!LI.overlaps(Start, End))
      continue;
    for (unsigned P = 0; P != N; ++P)
      if (LI.liveAt(LIS->getInstructionIndex(*Order[P]).getRegSlot()))
        Pressure[P] += Weight;
  }

  // Pressure change after position P when W is split.
  auto Delta = [&](const WideLoad &W, unsigned P) {
    int D = 0;
    if (P >= W.Def && P < W.Last)
      D -= fpWeight(W.Dst);
    for (unsigned Word = 0, E = W.Words.size(); Word != E; ++Word)
      if (!W.Uses[Word].empty() && P >= getWordStart(W, Word) &&
          P < W.Words[Word].second)
        D += 1;
    return D;
  };
  auto Excess = [&](unsigned P, int D) {
    return std::max(0, Pressure[P] + D - int(FPLimit));
  };

  bool Changed = false;
  while (true) {
    WideLoad *Best = nullptr;
    int BestGain = 0;
    for (WideLoad &W : Loads) {
      if (W.Split)
        continue;
      int Gain = 0;
      for (unsigned P = W.Def; P <= W.Last; ++P)
        Gain += Excess(P, 0) - Excess(P, Delta(W, P));
      if (Gain > BestGain) {
        BestGain = Gain;
        Best = &W;
      }
    }
    if (!Best)
      break;
    LLVM_DEBUG(dbgs() << "Splitting (gain " << BestGain << "): " << *Best->MI);
    for (unsigned P = Best->Def; P <= Best->Last; ++P)
      Pressure[P] += Delta(*Best, P);
    Best->Split = true;
    Changed = true;
  }

  for (WideLoad &W : Loads)
    if (W.Split)
      split(W, Order);
  return Changed;
}

bool RISCVVortexWideLoadSplit::runOnMachineFunction(MachineFunction &MF) {
  const auto &ST = MF.getSubtarget<RISCVSubtarget>();
  if (!EnableWideLoadSplit || !ST.hasVortexWideMem() ||
      skipFunction(MF.getFunction()))
    return false;
  TII = ST.getInstrInfo();
  TRI = ST.getRegisterInfo();
  MRI = &MF.getRegInfo();
  LIS = &getAnalysis<LiveIntervalsWrapperPass>().getLIS();

  RegisterClassInfo RCI;
  RCI.runOnMachineFunction(MF);
  // The FP pressure set: the widest one FPR32 belongs to.
  FPLimit = 0;
  for (const int *PSet = TRI->getRegClassPressureSets(&RISCV::FPR32RegClass);
       *PSet != -1; ++PSet) {
    unsigned Limit = RCI.getRegPressureSetLimit(*PSet);
    if (Limit > FPLimit) {
      FPLimit = Limit;
      FPSet = *PSet;
    }
  }

  bool Changed = false;
  for (MachineBasicBlock &MBB : MF)
    Changed |= processBlock(MBB);
  return Changed;
}

FunctionPass *llvm::createRISCVVortexWideLoadSplitPass() {
  return new RISCVVortexWideLoadSplit();
}
