; Test the SCS-only fused divergence loop-branch (vx_pbr).
; The predicate fusion replaces the setcc+vx_pred+branch at a divergent loop exit
; with a single vx_pbr, and is honored ONLY under -vortex-divergence-arch=scs.
;
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=scs -vortex-fused-divergence=true -O2 < %s | FileCheck %s --check-prefixes=SCSFUSED
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=scs -O2 < %s | FileCheck %s --check-prefixes=SCS
; RUN: llc -march=riscv32 -mattr=+m,+a,+xvortex -vortex-divergence-arch=ipdom -vortex-fused-divergence=true -O2 < %s | FileCheck %s --check-prefixes=IPDOM

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
target triple = "riscv32"

declare i32 @llvm.riscv.vx.tid.i32()

; A loop whose trip count depends on the thread id -> divergent exit.
define void @loop_kernel(ptr %out) {
entry:
  %tid = call i32 @llvm.riscv.vx.tid.i32()
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %nacc, %loop ]
  %nacc = add i32 %acc, %i
  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, %tid
  br i1 %cmp, label %loop, label %exit
exit:
  store i32 %acc, ptr %out
  ret void
}

; SCS+fused: the loop exit is a vx_pbr; no legacy vx_pred survives.
; SCSFUSED-LABEL: loop_kernel:
; SCSFUSED: vx_pbr
; SCSFUSED-NOT: vx_pred
; SCSFUSED-NOT: vx_pred_n

; SCS (no fusion): the legacy loop predicate.
; SCS-LABEL: loop_kernel:
; SCS: vx_pred
; SCS-NOT: vx_pbr

; IPDOM ignores -vortex-fused-divergence (SCS-only): legacy predicate, no vx_pbr.
; IPDOM-LABEL: loop_kernel:
; IPDOM: vx_pred
; IPDOM-NOT: vx_pbr
