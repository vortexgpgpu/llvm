//===-- RISCVInstrInfoXVortex.td ---------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the Vortex GPGPU vendor extension (XVortex).
//
// Instruction layout (R-type, OPC_CUSTOM_0):
//   funct7 selects the op family (0010000 = vxadd).
//   Funct7 values 0..4 in OPC_CUSTOM_0 are already used by Vortex's existing
//   WCTL/VOTE/WMMA/DXA/Pack extensions, so vxadd uses 0x10 to stay clear.
//   funct3 selects the operand-type variant:
//     000 vxadd.x.g2  (GPRG2 / v2i32)
//     001 vxadd.x.g4  (GPRG4 / v4i32)
//     010 vxadd.x.g8  (GPRG8 / v8i32)
//     100 vxadd.f.g2  (FPRG2 / v2f32) — requires F
//     101 vxadd.f.g4  (FPRG4 / v4f32) — requires F
//     110 vxadd.f.g8  (FPRG8 / v8f32) — requires F
//
// Aligned-register encoding: rd/rs1/rs2 fields carry the index of the FIRST
// sub-register in the group. Hardware ignores the bottom log2(group_size)
// bits. No custom encoder needed — default getMachineOpValue returns the
// physical encoding of the tuple register, which equals the first lane.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction class
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0,
    DecoderNamespace = "XVortex" in
class VXAdd_RR<bits<3> funct3, RegisterClass RC, string opcodestr>
    : RVInstR<0b0010000, funct3, OPC_CUSTOM_0,
              (outs RC:$rd), (ins RC:$rs1, RC:$rs2),
              opcodestr, "$rd, $rs1, $rs2">;

//===----------------------------------------------------------------------===//
// Instruction definitions
//===----------------------------------------------------------------------===//

let Predicates = [HasVendorXVortex] in {
  def VXADD_X_G2 : VXAdd_RR<0b000, GPRG2, "vxadd.x.g2">, Sched<[]>;
  def VXADD_X_G4 : VXAdd_RR<0b001, GPRG4, "vxadd.x.g4">, Sched<[]>;
  def VXADD_X_G8 : VXAdd_RR<0b010, GPRG8, "vxadd.x.g8">, Sched<[]>;
}

let Predicates = [HasVendorXVortex, HasStdExtF] in {
  def VXADD_F_G2 : VXAdd_RR<0b100, FPRG2, "vxadd.f.g2">, Sched<[]>;
  def VXADD_F_G4 : VXAdd_RR<0b101, FPRG4, "vxadd.f.g4">, Sched<[]>;
  def VXADD_F_G8 : VXAdd_RR<0b110, FPRG8, "vxadd.f.g8">, Sched<[]>;
}

//===----------------------------------------------------------------------===//
// Intrinsic → instruction patterns
//===----------------------------------------------------------------------===//

// Integer Pats — one per lane width (v*i32 for RV32, v*i64 for RV64). Same
// instruction, two patterns; only the type-legal one fires per build.
let Predicates = [HasVendorXVortex] in {
  def : Pat<(v2i32 (int_riscv_vx_add_x_g2 (v2i32 GPRG2:$rs1), (v2i32 GPRG2:$rs2))),
            (VXADD_X_G2 GPRG2:$rs1, GPRG2:$rs2)>;
  def : Pat<(v4i32 (int_riscv_vx_add_x_g4 (v4i32 GPRG4:$rs1), (v4i32 GPRG4:$rs2))),
            (VXADD_X_G4 GPRG4:$rs1, GPRG4:$rs2)>;
  def : Pat<(v8i32 (int_riscv_vx_add_x_g8 (v8i32 GPRG8:$rs1), (v8i32 GPRG8:$rs2))),
            (VXADD_X_G8 GPRG8:$rs1, GPRG8:$rs2)>;
  def : Pat<(v2i64 (int_riscv_vx_add_x_g2 (v2i64 GPRG2:$rs1), (v2i64 GPRG2:$rs2))),
            (VXADD_X_G2 GPRG2:$rs1, GPRG2:$rs2)>;
  def : Pat<(v4i64 (int_riscv_vx_add_x_g4 (v4i64 GPRG4:$rs1), (v4i64 GPRG4:$rs2))),
            (VXADD_X_G4 GPRG4:$rs1, GPRG4:$rs2)>;
  def : Pat<(v8i64 (int_riscv_vx_add_x_g8 (v8i64 GPRG8:$rs1), (v8i64 GPRG8:$rs2))),
            (VXADD_X_G8 GPRG8:$rs1, GPRG8:$rs2)>;
}

let Predicates = [HasVendorXVortex, HasStdExtF] in {
  def : Pat<(v2f32 (int_riscv_vx_add_f_g2 (v2f32 FPRG2:$rs1), (v2f32 FPRG2:$rs2))),
            (VXADD_F_G2 FPRG2:$rs1, FPRG2:$rs2)>;
  def : Pat<(v4f32 (int_riscv_vx_add_f_g4 (v4f32 FPRG4:$rs1), (v4f32 FPRG4:$rs2))),
            (VXADD_F_G4 FPRG4:$rs1, FPRG4:$rs2)>;
  def : Pat<(v8f32 (int_riscv_vx_add_f_g8 (v8f32 FPRG8:$rs1), (v8f32 FPRG8:$rs2))),
            (VXADD_F_G8 FPRG8:$rs1, FPRG8:$rs2)>;
}
