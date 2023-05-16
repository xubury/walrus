function(file_download path url)
  get_filename_component(filename ${path} NAME)
  message(STATUS "Downloading ${filename} from ${url}")
  file(
    DOWNLOAD ${url} ${path}
    TLS_VERIFY ON
    STATUS DOWNLOAD_RESULT)
  list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
  if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
    file(REMOVE ${path})
    message(FATAL_ERROR "Download failed with error: ${DOWNLOAD_RESULT}")
  endif()
  message(STATUS "Download finished")
endfunction()

function(walrus_add_executable executable)
  set(multi_value_args SOURCES PUBLIC PRIVATE)
  cmake_parse_arguments(THIS "CONSOLE" "" "${multi_value_args}" ${ARGN})
  if(NOT "${THIS_UNPARSED_ARGUMENTS}" STREQUAL "")
    message(
      FATAL_ERROR
        "Extra unparsed arguments when calling walrus_add_executable: ${THIS_UNPARSED_ARGUMENTS}"
    )
  endif()

  add_executable(${executable} ${THIS_SOURCES})

  if(NOT MSVC)
    target_compile_options(${executable} PUBLIC -Wall -Wextra -Wundef -pedantic)
  endif()

  target_link_libraries(
    ${executable}
    PUBLIC ${THIS_PUBLIC}
    PRIVATE ${THIS_RPIVATE})

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${executable} PUBLIC WR_DEBUG_BUILD)
  endif()

  if(WASM)
    target_link_options(${executable} PUBLIC -Wl,--no-entry -Wl,-allow-undefined
                        -Wl,--import-memory -Wl,--export-table)
    add_custom_command(
      TARGET ${executable}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${executable}>
              ${WASM_RUNTIME_DIR}/${executable}.wasm)
  else()
    if(THIS_CONSOLE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_link_options(${executable} PUBLIC -mconsole)
    else()
      target_link_options(${executable} PUBLIC -mwindows)
    endif()
  endif()

  install(TARGETS ${executable})
endfunction()
