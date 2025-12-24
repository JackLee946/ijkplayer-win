# IJKMediaPlayer Windows Build Script (PowerShell)

param(
    [string]$Platform = "Win32",  # Win32 or x64
    [string]$BuildType = "Release",  # Release or Debug
    [switch]$Clean  # Pass -Clean to force removing the build directory
)

# 强制只使用 Win32（x86）平台构建，忽略传入的其他平台参数。
if ($PSBoundParameters.ContainsKey('Platform') -and $Platform -ne 'Win32') {
    Write-Host "Note: Forcing Platform to Win32 (x86). Ignoring requested platform: $Platform" -ForegroundColor Yellow
    $Platform = 'Win32'
} else {
    # Ensure canonical value
    $Platform = 'Win32'
}

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

# 控制是否执行完全清理。默认执行增量构建（保留现有 build 目录），
# 若需要完全重建可传入 -Clean 参数强制删除并重新配置。
if (Test-Path "build") {
    if ($Clean) {
        Write-Host "Cleaning old build directory (forced)..." -ForegroundColor Yellow
        Remove-Item -Path "build" -Recurse -Force
        Write-Host "Creating build directory..." -ForegroundColor Yellow
        New-Item -ItemType Directory -Path "build" -Force | Out-Null
    } else {
        Write-Host "Using existing build directory (incremental build)" -ForegroundColor Yellow
    }
} else {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" -Force | Out-Null
}

Set-Location "build"

# 仅在首次生成或显式清理后运行 CMake 配置，以支持增量构建。
if (-not (Test-Path "CMakeCache.txt")) {
    Write-Host "";
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    $cmakeArgs = @(
        "-S", "..",
        "-B", ".",
        "-G", "Visual Studio 16 2019",
        "-A", $Platform
    )
    & cmake $cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: CMake configuration failed" -ForegroundColor Red
        Set-Location ".."
        exit 1
    }
} else {
    Write-Host "CMake cache found — skipping configure step (incremental)." -ForegroundColor Green
}

# 编译/增量构建
Write-Host ""
Write-Host "Building project (incremental)..." -ForegroundColor Yellow
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

