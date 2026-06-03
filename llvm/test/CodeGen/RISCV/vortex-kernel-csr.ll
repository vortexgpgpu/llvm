; Vortex kernel entries do not preserve callee-saved registers: they are
; dispatched once by the KMU/__vx_cta_entry trampoline and discarded, so the
; s0-s11 / fs0-fs11 prologue/epilogue spills are pure overhead. `ra` must still
; be saved for non-leaf kernels so the return to the trampoline is correct.
; Ordinary functions keep the standard ABI. The marker is honored both as the
; "vortex-kernel" function attribute and as the `vortex.kernel` global
; annotation Clang emits for the `__kernel` macro.
;
; RUN: llc -mtriple=riscv32 < %s | FileCheck %s

declare i32 @ext(i32)

; Kernel via the function attribute. Non-leaf (three calls, values live across
; them) so it has real callee-saved pressure — yet emits no s-register saves;
; ra is still saved.
; CHECK-LABEL: kernel_attr:
; CHECK-NOT: sw s
; CHECK: sw ra,
; CHECK-NOT: sw s
; CHECK: ret
define void @kernel_attr(ptr %p) #0 {
  %a = call i32 @ext(i32 1)
  %b = call i32 @ext(i32 2)
  %c = call i32 @ext(i32 3)
  %s1 = add i32 %a, %b
  %s2 = add i32 %s1, %c
  store i32 %s2, ptr %p
  ret void
}

; Kernel via the `vortex.kernel` global annotation (Clang's __kernel form).
; CHECK-LABEL: kernel_annot:
; CHECK-NOT: sw s
; CHECK: sw ra,
; CHECK-NOT: sw s
; CHECK: ret
define void @kernel_annot(ptr %p) {
  %a = call i32 @ext(i32 1)
  %b = call i32 @ext(i32 2)
  %c = call i32 @ext(i32 3)
  %s1 = add i32 %a, %b
  %s2 = add i32 %s1, %c
  store i32 %s2, ptr %p
  ret void
}

; Same body, ordinary function: standard ABI, so it DOES spill callee-saved
; registers in the prologue.
; CHECK-LABEL: plain:
; CHECK: sw s{{[0-9]+}},
; CHECK: ret
define void @plain(ptr %p) {
  %a = call i32 @ext(i32 1)
  %b = call i32 @ext(i32 2)
  %c = call i32 @ext(i32 3)
  %s1 = add i32 %a, %b
  %s2 = add i32 %s1, %c
  store i32 %s2, ptr %p
  ret void
}

attributes #0 = { "vortex-kernel" }

@.str.ann = private unnamed_addr constant [14 x i8] c"vortex.kernel\00", section "llvm.metadata"
@.str.file = private unnamed_addr constant [5 x i8] c"k.cl\00", section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { ptr, ptr, ptr, i32, ptr }]
  [{ ptr, ptr, ptr, i32, ptr } { ptr @kernel_annot, ptr @.str.ann, ptr @.str.file, i32 1, ptr null }],
  section "llvm.metadata"
