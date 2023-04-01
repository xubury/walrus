add_library(wajs_core STATIC src/sys.c src/list.c src/queue.c)

target_include_directories(wajs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(wajs_core PRIVATE -Wall -Wextra -Wundef -pedantic)

target_link_libraries(wajs_core PUBLIC c)
