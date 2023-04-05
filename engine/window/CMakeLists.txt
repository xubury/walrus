add_library(wajs_window STATIC src/event.c src/window.c src/input_device.c
                               src/input.c)

if(WASM)
  target_link_options(
    wajs_window
    PUBLIC
    -Wl,-export=__on_exit
    -Wl,-export=__on_mouse_move
    -Wl,-export=__on_mouse_down
    -Wl,-export=__on_mouse_up
    -Wl,-export=__on_key_down
    -Wl,-export=__on_key_up)
  target_sources(wajs_window PRIVATE src/wajs_window.c)
else()
  target_sources(wajs_window PRIVATE src/glfw_window.c)

  find_package(glfw3 REQUIRED)
  target_link_libraries(wajs_window PRIVATE glfw::glfw)
endif()

target_compile_options(wajs_window PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(wajs_window
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(wajs_window PUBLIC wajs_core wajs_rhi)
