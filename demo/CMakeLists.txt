add_executable(webasm_demo src/main.cpp)

target_include_directories(webasm_demo PRIVATE ${CONAN_INCLUDE_DIRS})
target_link_libraries(webasm_demo PRIVATE ${CONAN_LIBS})

set_target_properties(webasm_demo PROPERTIES SUFFIX ".html" LINK_FLAGS
                                                            "--emrun")
