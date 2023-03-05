cmake_minimum_required(VERSION 3.20.0)

project(webasm VERSION 0.1.0)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

if(BUILD_SHARED_LIBS)
  set(CONAN_LIBS_SHARED True)
else()
  set(CONAN_LIBS_SHARED False)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/macro.cmake)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/conan.cmake")
  file_download(
    "${PROJECT_SOURCE_DIR}/conan.cmake"
    "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake")
endif()

include(${PROJECT_SOURCE_DIR}/conan.cmake)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

install(FILES ${Basic_Include} DESTINATION include)

conan_cmake_configure(
  REQUIRES
  fmt/8.1.1
  spdlog/1.10.0
  GENERATORS
  cmake_find_package
  IMPORTS
  "bin, *.dll -> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
  OPTIONS)

conan_cmake_autodetect(settings)

conan_cmake_install(
  PATH_OR_REFERENCE
  .
  BUILD
  missing
  REMOTE
  conancenter
  SETTINGS
  ${settings})


add_subdirectory(demo)
