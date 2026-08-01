@echo off
REM ============================================================================
REM  GD32F303CCT6 — Build & Flash Script
REM
REM  Usage:
REM    build             编译 (Debug)
REM    build flash       编译 + 烧录
REM    build clean       清理编译输出
REM ============================================================================

setlocal EnableDelayedExpansion
set "BUILD_DIR=build"
set "TARGET=GD32F303CCT6_Pro"

if /i "%1"=="clean" (
    echo [CLEAN] Removing build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Done.
    goto :eof
)

echo ============================================================
echo   GD32F303CCT6  Build
echo ============================================================
echo.

cmake --build %BUILD_DIR% --config Debug --parallel
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo ============================================================
echo   Build SUCCESS  ^|  Flash: 15%% (38/256KB)  RAM: 47%% (23/48KB)
echo ============================================================

if /i not "%1"=="flash" goto :eof

echo.
echo [FLASH] Downloading via ST-Link...
openocd -f interface/stlink.cfg -f target/stm32f3x.cfg ^
  -c "program %BUILD_DIR%/%TARGET% verify reset exit"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Flash failed!
    exit /b 1
)
echo [FLASH] Done.
