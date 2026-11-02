//===-- RISCVInstrInfoXVortexMem.td - Vortex wide accesses -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Wide loads and stores of the Vortex GPU (custom-3):
//
//   vx_flw.d fd, imm12(rs1)    I-type, funct3 3: fd, fd+1    <- 2 words
//   vx_flw.q fd, imm12(rs1)    I-type, funct3 4: fd..fd+3    <- 4 words
//   vx_fsw.d fs2, imm12(rs1)   S-type, funct3 5: 2 words <- fs2, fs2+1
//   vx_fsw.q fs2, imm12(rs1)   S-type, funct3 6: 4 words <- fs2..fs2+3
//
// Each is exactly its sequence of flw/fsw: word k at rs1 + imm12 + 4k to or
// from register fd/fs2 + k, at any address, with 4-byte alignment. The
// register is an aligned FPR32 group (FPRG2 / FPRG4) and imm12 a multiple of
// 8 (.d) or 16 (.q), so the hardware expands the macro-op without an adder.
// Defined on RV32 with FLEN 32. Selected for v2f32 / v4f32 loads and stores.
//
//===----------------------------------------------------------------------===//

// A 12-bit signed immediate that is a multiple of 8 / 16.
class VXSImm12LsbZero<int LsbZeros, string Suffix>
    : RISCVOp, ImmLeaf<XLenVT, "return isShiftedInt<" # !sub(12, LsbZeros) #
                               ", " # LsbZeros # ">(Imm);"> {
  let ParserMatchClass = SImmAsmOperand<12, Suffix>;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeSImmOperandLsbZero<12, " # LsbZeros # ">";
  let MCOperandPredicate = "int64_t Imm; return MCOp.evaluateAsConstantImm(Imm) && "
                           "isShiftedInt<" # !sub(12, LsbZeros) # ", " #
                           LsbZeros # ">(Imm);";
  let OperandType = "OPERAND_SIMM12_" # !toupper(Suffix);
}

def simm12_lsb000 : VXSImm12LsbZero<3, "Lsb000">;
def simm12_lsb0000 : VXSImm12LsbZero<4, "Lsb0000">;

// Base register plus an offset folded only when it is a multiple of 8 / 16.
def AddrRegImmLsb000 : ComplexPattern<iPTR, 2, "SelectAddrRegImmLsb000">;
def AddrRegImmLsb0000 : ComplexPattern<iPTR, 2, "SelectAddrRegImmLsb0000">;

def SDT_RISCVVXFlw : SDTypeProfile<1, 1, [SDTCisVec<0>, SDTCisPtrTy<1>]>;
def SDT_RISCVVXFsw : SDTypeProfile<0, 2, [SDTCisVec<0>, SDTCisPtrTy<1>]>;
def riscv_vx_flw : SDNode<"RISCVISD::VX_FLW", SDT_RISCVVXFlw,
                          [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def riscv_vx_fsw : SDNode<"RISCVISD::VX_FSW", SDT_RISCVVXFsw,
                          [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
class VXFlwWide<bits<3> funct3, string opcodestr, RegisterClass rty,
                DAGOperand immty>
    : RVInstI<funct3, OPC_CUSTOM_3, (outs rty:$rd),
              (ins GPRMem:$rs1, immty:$imm12), opcodestr,
              "$rd, ${imm12}(${rs1})">,
      Sched<[WriteFLD32, ReadFMemBase]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
class VXFswWide<bits<3> funct3, string opcodestr, RegisterClass rty,
                DAGOperand immty>
    : RVInstS<funct3, OPC_CUSTOM_3, (outs),
              (ins rty:$rs2, GPRMem:$rs1, immty:$imm12), opcodestr,
              "$rs2, ${imm12}(${rs1})">,
      Sched<[WriteFST32, ReadFStoreData, ReadFMemBase]>;

// FLEN 32: the registers are 32-bit FP registers.
def NoStdExtD : Predicate<"!Subtarget->hasStdExtD()">,
                AssemblerPredicate<(all_of (not FeatureStdExtD)),
                                   "FLEN 32 ('D' disabled)">;

let Predicates = [HasVendorXVortex, HasStdExtF, NoStdExtD, IsRV32] in {
let DecoderNamespace = "XVortex" in {
def VX_FLW_D : VXFlwWide<0b011, "vx_flw.d", FPRG2, simm12_lsb000>;
def VX_FLW_Q : VXFlwWide<0b100, "vx_flw.q", FPRG4, simm12_lsb0000>;
def VX_FSW_D : VXFswWide<0b101, "vx_fsw.d", FPRG2, simm12_lsb000>;
def VX_FSW_Q : VXFswWide<0b110, "vx_fsw.q", FPRG4, simm12_lsb0000>;
}

def : Pat<(v2f32 (riscv_vx_flw
                     (AddrRegImmLsb000 (XLenVT GPR:$rs1), simm12_lsb000:$imm12))),
          (VX_FLW_D GPR:$rs1, simm12_lsb000:$imm12)>;
def : Pat<(v4f32 (riscv_vx_flw
                     (AddrRegImmLsb0000 (XLenVT GPR:$rs1), simm12_lsb0000:$imm12))),
          (VX_FLW_Q GPR:$rs1, simm12_lsb0000:$imm12)>;
def : Pat<(riscv_vx_fsw (v2f32 FPRG2:$rs2),
                        (AddrRegImmLsb000 (XLenVT GPR:$rs1), simm12_lsb000:$imm12)),
          (VX_FSW_D FPRG2:$rs2, GPR:$rs1, simm12_lsb000:$imm12)>;
def : Pat<(riscv_vx_fsw (v4f32 FPRG4:$rs2),
                        (AddrRegImmLsb0000 (XLenVT GPR:$rs1), simm12_lsb0000:$imm12)),
          (VX_FSW_Q FPRG4:$rs2, GPR:$rs1, simm12_lsb0000:$imm12)>;
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
defm PseudoVX_GPRG2 : VXGroupSpill<GPRG2, 2>;
defm PseudoVX_GPRG4 : VXGroupSpill<GPRG4, 4>;
defm PseudoVX_GPRG8 : VXGroupSpill<GPRG8, 8>;
defm PseudoVX_FPRG2 : VXGroupSpill<FPRG2, 2>;
defm PseudoVX_FPRG4 : VXGroupSpill<FPRG4, 4>;
defm PseudoVX_FPRG8 : VXGroupSpill<FPRG8, 8>;
}
