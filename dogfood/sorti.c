#include <stdio.h>
#include "vortex.h"

#define NUM_ITEMS 16

typedef struct {
  int* src_ptr;
  int* dst_ptr;  
  int num_items;
} kernel_arg_t;

void kernel_body(int __DIVERGENT__ task_id, const kernel_arg_t* arg) {
  int* src_ptr  = arg->src_addr;
	int* dst_ptr  = arg->dst_addr;
  int num_items = arg->num_items;

  int ref = src_ptr[task_id];
  
  int pos = 0;
  for (int i = 0; i < num_items; ++i) {
    int cur = src_ptr[i];
    pos += (cur < ref) || ((cur == ref) && (i < task_id));
    /*int cl = (cur < ref);
    int ce = (cur == ref);
    int ls = (i < task_id);
    int x = ce && ls;
    int y = cl || x;
    pos += y;*/
  }
  dst_ptr[pos] = ref;
  //vx_printf("%d: %d->%d\n", ref, task_id, pos);
  /*vx_putint(ref, 10);
  vx_putchar(':');
  vx_putint(task_id, 10);
  vx_putchar('>');
  vx_putint(pos, 10);
  vx_putchar('\n');*/
}

int bufIn [NUM_ITEMS];
int bufOut [NUM_ITEMS];

int main() {
  for (int i = 0; i < NUM_ITEMS; ++i) {
    bufIn[i] = rand() % 1000;
  }

  {
    vx_printf("bufIn=[");
    for (int i = 0; i < NUM_ITEMS; ++i) {
      if (i) vx_printf(", ");
      vx_printf("%d", bufIn[i]);
    }
    vx_printf("]\n");
  }

  {
    kernel_arg_t arg;
    arg.src_ptr   = bufIn;
    arg.dst_ptr   = bufOut;
    arg.num_items = NUM_ITEMS;

    vx_spawn_tasks(NUM_ITEMS, kernel_body, &arg);
  }

  vx_fence();

  {
    vx_printf("bufOut=[");
    for (int i = 0; i < NUM_ITEMS; ++i) {
      if (i) vx_printf(", ");
      vx_printf("%d", bufOut[i]);
    }
    vx_printf("]\n");
  }

  return 0;
}