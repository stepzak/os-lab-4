add_library(os_lib STATIC
    os/posix/file/file.c
    os/posix/mmap.c
    os/posix/shm.c
    os/posix/mapped_shm/mapped_shm.c
    os/posix/process.c)
target_include_directories(os_lib PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(os_lib PRIVATE lab_options)

# Older POSIX systems provide shm_open in librt rather than libc.
include(CheckSymbolExists)
check_symbol_exists(shm_open "sys/mman.h" HAVE_SHM_OPEN_IN_LIBC)
if(NOT HAVE_SHM_OPEN_IN_LIBC)
    find_library(RT_LIBRARY rt)
    if(NOT RT_LIBRARY)
        message(FATAL_ERROR "POSIX shm_open is unavailable (including librt)")
    endif()
    target_link_libraries(os_lib PRIVATE "${RT_LIBRARY}")
endif()
