# Web Assembly project

Project for learning Web Assembly.

## Reference

-   [C to wasm](https://developer.mozilla.org/en-US/docs/WebAssembly/C_to_wasm)
-   [how to wasm](https://github.com/ern0/howto-wasm-minimal/)
-   [clang wasm](https://schellcode.github.io/webassembly-without-emscripten)

## Goals

-   Run a webassembly demo.
    -   run `configure.bat` and `build.bat` to build `demo` app
    -   `run.bat` to run `demo`
-   Debug wsm demo
-   Handle input in webassembly
-   OpenGL/Vulkan on webassembly
-   Porting my engine to webassembly?

## Build requirements

-   CMake
-   conan

# Notes

2023-03-06

-   Currently the demo is built with `Emscripten`.
    However, `LLVM` provide a `wasm-ld` that we can just use `clang` without `Emscripten`?
    See [how to wasm](https://github.com/ern0/howto-wasm-minimal/)
-   So, wasm code can be run by starting a http-server, `npm` install `http-server` and `assemblyscript`
    then just run `npx http-server` in the folder containing the `html`, `js` and `wasm` file.

2023-03-07

-   To build wasm with `libc` and `libc++`, we need headers from [Emscripten](https://github.com/emscripten-core/emscripten/tree/main/system).
