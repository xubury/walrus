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

set(WASI_SDK_PREFIX ${CMAKE_SOURCE_DIR}/wasi-sdk-${WASI_VERSION_FULL})
set(WASI_SYS_ROOT ${WASI_SDK_PREFIX}/share/wasi-sysroot)
set(build_platform mingw)

set(wasi_package
    ${CMAKE_SOURCE_DIR}/wasi-sdk-${WASI_VERSION_FULL}-${build_platform}.tar.gz)
set(wasi_url
    https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}/wasi-sdk-${WASI_VERSION_FULL}-${build_platform}.tar.gz
)

if(NOT EXISTS ${wasi_package})
  file_download(${wasi_package} ${wasi_url})
endif()

if(NOT EXISTS ${WASI_SDK_PREFIX})
  message("extracting ${wasi_package}")
  file(ARCHIVE_EXTRACT INPUT ${wasi_package} DESTINATION ${CMAKE_SOURCE_DIR})
  # TODO: remove wasi package
endif()

# TODO: wasi compiler for mingw64 is problematic?
option(WASI_COMPILER "Use wasi-sdk vendor compiler" OFF)

if(WASI_COMPILER)
    include(${WASI_SDK_PREFIX}/share/cmake/wasi-sdk.cmake)
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_SYSTEM_PROCESSOR wasm)
    set(triple wasm32)
    set(CMAKE_C_COMPILER_TARGET ${triple})
    set(CMAKE_CXX_COMPILER_TARGET ${triple})

    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)

    set(WASM_LINKER wasm-ld)
    set(CMAKE_C_LINK_EXECUTABLE
        "${WASM_LINKER} <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    set(CMAKE_CXX_LINK_EXECUTABLE
        "${WASM_LINKER} <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
endif()

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

add_compile_options(--sysroot=${WASI_SYS_ROOT} -target wasm32)
link_directories(${WASI_SYS_ROOT}/lib/wasm32-wasi)

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
