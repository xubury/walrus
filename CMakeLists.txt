set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(${CMAKE_SOURCE_DIR}/cmake/macro.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/bootstraps.cmake)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)

cmake_minimum_required(VERSION 3.20.0)
project(webasm VERSION 0.1.0)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

# ######### Conan configuration #####################
set(CONAN_CMAKE ${CMAKE_SOURCE_DIR}/cmake/conan.cmake)
set(CONAN_URL
    https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake)

if(NOT EXISTS ${CONAN_CMAKE})
  file_download(${CONAN_CMAKE} ${CONAN_URL})
endif()

include(${CONAN_CMAKE})

conan_cmake_run(
  CONANFILE conanfile.txt
  BASIC_SETUP
  ARCH wasm
  ENV CC=${CMAKE_C_COMPILER}
  ENV CXX=${CMAKE_CXX_COMPILER}
  ENV CFLAGS=${CMAKE_C_FLAGS}
  ENV CXXFLAGS=${CMAKE_CXX_FLAGS}
  ENV CONAN_CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
  PROFILE_AUTO ALL
  BUILD missing)
# ##############################################################################

add_subdirectory(${WASM_LIB_DIR}/wajs)
add_subdirectory(demo)
