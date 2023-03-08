add_executable(webasm_demo src/main.cpp)

target_compile_definitions(webasm_demo PUBLIC __wasi__)
target_compile_options(webasm_demo PUBLIC --sysroot=${sys_root} -target wasm32 -O0)
# target_link_directories(webasm_demo PUBLIC ${sys_root}/lib/wasm32-wasi)

set_target_properties(
  webasm_demo
  PROPERTIES
    LINK_FLAGS
    "--no-entry -export=malloc -export=free -export=main -allow-undefined --import-memory -L${sys_root}/lib/wasm32-wasi -lc"
)

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${PROJECT_SOURCE_DIR}/http/webasm_demo.wasm)
