add_library(wajs_window STATIC src/wajs_window.c src/event.c)

if(WASM)
  target_link_options(
    wajs_window
    PUBLIC
    -Wl,-export=__on_mouse_move
    -Wl,-export=__on_mouse_down
    -Wl,-export=__on_mouse_up
    -Wl,-export=__on_key_down
    -Wl,-export=__on_key_up)
endif()

target_compile_options(wajs_window PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(wajs_window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(wajs_window PRIVATE wajs_core)
