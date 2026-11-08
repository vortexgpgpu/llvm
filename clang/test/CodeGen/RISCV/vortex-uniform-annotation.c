// A stack variable annotated vortex.uniform (the Vortex __UNIFORM__ macro) has
// every value read from it marked uniform. A loop induction variable keeps its
// update: each iteration reads the stored increment and tests it. The variable
// is still promoted to registers, so reading it again costs no stack reload.
// REQUIRES: riscv-registered-target
// RUN: %clang_cc1 -triple riscv32 -target-feature +xvortex -O1 -emit-llvm %s \
// RUN:   -o - | FileCheck %s
// RUN: %clang_cc1 -triple riscv32 -target-feature +xvortex -O1 -S %s -o - \
// RUN:   | FileCheck %s --check-prefix=ASM

void body(unsigned t);

// CHECK-LABEL: define {{.*}} @loop(
// CHECK-NOT: alloca
// CHECK: [[START:%.*]] = {{.*}}call i32 @llvm.riscv.vx.uniform.i32.i32(i32 %start)
// CHECK: for.body:
// CHECK-NEXT: [[T:%.*]] = phi i32 [ [[NEXT:%.*]], %for.body ], [ [[START]], %entry ]
// CHECK-NEXT: call void @body(i32 noundef [[T]])
// CHECK-NEXT: [[ADD:%.*]] = add i32 [[T]], %step
// CHECK-NEXT: [[NEXT]] = {{.*}}call i32 @llvm.riscv.vx.uniform.i32.i32(i32 [[ADD]])
// CHECK-NEXT: [[CMP:%.*]] = icmp ult i32 [[NEXT]], %n
// CHECK-NEXT: br i1 [[CMP]], label %for.body, label %for.cond.cleanup
void loop(unsigned start, unsigned n, unsigned step) {
  for (unsigned __attribute__((annotate("vortex.uniform"))) t = start; t < n;
       t += step)
    body(t);
}

// A pointer read again after each memory clobber is the argument itself, not
// a reload of a stack slot.
// CHECK-LABEL: define {{.*}} @reuse(
// CHECK-NOT: alloca
// CHECK: [[P:%.*]] = {{.*}}call ptr @llvm.riscv.vx.uniform.p0.p0(ptr %p)
// CHECK: load i32, ptr [[P]]
// ASM-LABEL: reuse:
// ASM-NOT: (sp)
// ASM: lw {{a[0-9]}}, 0(a0)
// ASM-NOT: (sp)
// ASM: lw {{a[0-9]}}, 4(a0)
// ASM-NOT: (sp)
// ASM: sw {{a[0-9]}}, 8(a0)
// ASM-NOT: (sp)
// ASM: ret
void reuse(int *__attribute__((annotate("vortex.uniform"))) p) {
  int a = p[0];
  __asm__ volatile("" ::: "memory");
  int b = p[1];
  __asm__ volatile("" ::: "memory");
  p[2] = a + b;
}
