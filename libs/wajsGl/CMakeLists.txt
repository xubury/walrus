add_library(wajsGl STATIC wajsGl.c)

# target_link_libraries(wajsGl PUBLIC c)

target_link_options(wajsGl PUBLIC -export=__wajsGlDraw)

target_include_directories(wajsGl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/GL)
