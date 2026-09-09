; Test the -vortex-divergence-arch codegen modes.
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=ipdom -O2 < %s | FileCheck %s --check-prefixes=IPDOM
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=scs -O2 < %s | FileCheck %s --check-prefixes=SCS
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=its -O2 < %s | FileCheck %s --check-prefixes=ITS
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=its -vortex-scs-yield=0 -O2 < %s | FileCheck %s --check-prefixes=ITSCB
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -O2 < %s | FileCheck %s --check-prefixes=SCS

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
target triple = "riscv32"

declare i32 @llvm.riscv.vx.tid.i32()

; A divergent branch with side effects on both paths: split/join under
; ipdom/scs, bar_add/bar_wait under its.

; IPDOM-LABEL: branch_kernel:
; IPDOM: vx_split
; IPDOM: vx_join
; IPDOM-NOT: vx_yield
; IPDOM-NOT: vx_bar_add

; SCS-LABEL: branch_kernel:
; SCS: vx_split
; SCS: vx_join
; SCS-NOT: vx_bar_add

; ITS-LABEL: branch_kernel:
; ITS-NOT: vx_split
; ITS: vx_bar_add 0
; ITS: vx_bar_wait 0
; ITS-NOT: vx_join
define void @branch_kernel(ptr %out, ptr %in, ptr %flag) #0 {
entry:
  %tid = call i32 @llvm.riscv.vx.tid.i32()
  %p = getelementptr inbounds i32, ptr %in, i32 %tid
  %v = load i32, ptr %p, align 4
  %c = icmp sgt i32 %v, 0
  br i1 %c, label %then, label %else

then:
  %fq = getelementptr inbounds i32, ptr %flag, i32 %tid
  store i32 1, ptr %fq, align 4
  br label %merge

else:
  %b = add i32 %v, 7
  %q0 = getelementptr inbounds i32, ptr %out, i32 %tid
  store i32 %b, ptr %q0, align 4
  br label %merge

merge:
  %q = getelementptr inbounds i32, ptr %out, i32 %tid
  store i32 %v, ptr %q, align 4
  ret void
}

; A divergent-exit loop: pred under ipdom/scs, bar_add (preheader) +
; bar_wait (exit) under its.

; IPDOM-LABEL: loop_kernel:
; IPDOM: vx_pred
; IPDOM-NOT: vx_yield
; IPDOM-NOT: vx_bar_add

; SCS-LABEL: loop_kernel:
; SCS: vx_pred

; ITS-LABEL: loop_kernel:
; ITS: vx_bar_add 0
; ITS-NOT: vx_pred
; ITS: vx_bar_wait 0
define void @loop_kernel(ptr %out, ptr %in) #0 {
entry:
  %tid = call i32 @llvm.riscv.vx.tid.i32()
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %p = getelementptr inbounds i32, ptr %in, i32 %i
  %v = load i32, ptr %p, align 4
  %inc = add nuw i32 %i, 1
  %done = icmp sge i32 %inc, %tid
  br i1 %done, label %exit, label %loop

exit:
  %q = getelementptr inbounds i32, ptr %out, i32 %tid
  store i32 %tid, ptr %q, align 4
  ret void
}

; A blocking (atomic) divergent loop: vx_yield on the back-edge under scs
; and its (forward progress); the ipdom baseline never yields, and
; -vortex-scs-yield=0 restores the barriers-only its arm.

; IPDOM-LABEL: lock_kernel:
; IPDOM: vx_pred
; IPDOM-NOT: vx_yield

; SCS-LABEL: lock_kernel:
; SCS: vx_pred
; SCS: vx_yield

; ITS-LABEL: lock_kernel:
; ITS: vx_bar_add 0
; ITS: vx_yield
; ITS: vx_bar_wait 0

; ITSCB-LABEL: lock_kernel:
; ITSCB-NOT: vx_yield
; ITSCB: vx_bar_add 0
; ITSCB: vx_bar_wait 0
define void @lock_kernel(ptr %lock, ptr %out) #0 {
entry:
  %tid = call i32 @llvm.riscv.vx.tid.i32()
  br label %spin

spin:
  %old = atomicrmw xchg ptr %lock, i32 1 acquire
  %free = icmp eq i32 %old, 0
  br i1 %free, label %crit, label %spin

crit:
  %q = getelementptr inbounds i32, ptr %out, i32 %tid
  store i32 %tid, ptr %q, align 4
  store atomic i32 0, ptr %lock release, align 4
  ret void
}

attributes #0 = { nounwind "target-features"="+m,+a,+xvortex" }
