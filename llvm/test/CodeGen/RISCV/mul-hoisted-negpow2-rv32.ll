; RUN: llc -mtriple=riscv32 -mattr=+m -O1 < %s | FileCheck %s
; RUN: llc -mtriple=riscv32 -mattr=+m,+zba -O1 < %s | FileCheck %s

; ConstantHoisting turns the repeated i64 constant into an opaque constant.
; Type legalization splits it, and the low half (0xFFFFE000 = -2^13) reaches
; RISC-V expandMul, which zero-extends it: 0xFFFFE000 + 0x2000 == 2^32 matched
; the "2^N - 2^M" decomposition and emitted (shl X, 32), poison on i32, so the
; product's low word was dropped. The low word must stay -(x << 13).

define i32 @mul_hoisted_lo_negpow2(i64 %x, i64 %y) {
; CHECK-LABEL: mul_hoisted_lo_negpow2:
; CHECK:       lui [[C:a[0-9]+]], 1048574
; CHECK-DAG:   mul {{a[0-9]+}}, a0, [[C]]
; CHECK-DAG:   mul {{a[0-9]+}}, a2, [[C]]
; CHECK:       ret
  %a = mul i64 %x, 281474976702464
  %b = mul i64 %y, 281474976702464
  %ta = trunc i64 %a to i32
  %tb = trunc i64 %b to i32
  %r = add i32 %ta, %tb
  ret i32 %r
}

; Same with a dead second use of the constant (from the original reducer).
define i32 @mul_hoisted_lo_negpow2_dead_use(i64 %x) {
; CHECK-LABEL: mul_hoisted_lo_negpow2_dead_use:
; CHECK:       lui [[C:a[0-9]+]], 1048574
; CHECK-NEXT:  mul a0, a0, [[C]]
; CHECK-NEXT:  ret
  %a = mul i64 %x, 281474976702464
  %d = add i64 281474976702464, 0
  %t = trunc i64 %a to i32
  ret i32 %t
}

; Low half 0x80000000 (2^31): 2^31 + 2^31 == 2^32 hit the same path.
define i32 @mul_hoisted_lo_2pow31(i64 %x, i64 %y) {
; CHECK-LABEL: mul_hoisted_lo_2pow31:
; CHECK:       lui [[C:a[0-9]+]], 524288
; CHECK-DAG:   mul {{a[0-9]+}}, a0, [[C]]
; CHECK-DAG:   mul {{a[0-9]+}}, a2, [[C]]
; CHECK:       ret
  %a = mul i64 %x, 320256383909888
  %b = mul i64 %y, 320256383909888
  %ta = trunc i64 %a to i32
  %tb = trunc i64 %b to i32
  %r = xor i32 %ta, %tb
  ret i32 %r
}
