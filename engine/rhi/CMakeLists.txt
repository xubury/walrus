add_library(wajs_rhi INTERFACE)

target_compile_options(wajs_rhi INTERFACE -Wall -Wextra -Wundef -pedantic)

target_include_directories(wajs_rhi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(wajs_rhi INTERFACE wajs_core)
