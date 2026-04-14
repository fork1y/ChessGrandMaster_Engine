@echo off
setlocal

where gcc >nul 2>nul
if %errorlevel%==0 (
    gcc -O2 -std=c11 -Wall -Wextra -o engine.exe src\engine.c
    echo Built engine.exe with gcc
    exit /b %errorlevel%
)

where clang >nul 2>nul
if %errorlevel%==0 (
    clang -O2 -std=c11 -Wall -Wextra -o engine.exe src\engine.c
    echo Built engine.exe with clang
    exit /b %errorlevel%
)

echo No C compiler found (need gcc or clang in PATH).
exit /b 1
