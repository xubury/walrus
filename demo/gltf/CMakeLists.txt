add_executable(gltf_demo src/main.c)

target_compile_options(gltf_demo PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(gltf_demo PUBLIC walrus_engine)

if(WASM)
  target_link_options(gltf_demo PUBLIC -Wl,--no-entry -Wl,-allow-undefined
                      -Wl,--import-memory -Wl,--export-table)
  add_custom_command(
    TARGET gltf_demo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gltf_demo>
            ${WASM_RUNTIME_DIR}/gltf_demo.wasm)
else()
  if(THIS_CONSOLE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(gltf_demo PUBLIC -mconsole)
  else()
    target_link_options(gltf_demo PUBLIC -mwindows)
  endif()
endif()
