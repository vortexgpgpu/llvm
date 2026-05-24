//===- IntrinsicsRISCVXVortex.td - Vortex GPGPU intrinsics --*- tablegen -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// LLVM IR intrinsics for the Vortex GPGPU vendor extension (XVortex).
//
// Naming: int_riscv_vx_<op>_<lane-type>_g<group-size>
//
// Group-N variants operate elementwise across N parallel lanes held in an
// aligned register tuple. See RISCVRegisterInfoXVortex.td for register classes.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "riscv" in {

// vxadd integer: elementwise add of two grouped vectors. Overloaded by
// element width so the same intrinsic name works for both RV32 (v*i32) and
// RV64 (v*i64) — the lane width follows XLen.
def int_riscv_vx_add_x_g2
  : DefaultAttrsIntrinsic<[llvm_anyvector_ty],
                          [LLVMMatchType<0>, LLVMMatchType<0>],
                          [IntrNoMem, IntrSpeculatable]>;

def int_riscv_vx_add_x_g4
  : DefaultAttrsIntrinsic<[llvm_anyvector_ty],
                          [LLVMMatchType<0>, LLVMMatchType<0>],
                          [IntrNoMem, IntrSpeculatable]>;

def int_riscv_vx_add_x_g8
  : DefaultAttrsIntrinsic<[llvm_anyvector_ty],
                          [LLVMMatchType<0>, LLVMMatchType<0>],
                          [IntrNoMem, IntrSpeculatable]>;

// vxadd.f: elementwise single-precision fadd of two grouped f32 vectors.
def int_riscv_vx_add_f_g2
  : DefaultAttrsIntrinsic<[llvm_v2f32_ty],
                          [llvm_v2f32_ty, llvm_v2f32_ty],
                          [IntrNoMem, IntrSpeculatable]>;

def int_riscv_vx_add_f_g4
  : DefaultAttrsIntrinsic<[llvm_v4f32_ty],
                          [llvm_v4f32_ty, llvm_v4f32_ty],
                          [IntrNoMem, IntrSpeculatable]>;

def int_riscv_vx_add_f_g8
  : DefaultAttrsIntrinsic<[llvm_v8f32_ty],
                          [llvm_v8f32_ty, llvm_v8f32_ty],
                          [IntrNoMem, IntrSpeculatable]>;

} // TargetPrefix = "riscv"
