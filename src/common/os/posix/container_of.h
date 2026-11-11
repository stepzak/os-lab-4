#ifndef POSIX_CONTAINER_OF_H
#define POSIX_CONTAINER_OF_H

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)

#define container_of(ptr, stct, field) ({ \
    const __typeof__( ((stct*)0)->field) *__mptr = (ptr); \
    (stct*) ((char*) __mptr - offsetof(stct, field)); \
})

#else
#error "This file requires GCC or Clang"
#endif

#endif //POSIX_CONTAINER_OF_H
