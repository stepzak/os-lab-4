#ifndef OS_PROCESS_H
#define OS_PROCESS_H

#include "common/os/common.h"

os_process_handle_t os_process_spawn(const char *path, char *const argv[]);
int os_process_poll(os_process_handle_t process, int *exit_code);
int os_process_wait(os_process_handle_t process, int *exit_code);
void os_process_terminate(os_process_handle_t process);
os_process_handle_t os_process_id(void);
os_process_handle_t os_process_parent_id(void);

#endif
