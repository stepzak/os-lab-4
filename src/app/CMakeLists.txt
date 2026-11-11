add_library(lab_logic STATIC spsc.c parser.c)
target_include_directories(lab_logic PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(lab_logic PRIVATE lab_options)
