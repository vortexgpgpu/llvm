// RUN: %clang_cc1 -triple riscv32 -target-feature +xvortex -cl-std=CL1.2 \
// RUN:   -emit-llvm -o - %s | FileCheck %s --check-prefix=VORTEX
// RUN: %clang_cc1 -triple riscv64 -target-feature +xvortex -cl-std=CL1.2 \
// RUN:   -emit-llvm -o - %s | FileCheck %s --check-prefix=VORTEX
// RUN: %clang_cc1 -triple riscv32 -cl-std=CL1.2 -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=NOVORTEX

// With +xvortex the OpenCL address spaces map to the Vortex LLVM address
// spaces: global and constant 1, local 3, private 0. Without it they all
// stay 0. Mangling keeps the language names either way.

// VORTEX: @c = {{.*}}addrspace(1) constant i32 7
// NOVORTEX: @c = {{.*}}constant i32 7
constant int c = 7;

// VORTEX-LABEL: define{{.*}} void @_Z6helperPU7CLlocaliPU8CLglobali(ptr addrspace(3) {{.*}}%l, ptr addrspace(1) {{.*}}%g)
// NOVORTEX-LABEL: define{{.*}} void @_Z6helperPU7CLlocaliPU8CLglobali(ptr {{.*}}%l, ptr {{.*}}%g)
__attribute__((overloadable)) void helper(local int *l, global int *g) {
  *g = *l;
}

// VORTEX-LABEL: define{{.*}} spir_kernel void @k(ptr addrspace(1) {{.*}}%g, ptr addrspace(3) {{.*}}%l, ptr addrspace(1) {{.*}}%k)
// VORTEX: load float, ptr addrspace(1)
// VORTEX: store float {{.*}}, ptr addrspace(3)
// NOVORTEX-LABEL: define{{.*}} spir_kernel void @k(ptr {{.*}}%g, ptr {{.*}}%l, ptr {{.*}}%k)
kernel void k(global float *g, local float *l, constant float *k) {
  l[0] = g[0] + k[0] + c;
}
