; fastcc is a private convention: it passes arguments around the registers the
; user reserved (-ffixed-xN) instead of rejecting the call. Thirteen arguments
; fill a0-a7 and t3-t6 and spill to the stack; each reserved register moves one
; more onto the stack.
;
; RUN: llc -mtriple=riscv32 -mattr=+reserve-x31 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,X31
; RUN: llc -mtriple=riscv64 -mattr=+reserve-x31 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,X31
; RUN: llc -mtriple=riscv32 -mattr=+reserve-x28,+reserve-x30 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,X28X30

; The callee reads the same locations: arguments 8-10 from t3-t5 and the rest
; from the stack, or 8-9 from t4 and t6.
; CHECK-LABEL: callee:
; X31-DAG: lw {{a[0-9]}}, 0(sp)
; X31-DAG: add t3, t3, t4
; X31-DAG: add t3, t3, t5
; X31-NOT: t6
; X28X30-NOT: {{t3|t5}}
; X28X30: add {{t4}}, t4, t6
; X28X30-NOT: {{t3|t5}}
; CHECK: ret
define internal fastcc i32 @callee(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4,
                                   i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9,
                                   i32 %a10, i32 %a11, i32 %a12) noinline {
  %s0 = add i32 %a8, %a9
  %s1 = add i32 %s0, %a10
  %s2 = add i32 %s1, %a11
  %s3 = add i32 %s2, %a12
  ret i32 %s3
}

; CHECK-LABEL: caller:
; X31-NOT: t6
; X31-DAG: li t3, 8
; X31-DAG: li t4, 9
; X31-DAG: li t5, 10
; X31-NOT: t6
; X28X30-NOT: {{t3|t5}}
; X28X30-DAG: li t4, 8
; X28X30-DAG: li t6, 9
; X28X30-NOT: {{t3|t5}}
; CHECK: call callee
define i32 @caller() nounwind {
  %r = call fastcc i32 @callee(i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6,
                               i32 7, i32 8, i32 9, i32 10, i32 11, i32 12)
  ret i32 %r
}
