function(file_download path url)
  get_filename_component(filename ${path} NAME)
  message(STATUS "Downloading ${filename} from ${url}")
  file(
    DOWNLOAD "${url}" "${path}"
    TLS_VERIFY ON
    STATUS DOWNLOAD_RESULT)
  list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
  if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
    file(REMOVE ${path})
    message(FATAL_ERROR "Download failed with error: ${DOWNLOAD_RESULT}")
  endif()
  message(STATUS "Download finished")
endfunction()

set(WASI_VERSION 14)
set(WASI_VERSION_FULL ${WASI_VERSION}.0)

set(THIRD_PARTY_LIBS_DIR ${CMAKE_SOURCE_DIR}/libs)
set(WASI_SDK_PREFIX ${THIRD_PARTY_LIBS_DIR}/wasi-sdk-${WASI_VERSION_FULL})
set(WASI_SYS_ROOT ${WASI_SDK_PREFIX}/share/wasi-sysroot)

set(WASM_RUNTIME_PATH ${CMAKE_SOURCE_DIR}/server/wasm)

if(WIN32)
  set(BUILD_PLATFORM mingw)
elseif(UNIX)
  set(BUILD_PLATFORM linux)
else()
  set(BUILD_PLATFORM macos)
endif()

set(WASI_HOST_URL
    https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}
)
set(WASI_PACKAGE_NAME wasi-sdk-${WASI_VERSION_FULL}-${BUILD_PLATFORM}.tar.gz)
set(WASI_URL ${WASI_HOST_URL}/${WASI_PACKAGE_NAME})

# FIXME: should validate MD5 here?
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/${WASI_PACKAGE})
  file_download(${CMAKE_SOURCE_DIR}/${WASI_PACKAGE} ${WASI_URL})
endif()

if(NOT EXISTS ${WASI_SDK_PREFIX})
  message("extracting ${WASI_PACKAGE_NAME}")
  file(ARCHIVE_EXTRACT INPUT ${CMAKE_SOURCE_DIR}/${WASI_PACKAGE_NAME}
       DESTINATION ${THIRD_PARTY_LIBS_DIR})
  # TODO: remove wasi package
endif()

if(WIN32)
  set(WASI_HOST_EXE_SUFFIX ".exe")
  set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
else()
  set(WASI_HOST_EXE_SUFFIX "")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm)
set(triple wasm32)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(CMAKE_C_COMPILER ${WASI_SDK_PREFIX}/bin/clang${WASI_HOST_EXE_SUFFIX})
set(CMAKE_CXX_COMPILER ${WASI_SDK_PREFIX}/bin/clang++${WASI_HOST_EXE_SUFFIX})
set(CMAKE_AR ${WASI_SDK_PREFIX}/bin/llvm-ar${WASI_HOST_EXE_SUFFIX})
set(CMAKE_RANLIB ${WASI_SDK_PREFIX}/bin/llvm-ranlib${WASI_HOST_EXE_SUFFIX})

set(WASM_LINKER ${WASI_SDK_PREFIX}/bin/wasm-ld${WASI_HOST_EXE_SUFFIX})
set(CMAKE_C_LINK_EXECUTABLE
    "${WASM_LINKER} <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE
    "${WASM_LINKER} <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Only look in the sysroot (not in the host paths) for the rest
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

add_compile_options(--sysroot=${WASI_SYS_ROOT} --target=wasm32-wasi)
add_compile_definitions(__wasi__)
link_directories(${WASI_SYS_ROOT}/lib/wasm32-wasi)
include_directories(${THIRD_PARTY_LIBS_DIR})

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

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(demo)
add_subdirectory(libs/wajsGl)
