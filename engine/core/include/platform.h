#pragma once

#define PLATFORM_UNKNOWN     0x00000000
#define PLATFORM_WINDOWS     0x00000001
#define PLATFORM_LINUX       0x00000002
#define PLATFORM_APPLE       0x00000003
#define PLATFORM_ANDROID     0x00000004
#define PLATFORM_CHROME_NACL 0x00000005
#define PLATFORM_UNIX        0x00000006
#define PLATFORM_QNXNTO      0x00000007
#define PLATFORM_WINCE       0x00000008
#define PLATFORM_CYGWIN      0x00000009
#define PLATFORM_WASI        0x00000010

#ifdef FORCE_PLATFORM_UNKNOWN
#define PLATFORM PLATFORM_UNKNOWN
#elif defined(__CYGWIN__)
#define PLATFORM PLATFORM_CYGWIN
#elif defined(__QNXNTO__)
#define PLATFORM PLATFORM_QNXNTO
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_APPLE
#elif defined(WINCE)
#define PLATFORM PLATFORM_WINCE
#elif defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__native_client__)
#define PLATFORM PLATFORM_CHROME_NACL
#elif defined(__ANDROID__)
#define PLATFORM PLATFORM_ANDROID
#elif defined(__linux)
#define PLATFORM PLATFORM_LINUX
#elif defined(__unix)
#define PLATFORM PLATFORM_UNIX
#elif defined(__wasi__)
#define PLATFORM PLATFORM_WASI
#else
#define PLATFORM PLATFORM_UNKNOWN
#endif

///////////////////////////////////////////////////////////////////////////////////
// Compiler

#define COMPILER_UNKNOWN 0x00000000

// Intel
#define COMPILER_INTEL 0x00000001

// Visual C++ defines
#define COMPILER_VC 0x00000002

// GCC defines
#define COMPILER_GCC 0x00000003

// CUDA
#define COMPILER_CUDA 0x00000004

// SYCL
#define COMPILER_SYCL 0x00000005

// Clang
#define COMPILER_CLANG 0x00000006

// Build model
#define MODEL_32 0x00000010
#define MODEL_64 0x00000020

// Force generic C++ compiler
#ifdef FORCE_COMPILER_UNKNOWN
#define COMPILER COMPILER_UNKNOWN

#elif defined(__INTEL_COMPILER)
#define COMPILER COMPILER_INTEL

// CUDA
#elif defined(__CUDACC__)
#if !defined(CUDA_VERSION) && !defined(FORCE_CUDA)
#include <cuda.h>  // make sure version is defined since nvcc does not define it itself!
#endif
#define COMPILER COMPILER_CUDA

// SYCL
#elif defined(__SYCL_DEVICE_ONLY__)
#define COMPILER COMPILER_SYCL

// Clang
#elif defined(__clang__)
#define COMPILER COMPILER_CLANG

// Visual C++
#elif defined(_MSC_VER)
#define COMPILER COMPILER_VC

// G++
#elif defined(__GNUC__) || defined(__MINGW32__)
#define COMPILER COMPILER_GCC

#else
#define COMPILER COMPILER_UNKNOWN
#endif
