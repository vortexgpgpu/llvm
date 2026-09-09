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

def VX_YIELD : RVInstR<5, 0, RISCV_CUSTOM0, (outs), (ins), "vx_yield", "">, Sched<[]> {
    let rd  = 0;
    let rs1 = 0;
    let rs2 = 0;
}

def VX_BAR_ADD : RVInstR<6, 0, RISCV_CUSTOM0, (outs), (ins uimm5:$rs1), "vx_bar_add", "$rs1">, Sched<[]> {
    let rd  = 0;
    let rs2 = 0;
}

def VX_BAR_WAIT : RVInstR<6, 1, RISCV_CUSTOM0, (outs), (ins uimm5:$rs1), "vx_bar_wait", "$rs1">, Sched<[]> {
    let rd  = 0;
    let rs2 = 0;
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

def : Pat<(int_riscv_vx_yield), (VX_YIELD)>;
def : Pat<(int_riscv_vx_bar_add timm:$rs1), (VX_BAR_ADD timm:$rs1)>;
def : Pat<(int_riscv_vx_bar_wait timm:$rs1), (VX_BAR_WAIT timm:$rs1)>;
