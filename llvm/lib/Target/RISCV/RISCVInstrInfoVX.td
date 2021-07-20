// Vortex instructions definitions

def OPC_VORTEX : RISCVOpcode<0b0000110>;

let hasSideEffects = 1, mayStore = 1 , mayLoad = 1 in {

def VX_SPLIT : RVInstS<2, OPC_VORTEX, (outs), (ins GPR:$rs1), "vx_split", "$rs1">, Sched<[]> {
    let rs2   = 0;
    let imm12 = 0;
}

def VX_JOIN : RVInstS<3, OPC_VORTEX, (outs), (ins), "vx_join", "">, Sched<[]> {
    let rs1   = 0;
    let rs2   = 0;
    let imm12 = 0;
}
}

def : Pat<(int_riscv_vx_split i32:$rs1), (VX_SPLIT GPR:$rs1)>;

def : Pat<(int_riscv_vx_join), (VX_JOIN)>;