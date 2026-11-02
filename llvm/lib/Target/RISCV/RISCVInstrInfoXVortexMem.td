//===-- RISCVInstrInfoXVortexMem.td - Vortex wide loads ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Wide local-memory loads of the Vortex GPU (custom-3, I-type, the fld/flq
// layout):
//
//   vx_flds.d fd, imm12(rs1)   funct3 3: 64 bits per thread into fd, fd+1
//   vx_flds.q fd, imm12(rs1)   funct3 4: 128 bits per thread into fd..fd+3
//
// fd is an aligned FPR32 group (FPRG2 / FPRG4), imm12 a signed byte offset,
// and the address must be 8- / 16-byte aligned. They are defined on RV32 with
// FLEN 32 and read local memory only, which decodes the low address bits, so
// a local pointer's window address works as is. Selected for aligned
// addrspace(3) v2f32 / v4f32 loads; the LoadStoreVectorizer forms those from
// scalar local loads.
//
//===----------------------------------------------------------------------===//

def SDT_RISCVVXFlds : SDTypeProfile<1, 1, [SDTCisVec<0>, SDTCisPtrTy<1>]>;
def riscv_vx_flds : SDNode<"RISCVISD::VX_FLDS", SDT_RISCVVXFlds,
                           [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
class VXFldsWide<bits<3> funct3, string opcodestr, RegisterClass rty>
    : RVInstI<funct3, OPC_CUSTOM_3, (outs rty:$rd),
              (ins GPRMem:$rs1, simm12:$imm12), opcodestr,
              "$rd, ${imm12}(${rs1})">,
      Sched<[WriteFLD32, ReadFMemBase]>;

// FLEN 32: the destinations are 32-bit FP registers.
def NoStdExtD : Predicate<"!Subtarget->hasStdExtD()">,
                AssemblerPredicate<(all_of (not FeatureStdExtD)),
                                   "FLEN 32 ('D' disabled)">;

let Predicates = [HasVendorXVortex, HasStdExtF, NoStdExtD, IsRV32] in {
let DecoderNamespace = "XVortex" in {
def VX_FLDS_D : VXFldsWide<0b011, "vx_flds.d", FPRG2>;
def VX_FLDS_Q : VXFldsWide<0b100, "vx_flds.q", FPRG4>;
}

def : Pat<(v2f32 (riscv_vx_flds
                     (AddrRegImm (XLenVT GPR:$rs1), simm12:$imm12))),
          (VX_FLDS_D GPR:$rs1, simm12:$imm12)>;
def : Pat<(v4f32 (riscv_vx_flds
                     (AddrRegImm (XLenVT GPR:$rs1), simm12:$imm12))),
          (VX_FLDS_Q GPR:$rs1, simm12:$imm12)>;
} // Predicates = [HasVendorXVortex, HasStdExtF, NoStdExtD, IsRV32]

// Spill and reload of the XVortex register groups. Each stays one instruction
// through register allocation, so the group's live interval is one def or
// use, and RISCVExpandPseudo splits it into one scalar access per register.
multiclass VXGroupSpill<RegisterClass RC, int NumRegs> {
  let mayLoad = 0, mayStore = 1, Size = !mul(NumRegs, 4) in
  def _SPILL : Pseudo<(outs), (ins RC:$rs2, GPR:$rs1, simm12:$imm12), []>;
  let mayLoad = 1, mayStore = 0, Size = !mul(NumRegs, 4) in
  def _RELOAD : Pseudo<(outs RC:$rd), (ins GPR:$rs1, simm12:$imm12), []>;
}

let hasSideEffects = 0 in {
defm PseudoVX_GPRG4 : VXGroupSpill<GPRG4, 4>;
defm PseudoVX_GPRG8 : VXGroupSpill<GPRG8, 8>;
defm PseudoVX_FPRG2 : VXGroupSpill<FPRG2, 2>;
defm PseudoVX_FPRG4 : VXGroupSpill<FPRG4, 4>;
defm PseudoVX_FPRG8 : VXGroupSpill<FPRG8, 8>;
}
