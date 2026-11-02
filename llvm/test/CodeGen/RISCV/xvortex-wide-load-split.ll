; RUN: llc -mtriple=riscv32 -mattr=+m,+f,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=riscv32 -mattr=+m,+f,+xvortex -verify-machineinstrs \
; RUN:   -riscv-vortex-split-wide-loads=false < %s \
; RUN:   | FileCheck %s --check-prefix=NOSPLIT
; RUN: llc -mtriple=riscv32 -mattr=+m,+f -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

; A 4x4 register tile (sgemmx): 16 accumulators, and per k step one word of
; each A row and one row of B. The LoadStoreVectorizer merges each A row into
; a quad whose words are read in four different k steps, so the quad keeps 4
; registers live where scalar loads would keep 1; with the B quads the FP
; pressure exceeds 32 and the allocator spills accumulators inside the loop.
; The scheduled pressure is over the register file, so the A quads that
; cause it are split back into flw at their reads, and the loop has no
; spills. The B quads, read in consecutive instructions, stay wide. The
; functions carry XVortex in their attributes, so the last run checks that
; the vectorizer and the split follow the function's subtarget.

; CHECK-LABEL: mm4x4:
; CHECK:       .LBB0_1: # %loop
; CHECK-NOT:   (sp)
; CHECK:       vx_flw.q
; CHECK-NOT:   (sp)
; CHECK:       # %exit

; NOSPLIT-LABEL: mm4x4:
; NOSPLIT:       .LBB0_1: # %loop
; NOSPLIT:       # 4-byte Folded Spill
; NOSPLIT:       # %exit

define void @mm4x4(ptr addrspace(3) %As, ptr addrspace(3) %Bs, ptr addrspace(1) %C, i32 %n) #0 {
entry:
  br label %loop
loop:
  %it = phi i32 [ 0, %entry ], [ %it.next, %loop ]
  %pa = phi ptr addrspace(3) [ %As, %entry ], [ %pa.next, %loop ]
  %pb = phi ptr addrspace(3) [ %Bs, %entry ], [ %pb.next, %loop ]
  %acc00 = phi float [ 0.0, %entry ], [ %s3_00, %loop ]
  %acc01 = phi float [ 0.0, %entry ], [ %s3_01, %loop ]
  %acc02 = phi float [ 0.0, %entry ], [ %s3_02, %loop ]
  %acc03 = phi float [ 0.0, %entry ], [ %s3_03, %loop ]
  %acc10 = phi float [ 0.0, %entry ], [ %s3_10, %loop ]
  %acc11 = phi float [ 0.0, %entry ], [ %s3_11, %loop ]
  %acc12 = phi float [ 0.0, %entry ], [ %s3_12, %loop ]
  %acc13 = phi float [ 0.0, %entry ], [ %s3_13, %loop ]
  %acc20 = phi float [ 0.0, %entry ], [ %s3_20, %loop ]
  %acc21 = phi float [ 0.0, %entry ], [ %s3_21, %loop ]
  %acc22 = phi float [ 0.0, %entry ], [ %s3_22, %loop ]
  %acc23 = phi float [ 0.0, %entry ], [ %s3_23, %loop ]
  %acc30 = phi float [ 0.0, %entry ], [ %s3_30, %loop ]
  %acc31 = phi float [ 0.0, %entry ], [ %s3_31, %loop ]
  %acc32 = phi float [ 0.0, %entry ], [ %s3_32, %loop ]
  %acc33 = phi float [ 0.0, %entry ], [ %s3_33, %loop ]
  %pa00 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 0
  %a00 = load float, ptr addrspace(3) %pa00, align 4
  %pa01 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 4
  %a01 = load float, ptr addrspace(3) %pa01, align 4
  %pa02 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 8
  %a02 = load float, ptr addrspace(3) %pa02, align 4
  %pa03 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 12
  %a03 = load float, ptr addrspace(3) %pa03, align 4
  %pb00 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 0
  %b00 = load float, ptr addrspace(3) %pb00, align 4
  %pb01 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 1
  %b01 = load float, ptr addrspace(3) %pb01, align 4
  %pb02 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 2
  %b02 = load float, ptr addrspace(3) %pb02, align 4
  %pb03 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 3
  %b03 = load float, ptr addrspace(3) %pb03, align 4
  %s0_00 = call float @llvm.fmuladd.f32(float %a00, float %b00, float %acc00)
  %s0_01 = call float @llvm.fmuladd.f32(float %a00, float %b01, float %acc01)
  %s0_02 = call float @llvm.fmuladd.f32(float %a00, float %b02, float %acc02)
  %s0_03 = call float @llvm.fmuladd.f32(float %a00, float %b03, float %acc03)
  %s0_10 = call float @llvm.fmuladd.f32(float %a01, float %b00, float %acc10)
  %s0_11 = call float @llvm.fmuladd.f32(float %a01, float %b01, float %acc11)
  %s0_12 = call float @llvm.fmuladd.f32(float %a01, float %b02, float %acc12)
  %s0_13 = call float @llvm.fmuladd.f32(float %a01, float %b03, float %acc13)
  %s0_20 = call float @llvm.fmuladd.f32(float %a02, float %b00, float %acc20)
  %s0_21 = call float @llvm.fmuladd.f32(float %a02, float %b01, float %acc21)
  %s0_22 = call float @llvm.fmuladd.f32(float %a02, float %b02, float %acc22)
  %s0_23 = call float @llvm.fmuladd.f32(float %a02, float %b03, float %acc23)
  %s0_30 = call float @llvm.fmuladd.f32(float %a03, float %b00, float %acc30)
  %s0_31 = call float @llvm.fmuladd.f32(float %a03, float %b01, float %acc31)
  %s0_32 = call float @llvm.fmuladd.f32(float %a03, float %b02, float %acc32)
  %s0_33 = call float @llvm.fmuladd.f32(float %a03, float %b03, float %acc33)
  %pa10 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 1
  %a10 = load float, ptr addrspace(3) %pa10, align 4
  %pa11 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 5
  %a11 = load float, ptr addrspace(3) %pa11, align 4
  %pa12 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 9
  %a12 = load float, ptr addrspace(3) %pa12, align 4
  %pa13 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 13
  %a13 = load float, ptr addrspace(3) %pa13, align 4
  %pb10 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 16
  %b10 = load float, ptr addrspace(3) %pb10, align 4
  %pb11 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 17
  %b11 = load float, ptr addrspace(3) %pb11, align 4
  %pb12 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 18
  %b12 = load float, ptr addrspace(3) %pb12, align 4
  %pb13 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 19
  %b13 = load float, ptr addrspace(3) %pb13, align 4
  %s1_00 = call float @llvm.fmuladd.f32(float %a10, float %b10, float %s0_00)
  %s1_01 = call float @llvm.fmuladd.f32(float %a10, float %b11, float %s0_01)
  %s1_02 = call float @llvm.fmuladd.f32(float %a10, float %b12, float %s0_02)
  %s1_03 = call float @llvm.fmuladd.f32(float %a10, float %b13, float %s0_03)
  %s1_10 = call float @llvm.fmuladd.f32(float %a11, float %b10, float %s0_10)
  %s1_11 = call float @llvm.fmuladd.f32(float %a11, float %b11, float %s0_11)
  %s1_12 = call float @llvm.fmuladd.f32(float %a11, float %b12, float %s0_12)
  %s1_13 = call float @llvm.fmuladd.f32(float %a11, float %b13, float %s0_13)
  %s1_20 = call float @llvm.fmuladd.f32(float %a12, float %b10, float %s0_20)
  %s1_21 = call float @llvm.fmuladd.f32(float %a12, float %b11, float %s0_21)
  %s1_22 = call float @llvm.fmuladd.f32(float %a12, float %b12, float %s0_22)
  %s1_23 = call float @llvm.fmuladd.f32(float %a12, float %b13, float %s0_23)
  %s1_30 = call float @llvm.fmuladd.f32(float %a13, float %b10, float %s0_30)
  %s1_31 = call float @llvm.fmuladd.f32(float %a13, float %b11, float %s0_31)
  %s1_32 = call float @llvm.fmuladd.f32(float %a13, float %b12, float %s0_32)
  %s1_33 = call float @llvm.fmuladd.f32(float %a13, float %b13, float %s0_33)
  %pa20 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 2
  %a20 = load float, ptr addrspace(3) %pa20, align 4
  %pa21 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 6
  %a21 = load float, ptr addrspace(3) %pa21, align 4
  %pa22 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 10
  %a22 = load float, ptr addrspace(3) %pa22, align 4
  %pa23 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 14
  %a23 = load float, ptr addrspace(3) %pa23, align 4
  %pb20 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 32
  %b20 = load float, ptr addrspace(3) %pb20, align 4
  %pb21 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 33
  %b21 = load float, ptr addrspace(3) %pb21, align 4
  %pb22 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 34
  %b22 = load float, ptr addrspace(3) %pb22, align 4
  %pb23 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 35
  %b23 = load float, ptr addrspace(3) %pb23, align 4
  %s2_00 = call float @llvm.fmuladd.f32(float %a20, float %b20, float %s1_00)
  %s2_01 = call float @llvm.fmuladd.f32(float %a20, float %b21, float %s1_01)
  %s2_02 = call float @llvm.fmuladd.f32(float %a20, float %b22, float %s1_02)
  %s2_03 = call float @llvm.fmuladd.f32(float %a20, float %b23, float %s1_03)
  %s2_10 = call float @llvm.fmuladd.f32(float %a21, float %b20, float %s1_10)
  %s2_11 = call float @llvm.fmuladd.f32(float %a21, float %b21, float %s1_11)
  %s2_12 = call float @llvm.fmuladd.f32(float %a21, float %b22, float %s1_12)
  %s2_13 = call float @llvm.fmuladd.f32(float %a21, float %b23, float %s1_13)
  %s2_20 = call float @llvm.fmuladd.f32(float %a22, float %b20, float %s1_20)
  %s2_21 = call float @llvm.fmuladd.f32(float %a22, float %b21, float %s1_21)
  %s2_22 = call float @llvm.fmuladd.f32(float %a22, float %b22, float %s1_22)
  %s2_23 = call float @llvm.fmuladd.f32(float %a22, float %b23, float %s1_23)
  %s2_30 = call float @llvm.fmuladd.f32(float %a23, float %b20, float %s1_30)
  %s2_31 = call float @llvm.fmuladd.f32(float %a23, float %b21, float %s1_31)
  %s2_32 = call float @llvm.fmuladd.f32(float %a23, float %b22, float %s1_32)
  %s2_33 = call float @llvm.fmuladd.f32(float %a23, float %b23, float %s1_33)
  %pa30 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 3
  %a30 = load float, ptr addrspace(3) %pa30, align 4
  %pa31 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 7
  %a31 = load float, ptr addrspace(3) %pa31, align 4
  %pa32 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 11
  %a32 = load float, ptr addrspace(3) %pa32, align 4
  %pa33 = getelementptr inbounds float, ptr addrspace(3) %pa, i32 15
  %a33 = load float, ptr addrspace(3) %pa33, align 4
  %pb30 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 48
  %b30 = load float, ptr addrspace(3) %pb30, align 4
  %pb31 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 49
  %b31 = load float, ptr addrspace(3) %pb31, align 4
  %pb32 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 50
  %b32 = load float, ptr addrspace(3) %pb32, align 4
  %pb33 = getelementptr inbounds float, ptr addrspace(3) %pb, i32 51
  %b33 = load float, ptr addrspace(3) %pb33, align 4
  %s3_00 = call float @llvm.fmuladd.f32(float %a30, float %b30, float %s2_00)
  %s3_01 = call float @llvm.fmuladd.f32(float %a30, float %b31, float %s2_01)
  %s3_02 = call float @llvm.fmuladd.f32(float %a30, float %b32, float %s2_02)
  %s3_03 = call float @llvm.fmuladd.f32(float %a30, float %b33, float %s2_03)
  %s3_10 = call float @llvm.fmuladd.f32(float %a31, float %b30, float %s2_10)
  %s3_11 = call float @llvm.fmuladd.f32(float %a31, float %b31, float %s2_11)
  %s3_12 = call float @llvm.fmuladd.f32(float %a31, float %b32, float %s2_12)
  %s3_13 = call float @llvm.fmuladd.f32(float %a31, float %b33, float %s2_13)
  %s3_20 = call float @llvm.fmuladd.f32(float %a32, float %b30, float %s2_20)
  %s3_21 = call float @llvm.fmuladd.f32(float %a32, float %b31, float %s2_21)
  %s3_22 = call float @llvm.fmuladd.f32(float %a32, float %b32, float %s2_22)
  %s3_23 = call float @llvm.fmuladd.f32(float %a32, float %b33, float %s2_23)
  %s3_30 = call float @llvm.fmuladd.f32(float %a33, float %b30, float %s2_30)
  %s3_31 = call float @llvm.fmuladd.f32(float %a33, float %b31, float %s2_31)
  %s3_32 = call float @llvm.fmuladd.f32(float %a33, float %b32, float %s2_32)
  %s3_33 = call float @llvm.fmuladd.f32(float %a33, float %b33, float %s2_33)
  %pa.next = getelementptr inbounds float, ptr addrspace(3) %pa, i32 64
  %pb.next = getelementptr inbounds float, ptr addrspace(3) %pb, i32 64
  %it.next = add i32 %it, 1
  %done = icmp eq i32 %it.next, %n
  br i1 %done, label %exit, label %loop
exit:
  %pc00 = getelementptr inbounds float, ptr addrspace(1) %C, i32 0
  store float %s3_00, ptr addrspace(1) %pc00, align 4
  %pc01 = getelementptr inbounds float, ptr addrspace(1) %C, i32 1
  store float %s3_01, ptr addrspace(1) %pc01, align 4
  %pc02 = getelementptr inbounds float, ptr addrspace(1) %C, i32 2
  store float %s3_02, ptr addrspace(1) %pc02, align 4
  %pc03 = getelementptr inbounds float, ptr addrspace(1) %C, i32 3
  store float %s3_03, ptr addrspace(1) %pc03, align 4
  %pc10 = getelementptr inbounds float, ptr addrspace(1) %C, i32 16
  store float %s3_10, ptr addrspace(1) %pc10, align 4
  %pc11 = getelementptr inbounds float, ptr addrspace(1) %C, i32 17
  store float %s3_11, ptr addrspace(1) %pc11, align 4
  %pc12 = getelementptr inbounds float, ptr addrspace(1) %C, i32 18
  store float %s3_12, ptr addrspace(1) %pc12, align 4
  %pc13 = getelementptr inbounds float, ptr addrspace(1) %C, i32 19
  store float %s3_13, ptr addrspace(1) %pc13, align 4
  %pc20 = getelementptr inbounds float, ptr addrspace(1) %C, i32 32
  store float %s3_20, ptr addrspace(1) %pc20, align 4
  %pc21 = getelementptr inbounds float, ptr addrspace(1) %C, i32 33
  store float %s3_21, ptr addrspace(1) %pc21, align 4
  %pc22 = getelementptr inbounds float, ptr addrspace(1) %C, i32 34
  store float %s3_22, ptr addrspace(1) %pc22, align 4
  %pc23 = getelementptr inbounds float, ptr addrspace(1) %C, i32 35
  store float %s3_23, ptr addrspace(1) %pc23, align 4
  %pc30 = getelementptr inbounds float, ptr addrspace(1) %C, i32 48
  store float %s3_30, ptr addrspace(1) %pc30, align 4
  %pc31 = getelementptr inbounds float, ptr addrspace(1) %C, i32 49
  store float %s3_31, ptr addrspace(1) %pc31, align 4
  %pc32 = getelementptr inbounds float, ptr addrspace(1) %C, i32 50
  store float %s3_32, ptr addrspace(1) %pc32, align 4
  %pc33 = getelementptr inbounds float, ptr addrspace(1) %C, i32 51
  store float %s3_33, ptr addrspace(1) %pc33, align 4
  ret void
}
declare float @llvm.fmuladd.f32(float, float, float)

; Each quad's words are read one after another: the pressure fits, and the
; loads stay wide.
; CHECK-LABEL: dot16:
; CHECK-COUNT-8: vx_flw.q
; CHECK-NOT:     {{[[:space:]]}}flw{{[[:space:]]}}
; CHECK:         ret
define float @dot16(ptr addrspace(3) %a, ptr addrspace(1) %b) #0 {
  %pa0 = getelementptr inbounds float, ptr addrspace(3) %a, i32 0
  %a0 = load float, ptr addrspace(3) %pa0, align 4
  %pb0 = getelementptr inbounds float, ptr addrspace(1) %b, i32 0
  %b0 = load float, ptr addrspace(1) %pb0, align 4
  %s0 = call float @llvm.fmuladd.f32(float %a0, float %b0, float 0.0)
  %pa1 = getelementptr inbounds float, ptr addrspace(3) %a, i32 1
  %a1 = load float, ptr addrspace(3) %pa1, align 4
  %pb1 = getelementptr inbounds float, ptr addrspace(1) %b, i32 1
  %b1 = load float, ptr addrspace(1) %pb1, align 4
  %s1 = call float @llvm.fmuladd.f32(float %a1, float %b1, float %s0)
  %pa2 = getelementptr inbounds float, ptr addrspace(3) %a, i32 2
  %a2 = load float, ptr addrspace(3) %pa2, align 4
  %pb2 = getelementptr inbounds float, ptr addrspace(1) %b, i32 2
  %b2 = load float, ptr addrspace(1) %pb2, align 4
  %s2 = call float @llvm.fmuladd.f32(float %a2, float %b2, float %s1)
  %pa3 = getelementptr inbounds float, ptr addrspace(3) %a, i32 3
  %a3 = load float, ptr addrspace(3) %pa3, align 4
  %pb3 = getelementptr inbounds float, ptr addrspace(1) %b, i32 3
  %b3 = load float, ptr addrspace(1) %pb3, align 4
  %s3 = call float @llvm.fmuladd.f32(float %a3, float %b3, float %s2)
  %pa4 = getelementptr inbounds float, ptr addrspace(3) %a, i32 4
  %a4 = load float, ptr addrspace(3) %pa4, align 4
  %pb4 = getelementptr inbounds float, ptr addrspace(1) %b, i32 4
  %b4 = load float, ptr addrspace(1) %pb4, align 4
  %s4 = call float @llvm.fmuladd.f32(float %a4, float %b4, float %s3)
  %pa5 = getelementptr inbounds float, ptr addrspace(3) %a, i32 5
  %a5 = load float, ptr addrspace(3) %pa5, align 4
  %pb5 = getelementptr inbounds float, ptr addrspace(1) %b, i32 5
  %b5 = load float, ptr addrspace(1) %pb5, align 4
  %s5 = call float @llvm.fmuladd.f32(float %a5, float %b5, float %s4)
  %pa6 = getelementptr inbounds float, ptr addrspace(3) %a, i32 6
  %a6 = load float, ptr addrspace(3) %pa6, align 4
  %pb6 = getelementptr inbounds float, ptr addrspace(1) %b, i32 6
  %b6 = load float, ptr addrspace(1) %pb6, align 4
  %s6 = call float @llvm.fmuladd.f32(float %a6, float %b6, float %s5)
  %pa7 = getelementptr inbounds float, ptr addrspace(3) %a, i32 7
  %a7 = load float, ptr addrspace(3) %pa7, align 4
  %pb7 = getelementptr inbounds float, ptr addrspace(1) %b, i32 7
  %b7 = load float, ptr addrspace(1) %pb7, align 4
  %s7 = call float @llvm.fmuladd.f32(float %a7, float %b7, float %s6)
  %pa8 = getelementptr inbounds float, ptr addrspace(3) %a, i32 8
  %a8 = load float, ptr addrspace(3) %pa8, align 4
  %pb8 = getelementptr inbounds float, ptr addrspace(1) %b, i32 8
  %b8 = load float, ptr addrspace(1) %pb8, align 4
  %s8 = call float @llvm.fmuladd.f32(float %a8, float %b8, float %s7)
  %pa9 = getelementptr inbounds float, ptr addrspace(3) %a, i32 9
  %a9 = load float, ptr addrspace(3) %pa9, align 4
  %pb9 = getelementptr inbounds float, ptr addrspace(1) %b, i32 9
  %b9 = load float, ptr addrspace(1) %pb9, align 4
  %s9 = call float @llvm.fmuladd.f32(float %a9, float %b9, float %s8)
  %pa10 = getelementptr inbounds float, ptr addrspace(3) %a, i32 10
  %a10 = load float, ptr addrspace(3) %pa10, align 4
  %pb10 = getelementptr inbounds float, ptr addrspace(1) %b, i32 10
  %b10 = load float, ptr addrspace(1) %pb10, align 4
  %s10 = call float @llvm.fmuladd.f32(float %a10, float %b10, float %s9)
  %pa11 = getelementptr inbounds float, ptr addrspace(3) %a, i32 11
  %a11 = load float, ptr addrspace(3) %pa11, align 4
  %pb11 = getelementptr inbounds float, ptr addrspace(1) %b, i32 11
  %b11 = load float, ptr addrspace(1) %pb11, align 4
  %s11 = call float @llvm.fmuladd.f32(float %a11, float %b11, float %s10)
  %pa12 = getelementptr inbounds float, ptr addrspace(3) %a, i32 12
  %a12 = load float, ptr addrspace(3) %pa12, align 4
  %pb12 = getelementptr inbounds float, ptr addrspace(1) %b, i32 12
  %b12 = load float, ptr addrspace(1) %pb12, align 4
  %s12 = call float @llvm.fmuladd.f32(float %a12, float %b12, float %s11)
  %pa13 = getelementptr inbounds float, ptr addrspace(3) %a, i32 13
  %a13 = load float, ptr addrspace(3) %pa13, align 4
  %pb13 = getelementptr inbounds float, ptr addrspace(1) %b, i32 13
  %b13 = load float, ptr addrspace(1) %pb13, align 4
  %s13 = call float @llvm.fmuladd.f32(float %a13, float %b13, float %s12)
  %pa14 = getelementptr inbounds float, ptr addrspace(3) %a, i32 14
  %a14 = load float, ptr addrspace(3) %pa14, align 4
  %pb14 = getelementptr inbounds float, ptr addrspace(1) %b, i32 14
  %b14 = load float, ptr addrspace(1) %pb14, align 4
  %s14 = call float @llvm.fmuladd.f32(float %a14, float %b14, float %s13)
  %pa15 = getelementptr inbounds float, ptr addrspace(3) %a, i32 15
  %a15 = load float, ptr addrspace(3) %pa15, align 4
  %pb15 = getelementptr inbounds float, ptr addrspace(1) %b, i32 15
  %b15 = load float, ptr addrspace(1) %pb15, align 4
  %s15 = call float @llvm.fmuladd.f32(float %a15, float %b15, float %s14)
  ret float %s15
}

attributes #0 = { "target-features"="+m,+f,+xvortex" }
