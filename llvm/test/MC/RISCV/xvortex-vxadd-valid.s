# XVortex - Vortex GPGPU vendor extension - grouped vxadd
# RUN: llvm-mc %s -triple=riscv32 -mattr=+xvortex,+f -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ENC,CHECK-INST %s
# RUN: llvm-mc -filetype=obj -triple riscv32 -mattr=+xvortex,+f < %s \
# RUN:     | llvm-objdump --mattr=+xvortex,+f -M no-aliases -d - \
# RUN:     | FileCheck -check-prefix=CHECK-INST %s

# Integer grouped add.

# CHECK-INST: vxadd.x.g2   tp, t1, s0
# CHECK-ENC: encoding: [0x0b,0x02,0x83,0x20]
vxadd.x.g2 x4, x6, x8

# CHECK-INST: vxadd.x.g4   tp, s0, a2
# CHECK-ENC: encoding: [0x0b,0x12,0xc4,0x20]
vxadd.x.g4 x4, x8, x12

# CHECK-INST: vxadd.x.g8   s0, a6, s8
# CHECK-ENC: encoding: [0x0b,0x24,0x88,0x21]
vxadd.x.g8 x8, x16, x24

# Single-precision float grouped add.

# CHECK-INST: vxadd.f.g2   ft4, ft6, fs0
# CHECK-ENC: encoding: [0x0b,0x42,0x83,0x20]
vxadd.f.g2 f4, f6, f8

# CHECK-INST: vxadd.f.g4   ft4, fs0, fa2
# CHECK-ENC: encoding: [0x0b,0x52,0xc4,0x20]
vxadd.f.g4 f4, f8, f12

# CHECK-INST: vxadd.f.g8   fs0, fa6, fs8
# CHECK-ENC: encoding: [0x0b,0x64,0x88,0x21]
vxadd.f.g8 f8, f16, f24
