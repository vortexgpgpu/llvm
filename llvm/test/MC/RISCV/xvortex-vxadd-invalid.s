# XVortex - Vortex GPGPU vendor extension - vxadd alignment + feature errors
# RUN: not llvm-mc -triple riscv32 -mattr=+xvortex,+f < %s 2>&1 \
# RUN:     | FileCheck -check-prefix=CHECK %s
# RUN: not llvm-mc -triple riscv32 -mattr=-xvortex < %s 2>&1 \
# RUN:     | FileCheck -check-prefixes=CHECK,CHECK-MINUS %s

# group-2 requires even-aligned base
# CHECK: error: invalid operand for instruction
vxadd.x.g2 x5, x6, x8

# group-4 requires 4-aligned base
# CHECK: error: invalid operand for instruction
vxadd.x.g4 x5, x8, x12
# CHECK: error: invalid operand for instruction
vxadd.x.g4 x4, x9, x12
# CHECK: error: invalid operand for instruction
vxadd.x.g4 x4, x8, x13

# group-8 requires 8-aligned base
# CHECK: error: invalid operand for instruction
vxadd.x.g8 x4, x16, x24
# CHECK: error: invalid operand for instruction
vxadd.x.g8 x8, x12, x24

# float side: group-4 requires 4-aligned base
# CHECK: error: invalid operand for instruction
vxadd.f.g4 f1, f8, f12

# without xvortex feature, every variant is rejected
# CHECK-MINUS: error: instruction requires the following: 'XVortex' (Vortex ISA Extension)
vxadd.x.g4 x4, x8, x12
