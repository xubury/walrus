find_package(stb REQUIRED)

add_executable(ttf_demo src/main.c)

target_compile_options(ttf_demo PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(ttf_demo PUBLIC walrus_engine)

if(WASM)
  target_link_options(ttf_demo PUBLIC -Wl,--no-entry -Wl,-allow-undefined
                      -Wl,--import-memory -Wl,--export-table)
  add_custom_command(
    TARGET ttf_demo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ttf_demo>
            ${WASM_RUNTIME_DIR}/ttf_demo.wasm)
else()
  if(THIS_CONSOLE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(ttf_demo PUBLIC -mconsole)
  else()
    target_link_options(ttf_demo PUBLIC -mwindows)
  endif()
endif()
