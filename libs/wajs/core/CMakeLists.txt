add_library(wajs_core STATIC src/sys.c src/list.c src/queue.c)

target_include_directories(wajs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(wajs_core PRIVATE -Wall -Wextra -Wundef -pedantic)

if(DEFINED BUILD_TEST)
  add_executable(list_test test/list_test.c)
  add_executable(queue_test test/queue_test.c)

  target_link_libraries(list_test PRIVATE wajs_core)
  target_link_libraries(queue_test PRIVATE wajs_core)

  enable_testing()

  add_test(NAME list_test COMMAND $<TARGET_FILE:list_test>)
  add_test(NAME queue_test COMMAND $<TARGET_FILE:queue_test>)
endif()
