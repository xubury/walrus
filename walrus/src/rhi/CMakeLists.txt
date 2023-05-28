add_library(
  walrus_rhi STATIC
  command_buffer.c
  frame.c
  rhi.c
  gl_context.c
  gl_framebuffer.c
  gl_shader.c
  gl_texture.c
  uniform_buffer.c
  vertex_layout.c)

if(NOT WASM)
  target_sources(walrus_rhi PRIVATE glew.c)

  find_package(opengl REQUIRED)
  find_package(GLEW REQUIRED)
  target_link_libraries(walrus_rhi PUBLIC opengl::opengl GLEW::GLEW)
else()
  target_include_directories(walrus_rhi PUBLIC ${walrus_library_dir}/webgl)
endif()

target_compile_options(walrus_rhi PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(walrus_rhi PUBLIC walrus_core)

target_include_directories(walrus_rhi PUBLIC ${walrus_root_dir}/include)
