; RUN: split-file %s %t
; RUN: llc -mtriple=riscv32 -mattr=+m,+xvortex -verify-machineinstrs \
; RUN:   < %t/rv32.ll | FileCheck %s --check-prefix=RV32
; RUN: llc -mtriple=riscv64 -mattr=+m,+xvortex -verify-machineinstrs \
; RUN:   < %t/rv64.ll | FileCheck %s --check-prefix=RV64
; RUN: not llc -mtriple=riscv64 -mattr=+m,+xvortex < %t/rv32.ll 2>&1 \
; RUN:   | FileCheck %s --check-prefix=ERR
; RUN: not llc -mtriple=riscv32 -mattr=+m,+xvortex < %t/rv64.ll 2>&1 \
; RUN:   | FileCheck %s --check-prefix=ERR

; The "cg" constraint binds an XVortex register group as one inline-asm
; operand, chosen by the operand's type. An integer group has XLen lanes:
; 2, 4 or 8 uintptr_t (v2i32/v4i32/v8i32 on RV32, v2i64/v4i64/v8i64 on RV64)
; bind a GPR pair, quad or octet (GPRG2/4/8), aligned to its size, and print
; as the group's first register. Lanes built from scalars are computed into
; the group's registers. Another lane width is rejected, not miscompiled.

; RV32-LABEL: pair:
; RV32-NOT:     sp
; RV32:         # pair a0
; RV32-LABEL: quad:
; RV32:         mv a2, a0
; RV32-NEXT:    slli a0, a1, 1
; RV32-NEXT:    add a3, a2, a1
; RV32-NEXT:    add a4, a2, a0
; RV32-NEXT:    add a1, a2, a1
; RV32-NEXT:    add a5, a1, a0
; RV32-NEXT:    #APP
; RV32-NEXT:    # quad a2
; RV32-LABEL: quad_out:
; RV32:         # def a2
; RV32:         add a0, a2, a5
; RV32-LABEL: oct:
; RV32-NOT:     andi sp
; RV32:         # oct s0

; RV64-LABEL: pair:
; RV64-NOT:     sp
; RV64:         # pair a0
; RV64-LABEL: quad:
; RV64:         mv a2, a0
; RV64-NEXT:    slli a0, a1, 1
; RV64-NEXT:    add a3, a2, a1
; RV64-NEXT:    add a4, a2, a0
; RV64-NEXT:    add a1, a2, a1
; RV64-NEXT:    add a5, a1, a0
; RV64-NEXT:    #APP
; RV64-NEXT:    # quad a2
; RV64-LABEL: quad_out:
; RV64:         # def a2
; RV64:         add a0, a2, a5
; RV64-LABEL: oct:
; RV64-NOT:     andi sp
; RV64:         # oct s0

; ERR: error: couldn't allocate input reg for constraint 'cg'
; ERR: error: couldn't allocate input reg for constraint 'cg'
; ERR: error: couldn't allocate output register for constraint 'cg'
; ERR: error: couldn't allocate input reg for constraint 'cg'

;--- rv32.ll
define void @pair(i32 %a, i32 %b) {
  %v0 = insertelement <2 x i32> poison, i32 %a, i32 0
  %v1 = insertelement <2 x i32> %v0, i32 %b, i32 1
  call void asm sideeffect "# pair $0", "^cg"(<2 x i32> %v1)
  ret void
}
define void @quad(i32 %base, i32 %row) {
  %r2 = shl i32 %row, 1
  %r3 = mul i32 %row, 3
  %g1 = add i32 %base, %row
  %g2 = add i32 %base, %r2
  %g3 = add i32 %base, %r3
  %v0 = insertelement <4 x i32> poison, i32 %base, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %g1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %g2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %g3, i32 3
  call void asm sideeffect "# quad $0", "^cg"(<4 x i32> %v3)
  ret void
}
define i32 @quad_out(i32 %s) {
  %g = call <4 x i32> asm sideeffect "# def $0", "=^cg"()
  %e0 = extractelement <4 x i32> %g, i32 0
  %e3 = extractelement <4 x i32> %g, i32 3
  %r = add i32 %e0, %e3
  ret i32 %r
}
define void @oct(i32 %a) {
  %v0 = insertelement <8 x i32> poison, i32 %a, i32 0
  %v = shufflevector <8 x i32> %v0, <8 x i32> poison, <8 x i32> zeroinitializer
  call void asm sideeffect "# oct $0", "^cg"(<8 x i32> %v)
  ret void
}

;--- rv64.ll
define void @pair(i64 %a, i64 %b) {
  %v0 = insertelement <2 x i64> poison, i64 %a, i32 0
  %v1 = insertelement <2 x i64> %v0, i64 %b, i32 1
  call void asm sideeffect "# pair $0", "^cg"(<2 x i64> %v1)
  ret void
}
define void @quad(i64 %base, i64 %row) {
  %r2 = shl i64 %row, 1
  %r3 = mul i64 %row, 3
  %g1 = add i64 %base, %row
  %g2 = add i64 %base, %r2
  %g3 = add i64 %base, %r3
  %v0 = insertelement <4 x i64> poison, i64 %base, i32 0
  %v1 = insertelement <4 x i64> %v0, i64 %g1, i32 1
  %v2 = insertelement <4 x i64> %v1, i64 %g2, i32 2
  %v3 = insertelement <4 x i64> %v2, i64 %g3, i32 3
  call void asm sideeffect "# quad $0", "^cg"(<4 x i64> %v3)
  ret void
}
define i64 @quad_out(i64 %s) {
  %g = call <4 x i64> asm sideeffect "# def $0", "=^cg"()
  %e0 = extractelement <4 x i64> %g, i32 0
  %e3 = extractelement <4 x i64> %g, i32 3
  %r = add i64 %e0, %e3
  ret i64 %r
}
define void @oct(i64 %a) {
  %v0 = insertelement <8 x i64> poison, i64 %a, i32 0
  %v = shufflevector <8 x i64> %v0, <8 x i64> poison, <8 x i32> zeroinitializer
  call void asm sideeffect "# oct $0", "^cg"(<8 x i64> %v)
  ret void
}
