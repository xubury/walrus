@echo off

if exist "build" (
    (
    call "build\activate.bat"
    emrun .\build\bin\webasm_demo.html
    ) & (
    call "build\deactivate.bat"
    )
)
