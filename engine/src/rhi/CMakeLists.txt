add_library(wajs_rhi STATIC rhi.c rhi_gl.c)

if(NOT WASM)
  target_sources(wajs_rhi PRIVATE glew.c)

  find_package(opengl REQUIRED)
  find_package(GLEW REQUIRED)
  target_link_libraries(wajs_rhi PUBLIC opengl::opengl GLEW::GLEW)
else()
  target_include_directories(wajs_rhi PUBLIC ${CMAKE_SOURCE_DIR}/engine/include/rhi/webgl)
endif()

target_compile_options(wajs_rhi PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(wajs_rhi PUBLIC wajs_core)

target_include_directories(wajs_rhi PUBLIC ${CMAKE_SOURCE_DIR}/engine/include)
