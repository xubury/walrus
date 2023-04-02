add_library(wajs_gl STATIC src/wajs_gl.c)

# target_link_options(wajs_gl PUBLIC -Wl,--export-table -Wl,-export=__wajs_update_frametime)

target_compile_options(wajs_gl PRIVATE -Wall -Wextra -Wundef -pedantic)

target_link_libraries(wajs_gl PRIVATE wajs_core)

target_include_directories(wajs_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
