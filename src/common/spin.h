#ifndef COMMON_SPIN_H
#define COMMON_SPIN_H

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#else
#include <stdatomic.h>
#endif

static void cpu_relax(void) {
#if defined(__i386__) || defined(__x86_64__)
    _mm_pause();
#elif defined(__aarch64__) || (defined(__arm__) && __ARM_ARCH >= 7)
    __asm__ __volatile__("yield");
#else
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

#endif
