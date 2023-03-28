add_library(wajs_core STATIC src/sys.c)

target_include_directories(wajs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(wajs_core PUBLIC c)
