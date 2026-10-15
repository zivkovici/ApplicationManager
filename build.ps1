# Quick Build Script for Application Manager
# PowerShell version for modern Windows development
# 
# Usage: .\build.ps1 [-Config Release] [-Qt6Path "C:\Qt\6.x.x\msvc2022_64"]

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    
    [string]$Qt6Path = $null
)

# Enable strict error handling
$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "   Application Manager - Build Script" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

# Helper function to check if command exists
function Test-Command {
    param([string]$Command)
    try {
        if (Get-Command $Command -ErrorAction Stop) {
            return $true
        }
    }
    catch {
        return $false
    }
}

# Check for required tools
Write-Host "Checking dependencies..." -ForegroundColor Yellow
Write-Host ""

# Check CMake
if (Test-Command cmake) {
    Write-Host "[OK] CMake found" -ForegroundColor Green
    $cmakeVersion = (cmake --version | Select-Object -First 1)
    Write-Host "     $cmakeVersion"
} else {
    Write-Host "[FAIL] CMake not found" -ForegroundColor Red
    Write-Host "     Install from: https://cmake.org/download" -ForegroundColor Yellow
    exit 1
}

# Check Visual Studio C++ compiler
if (Test-Path "C:\Program Files\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe") {
    Write-Host "[OK] Visual Studio 2022 C++ compiler found" -ForegroundColor Green
} else {
    Write-Host "[FAIL] Visual Studio C++ compiler not found" -ForegroundColor Red
    Write-Host "     Please install Visual Studio 2022 with C++ workload" -ForegroundColor Yellow
    exit 1
}

# Check winget (optional but recommended)
if (Test-Command winget) {
    Write-Host "[OK] Windows Package Manager (winget) found" -ForegroundColor Green
} else {
    Write-Host "[WARN] winget not found (optional)" -ForegroundColor Yellow
    Write-Host "      Install from Microsoft Store for full functionality" -ForegroundColor Yellow
}

# Check Qt6
$qt6Found = $false
if ($Qt6Path -and (Test-Path "$Qt6Path\bin\qmake.exe")) {
    Write-Host "[OK] Qt6 found at specified path" -ForegroundColor Green
    $qt6Found = $true
} elseif ($env:Qt6_DIR) {
    if (Test-Path "$($env:Qt6_DIR)\bin\qmake.exe") {
        Write-Host "[OK] Qt6 found via environment variable" -ForegroundColor Green
        $Qt6Path = $env:Qt6_DIR
        $qt6Found = $true
    }
}

if (-not $qt6Found) {
    Write-Host "[WARN] Qt6 not found" -ForegroundColor Yellow
    Write-Host "      Please install Qt6 from https://www.qt.io/download-open-source" -ForegroundColor Yellow
    Write-Host "      Or set Qt6_DIR environment variable" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Continuing with build... (may fail if Qt6 is not properly configured)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Configuration: $Config" -ForegroundColor Cyan
Write-Host ""

# Create build directory
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Change to build directory
Push-Location -Path "build"

try {
    # Configure CMake
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    Write-Host ""
    
    $cmakeArgs = @(
        "..",
        "-G", "Visual Studio 17 2022",
        "-A", "x64"
    )
    
    if ($Qt6Path) {
        $cmakeArgs += @("-DCMAKE_PREFIX_PATH=$Qt6Path")
    }
    
    & cmake @cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
        Write-Host ""
        Write-Host "Troubleshooting:" -ForegroundColor Yellow
        Write-Host "1. Ensure Qt6 is installed"
        Write-Host "2. Set Qt6_DIR environment variable to Qt installation path"
        Write-Host "3. Run from Developer Command Prompt for VS 2022"
        Write-Host ""
        exit 1
    }
    
    Write-Host ""
    Write-Host "Building $Config version..." -ForegroundColor Yellow
    Write-Host ""
    
    # Build project
    & cmake --build . --config $Config
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: Build failed" -ForegroundColor Red
        Write-Host "Please check the error messages above"
        exit 1
    }
    
    Write-Host ""
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host "   Build completed successfully!" -ForegroundColor Green
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Executable location: build\$Config\ApplicationManager.exe" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To run the application:" -ForegroundColor Yellow
    Write-Host "   .\$Config\ApplicationManager.exe"
    Write-Host ""
    Write-Host "To open in Visual Studio 2022:" -ForegroundColor Yellow
    Write-Host "   start ApplicationManager.sln"
    Write-Host ""
    
}
finally {
    Pop-Location
}
