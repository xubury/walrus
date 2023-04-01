add_subdirectory(core)

if (WASM)
add_subdirectory(window)
add_subdirectory(gl)
endif()
