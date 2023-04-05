find_package(cglm REQUIRED)

add_library(wajs_engine STATIC src/engine.c src/app.c)

target_compile_options(wajs_engine PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(wajs_engine
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(wajs_engine PUBLIC wajs_core wajs_window)
