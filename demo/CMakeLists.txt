find_package(stb REQUIRED)
find_package(imgui REQUIRED)
find_package(cglm REQUIRED)
add_executable(webasm_demo src/main.c)

target_compile_options(webasm_demo PUBLIC -O0 PRIVATE -Wall -Wextra -Wundef -pedantic)

target_link_libraries(webasm_demo PUBLIC wajs_engine wajs_rhi stb::stb imgui::imgui cglm::cglm)

target_link_options(webasm_demo PUBLIC -Wl,--no-entry -Wl,-allow-undefined
                    -Wl,--import-memory -Wl,--export-table)

add_custom_command(
  TARGET webasm_demo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:webasm_demo>
          ${WASM_RUNTIME_ENTRY})
