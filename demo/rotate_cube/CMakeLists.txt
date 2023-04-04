find_package(stb REQUIRED)

add_executable(rotate_cube src/main.c)

target_compile_options(
  rotate_cube
  PUBLIC -O0
  PRIVATE -Wall -Wextra -Wundef -pedantic)

target_link_libraries(rotate_cube PUBLIC wajs_engine wajs_rhi stb::stb)

target_link_options(rotate_cube PUBLIC -Wl,--no-entry -Wl,-allow-undefined
                    -Wl,--import-memory -Wl,--export-table)

add_custom_command(
  TARGET rotate_cube
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:rotate_cube>
          ${WASM_RUNTIME_DIR}/rotate_cube.wasm)
