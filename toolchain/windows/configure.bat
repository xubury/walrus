@echo off

if not exist "build" (
    mkdir "build"
)

conan install conan/conanfile.txt  --profile:host=conan/emscripten.profile --profile:build=default --build missing -if build
(
call "build\activate.bat"
emcmake cmake -G "MinGW Makefiles" -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
) & (
call "build\deactivate.bat"
)
