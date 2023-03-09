add_executable(webasm_demo src/main.cpp)

target_compile_options(webasm_demo PUBLIC -O0)

target_link_libraries(webasm_demo PUBLIC c)

if(NOT WASI_COMPILER)
  set_target_properties(
    webasm_demo PROPERTIES LINK_FLAGS
                           "--no-entry -allow-undefined -export=__main_argc_argv --import-memory")
endif()

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${PROJECT_SOURCE_DIR}/http/webasm_demo.wasm)
