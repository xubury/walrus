set(WASI_VERSION 14)
set(WASI_VERSION_FULL ${WASI_VERSION}.0)

if(WIN32)
  set(BUILD_PLATFORM mingw)
elseif(UNIX)
  set(BUILD_PLATFORM linux)
else()
  set(BUILD_PLATFORM macos)
endif()

set(WASM_LIB_DIR ${CMAKE_SOURCE_DIR}/libs)
set(WASI_SDK_PREFIX ${WASM_LIB_DIR}/wasi-sdk-${WASI_VERSION_FULL})
set(WASI_SYS_ROOT ${WASI_SDK_PREFIX}/share/wasi-sysroot)

set(WASM_RUNTIME_DIR ${CMAKE_SOURCE_DIR}/web/res)
set(WASI_HOST_URL
    https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}
)
set(WASI_PACKAGE_NAME wasi-sdk-${WASI_VERSION_FULL}-${BUILD_PLATFORM}.tar.gz)
set(WASI_PACKAGE ${CMAKE_SOURCE_DIR}/${WASI_PACKAGE_NAME})
set(WASI_URL ${WASI_HOST_URL}/${WASI_PACKAGE_NAME})

if(NOT EXISTS ${WASI_SDK_PREFIX})
  if(NOT EXISTS ${WASI_PACKAGE})
    file_download(${WASI_PACKAGE} ${WASI_URL})
  endif()
  message("extracting ${WASI_PACKAGE}")
  file(ARCHIVE_EXTRACT INPUT ${WASI_PACKAGE} DESTINATION ${WASM_LIB_DIR})
  file(REMOVE ${WASI_PACKAGE})
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

if(NOT BUILD_TEST)
  set(WASM 1)
  add_compile_options(--sysroot=${WASI_SYS_ROOT} --target=wasm32-wasi)
  add_compile_definitions(__wasi__)
  link_directories(${WASI_SYS_ROOT}/lib/wasm32-wasi)
else()
  set(WASM 0)
endif()
