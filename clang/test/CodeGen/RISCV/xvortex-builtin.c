// RUN: %clang_cc1 -triple riscv32 -target-feature +xvortex -target-feature +f \
// RUN:     -emit-llvm %s -o - | FileCheck %s

typedef int   v2i32 __attribute__((vector_size(8)));
typedef int   v4i32 __attribute__((vector_size(16)));
typedef int   v8i32 __attribute__((vector_size(32)));
typedef float v2f32 __attribute__((vector_size(8)));
typedef float v4f32 __attribute__((vector_size(16)));
typedef float v8f32 __attribute__((vector_size(32)));

// Integer intrinsics are overloaded on lane width — the type-suffix
// "<2 x i32>" appears in the intrinsic name (e.g. llvm.riscv.vx.add.x.g2.v2i32).
// On RV32, `long` is 32-bit so the builtin's vector type is v*i32.
// CHECK-LABEL: @call_x_g2
// CHECK: call <2 x i32> @llvm.riscv.vx.add.x.g2.v2i32(<2 x i32> %{{.*}}, <2 x i32> %{{.*}})
v2i32 call_x_g2(v2i32 a, v2i32 b) { return __builtin_riscv_vx_add_x_g2(a, b); }

// CHECK-LABEL: @call_x_g4
// CHECK: call <4 x i32> @llvm.riscv.vx.add.x.g4.v4i32(<4 x i32> %{{.*}}, <4 x i32> %{{.*}})
v4i32 call_x_g4(v4i32 a, v4i32 b) { return __builtin_riscv_vx_add_x_g4(a, b); }

// CHECK-LABEL: @call_x_g8
// CHECK: call <8 x i32> @llvm.riscv.vx.add.x.g8.v8i32(<8 x i32> %{{.*}}, <8 x i32> %{{.*}})
v8i32 call_x_g8(v8i32 a, v8i32 b) { return __builtin_riscv_vx_add_x_g8(a, b); }

// CHECK-LABEL: @call_f_g2
// CHECK: call <2 x float> @llvm.riscv.vx.add.f.g2(<2 x float> %{{.*}}, <2 x float> %{{.*}})
v2f32 call_f_g2(v2f32 a, v2f32 b) { return __builtin_riscv_vx_add_f_g2(a, b); }

// CHECK-LABEL: @call_f_g4
// CHECK: call <4 x float> @llvm.riscv.vx.add.f.g4(<4 x float> %{{.*}}, <4 x float> %{{.*}})
v4f32 call_f_g4(v4f32 a, v4f32 b) { return __builtin_riscv_vx_add_f_g4(a, b); }

// CHECK-LABEL: @call_f_g8
// CHECK: call <8 x float> @llvm.riscv.vx.add.f.g8(<8 x float> %{{.*}}, <8 x float> %{{.*}})
v8f32 call_f_g8(v8f32 a, v8f32 b) { return __builtin_riscv_vx_add_f_g8(a, b); }
