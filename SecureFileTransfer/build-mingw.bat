@echo off
REM build-mingw.bat - Quick build script for MinGW + Make
REM Usage: build-mingw.bat [all|clean|run-server|run-client]
REM For Windows with GCC 15.2.0 + Make 4.4.1

setlocal enabledelayedexpansion

set TARGET=%1
if "!TARGET!"=="" set TARGET=all

echo ======================================
echo SecureFileTransfer - Windows MinGW Build
echo ======================================
echo.

REM Check if make is available
where make >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Make not found!
    echo Please install MinGW with Make:
    echo   choco install mingw make
    exit /b 1
)

REM Check if g++ is available
where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ not found!
    echo Please install MinGW:
    echo   choco install mingw
    exit /b 1
)

echo [1] Compiler Info:
g++ --version | findstr /R ".*" | head -1
make --version | findstr /R ".*" | head -1
echo.

echo [2] Building: make !TARGET!
echo.

make !TARGET!

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo ======================================
echo [SUCCESS] Build completed!
echo ======================================
echo.

if "!TARGET!"=="all" (
    echo Run server (Terminal 1):
    echo   bin\server_recv.exe 5000 output.txt 12345678
    echo.
    echo Run client (Terminal 2):
    echo   bin\client_send.exe 127.0.0.1 5000 data\plain.txt 12345678
    echo.
    echo Check result:
    echo   fc data\plain.txt output.txt
)

exit /b 0
