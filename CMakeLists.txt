list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(macro)
include(bootstraps)

cmake_minimum_required(VERSION 3.20.0)
project(webasm VERSION 0.1.0)

if(WASM)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)
endif()

if(BUILD_TEST)
  include(CTest)
endif()

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

conan_cmake_configure(
  REQUIRES stb/cci.20210910 imgui/cci.20230105+1.89.2.docking GENERATORS
  cmake_find_package)

conan_cmake_autodetect(settings)

if(WASM)
  conan_cmake_install(
    PATH_OR_REFERENCE
    .
    BUILD
    missing
    REMOTE
    conancenter
    SETTINGS_BUILD
    ${settings}
    PROFILE_BUILD
    default
    PROFILE_HOST
    wasm)
else()
  conan_cmake_install(
    PATH_OR_REFERENCE
    .
    BUILD
    missing
    REMOTE
    conancenter
    SETTINGS
    ${settings})
endif()
# ##############################################################################

if(WASM)
  add_subdirectory(demo)
endif()
add_subdirectory(engine)
