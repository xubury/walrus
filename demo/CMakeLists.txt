add_executable(webasm_demo src/main.cpp)

target_include_directories(webasm_demo PRIVATE ${CONAN_INCLUDE_DIRS})
target_link_libraries(webasm_demo PRIVATE ${CONAN_LIBS})

target_compile_options(
  webasm_demo PUBLIC "--sysroot=${PROJECT_SOURCE_DIR}/sysroot")

set_target_properties(
  webasm_demo
  PROPERTIES
    LINK_FLAGS
    "--strip-all --no-entry --export-all --lto-O3 --allow-undefined --import-memory -L${PROJECT_SOURCE_DIR}/sysroot/lib/wasm32-wasi -lc"
)

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${PROJECT_SOURCE_DIR}/http/webasm_demo.wasm)
