#pragma once

#define WR_PLATFORM_UNKNOWN     0x00000000
#define WR_PLATFORM_WINDOWS     0x00000001
#define WR_PLATFORM_LINUX       0x00000002
#define WR_PLATFORM_APPLE       0x00000003
#define WR_PLATFORM_ANDROID     0x00000004
#define WR_PLATFORM_CHROME_NACL 0x00000005
#define WR_PLATFORM_UNIX        0x00000006
#define WR_PLATFORM_QNXNTO      0x00000007
#define WR_PLATFORM_WINCE       0x00000008
#define WR_PLATFORM_CYGWIN      0x00000009
#define WR_PLATFORM_WASM        0x00000010

#ifdef FORCE_WR_PLATFORM_UNKNOWN
#define WR_PLATFORM WR_PLATFORM_UNKNOWN
#elif defined(__CYGWIN__)
#define WR_PLATFORM WR_PLATFORM_CYGWIN
#elif defined(__QNXNTO__)
#define WR_PLATFORM WR_PLATFORM_QNXNTO
#elif defined(__APPLE__)
#define WR_PLATFORM WR_PLATFORM_APPLE
#elif defined(WINCE)
#define WR_PLATFORM WR_PLATFORM_WINCE
#elif defined(_WIN32)
#define WR_PLATFORM WR_PLATFORM_WINDOWS
#elif defined(__native_client__)
#define WR_PLATFORM WR_PLATFORM_CHROME_NACL
#elif defined(__ANDROID__)
#define WR_PLATFORM WR_PLATFORM_ANDROID
#elif defined(__linux)
#define WR_PLATFORM WR_PLATFORM_LINUX
#elif defined(__unix)
#define WR_PLATFORM WR_PLATFORM_UNIX
#elif defined(__wasi__)
#define WR_PLATFORM WR_PLATFORM_WASM
#else
#define WR_PLATFORM WR_PLATFORM_UNKNOWN
#endif

///////////////////////////////////////////////////////////////////////////////////
// Compiler

#define WR_COMPILER_UNKNOWN 0x00000000

// Intel
#define WR_COMPILER_INTEL 0x00000001

// Visual C++ defines
#define WR_COMPILER_VC 0x00000002

// GCC defines
#define WR_COMPILER_GCC 0x00000003

// CUDA
#define WR_COMPILER_CUDA 0x00000004

// SYCL
#define WR_COMPILER_SYCL 0x00000005

// Clang
#define WR_COMPILER_CLANG 0x00000006

// Build model
#define MODEL_32 0x00000010
#define MODEL_64 0x00000020

// Force generic C++ compiler
#ifdef FORCE_WR_COMPILER_UNKNOWN
#define WR_COMPILER WR_COMPILER_UNKNOWN

#elif defined(__INTEL_COMPILER)
#define WR_COMPILER WR_COMPILER_INTEL

// CUDA
#elif defined(__CUDACC__)
#if !defined(CUDA_VERSION) && !defined(FORCE_CUDA)
#include <cuda.h>  // make sure version is defined since nvcc does not define it itself!
#endif
#define WR_COMPILER WR_COMPILER_CUDA

// SYCL
#elif defined(__SYCL_DEVICE_ONLY__)
#define WR_COMPILER WR_COMPILER_SYCL

// Clang
#elif defined(__clang__)
#define WR_COMPILER WR_COMPILER_CLANG

// Visual C++
#elif defined(_MSC_VER)
#define WR_COMPILER WR_COMPILER_VC

// G++
#elif defined(__GNUC__) || defined(__MINGW32__)
#define WR_COMPILER WR_COMPILER_GCC

#else
#define WR_COMPILER WR_COMPILER_UNKNOWN
#endif
