@echo off
REM Quick Build Script for Application Manager
REM This script sets up and builds the Application Manager project

setlocal enabledelayedexpansion

echo.
echo ===============================================
echo   Application Manager - Quick Build Script
echo ===============================================
echo.

REM Check if CMake is installed
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download
    echo and add it to your system PATH
    pause
    exit /b 1
)

REM Check if Visual Studio is installed
where cl.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: Visual Studio C++ compiler not found
    echo Please install Visual Studio 2022 with C++ workload
    echo or run this script from "Developer Command Prompt"
    pause
    exit /b 1
)

REM Check if winget is available
where winget >nul 2>nul
if errorlevel 1 (
    echo WARNING: winget not found
    echo Install Windows Package Manager from Microsoft Store
    echo for full functionality
    echo.
)

echo Checking dependencies...
echo [OK] CMake found
echo [OK] Visual Studio compiler found
echo.

REM Create build directory
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Run CMake
echo Configuring CMake...
echo.

REM Try to find Qt6
set QT_PATH=
if defined Qt6_DIR (
    set "QT_PATH=-DCMAKE_PREFIX_PATH=!Qt6_DIR!"
)

cmake .. -G "Visual Studio 17 2022" -A x64 !QT_PATH!

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed
    echo.
    echo If Qt6 is not found, please:
    echo 1. Install Qt 6 from https://www.qt.io/download-open-source
    echo 2. Set environment variable: set Qt6_DIR=C:\Qt\6.x.x\msvc2022_64
    echo 3. Run this script again
    echo.
    pause
    cd ..
    exit /b 1
)

echo.
echo Building Release version...
echo.

cmake --build . --config Release

if errorlevel 1 (
    echo.
    echo ERROR: Build failed
    echo Please check the error messages above
    echo.
    pause
    cd ..
    exit /b 1
)

echo.
echo ===============================================
echo   Build completed successfully!
echo ===============================================
echo.
echo Executable location: build\Release\ApplicationManager.exe
echo.
echo To run the application:
echo   .\Release\ApplicationManager.exe
echo.
echo To open in Visual Studio 2022:
echo   start ApplicationManager.sln
echo.
pause
cd ..
