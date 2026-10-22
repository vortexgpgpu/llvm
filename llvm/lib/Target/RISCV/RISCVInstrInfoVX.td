// Vortex instructions definitions

def RISCV_CUSTOM0 : RISCVOpcode<"CUSTOM0", 0b0001011>;   // 0x0B
def RISCV_CUSTOM1 : RISCVOpcode<"CUSTOM1", 0b0101011>;   // 0x2B
def RISCV_CUSTOM2 : RISCVOpcode<"CUSTOM2", 0b1011011>;   // 0x5B
def RISCV_CUSTOM3 : RISCVOpcode<"CUSTOM3", 0b1111011>;   // 0x7B

let hasSideEffects = 1, mayStore = 0 , mayLoad = 0 in {

def VX_TMC : RVInstR<0, 0, RISCV_CUSTOM0, (outs), (ins GPR:$rs1), "vx_tmc", "$rs1">, Sched<[]> {
    let rd  = 0;
    let rs2 = 0;
}

def VX_WSPAWN : RVInstR<0, 1, RISCV_CUSTOM0, (outs), (ins GPR:$rs1, GPR:$rs2), "vx_wspawn", "$rs1, $rs2">, Sched<[]> {
    let rd = 0;
}

def VX_SPLIT : RVInstR<0, 2, RISCV_CUSTOM0, (outs GPR:$rd), (ins GPR:$rs1), "vx_split", "$rd, $rs1">, Sched<[]> {
    let rs2 = 0;
}

def VX_SPLIT_N : RVInstR<0, 2, RISCV_CUSTOM0, (outs GPR:$rd), (ins GPR:$rs1), "vx_split_n", "$rd, $rs1">, Sched<[]> {
    let rs2 = 1;
}

def VX_JOIN : RVInstR<0, 3, RISCV_CUSTOM0, (outs), (ins GPR:$rs1), "vx_join", "$rs1">, Sched<[]> {
    let rd  = 0;
    let rs2 = 0;
}

def VX_BAR : RVInstR<0, 4, RISCV_CUSTOM0, (outs), (ins GPR:$rs1, GPR:$rs2), "vx_bar", "$rs1, $rs2">, Sched<[]> {
    let rd = 0;
    let isBarrier = 1;
}

def VX_PRED : RVInstR<0, 5, RISCV_CUSTOM0, (outs), (ins GPR:$rs1, GPR:$rs2), "vx_pred", "$rs1, $rs2">, Sched<[]> {
    let rd = 0;
}

def VX_PRED_N : RVInstR<0, 5, RISCV_CUSTOM0, (outs), (ins GPR:$rs1, GPR:$rs2), "vx_pred_n", "$rs1, $rs2">, Sched<[]> {
    let rd = 1;
}

def VX_RAST : RVInstR<1, 0, RISCV_CUSTOM0, (outs GPR:$rd), (ins), "vx_rast", "">, Sched<[]> {
    let rs1 = 0;
    let rs2 = 0;
}

def VX_TEX : RVInstR4<0, 0, RISCV_CUSTOM1, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2, GPR:$rs3), "vx_tex", "$rd, $rs1, $rs2, $rs3">, Sched<[]> {}

def VX_ROP : RVInstR4<0, 1, RISCV_CUSTOM1, (outs), (ins GPR:$rs1, GPR:$rs2, GPR:$rs3), "vx_rop", "$rs1, $rs2, $rs3">, Sched<[]> {
    let rd = 0;
}

}

def VX_MOV : Pseudo<(outs GPR:$dst), (ins GPR:$src), [], "vx_mov", "$dst, $src"> {}

def CSR_NT : SysReg<"nt", 0xFC0>;
def CSR_NW : SysReg<"nw", 0xFC1>;
def CSR_NC : SysReg<"nc", 0xFC2>;
def CSR_NG : SysReg<"ng", 0xFC3>;
def CSR_TID : SysReg<"tid", 0xCC0>;
def CSR_WID : SysReg<"wid", 0xCC1>;
def CSR_CID : SysReg<"cid", 0xCC2>;
def CSR_GID : SysReg<"gid", 0xCC3>;
def CSR_TMASK : SysReg<"tmask", 0xCC4>;

def : Pat<(int_riscv_vx_tid_i32), (CSRRS CSR_TID.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_tid_i64), (CSRRS CSR_TID.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_wid_i32), (CSRRS CSR_WID.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_wid_i64), (CSRRS CSR_WID.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_cid_i32), (CSRRS CSR_CID.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_cid_i64), (CSRRS CSR_CID.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_nt_i32), (CSRRS CSR_NT.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_nt_i64), (CSRRS CSR_NT.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_nw_i32), (CSRRS CSR_NW.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_nw_i64), (CSRRS CSR_NW.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_nc_i32), (CSRRS CSR_NC.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_nc_i64), (CSRRS CSR_NC.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_tmask_i32), (CSRRS CSR_TMASK.Encoding, (XLenVT X0))>;
def : Pat<(int_riscv_vx_tmask_i64), (CSRRS CSR_TMASK.Encoding, (XLenVT X0))>;

def : Pat<(int_riscv_vx_tmc_i32 GPR:$rs1), (VX_TMC GPR:$rs1)>;
def : Pat<(int_riscv_vx_tmc_i64 GPR:$rs1), (VX_TMC GPR:$rs1)>;

def : Pat<(int_riscv_vx_pred_i32 GPR:$rs1, GPR:$rs2), (VX_PRED GPR:$rs1, GPR:$rs2)>;
def : Pat<(int_riscv_vx_pred_i64 GPR:$rs1, GPR:$rs2), (VX_PRED GPR:$rs1, GPR:$rs2)>;

def : Pat<(int_riscv_vx_pred_n_i32 GPR:$rs1, GPR:$rs2), (VX_PRED_N GPR:$rs1, GPR:$rs2)>;
def : Pat<(int_riscv_vx_pred_n_i64 GPR:$rs1, GPR:$rs2), (VX_PRED_N GPR:$rs1, GPR:$rs2)>;

def : Pat<(int_riscv_vx_split_i32 GPR:$rs1), (VX_SPLIT GPR:$rs1)>;
def : Pat<(int_riscv_vx_split_i64 GPR:$rs1), (VX_SPLIT GPR:$rs1)>;

def : Pat<(int_riscv_vx_split_n_i32 GPR:$rs1), (VX_SPLIT_N GPR:$rs1)>;
def : Pat<(int_riscv_vx_split_n_i64 GPR:$rs1), (VX_SPLIT_N GPR:$rs1)>;

def : Pat<(int_riscv_vx_join_i32 GPR:$rs1), (VX_JOIN GPR:$rs1)>;
def : Pat<(int_riscv_vx_join_i64 GPR:$rs1), (VX_JOIN GPR:$rs1)>;

def : Pat<(int_riscv_vx_mov_i32 GPR:$src), (VX_MOV GPR:$src)>;
def : Pat<(int_riscv_vx_mov_i64 GPR:$src), (VX_MOV GPR:$src)>;

def : Pat<(int_riscv_vx_bar_i32 GPR:$rs1, GPR:$rs2), (VX_BAR GPR:$rs1, GPR:$rs2)>;
def : Pat<(int_riscv_vx_bar_i64 GPR:$rs1, GPR:$rs2), (VX_BAR GPR:$rs1, GPR:$rs2)>;

//===----------------------------------------------------------------------===//
// XVortexAMOD: 64-bit atomics on RV32
//===----------------------------------------------------------------------===//

// The RV64 .D encodings, naming even-aligned register pairs for the operand and
// the old value, as Zacas's AMOCAS_D_RV32 does. Selected from i64 atomics by
// RISCVTargetLowering (replaceAMO64Results).
// The pair parser declines without the extension, so a plain RV32 assembler
// still reports the .D atomics as RV64-only.
def GPRPairXVortexAMODOperand : AsmOperandClass {
  let Name = "GPRPairXVortexAMOD";
  let ParserMethod = "parseGPRPairXVortexAMOD";
  let PredicateMethod = "isGPRPair";
  let RenderMethod = "addRegOperands";
}

def GPRPairXVortexAMOD : RegisterOperand<GPRPair> {
  let ParserMatchClass = GPRPairXVortexAMODOperand;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 1 in
class VXAMO64_rr<bits<5> funct5, bit aq, bit rl, string opcodestr>
    : RVInstRAtomic<funct5, aq, rl, 0b011, OPC_AMO,
                    (outs GPRPairXVortexAMOD:$rd),
                    (ins GPRMemZeroOffset:$rs1, GPRPairXVortexAMOD:$rs2),
                    opcodestr, "$rd, $rs2, $rs1">,
      Sched<[WriteAtomicD, ReadAtomicDA, ReadAtomicDD]>;

multiclass VXAMO64_rr_aq_rl<bits<5> funct5, string opcodestr> {
  def ""     : VXAMO64_rr<funct5, 0, 0, opcodestr>;
  def _AQ    : VXAMO64_rr<funct5, 1, 0, opcodestr # ".aq">;
  def _RL    : VXAMO64_rr<funct5, 0, 1, opcodestr # ".rl">;
  def _AQ_RL : VXAMO64_rr<funct5, 1, 1, opcodestr # ".aqrl">;
}

let Predicates = [HasVendorXVortexAMOD, IsRV32],
    DecoderNamespace = "RV32XVortexAMOD" in {
defm AMOSWAP_D_RV32 : VXAMO64_rr_aq_rl<0b00001, "amoswap.d">;
defm AMOADD_D_RV32  : VXAMO64_rr_aq_rl<0b00000, "amoadd.d">;
defm AMOXOR_D_RV32  : VXAMO64_rr_aq_rl<0b00100, "amoxor.d">;
defm AMOAND_D_RV32  : VXAMO64_rr_aq_rl<0b01100, "amoand.d">;
defm AMOOR_D_RV32   : VXAMO64_rr_aq_rl<0b01000, "amoor.d">;
defm AMOMIN_D_RV32  : VXAMO64_rr_aq_rl<0b10000, "amomin.d">;
defm AMOMAX_D_RV32  : VXAMO64_rr_aq_rl<0b10100, "amomax.d">;
defm AMOMINU_D_RV32 : VXAMO64_rr_aq_rl<0b11000, "amominu.d">;
defm AMOMAXU_D_RV32 : VXAMO64_rr_aq_rl<0b11100, "amomaxu.d">;
} // Predicates = [HasVendorXVortexAMOD, IsRV32]
