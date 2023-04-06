find_package(cglm REQUIRED)

add_library(wajs_engine STATIC src/engine.c src/app.c src/event.c src/window.c
                               src/input_device.c src/input.c)

target_compile_options(wajs_engine PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(wajs_engine
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(wajs_engine PUBLIC wajs_core wajs_rhi)

if(WASM)
  target_link_options(wajs_engine PUBLIC -Wl,-export=__engine_should_close)
  target_link_options(
    wajs_engine
    PUBLIC
    -Wl,-export=__on_exit
    -Wl,-export=__on_mouse_move
    -Wl,-export=__on_mouse_down
    -Wl,-export=__on_mouse_up
    -Wl,-export=__on_key_down
    -Wl,-export=__on_key_up)
  target_sources(wajs_engine PRIVATE src/wajs_window.c)
else()
  target_sources(wajs_engine PRIVATE src/glfw_window.c)

  find_package(glfw3 REQUIRED)
  target_link_libraries(wajs_engine PRIVATE glfw::glfw)
endif()
