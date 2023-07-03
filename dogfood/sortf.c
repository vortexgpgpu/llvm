#include <stdio.h>
#include <stdlib.h>
#include "vortex.h"

#define NUM_ITEMS 16

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
}
*/

typedef struct {
  float* src_ptr;
  float* dst_ptr;  
  int num_items;
} kernel_arg_t;

void kernel_body(int __DIVERGENT__ task_id, const kernel_arg_t* arg) {
  float* src_ptr = arg->src_ptr;
	float* dst_ptr = arg->dst_ptr;
  int num_items  = arg->num_items;

  float ref = src_ptr[task_id];
  
  int pos = 0;
  for (int i = 0; i < num_items; ++i) {
    float cur = src_ptr[i];
    pos += (cur < ref) || ((cur == ref) && (i < task_id));
    /*int cl = (cur < ref);
    int ce = (cur == ref);
    int ls = (i < task_id);
    int x = ce && ls;
    int y = cl || x;
    pos += y;*/
  }
  dst_ptr[pos] = ref;
  //vx_printf("%.6f: %d->%d\n", ref, task_id, pos);
  /*vx_putfloat(ref, 3);
  vx_putchar(':');
  vx_putint(task_id, 10);
  vx_putchar('>');
  vx_putint(pos, 10);
  vx_putchar('\n');*/
  //vx_putfloat(ref, 3);
  //vx_putchar('\n');

  /*vx_putint(vx_warp_id(), 10);
  vx_putchar(':');
  vx_putint(vx_thread_id(), 10);
  vx_putchar('>');
  vx_putint(pos, 16);
  vx_putchar('\n');*/

  //int sum = variadic_add(3, 0, vx_thread_gid(), 10);
  //vx_putint(sum, 10);
}

float bufIn [NUM_ITEMS];
float bufOut [NUM_ITEMS];

int main() {
  float inv_rnd = 1.0f / RAND_MAX;
  for (int i = 0; i < NUM_ITEMS; ++i) {
    bufIn[i] = inv_rnd * rand();
  }

  for (int i = 0; i < NUM_ITEMS; ++i) {
    bufOut[i] = -99999.0f;
  }

  /*{
    vx_printf("bufIn=[");
    for (int i = 0; i < NUM_ITEMS; ++i) {
      if (i) vx_printf(", ");
      vx_printf("%.6f", bufIn[i]);
      //vx_printf("%x", *(int*)&bufIn[i]);
    }
    vx_printf("]\n");
  }*/

  {
    kernel_arg_t arg;
    arg.src_ptr   = bufIn;
    arg.dst_ptr   = bufOut;
    arg.num_items = NUM_ITEMS;

    vx_spawn_tasks(arg.num_items, kernel_body, &arg);
  }

  {
    vx_printf("bufOut=[");
    for (int i = 0; i < NUM_ITEMS; ++i) {
      if (i) vx_printf(", ");
      vx_printf("%.6f", bufOut[i]);
      //vx_printf("%x", *(int*)&bufOut[i]);
    }
    vx_printf("]\n");
  }

  return 0;
}