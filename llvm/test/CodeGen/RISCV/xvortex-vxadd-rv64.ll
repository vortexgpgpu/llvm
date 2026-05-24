; RUN: llc -mtriple=riscv64 -mattr=+xvortex,+f -verify-machineinstrs < %s \
; RUN:     | FileCheck %s
;
; RV64 mirror of xvortex-vxadd.ll. Integer lane width follows XLen: GPRG4
; carries v4i64 on RV64. Float side stays at f32 lanes (XLen-independent).

define void @test_vxadd_x_g2(ptr %out, ptr %a, ptr %b) {
; CHECK-LABEL: test_vxadd_x_g2:
; CHECK: vxadd.x.g2
  %va = load <2 x i64>, ptr %a
  %vb = load <2 x i64>, ptr %b
  %vr = call <2 x i64> @llvm.riscv.vx.add.x.g2.v2i64(<2 x i64> %va, <2 x i64> %vb)
  store <2 x i64> %vr, ptr %out
  ret void
}

define void @test_vxadd_x_g4(ptr %out, ptr %a, ptr %b) {
; CHECK-LABEL: test_vxadd_x_g4:
; CHECK: vxadd.x.g4
  %va = load <4 x i64>, ptr %a
  %vb = load <4 x i64>, ptr %b
  %vr = call <4 x i64> @llvm.riscv.vx.add.x.g4.v4i64(<4 x i64> %va, <4 x i64> %vb)
  store <4 x i64> %vr, ptr %out
  ret void
}

define void @test_vxadd_x_g8(ptr %out, ptr %a, ptr %b) {
; CHECK-LABEL: test_vxadd_x_g8:
; CHECK: vxadd.x.g8
  %va = load <8 x i64>, ptr %a
  %vb = load <8 x i64>, ptr %b
  %vr = call <8 x i64> @llvm.riscv.vx.add.x.g8.v8i64(<8 x i64> %va, <8 x i64> %vb)
  store <8 x i64> %vr, ptr %out
  ret void
}

define void @test_vxadd_f_g2(ptr %out, ptr %a, ptr %b) {
; CHECK-LABEL: test_vxadd_f_g2:
; CHECK: vxadd.f.g2
  %va = load <2 x float>, ptr %a
  %vb = load <2 x float>, ptr %b
  %vr = call <2 x float> @llvm.riscv.vx.add.f.g2(<2 x float> %va, <2 x float> %vb)
  store <2 x float> %vr, ptr %out
  ret void
}

define void @test_vxadd_f_g4(ptr %out, ptr %a, ptr %b) {
; CHECK-LABEL: test_vxadd_f_g4:
; CHECK: vxadd.f.g4
  %va = load <4 x float>, ptr %a
  %vb = load <4 x float>, ptr %b
  %vr = call <4 x float> @llvm.riscv.vx.add.f.g4(<4 x float> %va, <4 x float> %vb)
  store <4 x float> %vr, ptr %out
  ret void
}

define void @test_vxadd_f_g8(ptr %out, ptr %a, ptr %b) {
; CHECK-LABEL: test_vxadd_f_g8:
; CHECK: vxadd.f.g8
  %va = load <8 x float>, ptr %a
  %vb = load <8 x float>, ptr %b
  %vr = call <8 x float> @llvm.riscv.vx.add.f.g8(<8 x float> %va, <8 x float> %vb)
  store <8 x float> %vr, ptr %out
  ret void
}

declare <2 x i64>   @llvm.riscv.vx.add.x.g2.v2i64(<2 x i64>, <2 x i64>)
declare <4 x i64>   @llvm.riscv.vx.add.x.g4.v4i64(<4 x i64>, <4 x i64>)
declare <8 x i64>   @llvm.riscv.vx.add.x.g8.v8i64(<8 x i64>, <8 x i64>)
declare <2 x float> @llvm.riscv.vx.add.f.g2(<2 x float>, <2 x float>)
declare <4 x float> @llvm.riscv.vx.add.f.g4(<4 x float>, <4 x float>)
declare <8 x float> @llvm.riscv.vx.add.f.g8(<8 x float>, <8 x float>)
