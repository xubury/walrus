add_library(wajs_gl STATIC src/wajs_gl.c)

target_link_options(wajs_gl PUBLIC -Wl,-export=__wajsUpdateFrameTime)

target_include_directories(wajs_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
