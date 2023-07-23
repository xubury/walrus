find_package(stb REQUIRED)
find_package(cgltf REQUIRED)
find_package(mikktspace REQUIRED)
find_package(flecs REQUIRED)

add_library(
  walrus_engine STATIC
  systems/animator_system.c
  systems/camera_system.c
  systems/controller_system.c
  systems/model_system.c
  systems/render_system.c
  systems/transform_system.c
  systems/pipelines/culling_pipeline.c
  systems/pipelines/deferred_pipeline.c
  systems/pipelines/hdr_pipeline.c
  animator.c
  app.c
  batch_renderer.c
  camera.c
  deferred_renderer.c
  engine.c
  event.c
  font.c
  fps_controller.c
  frame_graph.c
  geometry.c
  imgui.c
  input.c
  input_device.c
  input_map.c
  material.c
  model.c
  shader_library.c
  thread_pool.c
  window.c)

target_compile_options(walrus_engine PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(walrus_engine PUBLIC ${walrus_root_dir}/include)

target_link_libraries(
  walrus_engine
  PUBLIC walrus_core walrus_rhi cimgui flecs::flecs
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
