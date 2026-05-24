//==- BuiltinsRISCVXVortex.td - Vortex GPGPU builtin database  -*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Clang builtins for the Vortex GPGPU vendor extension (XVortex).
//
// Spelling: __builtin_riscv_vx_<op>_<lane-type>_g<group-size>
//
//===----------------------------------------------------------------------===//

class RISCVXVortexBuiltin<string prototype, string features = "xvortex"> : TargetBuiltin {
  let Spellings = ["__builtin_riscv_vx_" # NAME];
  let Prototype = prototype;
  let Features = features;
}

let Attributes = [NoThrow, Const] in {

// Prototype: ClangBuiltinsEmitter parses C-style "ReturnType(Arg1, Arg2)".
// GCC-style vectors use the _Vector<N, type> template syntax.

// Integer-side prototypes use `long int` so the lane width follows the ABI:
// ilp32 -> i32, lp64 -> i64. Same builtin name works on both RV32 and RV64.
def add_x_g2 : RISCVXVortexBuiltin<"_Vector<2, long int>(_Vector<2, long int>, _Vector<2, long int>)">;
def add_x_g4 : RISCVXVortexBuiltin<"_Vector<4, long int>(_Vector<4, long int>, _Vector<4, long int>)">;
def add_x_g8 : RISCVXVortexBuiltin<"_Vector<8, long int>(_Vector<8, long int>, _Vector<8, long int>)">;

def add_f_g2 : RISCVXVortexBuiltin<"_Vector<2, float>(_Vector<2, float>, _Vector<2, float>)", "xvortex,f">;
def add_f_g4 : RISCVXVortexBuiltin<"_Vector<4, float>(_Vector<4, float>, _Vector<4, float>)", "xvortex,f">;
def add_f_g8 : RISCVXVortexBuiltin<"_Vector<8, float>(_Vector<8, float>, _Vector<8, float>)", "xvortex,f">;

} // Attributes = [NoThrow, Const]
