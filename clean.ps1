# Clean build artifacts

Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow

if (Test-Path "build") {
    Write-Host "Removing build directory..." -ForegroundColor Yellow
    Remove-Item -Path "build" -Recurse -Force
}

if (Test-Path "output") {
    Write-Host "Removing output directory..." -ForegroundColor Yellow
    Remove-Item -Path "output" -Recurse -Force
}

Write-Host "Clean completed." -ForegroundColor Green

