; RUN: llc -mtriple=riscv32 -mattr=+m,+f,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

; XVortex register groups that stay live across a call, with every lane used
; after it, are spilled and reloaded through one pseudo per group. The
; pseudos expand into one scalar access per register after register
; allocation. xvortex-group-spill-expand.mir checks the expansion of a partly
; defined group and of a reload whose base register is one of its lanes.

declare void @clobber()
declare void @use_buf(ptr)

define float @spill_fprg2(ptr addrspace(1) %p) {
; CHECK-LABEL: spill_fprg2:
; CHECK:       # 4-byte Folded Spill
; CHECK:       call clobber
; CHECK:       # 4-byte Folded Reload
  %p0 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 0
  %v0 = load <2 x float>, ptr addrspace(1) %p0, align 8
  %p1 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 1
  %v1 = load <2 x float>, ptr addrspace(1) %p1, align 8
  %p2 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 2
  %v2 = load <2 x float>, ptr addrspace(1) %p2, align 8
  %p3 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 3
  %v3 = load <2 x float>, ptr addrspace(1) %p3, align 8
  %p4 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 4
  %v4 = load <2 x float>, ptr addrspace(1) %p4, align 8
  %p5 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 5
  %v5 = load <2 x float>, ptr addrspace(1) %p5, align 8
  %p6 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 6
  %v6 = load <2 x float>, ptr addrspace(1) %p6, align 8
  %p7 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 7
  %v7 = load <2 x float>, ptr addrspace(1) %p7, align 8
  %p8 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 8
  %v8 = load <2 x float>, ptr addrspace(1) %p8, align 8
  %p9 = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i32 9
  %v9 = load <2 x float>, ptr addrspace(1) %p9, align 8
  call void @clobber()
  %s1 = fadd <2 x float> %v0, %v1
  %s2 = fadd <2 x float> %s1, %v2
  %s3 = fadd <2 x float> %s2, %v3
  %s4 = fadd <2 x float> %s3, %v4
  %s5 = fadd <2 x float> %s4, %v5
  %s6 = fadd <2 x float> %s5, %v6
  %s7 = fadd <2 x float> %s6, %v7
  %s8 = fadd <2 x float> %s7, %v8
  %s9 = fadd <2 x float> %s8, %v9
  %e0 = extractelement <2 x float> %s9, i32 0
  %e1 = extractelement <2 x float> %s9, i32 1
  %r1 = fadd float %e0, %e1
  ret float %r1
}

define float @spill_fprg8(ptr addrspace(1) %p) {
; CHECK-LABEL: spill_fprg8:
; CHECK:       # 4-byte Folded Spill
; CHECK:       call clobber
; CHECK:       # 4-byte Folded Reload
  %p0 = getelementptr inbounds <8 x float>, ptr addrspace(1) %p, i32 0
  %v0 = load <8 x float>, ptr addrspace(1) %p0, align 32
  %p1 = getelementptr inbounds <8 x float>, ptr addrspace(1) %p, i32 1
  %v1 = load <8 x float>, ptr addrspace(1) %p1, align 32
  call void @clobber()
  %s1 = fmul <8 x float> %v0, %v1
  %e0 = extractelement <8 x float> %s1, i32 0
  %e1 = extractelement <8 x float> %s1, i32 1
  %e2 = extractelement <8 x float> %s1, i32 2
  %e3 = extractelement <8 x float> %s1, i32 3
  %e4 = extractelement <8 x float> %s1, i32 4
  %e5 = extractelement <8 x float> %s1, i32 5
  %e6 = extractelement <8 x float> %s1, i32 6
  %e7 = extractelement <8 x float> %s1, i32 7
  %r1 = fadd float %e0, %e1
  %r2 = fadd float %r1, %e2
  %r3 = fadd float %r2, %e3
  %r4 = fadd float %r3, %e4
  %r5 = fadd float %r4, %e5
  %r6 = fadd float %r5, %e6
  %r7 = fadd float %r6, %e7
  ret float %r7
}

define i32 @spill_gprg4(ptr addrspace(1) %p) {
; CHECK-LABEL: spill_gprg4:
; CHECK:       # 4-byte Folded Spill
; CHECK:       call clobber
; CHECK:       # 4-byte Folded Reload
  %p0 = getelementptr inbounds <4 x i32>, ptr addrspace(1) %p, i32 0
  %v0 = load <4 x i32>, ptr addrspace(1) %p0, align 16
  %p1 = getelementptr inbounds <4 x i32>, ptr addrspace(1) %p, i32 1
  %v1 = load <4 x i32>, ptr addrspace(1) %p1, align 16
  %p2 = getelementptr inbounds <4 x i32>, ptr addrspace(1) %p, i32 2
  %v2 = load <4 x i32>, ptr addrspace(1) %p2, align 16
  %p3 = getelementptr inbounds <4 x i32>, ptr addrspace(1) %p, i32 3
  %v3 = load <4 x i32>, ptr addrspace(1) %p3, align 16
  call void @clobber()
  %s1 = add <4 x i32> %v0, %v1
  %s2 = add <4 x i32> %s1, %v2
  %s3 = add <4 x i32> %s2, %v3
  %e0 = extractelement <4 x i32> %s3, i32 0
  %e1 = extractelement <4 x i32> %s3, i32 1
  %e2 = extractelement <4 x i32> %s3, i32 2
  %e3 = extractelement <4 x i32> %s3, i32 3
  %r1 = add i32 %e0, %e1
  %r2 = add i32 %r1, %e2
  %r3 = add i32 %r2, %e3
  ret i32 %r3
}

; GPRG2 has the registers of GPRPair and spills through its pseudo.
define i32 @spill_gprg2(ptr addrspace(1) %p) {
; CHECK-LABEL: spill_gprg2:
; CHECK:       # 4-byte Folded Spill
; CHECK:       call clobber
; CHECK:       # 4-byte Folded Reload
  %p0 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 0
  %v0 = load <2 x i32>, ptr addrspace(1) %p0, align 8
  %p1 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 1
  %v1 = load <2 x i32>, ptr addrspace(1) %p1, align 8
  %p2 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 2
  %v2 = load <2 x i32>, ptr addrspace(1) %p2, align 8
  %p3 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 3
  %v3 = load <2 x i32>, ptr addrspace(1) %p3, align 8
  %p4 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 4
  %v4 = load <2 x i32>, ptr addrspace(1) %p4, align 8
  %p5 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 5
  %v5 = load <2 x i32>, ptr addrspace(1) %p5, align 8
  %p6 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 6
  %v6 = load <2 x i32>, ptr addrspace(1) %p6, align 8
  %p7 = getelementptr inbounds <2 x i32>, ptr addrspace(1) %p, i32 7
  %v7 = load <2 x i32>, ptr addrspace(1) %p7, align 8
  call void @clobber()
  %s1 = add <2 x i32> %v0, %v1
  %s2 = add <2 x i32> %s1, %v2
  %s3 = add <2 x i32> %s2, %v3
  %s4 = add <2 x i32> %s3, %v4
  %s5 = add <2 x i32> %s4, %v5
  %s6 = add <2 x i32> %s5, %v6
  %s7 = add <2 x i32> %s6, %v7
  %e0 = extractelement <2 x i32> %s7, i32 0
  %e1 = extractelement <2 x i32> %s7, i32 1
  %r1 = add i32 %e0, %e1
  ret i32 %r1
}

; The spill slots lie beyond the 12-bit offset range from sp: the offset goes
; into a register, which the expanded accesses use as their base.
define float @spill_fprg4_large_frame(ptr addrspace(1) %p) {
; CHECK-LABEL: spill_fprg4_large_frame:
; CHECK:       # 4-byte Folded Spill
; CHECK:       call use_buf
; CHECK:       # 4-byte Folded Reload
  %buf = alloca [1024 x i32], align 4
  %p0 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 0
  %v0 = load <4 x float>, ptr addrspace(1) %p0, align 16
  %p1 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 1
  %v1 = load <4 x float>, ptr addrspace(1) %p1, align 16
  %p2 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 2
  %v2 = load <4 x float>, ptr addrspace(1) %p2, align 16
  %p3 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 3
  %v3 = load <4 x float>, ptr addrspace(1) %p3, align 16
  %p4 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 4
  %v4 = load <4 x float>, ptr addrspace(1) %p4, align 16
  %p5 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 5
  %v5 = load <4 x float>, ptr addrspace(1) %p5, align 16
  %p6 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 6
  %v6 = load <4 x float>, ptr addrspace(1) %p6, align 16
  %p7 = getelementptr inbounds <4 x float>, ptr addrspace(1) %p, i32 7
  %v7 = load <4 x float>, ptr addrspace(1) %p7, align 16
  call void @use_buf(ptr %buf)
  %s1 = fadd <4 x float> %v0, %v1
  %s2 = fadd <4 x float> %s1, %v2
  %s3 = fadd <4 x float> %s2, %v3
  %s4 = fadd <4 x float> %s3, %v4
  %s5 = fadd <4 x float> %s4, %v5
  %s6 = fadd <4 x float> %s5, %v6
  %s7 = fadd <4 x float> %s6, %v7
  %e0 = extractelement <4 x float> %s7, i32 0
  %e1 = extractelement <4 x float> %s7, i32 1
  %e2 = extractelement <4 x float> %s7, i32 2
  %e3 = extractelement <4 x float> %s7, i32 3
  %r1 = fadd float %e0, %e1
  %r2 = fadd float %r1, %e2
  %r3 = fadd float %r2, %e3
  ret float %r3
}

; Two defined lanes of a group, live across the call. The DAG keeps them in
; separate registers; the .mir test covers a partly defined group register.
define void @spill_fprg4_partial(float %x, float %y, ptr addrspace(1) %q) {
; CHECK-LABEL: spill_fprg4_partial:
; CHECK:       # 4-byte Folded Spill
; CHECK:       call clobber
; CHECK:       # 4-byte Folded Reload
  %v0 = insertelement <4 x float> poison, float %x, i32 0
  %v = insertelement <4 x float> %v0, float %y, i32 1
  call void @clobber()
  store <4 x float> %v, ptr addrspace(1) %q, align 16
  ret void
}
