// Vortex instructions definitions

def RISCV_CUSTOM0 : RISCVOpcode<"CUSTOM0", 0b0001011>;   // 0x0B
def RISCV_CUSTOM1 : RISCVOpcode<"CUSTOM1", 0b0101011>;   // 0x2B
def RISCV_CUSTOM2 : RISCVOpcode<"CUSTOM2", 0b1011011>;   // 0x5B
def RISCV_CUSTOM3 : RISCVOpcode<"CUSTOM3", 0b1111011>;   // 0x7B

class VXInstR4<RISCVOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RVInst<outs, ins, opcodestr, argstr, [], InstFormatR4> {
  bits<5> rs3;
  bits<2> funct2;
  bits<5> rs2;
  bits<5> rs1;
  bits<3> funct3;
  bits<5> rd;

  let Inst{31-27} = rs3;
  let Inst{26-25} = funct2;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Opcode = opcode.Value;
}

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

def VX_JOIN : RVInstR<0, 3, RISCV_CUSTOM0, (outs), (ins GPR:$rs1), "vx_join", "$rs1">, Sched<[]> {
    let rd  = 0;
    let rs2 = 0;
}

def VX_BAR : RVInstR<0, 4, RISCV_CUSTOM0, (outs), (ins GPR:$rs1, GPR:$rs2), "vx_bar", "$rs1, $rs2">, Sched<[]> {
    let rd = 0;
    let isBarrier = 1;
}

def VX_PRED : RVInstR<0, 5, RISCV_CUSTOM0, (outs), (ins GPR:$rs1, GPR:$rs2), "vx_pred", "$rs1, $rs2">, Sched<[]> {
    let rd  = 0;
}

def VX_TEX : VXInstR4<RISCV_CUSTOM1, (outs GPR:$rd), (ins uimm2:$funct2, GPR:$rs1, GPR:$rs2, GPR:$rs3), "vx_tex", "$rd, $funct2, $rs1, $rs2, $rs3">, Sched<[]> {
    let funct3 = 0;
}

def VX_CMOV : VXInstR4<RISCV_CUSTOM1, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2, GPR:$rs3), "vx_cmov", "$rd, $rs1, $rs2, $rs3">, Sched<[]> {
    let funct3 = 1;
    let funct2 = 0;
}

def VX_ROP : VXInstR4<RISCV_CUSTOM1, (outs), (ins GPR:$rs1, GPR:$rs2, GPR:$rs3), "vx_rop", "$rs1, $rs2, $rs3">, Sched<[]> {
    let funct3 = 1;
    let funct2 = 1;
    let rd     = 0;
}

def VX_RAST : RVInstR<1, 0, RISCV_CUSTOM0, (outs GPR:$rd), (ins), "vx_rast", "">, Sched<[]> {
    let rs1 = 0;
    let rs2 = 0;
}

}

def CSR_TMASK : SysReg<"tmask", 0xCC4>;

def : Pat<(int_riscv_vx_tmask), (CSRRS CSR_TMASK.Encoding, X0)>;

def : Pat<(int_riscv_vx_tmc GPR:$rs1), (VX_TMC GPR:$rs1)>;

def : Pat<(int_riscv_vx_pred GPR:$rs1, GPR:$rs2), (VX_PRED GPR:$rs1, GPR:$rs2)>;

def : Pat<(int_riscv_vx_split GPR:$rs1), (VX_SPLIT GPR:$rs1)>;

def : Pat<(int_riscv_vx_join GPR:$rs1), (VX_JOIN GPR:$rs1)>;