#include <stdio.h>

#define __UNIFORM__ __attribute__((uniform))

/*
void __attribute__ ((noinline)) single_branch1(int x) {
  vx_printf("single_branch1: %d\n", x);
}

void __attribute__ ((noinline)) uniform_loop1(int x) {
  vx_printf("uniform_loop1: %d\n", x);
}

void __attribute__ ((noinline)) uniform_loop2(int x) {
  vx_printf("uniform_loop2: %d\n", x);
}

void __attribute__ ((noinline)) nested_branch1(int x) {
  vx_printf("nested_branch1: %d\n", x);
}

void __attribute__ ((noinline)) nested_branch2(int x) {
  vx_printf("nested_branch2: %d\n", x);
}

void __attribute__ ((noinline)) loop_break1(int x) {
  vx_printf("loop_break1: %d\n", x);
}

void __attribute__ ((noinline)) loop_break2(int x) {
  vx_printf("loop_break2: %d\n", x);
}*/

int simple_loop(int r, const int* __UNIFORM__ buf) {
  int __UNIFORM__ x;
  x = r;
  while (buf[x] != 0) {
    ++x;
  }
  return x - r;
}

/*
void __attribute__ ((noinline)) kernel(int __DIVERGENT__ task_id, void* arg) {
  int nt  = vx_num_threads();
  int __DIVERGENT__ tid;  

  if (task_id > 1) {
    single_branch1(0);
  }

  tid = vx_thread_id();
  if (tid > 1) {
    single_branch1(0);
    if (tid == 3) {
      nested_branch1(0);
    }
  }

  for (int i = 0; i < nt; ++i) {    
    uniform_loop1(i);
  }
    
  for (int i = tid; i < nt; ++i) {
    uniform_loop2(i); 
    if ((i % 2) == 0) {
      nested_branch1(i);
    } else {
      nested_branch2(i);
    }
    if (i == 1) {
      loop_break1(i);
      break;
    }
    if (i == 2) {
      loop_break2(i);
      break;
    }
  }
}*/

/*
typedef struct {
  int count;
  va_list* ap;
  int sum;
} foo_t;

void __attribute__ ((noinline)) foo(foo_t* q) {
  va_list ap;
  va_copy (ap, *q->ap);

  int count = q->count;

  int sum = 0;  
  for (int i = 0; i < count; ++i) {
    sum += va_arg(ap, int);
  }

  q->sum = sum;
  va_end (ap);
}

int variadic_add(int count, ...) {  
  va_list ap;
  va_start (ap, count);

  foo_t q;
  q.count = count;
  q.ap = &ap;

  foo(&q);

  va_end (ap);
  return q.sum;
}*/

/*
int main() {
  vx_spawn_tasks(16, kernel, NULL);
  //int sum = variadic_add(8, 1, 0, 4, 1, 2, 1, 0, 0);
  //vx_putchar(48 + sum);
  return 0;
}*/