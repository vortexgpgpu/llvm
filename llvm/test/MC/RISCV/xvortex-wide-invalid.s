# RUN: not llvm-mc -triple=riscv32 -mattr=+f,+xvortex < %s 2>&1 \
# RUN:     | FileCheck %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+f < %s 2>&1 \
# RUN:     | FileCheck --check-prefix=CHECK-NOVX %s
# RUN: not llvm-mc -triple=riscv64 -mattr=+f,+xvortex < %s 2>&1 \
# RUN:     | FileCheck --check-prefix=CHECK-RV64 %s
# RUN: not llvm-mc -triple=riscv32 -mattr=+d,+xvortex < %s 2>&1 \
# RUN:     | FileCheck --check-prefix=CHECK-D %s

# The .d forms need an even register, the .q forms a multiple of 4.
vx_flw.d ft1, 0(a0)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction
vx_flw.d fa1, 8(a0)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction
vx_flw.q ft2, 0(a0)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction
vx_flw.q f31, 0(a0)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction
vx_fsw.d ft1, 0(a0)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction
vx_fsw.q fa0, 16(a0)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

# The register is an FP register group.
vx_flw.q a0, 0(a1)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction
vx_fsw.d a0, 0(a1)
# CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

# The offset is a multiple of 8 (.d) or 16 (.q) in the signed 12-bit range.
vx_flw.d ft0, 4(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 8 bytes in the range [-2048, 2040]
vx_flw.d ft0, 2048(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 8 bytes in the range [-2048, 2040]
vx_flw.q ft0, 8(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 16 bytes in the range [-2048, 2032]
vx_flw.q ft0, 2040(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 16 bytes in the range [-2048, 2032]
vx_fsw.d ft0, -4(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 8 bytes in the range [-2048, 2040]
vx_fsw.q ft0, 24(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 16 bytes in the range [-2048, 2032]
vx_flw.q ft0, %lo(sym)(a0)
# CHECK: :[[@LINE-1]]:15: error: immediate must be a multiple of 16 bytes in the range [-2048, 2032]

# CHECK-NOVX: :[[@LINE+3]]:1: error: instruction requires the following: 'XVortex' (Vortex ISA Extension)
# CHECK-RV64: :[[@LINE+2]]:1: error: instruction requires the following: RV32I Base Instruction Set
# CHECK-D: :[[@LINE+1]]:1: error: instruction requires the following: FLEN 32 ('D' disabled)
vx_flw.q ft0, 0(a0)
# CHECK-D: :[[@LINE+1]]:1: error: instruction requires the following: FLEN 32 ('D' disabled)
vx_flw.d ft0, 0(a0)
# CHECK-NOVX: :[[@LINE+3]]:1: error: instruction requires the following: 'XVortex' (Vortex ISA Extension)
# CHECK-RV64: :[[@LINE+2]]:1: error: instruction requires the following: RV32I Base Instruction Set
# CHECK-D: :[[@LINE+1]]:1: error: instruction requires the following: FLEN 32 ('D' disabled)
vx_fsw.q ft0, 0(a0)
# CHECK-D: :[[@LINE+1]]:1: error: instruction requires the following: FLEN 32 ('D' disabled)
vx_fsw.d ft0, 0(a0)
