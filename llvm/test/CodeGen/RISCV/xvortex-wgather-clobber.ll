; XVortex WGATHER writes its destination on every lane of the warp, active or
; not, so no lane parked by divergence may hold a value in the register the
; intrinsics bind it to (x31, or x30 for a second packed operand) while a
; gather can run. Where lanes park -- split, join, predicate and thread-mask
; instructions, and inline asm statements -- the register is clobbered in any
; function that can reach a gather while lanes are parked; it stays allocatable
; everywhere else. Code outside the module, and a callback a side-effecting asm
; statement can trap to, may gather. Each test keeps 15 values live across a
; parking point, enough for the allocator to reach t5/t6 when they are free.
;
; RUN: llc -mtriple=riscv32 -mattr=+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=riscv64 -mattr=+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=riscv32 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=NOVX

declare void @external()


; The gather's destination is kept free for the parked lanes; the other
; warp-gather register is not.
; CHECK-LABEL: gather_x31:
; CHECK-NOT: t6
; CHECK: lw t5, 0(a0)
; CHECK-NOT: t6
; CHECK: vx_split
; CHECK-NOT: t6
; CHECK: ret
; NOVX-LABEL: gather_x31:
; NOVX: lw t6, 0(a0)
; NOVX: ret
define void @gather_x31(ptr %p, ptr %q, i1 %c) {
entry:
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  br i1 %c, label %gather, label %keep

gather:
  %g = call i32 asm sideeffect "# gather", "={x31},{x31}"(i32 poison)
  br label %done

keep:
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  br label %done

done:
  ret void
}

; CHECK-LABEL: gather_x30:
; CHECK-NOT: t5
; CHECK: lw t6, 0(a0)
; CHECK-NOT: t5
; CHECK: ret
define void @gather_x30(ptr %p, ptr %q, i1 %c) {
entry:
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  br i1 %c, label %gather, label %keep

gather:
  %g = call i32 asm sideeffect "# gather", "={t5},0"(i32 poison)
  br label %done

keep:
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  br label %done

done:
  ret void
}

; A function that cannot reach a gather keeps both.
; CHECK-LABEL: no_gather:
; CHECK: lw t5, 0(a0)
; CHECK: lw t6, 0(a0)
; CHECK: ret
define void @no_gather(ptr %p, ptr %q, i1 %c) {
entry:
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  br i1 %c, label %gather, label %keep

gather:
  call void @plain_helper()
  br label %done

keep:
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  br label %done

done:
  ret void
}

; Away from the parking points the register stays allocatable.
; CHECK-LABEL: gather_uniform:
; CHECK: lw t6, 0(a0)
; CHECK: sw t6, 0(a1)
; CHECK: # gather
define void @gather_uniform(ptr %p, ptr %q) {
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  %g = call i32 asm sideeffect "# gather", "={x31},{x31}"(i32 poison)
  ret void
}

; A side-effecting asm statement can park lanes (the intrinsics' thread-mask and
; split forms) and trap to a callback that gathers (a ray-tracing wait); an asm
; statement without side effects does neither.
; CHECK-LABEL: asm_parking:
; CHECK-NOT: {{t5|t6}}
; CHECK: # park
; CHECK-NOT: {{t5|t6}}
; CHECK: ret
define void @asm_parking(ptr %p, ptr %q) {
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  call void asm sideeffect "# park", "~{memory}"()
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  ret void
}

; CHECK-LABEL: asm_pure:
; CHECK: lw t6, 0(a0)
; CHECK: # pure
; CHECK: sw t6, 0(a1)
define void @asm_pure(ptr %p, ptr %q) {
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  %x = call i32 asm "# pure $0", "=r"()
  store volatile i32 %x, ptr %q
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  ret void
}

; A caller's lanes stay parked while the callee gathers.
; CHECK-LABEL: calls_gather:
; CHECK-NOT: t6
; CHECK: ret
define void @calls_gather(ptr %p, ptr %q, i1 %c) {
entry:
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  br i1 %c, label %gather, label %keep

gather:
  call void @gather_helper()
  br label %done

keep:
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  br label %done

done:
  ret void
}

; CHECK-LABEL: calls_external:
; CHECK-NOT: {{t5|t6}}
; CHECK: ret
define void @calls_external(ptr %p, ptr %q, i1 %c) {
entry:
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  br i1 %c, label %gather, label %keep

gather:
  call void @external()
  br label %done

keep:
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  br label %done

done:
  ret void
}

; CHECK-LABEL: calls_indirect:
; CHECK-NOT: {{t5|t6}}
; CHECK: ret
define void @calls_indirect(ptr %p, ptr %q, i1 %c, ptr %f) {
entry:
  %a0 = load volatile i32, ptr %p
  %a1 = load volatile i32, ptr %p
  %a2 = load volatile i32, ptr %p
  %a3 = load volatile i32, ptr %p
  %a4 = load volatile i32, ptr %p
  %a5 = load volatile i32, ptr %p
  %a6 = load volatile i32, ptr %p
  %a7 = load volatile i32, ptr %p
  %a8 = load volatile i32, ptr %p
  %a9 = load volatile i32, ptr %p
  %a10 = load volatile i32, ptr %p
  %a11 = load volatile i32, ptr %p
  %a12 = load volatile i32, ptr %p
  %a13 = load volatile i32, ptr %p
  %a14 = load volatile i32, ptr %p
  br i1 %c, label %gather, label %keep

gather:
  call void %f()
  br label %done

keep:
  store volatile i32 %a0, ptr %q
  store volatile i32 %a1, ptr %q
  store volatile i32 %a2, ptr %q
  store volatile i32 %a3, ptr %q
  store volatile i32 %a4, ptr %q
  store volatile i32 %a5, ptr %q
  store volatile i32 %a6, ptr %q
  store volatile i32 %a7, ptr %q
  store volatile i32 %a8, ptr %q
  store volatile i32 %a9, ptr %q
  store volatile i32 %a10, ptr %q
  store volatile i32 %a11, ptr %q
  store volatile i32 %a12, ptr %q
  store volatile i32 %a13, ptr %q
  store volatile i32 %a14, ptr %q
  br label %done

done:
  ret void
}

; CHECK-LABEL: gather_helper:
define void @gather_helper() noinline {
  %g = call i32 asm sideeffect "# gather", "={x31},{x31}"(i32 poison)
  ret void
}

; CHECK-LABEL: plain_helper:
define void @plain_helper() noinline {
  ret void
}
