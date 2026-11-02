; RUN: opt -mtriple=riscv32 -mattr=+xvortex -passes='default<O2>' -S < %s \
; RUN:   | FileCheck %s --check-prefix=VORTEX
; RUN: opt -mtriple=riscv32 -passes='default<O2>' -S < %s \
; RUN:   | FileCheck %s --check-prefix=NOVORTEX

; InferAddressSpaces runs in the middle-end pipeline for XVortex, after the
; local-memory helper is inlined.

define internal ptr @local_mem() alwaysinline {
  %v = call i32 asm sideeffect "csrr $0, 0xfc5", "=r"()
  %p3 = inttoptr i32 %v to ptr addrspace(3)
  %p = addrspacecast ptr addrspace(3) %p3 to ptr
  ret ptr %p
}

define void @kernel(ptr %out, i32 %i) {
  %p = call ptr @local_mem()
  %g = getelementptr inbounds float, ptr %p, i32 %i
  store float 1.0, ptr %g, align 4
  %x = load float, ptr %p, align 4
  store float %x, ptr %out, align 4
  ret void
}

; VORTEX-LABEL: define void @kernel(
; VORTEX: store float 1.000000e+00, ptr addrspace(3)
; VORTEX: load float, ptr addrspace(3)
; NOVORTEX-LABEL: define void @kernel(
; NOVORTEX: addrspacecast ptr addrspace(3) %{{.*}} to ptr
; NOVORTEX: store float 1.000000e+00, ptr %
; NOVORTEX: load float, ptr %
