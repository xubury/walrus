add_executable(webasm_demo src/main.cpp)

target_include_directories(webasm_demo PRIVATE ${CONAN_INCLUDE_DIRS})
target_link_libraries(webasm_demo PRIVATE ${CONAN_LIBS})

target_compile_options(
  webasm_demo PUBLIC "-nostdlib" "-O3" "-I${PROJECT_SOURCE_DIR}/system/libc"
                     "-I${PROJECT_SOURCE_DIR}/system/libc/musl/arch/emscripten")

set_target_properties(
  webasm_demo
  PROPERTIES
    LINK_FLAGS
    "--strip-all --no-entry --export-all --lto-O3 --allow-undefined --import-memory"
)

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${PROJECT_SOURCE_DIR}/http/webasm_demo.wasm)
