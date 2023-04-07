add_library(walrus_rhi STATIC rhi.c rhi_gl.c)

if(NOT WASM)
  target_sources(walrus_rhi PRIVATE glew.c)

  find_package(opengl REQUIRED)
  find_package(GLEW REQUIRED)
  target_link_libraries(walrus_rhi PUBLIC opengl::opengl GLEW::GLEW)
else()
  target_include_directories(walrus_rhi PUBLIC ${walrus_root_dir}/include/rhi/webgl)
endif()

target_compile_options(walrus_rhi PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(walrus_rhi PUBLIC walrus_core)

target_include_directories(walrus_rhi PUBLIC ${walrus_root_dir}/include)
