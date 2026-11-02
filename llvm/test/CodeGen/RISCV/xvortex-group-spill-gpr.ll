; RUN: llc -mtriple=riscv32 -mattr=+m,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=RV32
; RUN: llc -mtriple=riscv64 -mattr=+m,+xvortex -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=RV64

; GPR register groups live across a call spill through their group pseudos.
; GPRG8 spill slots need only XLEN alignment (4 bytes on RV32, 8 on RV64), so
; no stack realignment appears. On RV64 a GPRG2 (v2i64) group shares the
; registers of GPRPair, whose Zdinx spill pseudo stores 32-bit halves; it
; spills with sd/ld instead.

; RV32-LABEL: spill_v8i32:
; RV32-NOT:   andi sp
; RV32:       # 4-byte Folded Spill
; RV32-NOT:   andi sp
; RV32:       ret

; RV64-LABEL: spill_v8i32:
; RV64-NOT:   andi sp
; RV64:       # 8-byte Folded Spill
; RV64-NOT:   andi sp
; RV64:       ret

; RV64-LABEL: spill_v2i64:
; RV64-NOT:   {{(sw|lw) .* Folded}}
; RV64:       sd {{.*}} # 8-byte Folded Spill
; RV64-NOT:   {{(sw|lw) .* Folded}}
; RV64:       ld {{.*}} # 8-byte Folded Reload
; RV64-NOT:   {{(sw|lw) .* Folded}}
; RV64:       ret

declare void @clobber()
define void @spill_v8i32(ptr %p, ptr %q) {
  %p0 = getelementptr inbounds <8 x i32>, ptr %p, i32 0
  %v0 = load <8 x i32>, ptr %p0, align 4
  %p1 = getelementptr inbounds <8 x i32>, ptr %p, i32 1
  %v1 = load <8 x i32>, ptr %p1, align 4
  %p2 = getelementptr inbounds <8 x i32>, ptr %p, i32 2
  %v2 = load <8 x i32>, ptr %p2, align 4
  call void @clobber()
  %q0 = getelementptr inbounds <8 x i32>, ptr %q, i32 0
  store <8 x i32> %v0, ptr %q0, align 4
  %q1 = getelementptr inbounds <8 x i32>, ptr %q, i32 1
  store <8 x i32> %v1, ptr %q1, align 4
  %q2 = getelementptr inbounds <8 x i32>, ptr %q, i32 2
  store <8 x i32> %v2, ptr %q2, align 4
  ret void
}
define void @spill_v2i64(ptr %p, ptr %q) {
  %p0 = getelementptr inbounds <2 x i64>, ptr %p, i32 0
  %v0 = load <2 x i64>, ptr %p0, align 4
  %p1 = getelementptr inbounds <2 x i64>, ptr %p, i32 1
  %v1 = load <2 x i64>, ptr %p1, align 4
  %p2 = getelementptr inbounds <2 x i64>, ptr %p, i32 2
  %v2 = load <2 x i64>, ptr %p2, align 4
  %p3 = getelementptr inbounds <2 x i64>, ptr %p, i32 3
  %v3 = load <2 x i64>, ptr %p3, align 4
  %p4 = getelementptr inbounds <2 x i64>, ptr %p, i32 4
  %v4 = load <2 x i64>, ptr %p4, align 4
  %p5 = getelementptr inbounds <2 x i64>, ptr %p, i32 5
  %v5 = load <2 x i64>, ptr %p5, align 4
  %p6 = getelementptr inbounds <2 x i64>, ptr %p, i32 6
  %v6 = load <2 x i64>, ptr %p6, align 4
  %p7 = getelementptr inbounds <2 x i64>, ptr %p, i32 7
  %v7 = load <2 x i64>, ptr %p7, align 4
  call void @clobber()
  %q0 = getelementptr inbounds <2 x i64>, ptr %q, i32 0
  store <2 x i64> %v0, ptr %q0, align 4
  %q1 = getelementptr inbounds <2 x i64>, ptr %q, i32 1
  store <2 x i64> %v1, ptr %q1, align 4
  %q2 = getelementptr inbounds <2 x i64>, ptr %q, i32 2
  store <2 x i64> %v2, ptr %q2, align 4
  %q3 = getelementptr inbounds <2 x i64>, ptr %q, i32 3
  store <2 x i64> %v3, ptr %q3, align 4
  %q4 = getelementptr inbounds <2 x i64>, ptr %q, i32 4
  store <2 x i64> %v4, ptr %q4, align 4
  %q5 = getelementptr inbounds <2 x i64>, ptr %q, i32 5
  store <2 x i64> %v5, ptr %q5, align 4
  %q6 = getelementptr inbounds <2 x i64>, ptr %q, i32 6
  store <2 x i64> %v6, ptr %q6, align 4
  %q7 = getelementptr inbounds <2 x i64>, ptr %q, i32 7
  store <2 x i64> %v7, ptr %q7, align 4
  ret void
}
