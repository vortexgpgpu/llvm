// XVortexAMOD with Zacas makes the 64-bit atomics lock-free on RV32, the 8-byte
// compare-and-swap included. Without Zacas they are not: nand, compare-and-swap
// and the floating-point read-modify-writes have no 64-bit form.
// RUN: %clang_cc1 -triple riscv32 -target-feature +a -target-feature +zacas \
// RUN:   -E -dM %s -o - | FileCheck %s --check-prefix=BASE \
// RUN:   --implicit-check-not=SWAP_8
// RUN: %clang_cc1 -triple riscv32 -target-feature +a \
// RUN:   -target-feature +xvortexamod -E -dM %s -o - \
// RUN:   | FileCheck %s --check-prefix=BASE --implicit-check-not=SWAP_8
// RUN: %clang_cc1 -triple riscv32 -target-feature +a -target-feature +zacas \
// RUN:   -target-feature +xvortexamod -E -dM %s -o - \
// RUN:   | FileCheck %s --check-prefix=AMO64

// BASE: #define __CLANG_ATOMIC_LLONG_LOCK_FREE 1
// AMO64-DAG: #define __CLANG_ATOMIC_LLONG_LOCK_FREE 2
// AMO64-DAG: #define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 1
