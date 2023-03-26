# Web Assembly project

Project for learning Web Assembly.

## Reference

-   [C to wasm](https://developer.mozilla.org/en-US/docs/WebAssembly/C_to_wasm)
-   [how to wasm](https://github.com/ern0/howto-wasm-minimal/)
-   [clang wasm](https://schellcode.github.io/webassembly-without-emscripten)
-   [React Electron build](https://weirenxue.github.io/2021/08/04/react_electron_build/)

## Goals

-   Run a webassembly demo.
    -   Run cmake to `configure` and `build`.
    -   `cd` to `server` folder, edit `views/webasm_demo.html` 's `filename` to the output `*.wasm` accordingly.
    -   run `npm run server`.
-   Debug wsm demo.

    -   To debug wsm, add `-g` compile option (default by CMake `Debug` build type), install chrome extension [wasm-debugging-extension](https://goo.gle/wasm-debugging-extension).
        In the DevTools, go to the Experiments panel, and tick `WebAssembly Debugging: Enable DWARF support`. After that, reboot the browser, there should be a new `file:\\` section in `Sources` where you can set breakpoints in `c/cpp` sources files.

-   Handle input in webassembly
-   OpenGL/Vulkan on webassembly
-   Packaging the web project using Webpack, adding hot reload feature.
-   Porting my engine to webassembly?

## Build requirements

-   CMake
-   Node.js

# Notes

2023-03-06

-   Currently the demo is built with `Emscripten`.
    However, `LLVM` provide a `wasm-ld` that we can just use `clang` without `Emscripten`?
    See [how to wasm](https://github.com/ern0/howto-wasm-minimal/)
-   So, wasm code can be run by starting a http-server, `npm` install `http-server` and `assemblyscript`
    then just run `npx http-server` in the folder containing the `html`, `js` and `wasm` file.

2023-03-07

-   To build wasm with `libc` and `libc++`, we need sysroot from [Emscripten](https://github.com/emscripten-core/emscripten/tree/main/system) or [wasi-libc](https://github.com/WebAssembly/wasi-libc/tree/main).

2023-03-08

-   Newer version of `wasi-libc` requires more implementation of `syscall` function, using the older one is fine.
-   Currently, the `printf` is broken, I think there's something wrong with heap memory, or `ReadHeapString` and `fd_write`.
-   `printf` bug is fixed now, I was using linux version of wasi-sdk smh..
-   Can't define `main` as entry, don't know why. Probably linker flags is incorrect. Using `wasm_main` as substitute for now.
-   I think I should link crt1?
    > `crt1.o` provides the `_start` symbol that the runtime linker, `ld.so.1`, jumps to in order to pass control to the executable.

2023-03-09

-   To use `main` properly, I have to export `__main_argc_argv`. `wasi-sdk` seems work this by linking `crt1.o` and `libclang_rt.builtins-wasm32.a`.
    But I never work that out in `wasi-sdk`'s way.

2023-03-10

-   To use c++ std library, I should link `libc++` and `libc++abi` (The wasm size is drastically increased after linking).

2023-03-12

-   Plan to build app in `Electron` which provide `node.js` runtime, skipping `wasi` syscall emulation.

2023-03-14

-   Using electron runtime is not good, can't run on browser. Using a server to deal with node and render to html.


2023-03-26
- function pointer from c/c++ will be converted to table index in function table. Add `-Wl,--export-table` then call `__indirect_function_table.get` from wasm instance to invoke the function.
