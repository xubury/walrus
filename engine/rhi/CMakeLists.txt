add_library(wajs_rhi STATIC src/rhi.c src/rhi_gl.c)

if(WASM)
  target_include_directories(wajs_rhi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/webgl)
else()
  target_sources(wajs_rhi PRIVATE src/glew.c)

  find_package(opengl REQUIRED)
  find_package(GLEW REQUIRED)
  target_link_libraries(wajs_rhi PUBLIC opengl::opengl GLEW::GLEW)
endif()

target_compile_options(wajs_rhi PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(wajs_rhi PUBLIC wajs_core)

target_include_directories(wajs_rhi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
