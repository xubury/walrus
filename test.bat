@echo off
echo Building test...
call cmake -G "Ninja" -B build/test -DBUILD_TEST=1 -DCMAKE_BUILD_TYPE=Debug
call cmake --build build/test
cd build/test
echo Running test...
call ctest
cd ../..
