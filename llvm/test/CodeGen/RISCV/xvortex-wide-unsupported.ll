; RUN: llc -mtriple=riscv32 -mattr=+f,+d,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --implicit-check-not=vx_
; RUN: llc -mtriple=riscv64 -mattr=+f,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --implicit-check-not=vx_
; RUN: llc -mtriple=riscv64 -mattr=+f,+d,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --implicit-check-not=vx_

; vx_flw.d/.q and vx_fsw.d/.q exist only on RV32 with FLEN 32. With D or on
; RV64, float accesses stay scalar in every address space.

define float @local_float4(ptr addrspace(3) align 16 %p) {
; CHECK-LABEL: local_float4:
; CHECK-COUNT-4: flw
  %p1 = getelementptr inbounds float, ptr addrspace(3) %p, i32 1
  %p2 = getelementptr inbounds float, ptr addrspace(3) %p, i32 2
  %p3 = getelementptr inbounds float, ptr addrspace(3) %p, i32 3
  %a = load float, ptr addrspace(3) %p, align 16
  %b = load float, ptr addrspace(3) %p1, align 4
  %c = load float, ptr addrspace(3) %p2, align 8
  %d = load float, ptr addrspace(3) %p3, align 4
  %ab = fadd float %a, %b
  %cd = fmul float %c, %d
  %r = fsub float %ab, %cd
  ret float %r
}

define float @local_v4f32(ptr addrspace(3) %p) {
; CHECK-LABEL: local_v4f32:
; CHECK-COUNT-4: flw
  %v = load <4 x float>, ptr addrspace(3) %p, align 16
  %a = extractelement <4 x float> %v, i32 0
  %b = extractelement <4 x float> %v, i32 1
  %c = extractelement <4 x float> %v, i32 2
  %d = extractelement <4 x float> %v, i32 3
  %ab = fadd float %a, %b
  %cd = fmul float %c, %d
  %r = fsub float %ab, %cd
  ret float %r
}

define float @local_float2(ptr addrspace(3) align 8 %p) {
; CHECK-LABEL: local_float2:
; CHECK-COUNT-2: flw
  %p1 = getelementptr inbounds float, ptr addrspace(3) %p, i32 1
  %a = load float, ptr addrspace(3) %p, align 8
  %b = load float, ptr addrspace(3) %p1, align 4
  %r = fsub float %a, %b
  ret float %r
}

define void @global_store_float4(ptr addrspace(1) %p, float %a, float %b, float %c, float %d) {
; CHECK-LABEL: global_store_float4:
; CHECK-COUNT-4: fsw
  %p1 = getelementptr inbounds float, ptr addrspace(1) %p, i32 1
  %p2 = getelementptr inbounds float, ptr addrspace(1) %p, i32 2
  %p3 = getelementptr inbounds float, ptr addrspace(1) %p, i32 3
  store float %a, ptr addrspace(1) %p, align 4
  store float %b, ptr addrspace(1) %p1, align 4
  store float %c, ptr addrspace(1) %p2, align 4
  store float %d, ptr addrspace(1) %p3, align 4
  ret void
}

define void @generic_copy_v4f32(ptr %p, ptr %q) {
; CHECK-LABEL: generic_copy_v4f32:
; CHECK-COUNT-4: flw
; CHECK-COUNT-4: fsw
  %v = load <4 x float>, ptr %p, align 4
  store <4 x float> %v, ptr %q, align 4
  ret void
}

define void @generic_copy_v2f32(ptr %p, ptr %q) {
; CHECK-LABEL: generic_copy_v2f32:
; CHECK-COUNT-2: flw
; CHECK-COUNT-2: fsw
  %v = load <2 x float>, ptr %p, align 4
  store <2 x float> %v, ptr %q, align 4
  ret void
}
