// A stack variable annotated vortex.uniform (the Vortex __UNIFORM__ macro) has
// every value read from it marked uniform. A loop induction variable keeps its
// update: each iteration reads the stored increment and tests it.
// RUN: %clang_cc1 -triple riscv32 -target-feature +xvortex -O1 -emit-llvm %s \
// RUN:   -o - | FileCheck %s

void body(unsigned t);

// CHECK-LABEL: define {{.*}} @loop(
// CHECK: for.body:
// CHECK: call void @body(
// CHECK: [[T:%.*]] = load i32, ptr %t
// CHECK-NEXT: [[U:%.*]] = call i32 @llvm.riscv.vx.uniform.i32.i32(i32 [[T]])
// CHECK-NEXT: [[ADD:%.*]] = add i32 [[U]], %step
// CHECK-NEXT: store i32 [[ADD]], ptr %t
// CHECK-NEXT: [[NEXT:%.*]] = call i32 @llvm.riscv.vx.uniform.i32.i32(i32 [[ADD]])
// CHECK-NEXT: [[CMP:%.*]] = icmp ult i32 [[NEXT]], %n
// CHECK-NEXT: br i1 [[CMP]], label %for.body, label %for.cond.cleanup
void loop(unsigned start, unsigned n, unsigned step) {
  for (unsigned __attribute__((annotate("vortex.uniform"))) t = start; t < n;
       t += step)
    body(t);
}
