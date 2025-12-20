# IJKMediaPlayer Windows Build Script (PowerShell)

param(
    [string]$Platform = "Win32",  # Win32 or x64
    [string]$BuildType = "Release"  # Release or Debug
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "IJKMediaPlayer Windows Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查CMake是否安装
$cmakePath = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakePath) {
    Write-Host "Error: CMake is not found in PATH" -ForegroundColor Red
    Write-Host "Please install CMake and add it to PATH" -ForegroundColor Red
    exit 1
}

Write-Host "Platform: $Platform" -ForegroundColor Green
Write-Host "Build Type: $BuildType" -ForegroundColor Green
Write-Host ""

# 清理旧的构建目录
if (Test-Path "build") {
    Write-Host "Cleaning old build directory..." -ForegroundColor Yellow
    Remove-Item -Path "build" -Recurse -Force
}

# 创建构建目录
Write-Host "Creating build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path "build" -Force | Out-Null
Set-Location "build"

# 配置CMake
Write-Host ""
Write-Host "Configuring CMake..." -ForegroundColor Yellow
$cmakeArgs = @(
    "..",
    "-G", "Visual Studio 16 2019",
    "-A", $Platform
)
& cmake $cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: CMake configuration failed" -ForegroundColor Red
    Set-Location ".."
    exit 1
}

# 编译项目
Write-Host ""
Write-Host "Building project..." -ForegroundColor Yellow
$buildArgs = @(
    "--build", ".",
    "--config", $BuildType,
    "--parallel"
)
& cmake $buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Build failed" -ForegroundColor Red
    Set-Location ".."
    exit 1
}

Set-Location ".."

# 检查输出目录
if (-not (Test-Path "output")) {
    Write-Host "Warning: output directory not found" -ForegroundColor Yellow
    Write-Host "Build may have failed" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Output directory: output" -ForegroundColor Green
Write-Host ""
Get-ChildItem "output" | Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
Write-Host ""

