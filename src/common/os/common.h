#ifndef COMMON_H
#define COMMON_H
#include <stddef.h>
#include <stdint.h>

#define OS_INVALID_HANDLE (-1)

typedef int64_t os_offset_t;
typedef intptr_t os_file_handle_t;
typedef intptr_t os_process_handle_t;
typedef ptrdiff_t os_ssize_t;

#endif //COMMON_H
