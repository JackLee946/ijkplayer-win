@echo off
setlocal enabledelayedexpansion

echo ========================================
echo IJKMediaPlayer Windows Build Script
echo ========================================
echo.

REM 检查CMake是否安装
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: CMake is not found in PATH
    echo Please install CMake and add it to PATH
    exit /b 1
)

REM 设置平台架构（默认Win32，可通过参数指定）
set PLATFORM=Win32
if "%1"=="x64" set PLATFORM=x64
if "%1"=="win32" set PLATFORM=Win32

REM 设置构建类型（默认Release，可通过参数指定）
set BUILD_TYPE=Release
if "%2"=="debug" set BUILD_TYPE=Debug
if "%2"=="Debug" set BUILD_TYPE=Debug

echo Platform: %PLATFORM%
echo Build Type: %BUILD_TYPE%
echo.

REM 创建构建目录（如果不存在）
if not exist build (
    echo Creating build directory...
    mkdir build
)
cd build

REM 配置CMake（仅当构建系统文件不存在时）
if not exist CMakeCache.txt (
    echo.
    echo Configuring CMake...
    cmake .. -G "Visual Studio 16 2019" -A %PLATFORM%
    if %errorlevel% neq 0 (
        echo Error: CMake configuration failed
        cd ..
        exit /b 1
    )
) else (
    echo.
    echo CMake already configured, skipping...
)

REM 编译项目
echo.
echo Building project...
cmake --build . --config %BUILD_TYPE% --parallel
if %errorlevel% neq 0 (
    echo Error: Build failed
    cd ..
    exit /b 1
)

cd ..

REM 检查输出目录
if not exist output (
    echo Warning: output directory not found
    echo Build may have failed
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo Output directory: output
echo.
dir output /b
echo.

endlocal