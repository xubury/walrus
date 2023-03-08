add_executable(webasm_demo src/main.cpp)

target_compile_definitions(webasm_demo PUBLIC __wasi__)
target_compile_options(webasm_demo PUBLIC -O0)

if(NOT WASI_COMPILER)
  set_target_properties(
    webasm_demo
    PROPERTIES
      LINK_FLAGS
      "--no-entry -export=wasm_main -allow-undefined --import-memory -lc"
  )
endif()

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${PROJECT_SOURCE_DIR}/http/webasm_demo.wasm)
