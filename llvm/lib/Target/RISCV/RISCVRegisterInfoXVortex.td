//===-- RISCVRegisterInfoXVortex.td -------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Aligned register-group tuples for the Vortex GPGPU vendor extension.
// Adds GPRG2/G4/G8 (integer) and FPRG2/G4/G8 (single-precision float) register
// classes. All definitions are gated by HasVendorXVortex at the instruction
// level; the register classes themselves are always present (cost: only
// TableGen-generated tables grow) but are inert unless instructions reference
// them.
//
// Group-N means N consecutive aligned XLen-wide GPRs (or 32-bit FPRs).
// On RV32 a group-4 GPR is 128 bits (v4i32); on RV64 it is 256 bits (v4i64).
// Only RV32 element-types are wired into ISel today; RV64 is a follow-up.
//
//===----------------------------------------------------------------------===//

let Namespace = "RISCV" in {

//===----------------------------------------------------------------------===//
// SubRegIndices for grouped GPR tuples
//===----------------------------------------------------------------------===//

// group-4 GPR: 4 lanes of XLen each.
def sub_gpr_g4_0 : SubRegIndex<32> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32>, SubRegRange<64>]>;
}
def sub_gpr_g4_1 : SubRegIndex<32, 32> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 32>, SubRegRange<64, 64>]>;
}
def sub_gpr_g4_2 : SubRegIndex<32, 64> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 64>, SubRegRange<64, 128>]>;
}
def sub_gpr_g4_3 : SubRegIndex<32, 96> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 96>, SubRegRange<64, 192>]>;
}

// group-8 GPR: 8 lanes of XLen each.
def sub_gpr_g8_0 : SubRegIndex<32> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32>, SubRegRange<64>]>;
}
def sub_gpr_g8_1 : SubRegIndex<32, 32> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 32>, SubRegRange<64, 64>]>;
}
def sub_gpr_g8_2 : SubRegIndex<32, 64> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 64>, SubRegRange<64, 128>]>;
}
def sub_gpr_g8_3 : SubRegIndex<32, 96> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 96>, SubRegRange<64, 192>]>;
}
def sub_gpr_g8_4 : SubRegIndex<32, 128> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 128>, SubRegRange<64, 256>]>;
}
def sub_gpr_g8_5 : SubRegIndex<32, 160> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 160>, SubRegRange<64, 320>]>;
}
def sub_gpr_g8_6 : SubRegIndex<32, 192> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 192>, SubRegRange<64, 384>]>;
}
def sub_gpr_g8_7 : SubRegIndex<32, 224> {
  let SubRegRanges = SubRegRangeByHwMode<[RV32, RV64],
                                         [SubRegRange<32, 224>, SubRegRange<64, 448>]>;
}

//===----------------------------------------------------------------------===//
// SubRegIndices for grouped FPR32 tuples
//===----------------------------------------------------------------------===//

def sub_fpr32_g2_0 : SubRegIndex<32>;
def sub_fpr32_g2_1 : SubRegIndex<32, 32>;

def sub_fpr32_g4_0 : SubRegIndex<32>;
def sub_fpr32_g4_1 : SubRegIndex<32, 32>;
def sub_fpr32_g4_2 : SubRegIndex<32, 64>;
def sub_fpr32_g4_3 : SubRegIndex<32, 96>;

def sub_fpr32_g8_0 : SubRegIndex<32>;
def sub_fpr32_g8_1 : SubRegIndex<32, 32>;
def sub_fpr32_g8_2 : SubRegIndex<32, 64>;
def sub_fpr32_g8_3 : SubRegIndex<32, 96>;
def sub_fpr32_g8_4 : SubRegIndex<32, 128>;
def sub_fpr32_g8_5 : SubRegIndex<32, 160>;
def sub_fpr32_g8_6 : SubRegIndex<32, 192>;
def sub_fpr32_g8_7 : SubRegIndex<32, 224>;

} // Namespace = "RISCV"

//===----------------------------------------------------------------------===//
// RegInfo records for grouped tuples
//===----------------------------------------------------------------------===//

// group-4 GPR: 4 × XLen bits (128 on RV32, 256 on RV64).
def XLenG4RI : RegInfoByHwMode<
      [RV32,                  RV64],
      [RegInfo<128, 128, 32>, RegInfo<256, 256, 64>]>;

// group-8 GPR: 8 × XLen bits (256 on RV32, 512 on RV64).
def XLenG8RI : RegInfoByHwMode<
      [RV32,                  RV64],
      [RegInfo<256, 256, 32>, RegInfo<512, 512, 64>]>;

//===----------------------------------------------------------------------===//
// Group-4 GPR register tuples (X0_X1_X2_X3, X4_X5_X6_X7, ..., X28_X29_X30_X31)
//===----------------------------------------------------------------------===//

let RegAltNameIndices = [ABIRegAltName] in {
  foreach I = 0-7 in {
    defvar Index = !shl(I, 2);
    defvar Reg0 = !cast<Register>("X"#Index);
    defvar Reg1 = !cast<Register>("X"#!add(Index, 1));
    defvar Reg2 = !cast<Register>("X"#!add(Index, 2));
    defvar Reg3 = !cast<Register>("X"#!add(Index, 3));
    def "X" # Index # "_X" # !add(Index, 1) # "_X" # !add(Index, 2) # "_X" # !add(Index, 3)
        : RISCVRegWithSubRegs<Index, Reg0.AsmName,
                              [Reg0, Reg1, Reg2, Reg3], Reg0.AltNames> {
      let SubRegIndices = [sub_gpr_g4_0, sub_gpr_g4_1, sub_gpr_g4_2, sub_gpr_g4_3];
      let CoveredBySubRegs = 1;
    }
  }
}

//===----------------------------------------------------------------------===//
// Group-8 GPR register tuples (X0..X7, X8..X15, X16..X23, X24..X31)
//===----------------------------------------------------------------------===//

let RegAltNameIndices = [ABIRegAltName] in {
  foreach I = 0-3 in {
    defvar Index = !shl(I, 3);
    defvar Reg0 = !cast<Register>("X"#Index);
    defvar Reg1 = !cast<Register>("X"#!add(Index, 1));
    defvar Reg2 = !cast<Register>("X"#!add(Index, 2));
    defvar Reg3 = !cast<Register>("X"#!add(Index, 3));
    defvar Reg4 = !cast<Register>("X"#!add(Index, 4));
    defvar Reg5 = !cast<Register>("X"#!add(Index, 5));
    defvar Reg6 = !cast<Register>("X"#!add(Index, 6));
    defvar Reg7 = !cast<Register>("X"#!add(Index, 7));
    def "X" # Index # "_X" # !add(Index, 1) # "_X" # !add(Index, 2) # "_X" # !add(Index, 3)
       # "_X" # !add(Index, 4) # "_X" # !add(Index, 5) # "_X" # !add(Index, 6) # "_X" # !add(Index, 7)
        : RISCVRegWithSubRegs<Index, Reg0.AsmName,
                              [Reg0, Reg1, Reg2, Reg3, Reg4, Reg5, Reg6, Reg7],
                              Reg0.AltNames> {
      let SubRegIndices = [sub_gpr_g8_0, sub_gpr_g8_1, sub_gpr_g8_2, sub_gpr_g8_3,
                           sub_gpr_g8_4, sub_gpr_g8_5, sub_gpr_g8_6, sub_gpr_g8_7];
      let CoveredBySubRegs = 1;
    }
  }
}

//===----------------------------------------------------------------------===//
// Group-2 FPR32 tuples (F0_F1, F2_F3, ..., F30_F31)
//===----------------------------------------------------------------------===//

let RegAltNameIndices = [ABIRegAltName] in {
  foreach I = 0-15 in {
    defvar Index = !shl(I, 1);
    defvar Reg0 = !cast<Register>("F"#Index#"_F");
    defvar Reg1 = !cast<Register>("F"#!add(Index, 1)#"_F");
    def "F" # Index # "_F" # !add(Index, 1) # "_FPair"
        : RISCVRegWithSubRegs<Index, Reg0.AsmName, [Reg0, Reg1], Reg0.AltNames> {
      let SubRegIndices = [sub_fpr32_g2_0, sub_fpr32_g2_1];
      let CoveredBySubRegs = 1;
    }
  }
}

//===----------------------------------------------------------------------===//
// Group-4 FPR32 tuples (F0..F3, F4..F7, ..., F28..F31)
//===----------------------------------------------------------------------===//

let RegAltNameIndices = [ABIRegAltName] in {
  foreach I = 0-7 in {
    defvar Index = !shl(I, 2);
    defvar Reg0 = !cast<Register>("F"#Index#"_F");
    defvar Reg1 = !cast<Register>("F"#!add(Index, 1)#"_F");
    defvar Reg2 = !cast<Register>("F"#!add(Index, 2)#"_F");
    defvar Reg3 = !cast<Register>("F"#!add(Index, 3)#"_F");
    def "F" # Index # "_F" # !add(Index, 1) # "_F" # !add(Index, 2) # "_F" # !add(Index, 3) # "_FQuad"
        : RISCVRegWithSubRegs<Index, Reg0.AsmName,
                              [Reg0, Reg1, Reg2, Reg3], Reg0.AltNames> {
      let SubRegIndices = [sub_fpr32_g4_0, sub_fpr32_g4_1, sub_fpr32_g4_2, sub_fpr32_g4_3];
      let CoveredBySubRegs = 1;
    }
  }
}

//===----------------------------------------------------------------------===//
// Group-8 FPR32 tuples (F0..F7, F8..F15, F16..F23, F24..F31)
//===----------------------------------------------------------------------===//

let RegAltNameIndices = [ABIRegAltName] in {
  foreach I = 0-3 in {
    defvar Index = !shl(I, 3);
    defvar Reg0 = !cast<Register>("F"#Index#"_F");
    defvar Reg1 = !cast<Register>("F"#!add(Index, 1)#"_F");
    defvar Reg2 = !cast<Register>("F"#!add(Index, 2)#"_F");
    defvar Reg3 = !cast<Register>("F"#!add(Index, 3)#"_F");
    defvar Reg4 = !cast<Register>("F"#!add(Index, 4)#"_F");
    defvar Reg5 = !cast<Register>("F"#!add(Index, 5)#"_F");
    defvar Reg6 = !cast<Register>("F"#!add(Index, 6)#"_F");
    defvar Reg7 = !cast<Register>("F"#!add(Index, 7)#"_F");
    def "F" # Index # "_F" # !add(Index, 1) # "_F" # !add(Index, 2) # "_F" # !add(Index, 3)
       # "_F" # !add(Index, 4) # "_F" # !add(Index, 5) # "_F" # !add(Index, 6) # "_F" # !add(Index, 7) # "_FOct"
        : RISCVRegWithSubRegs<Index, Reg0.AsmName,
                              [Reg0, Reg1, Reg2, Reg3, Reg4, Reg5, Reg6, Reg7],
                              Reg0.AltNames> {
      let SubRegIndices = [sub_fpr32_g8_0, sub_fpr32_g8_1, sub_fpr32_g8_2, sub_fpr32_g8_3,
                           sub_fpr32_g8_4, sub_fpr32_g8_5, sub_fpr32_g8_6, sub_fpr32_g8_7];
      let CoveredBySubRegs = 1;
    }
  }
}

//===----------------------------------------------------------------------===//
// Register classes
//===----------------------------------------------------------------------===//

// GPRG2 — reuses the physical regs of upstream GPRPair. Carries v2i32 on
// RV32 (2 × 32b) and v2i64 on RV64 (2 × 64b).
let RegInfos = XLenPairRI in
def GPRG2 : RISCVRegisterClass<[v2i32, v2i64], 64, (add GPRPair)>;

// GPRG4 — 4-aligned tuple of 4 XLen regs. Carries v4i32 on RV32, v4i64 on RV64.
// Allocation order: caller-save groups first, then callee-save.
let RegInfos = XLenG4RI in
def GPRG4 : RISCVRegisterClass<[v4i32, v4i64], 128, (add
    X8_X9_X10_X11,
    X12_X13_X14_X15,
    X16_X17_X18_X19,
    X20_X21_X22_X23,
    X24_X25_X26_X27,
    X28_X29_X30_X31,
    X4_X5_X6_X7,
    X0_X1_X2_X3
)>;

// GPRG8 — 8-aligned tuple of 8 XLen regs. Carries v8i32 on RV32, v8i64 on RV64.
let RegInfos = XLenG8RI in
def GPRG8 : RISCVRegisterClass<[v8i32, v8i64], 256, (add
    X8_X9_X10_X11_X12_X13_X14_X15,
    X16_X17_X18_X19_X20_X21_X22_X23,
    X24_X25_X26_X27_X28_X29_X30_X31,
    X0_X1_X2_X3_X4_X5_X6_X7
)>;

// FPRG2 — 2-aligned tuple of 2 FPR32 regs. Carries v2f32.
def FPRG2 : RISCVRegisterClass<[v2f32], 64, (add
    F10_F11_FPair, F12_F13_FPair, F14_F15_FPair,
    F0_F1_FPair, F2_F3_FPair, F4_F5_FPair, F6_F7_FPair,
    F16_F17_FPair, F28_F29_FPair, F30_F31_FPair,
    F8_F9_FPair, F18_F19_FPair, F20_F21_FPair, F22_F23_FPair,
    F24_F25_FPair, F26_F27_FPair
)>;

// FPRG4 — 4-aligned tuple of 4 FPR32 regs. Carries v4f32.
def FPRG4 : RISCVRegisterClass<[v4f32], 128, (add
    F8_F9_F10_F11_FQuad, F12_F13_F14_F15_FQuad,
    F16_F17_F18_F19_FQuad, F20_F21_F22_F23_FQuad,
    F24_F25_F26_F27_FQuad, F28_F29_F30_F31_FQuad,
    F0_F1_F2_F3_FQuad, F4_F5_F6_F7_FQuad
)>;

// FPRG8 — 8-aligned tuple of 8 FPR32 regs. Carries v8f32.
def FPRG8 : RISCVRegisterClass<[v8f32], 256, (add
    F8_F9_F10_F11_F12_F13_F14_F15_FOct,
    F16_F17_F18_F19_F20_F21_F22_F23_FOct,
    F24_F25_F26_F27_F28_F29_F30_F31_FOct,
    F0_F1_F2_F3_F4_F5_F6_F7_FOct
)>;
