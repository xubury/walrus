add_library(walrus_editor editor.c component_panel.c)

target_compile_options(walrus_editor PRIVATE -Wall -Wextra -Wundef -pedantic)

target_include_directories(walrus_editor PUBLIC ${walrus_root_dir}/include)

target_link_libraries(walrus_editor PUBLIC walrus_engine)
