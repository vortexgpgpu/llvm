; fastcc passes its arguments around the registers each side reserves, so a
; caller and callee that reserve different ones cannot agree on where they are.
; RUN: not llc -mtriple=riscv32 < %s 2>&1 | FileCheck %s

; CHECK: in function caller {{.*}} fastcc callee reserves different registers than its caller.

define internal fastcc i32 @callee(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4,
                                   i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9,
                                   i32 %a10, i32 %a11, i32 %a12) noinline #0 {
  ret i32 %a11
}

define i32 @caller() nounwind {
  %r = call fastcc i32 @callee(i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6,
                               i32 7, i32 8, i32 9, i32 10, i32 11, i32 12)
  ret i32 %r
}

attributes #0 = { "target-features"="+reserve-x31" }
