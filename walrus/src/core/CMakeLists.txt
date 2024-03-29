find_package(cglm REQUIRED)
find_package(stb REQUIRED)

add_library(
  walrus_core STATIC
  array.c
  handle_alloc.c
  hash.c
  image.c
  list.c
  log.c
  math.c
  memory.c
  mutex.c
  queue.c
  ray.c
  semaphore.c
  sort.c
  string.c
  sys.c
  thread.c
  transform.c)

target_include_directories(walrus_core PUBLIC ${walrus_root_dir}/include)

target_compile_options(walrus_core PRIVATE -Wall -Wextra -Wundef -pedantic)

target_link_libraries(walrus_core PUBLIC cglm::cglm PRIVATE stb::stb)

if(BUILD_TEST)
  add_executable(list_test test/list_test.c)
  add_executable(queue_test test/queue_test.c)

  target_link_libraries(list_test PRIVATE walrus_core)
  target_link_libraries(queue_test PRIVATE walrus_core)

  enable_testing()

  add_test(NAME list_test COMMAND $<TARGET_FILE:list_test>)
  add_test(NAME queue_test COMMAND $<TARGET_FILE:queue_test>)
endif()
