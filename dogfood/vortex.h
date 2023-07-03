#pragma once

#include <VX_types.h>

// Return active warp's thread id 
inline int vx_thread_id() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_WTID));
    return result;   
}

// Return active core's local thread id
inline int vx_thread_lid() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_LTID));
    return result;   
}

// Return processsor global thread id
inline int vx_thread_gid() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_GTID));
    return result;   
}

// Return active core's local warp id
inline int vx_warp_id() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_LWID));
    return result;   
}

// Return processsor's global warp id
inline int vx_warp_gid() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_GWID));
    return result;   
}

// Return processsor core id
inline int vx_core_id() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_GCID));
    return result; 
}

// Return current threadk mask
inline int vx_thread_mask() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_TMASK));
    return result; 
}

// Return the number of threads in a warp
inline int vx_num_threads() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_NUM_THREADS));
    return result; 
}

// Return the number of warps in a core
inline int vx_num_warps() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_NUM_WARPS));
    return result;   
}

// Return the number of cores in the processsor
inline int vx_num_cores() {
    int result;
    asm volatile ("csrr %0, %1" : "=r"(result) : "i"(CSR_NUM_CORES));
    return result;   
}

#define __DIVERGENT__ __attribute__((annotate("divergent")))

inline void vx_fence() {
    asm volatile ("fence iorw, iorw");
}

#include <vx_spawn.h>
#include <vx_print.h>