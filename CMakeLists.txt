include(${CMAKE_SOURCE_DIR}/cmake/macro.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)

cmake_minimum_required(VERSION 3.20.0)
project(webasm VERSION 0.1.0)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(${WASM_LIB_DIR}/wajs)
add_subdirectory(demo)

# ######### Conan configuration #####################
set(CONAN_CMAKE ${CMAKE_SOURCE_DIR}/cmake/conan.cmake)
set(CONAN_URL
    https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake)

if(NOT EXISTS ${CONAN_CMAKE})
  file_download(${CONAN_CMAKE} ${CONAN_URL})
endif()

include(${CONAN_CMAKE})

conan_cmake_configure(
  REQUIRES
  stb/cci.20210910
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
# ##############################################################################
