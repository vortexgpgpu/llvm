; RUN: llc -mtriple=riscv32 -mattr=+m,+f,+xvortex -target-abi=ilp32f \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=ILP32F
; RUN: llc -mtriple=riscv32 -mattr=+m,+f -target-abi=ilp32f \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=ILP32F
; RUN: llc -mtriple=riscv32 -mattr=+m,+f,+xvortex -target-abi=ilp32 \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=ILP32
; RUN: llc -mtriple=riscv32 -mattr=+m,+f -target-abi=ilp32 \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=ILP32
; RUN: llc -mtriple=riscv64 -mattr=+m,+f,+d,+xvortex -target-abi=lp64d \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=LP64D
; RUN: llc -mtriple=riscv64 -mattr=+m,+f,+d -target-abi=lp64d \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=LP64D
; RUN: llc -mtriple=riscv64 -mattr=+m,+xvortex -target-abi=lp64 \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=LP64
; RUN: llc -mtriple=riscv64 -mattr=+m -target-abi=lp64 \
; RUN:   -verify-machineinstrs -stop-after=finalize-isel < %s \
; RUN:   | FileCheck %s --check-prefix=LP64

; Every fixed-length vector type that is an XVortex register group, or that
; legalizes into one (promoted, widened or split), is passed and returned
; element by element in the registers and stack slots of the plain RISC-V
; ABI: each +xvortex run shares its check prefix with the run without it.
; Checked are the fixed stack objects, the live-in registers and the
; registers of the call and the return.

declare <2 x i32> @callee_v2i32(<2 x i32>)

define <2 x i32> @fwd_v2i32(<2 x i32> %v) {
; ILP32F-LABEL: name: fwd_v2i32
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v2i32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; ILP32F: PseudoRET implicit $x10, implicit $x11{{$}}
; ILP32-LABEL: name: fwd_v2i32
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v2i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; ILP32: PseudoRET implicit $x10, implicit $x11{{$}}
; LP64D-LABEL: name: fwd_v2i32
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v2i32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64D: PseudoRET implicit $x10, implicit $x11{{$}}
; LP64-LABEL: name: fwd_v2i32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v2i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64: PseudoRET implicit $x10, implicit $x11{{$}}
  %r = call <2 x i32> @callee_v2i32(<2 x i32> %v)
  ret <2 x i32> %r
}

declare <4 x i32> @callee_v4i32(<4 x i32>)

define <4 x i32> @fwd_v4i32(<4 x i32> %v) {
; ILP32F-LABEL: name: fwd_v4i32
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v4i32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v4i32
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v4i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v4i32
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v4i32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v4i32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v4i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <4 x i32> @callee_v4i32(<4 x i32> %v)
  ret <4 x i32> %r
}

declare <8 x i32> @callee_v8i32(<8 x i32>)

define <8 x i32> @fwd_v8i32(<8 x i32> %v) {
; ILP32F-LABEL: name: fwd_v8i32
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v8i32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v8i32
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v8i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v8i32
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v8i32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v8i32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v8i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <8 x i32> @callee_v8i32(<8 x i32> %v)
  ret <8 x i32> %r
}

declare <3 x i32> @callee_v3i32(<3 x i32>)

define <3 x i32> @fwd_v3i32(<3 x i32> %v) {
; ILP32F-LABEL: name: fwd_v3i32
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v3i32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v3i32
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v3i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v3i32
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v3i32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v3i32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v3i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <3 x i32> @callee_v3i32(<3 x i32> %v)
  ret <3 x i32> %r
}

declare <16 x i32> @callee_v16i32(<16 x i32>)

define <16 x i32> @fwd_v16i32(<16 x i32> %v) {
; ILP32F-LABEL: name: fwd_v16i32
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v16i32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v16i32
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v16i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v16i32
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v16i32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v16i32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v16i32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <16 x i32> @callee_v16i32(<16 x i32> %v)
  ret <16 x i32> %r
}

declare <2 x i64> @callee_v2i64(<2 x i64>)

define <2 x i64> @fwd_v2i64(<2 x i64> %v) {
; ILP32F-LABEL: name: fwd_v2i64
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v2i64, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v2i64
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v2i64, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v2i64
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v2i64, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64D: PseudoRET implicit $x10, implicit $x11{{$}}
; LP64-LABEL: name: fwd_v2i64
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v2i64, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64: PseudoRET implicit $x10, implicit $x11{{$}}
  %r = call <2 x i64> @callee_v2i64(<2 x i64> %v)
  ret <2 x i64> %r
}

declare <4 x i64> @callee_v4i64(<4 x i64>)

define <4 x i64> @fwd_v4i64(<4 x i64> %v) {
; ILP32F-LABEL: name: fwd_v4i64
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v4i64, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v4i64
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v4i64, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v4i64
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v4i64, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v4i64
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v4i64, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <4 x i64> @callee_v4i64(<4 x i64> %v)
  ret <4 x i64> %r
}

declare <8 x i64> @callee_v8i64(<8 x i64>)

define <8 x i64> @fwd_v8i64(<8 x i64> %v) {
; ILP32F-LABEL: name: fwd_v8i64
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v8i64, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v8i64
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v8i64, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v8i64
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v8i64, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v8i64
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v8i64, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <8 x i64> @callee_v8i64(<8 x i64> %v)
  ret <8 x i64> %r
}

declare <2 x float> @callee_v2f32(<2 x float>)

define <2 x float> @fwd_v2f32(<2 x float> %v) {
; ILP32F-LABEL: name: fwd_v2f32
; ILP32F: liveins: $f10_f, $f11_f{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v2f32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $f10_f, implicit $f11_f, implicit-def $x2, implicit-def $f10_f, implicit-def $f11_f{{$}}
; ILP32F: PseudoRET implicit $f10_f, implicit $f11_f{{$}}
; ILP32-LABEL: name: fwd_v2f32
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v2f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; ILP32: PseudoRET implicit $x10, implicit $x11{{$}}
; LP64D-LABEL: name: fwd_v2f32
; LP64D: liveins: $f10_f, $f11_f{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v2f32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $f10_f, implicit $f11_f, implicit-def $x2, implicit-def $f10_f, implicit-def $f11_f{{$}}
; LP64D: PseudoRET implicit $f10_f, implicit $f11_f{{$}}
; LP64-LABEL: name: fwd_v2f32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v2f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64: PseudoRET implicit $x10, implicit $x11{{$}}
  %r = call <2 x float> @callee_v2f32(<2 x float> %v)
  ret <2 x float> %r
}

declare <4 x float> @callee_v4f32(<4 x float>)

define <4 x float> @fwd_v4f32(<4 x float> %v) {
; ILP32F-LABEL: name: fwd_v4f32
; ILP32F: liveins: $x10, $f10_f, $f11_f, $f12_f, $f13_f{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v4f32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $f10_f, implicit $f11_f, implicit $f12_f, implicit $f13_f, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v4f32
; ILP32: liveins: $x10, $x11, $x12, $x13, $x14{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v4f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit $x12, implicit $x13, implicit $x14, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v4f32
; LP64D: liveins: $x10, $f10_f, $f11_f, $f12_f, $f13_f{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v4f32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $f10_f, implicit $f11_f, implicit $f12_f, implicit $f13_f, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v4f32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v4f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <4 x float> @callee_v4f32(<4 x float> %v)
  ret <4 x float> %r
}

declare <8 x float> @callee_v8f32(<8 x float>)

define <8 x float> @fwd_v8f32(<8 x float> %v) {
; ILP32F-LABEL: name: fwd_v8f32
; ILP32F: liveins: $x10, $f10_f, $f11_f, $f12_f, $f13_f, $f14_f, $f15_f, $f16_f, $f17_f{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v8f32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $f10_f, implicit $f11_f, implicit $f12_f, implicit $f13_f, implicit $f14_f, implicit $f15_f, implicit $f16_f, implicit $f17_f, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v8f32
; ILP32: offset: 0, size: 4,
; ILP32: liveins: $x10, $x11, $x12, $x13, $x14, $x15, $x16, $x17{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v8f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit $x12, implicit $x13, implicit $x14, implicit $x15, implicit $x16, implicit $x17, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v8f32
; LP64D: liveins: $x10, $f10_f, $f11_f, $f12_f, $f13_f, $f14_f, $f15_f, $f16_f, $f17_f{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v8f32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $f10_f, implicit $f11_f, implicit $f12_f, implicit $f13_f, implicit $f14_f, implicit $f15_f, implicit $f16_f, implicit $f17_f, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v8f32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v8f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <8 x float> @callee_v8f32(<8 x float> %v)
  ret <8 x float> %r
}

declare <16 x float> @callee_v16f32(<16 x float>)

define <16 x float> @fwd_v16f32(<16 x float> %v) {
; ILP32F-LABEL: name: fwd_v16f32
; ILP32F: offset: 0, size: 4,
; ILP32F: liveins: $x10, $f10_f, $f11_f, $f12_f, $f13_f, $f14_f, $f15_f, $f16_f, $f17_f, $x11, $x12, $x13, $x14, $x15, $x16, $x17{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v16f32, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $f10_f, implicit $f11_f, implicit $f12_f, implicit $f13_f, implicit $f14_f, implicit $f15_f, implicit $f16_f, implicit $f17_f, implicit $x11, implicit $x12, implicit $x13, implicit $x14, implicit $x15, implicit $x16, implicit $x17, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v16f32
; ILP32: offset: 32, size: 4,
; ILP32: offset: 28, size: 4,
; ILP32: offset: 24, size: 4,
; ILP32: offset: 20, size: 4,
; ILP32: offset: 16, size: 4,
; ILP32: offset: 12, size: 4,
; ILP32: offset: 8, size: 4,
; ILP32: offset: 4, size: 4,
; ILP32: offset: 0, size: 4,
; ILP32: liveins: $x10, $x11, $x12, $x13, $x14, $x15, $x16, $x17{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v16f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit $x12, implicit $x13, implicit $x14, implicit $x15, implicit $x16, implicit $x17, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v16f32
; LP64D: offset: 0, size: 4,
; LP64D: liveins: $x10, $f10_f, $f11_f, $f12_f, $f13_f, $f14_f, $f15_f, $f16_f, $f17_f, $x11, $x12, $x13, $x14, $x15, $x16, $x17{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v16f32, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $f10_f, implicit $f11_f, implicit $f12_f, implicit $f13_f, implicit $f14_f, implicit $f15_f, implicit $f16_f, implicit $f17_f, implicit $x11, implicit $x12, implicit $x13, implicit $x14, implicit $x15, implicit $x16, implicit $x17, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v16f32
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v16f32, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <16 x float> @callee_v16f32(<16 x float> %v)
  ret <16 x float> %r
}

declare <4 x i16> @callee_v4i16(<4 x i16>)

define <4 x i16> @fwd_v4i16(<4 x i16> %v) {
; ILP32F-LABEL: name: fwd_v4i16
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v4i16, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v4i16
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v4i16, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v4i16
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v4i16, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v4i16
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v4i16, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <4 x i16> @callee_v4i16(<4 x i16> %v)
  ret <4 x i16> %r
}

declare <8 x i8> @callee_v8i8(<8 x i8>)

define <8 x i8> @fwd_v8i8(<8 x i8> %v) {
; ILP32F-LABEL: name: fwd_v8i8
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v8i8, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32F: PseudoRET{{$}}
; ILP32-LABEL: name: fwd_v8i8
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v8i8, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; ILP32: PseudoRET{{$}}
; LP64D-LABEL: name: fwd_v8i8
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v8i8, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64D: PseudoRET{{$}}
; LP64-LABEL: name: fwd_v8i8
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v8i8, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2{{$}}
; LP64: PseudoRET{{$}}
  %r = call <8 x i8> @callee_v8i8(<8 x i8> %v)
  ret <8 x i8> %r
}

declare <2 x half> @callee_v2f16(<2 x half>)

define <2 x half> @fwd_v2f16(<2 x half> %v) {
; ILP32F-LABEL: name: fwd_v2f16
; ILP32F: liveins: $x10, $x11{{$}}
; ILP32F: PseudoCALL target-flags(riscv-call) @callee_v2f16, csr_ilp32f_lp64f, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; ILP32F: PseudoRET implicit $x10, implicit $x11{{$}}
; ILP32-LABEL: name: fwd_v2f16
; ILP32: liveins: $x10, $x11{{$}}
; ILP32: PseudoCALL target-flags(riscv-call) @callee_v2f16, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; ILP32: PseudoRET implicit $x10, implicit $x11{{$}}
; LP64D-LABEL: name: fwd_v2f16
; LP64D: liveins: $x10, $x11{{$}}
; LP64D: PseudoCALL target-flags(riscv-call) @callee_v2f16, csr_ilp32d_lp64d, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64D: PseudoRET implicit $x10, implicit $x11{{$}}
; LP64-LABEL: name: fwd_v2f16
; LP64: liveins: $x10, $x11{{$}}
; LP64: PseudoCALL target-flags(riscv-call) @callee_v2f16, csr_ilp32_lp64, implicit-def dead $x1, implicit $x10, implicit $x11, implicit-def $x2, implicit-def $x10, implicit-def $x11{{$}}
; LP64: PseudoRET implicit $x10, implicit $x11{{$}}
  %r = call <2 x half> @callee_v2f16(<2 x half> %v)
  ret <2 x half> %r
}
