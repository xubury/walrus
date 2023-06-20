find_package(stb REQUIRED)
find_package(cgltf REQUIRED)
find_package(mikktspace REQUIRED)

add_library(
  walrus_engine STATIC
  animator.c
  app.c
  batch_renderer.c
  camera.c
  control_map.c
  engine.c
  event.c
  font.c
  fps_controller.c
  imgui.c
  input_device.c
  input.c
  model.c
  shader_library.c
  thread_pool.c
  window.c)

target_compile_options(walrus_engine PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(walrus_engine PUBLIC ${walrus_root_dir}/include)

target_link_libraries(
  walrus_engine
  PUBLIC walrus_core walrus_rhi cimgui
  PRIVATE stb::stb cgltf::cgltf mikktspace::mikktspace)

if(WASM)
  target_link_options(walrus_engine PUBLIC -Wl,-export=__engine_should_close)
  target_link_options(
    walrus_engine
    PUBLIC
    -Wl,-export=__on_exit
    -Wl,-export=__on_mouse_move
    -Wl,-export=__on_mouse_scroll
    -Wl,-export=__on_mouse_down
    -Wl,-export=__on_mouse_up
    -Wl,-export=__on_key_down
    -Wl,-export=__on_key_up)
  target_sources(walrus_engine PRIVATE wajs_window.c)
else()
  target_sources(walrus_engine PRIVATE glfw_window.c)

  find_package(glfw3 REQUIRED)
  target_link_libraries(walrus_engine PRIVATE glfw::glfw)
endif()
