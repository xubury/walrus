find_package(stb REQUIRED)

add_executable(romantik src/main.c src/camera.c src/game_logic.c
                        src/game_state.c src/hex.c src/hex_map.c)

target_compile_options(romantik PUBLIC -Wall -Wextra -Wundef -pedantic)

target_link_libraries(romantik PUBLIC walrus_engine)

if(WASM)
  target_link_options(romantik PUBLIC -Wl,--no-entry -Wl,-allow-undefined
                      -Wl,--import-memory -Wl,--export-table)
  add_custom_command(
    TARGET romantik
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:romantik>
            ${WASM_RUNTIME_DIR}/romantik.wasm)
else()
  if(THIS_CONSOLE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(romantik PUBLIC -mconsole)
  else()
    target_link_options(romantik PUBLIC -mwindows)
  endif()
endif()
