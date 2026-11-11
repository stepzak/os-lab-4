cmake_minimum_required(VERSION 3.16)
project(os_lab_4 VERSION 1.0 LANGUAGES C)

if(NOT UNIX)
    message(FATAL_ERROR "os_lab_4 requires POSIX shared memory and processes")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(lab_options INTERFACE)
target_compile_definitions(lab_options INTERFACE _POSIX_C_SOURCE=200809L)
target_compile_options(lab_options INTERFACE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>)

add_subdirectory(common)
add_subdirectory(app)

add_executable(parent parent.c)
add_executable(child child.c)
target_link_libraries(parent PRIVATE lab_logic os_lib lab_options)
target_link_libraries(child PRIVATE lab_logic os_lib lab_options)
set_target_properties(parent child PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
