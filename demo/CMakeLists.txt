find_package(stb REQUIRED)
add_executable(webasm_demo src/main.c)

target_compile_options(webasm_demo PUBLIC -O0)

target_link_libraries(webasm_demo PUBLIC c wajs_gl stb::stb)

target_link_options(webasm_demo PUBLIC --no-entry -allow-undefined -export=__main_argc_argv --import-memory)

# clean up
target_include_directories(webasm_demo PUBLIC ${PROJECT_SOURCE_DIR})

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${WASM_RUNTIME_ENTRY})
