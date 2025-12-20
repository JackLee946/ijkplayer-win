@echo off
echo Cleaning build artifacts...

if exist build (
    echo Removing build directory...
    rmdir /s /q build
)

if exist output (
    echo Removing output directory...
    rmdir /s /q output
)

echo Clean completed.

