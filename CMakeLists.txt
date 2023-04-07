option(WASM "Build wasm program" OFF)
option(BUILD_TEST "Build test" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(macro)
include(bootstraps)

if(BUILD_TEST)
  set(WASM OFF)
endif()

if(WASM)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)
endif()

cmake_minimum_required(VERSION 3.20.0)
project(walrus VERSION 0.1.0)

if(BUILD_TEST)
  include(CTest)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

# ######### Conan configuration #####################
set(cnoan_cmake ${CMAKE_SOURCE_DIR}/cmake/conan.cmake)
set(cnoan_url
    https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake)

if(NOT EXISTS ${cnoan_cmake})
  file_download(${cnoan_cmake} ${cnoan_url})
endif()

include(${cnoan_cmake})

set(conan_packages stb/cci.20210910 cglm/0.8.5)

if(NOT WASM)
  list(APPEND conan_packages glew/2.2.0 glfw/3.3.8)
endif()

conan_cmake_configure(REQUIRES ${conan_packages} GENERATORS cmake_find_package)

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

set(walrus_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/walrus)
add_subdirectory(walrus/src)
add_subdirectory(demo)
