# Building and Transferring to Visual Studio 2022

This guide provides step-by-step instructions for building the Application Manager project and transferring it to Visual Studio 2022.

## Prerequisites Checklist

Before starting, ensure you have:
- [ ] Windows 10 or Windows 11
- [ ] Visual Studio 2022 with C++ workload installed
- [ ] CMake 3.21+ installed (add to PATH)
- [ ] Qt 6 installed (or will install during setup)
- [ ] Windows Package Manager (winget) installed
- [ ] Git installed (optional but recommended)

### Verifying Prerequisites

```powershell
# Check CMake
cmake --version

# Check Visual Studio (should show path)
where cl.exe

# Check winget
winget --version

# Check Qt (if already installed)
where qmake
```

## Quick Setup (5-10 minutes)

### Step 1: Install Qt 6 (if needed)

1. Download Qt 6 from https://www.qt.io/download-open-source
2. Run installer and select:
   - Qt 6.x version
   - Desktop MSVC components
   - MSVC 2019 or 2022 compiler
   - CMake, Ninja, or other build tools

3. Note the installation path (e.g., `C:\Qt\6.x.x`)

### Step 2: Build with CMake

```powershell
# Navigate to project directory
cd C:\Users\SESA666957\Desktop\citanje\work\ApplicationManager

# Create build directory
mkdir build
cd build

# Configure CMake (generates Visual Studio solution)
# Option A: Let CMake find Qt automatically
cmake .. -G "Visual Studio 17 2022" -A x64

# Option B: Specify Qt path explicitly
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"

# Build Release version
cmake --build . --config Release

# Or build Debug version
cmake --build . --config Debug
```

### Step 3: Run the Application

```powershell
# From the build folder
.\Release\ApplicationManager.exe

# Or Debug version
.\Debug\ApplicationManager.exe
```

## Opening in Visual Studio 2022

### Method 1: Use CMake Solution

After running CMake, a Visual Studio solution is automatically generated:

```powershell
# Open the generated solution
start build\ApplicationManager.sln
```

Or manually:
1. Open Visual Studio 2022
2. File → Open → Project/Solution
3. Navigate to `build\ApplicationManager.sln`
4. Click Open

### Method 2: Open Project Folder Directly

1. File → Open Folder
2. Select the `ApplicationManager` folder
3. VS Code will detect CMake configuration
4. Click on CMake in the sidebar
5. Select the configuration and click Build

## Visual Studio Build Configuration

### Setting Up for Development

1. **Configure Qt Path**
   - Tools → Options → CMake → CMake Variables
   - Or edit `CMakeUserPresets.json` and add:
     ```json
     "cacheVariables": {
       "CMAKE_PREFIX_PATH": "C:/Qt/6.x.x/msvc2022_64"
     }
     ```

2. **Configure for Debugging**
   - Debug → Debug Configuration
   - Ensure working directory is set to your build folder

3. **Set Default Configuration**
   - Build → Configuration Manager
   - Set Active Solution Configuration to "Release" or "Debug"

## Troubleshooting Build Issues

### CMake can't find Qt

**Problem**: `CMake Error: "Qt6 not found"`

**Solution**:
```powershell
# Option 1: Set environment variable
$env:Qt6_DIR="C:\Qt\6.x.x\msvc2022_64"
cmake .. -G "Visual Studio 17 2022" -A x64

# Option 2: Use CMAKE_PREFIX_PATH
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
```

### Compiler not found

**Problem**: `Visual Studio compiler (cl.exe) not found`

**Solution**:
1. Ensure Visual Studio is installed with C++ workload
2. Run from "Developer Command Prompt for VS 2022"
3. Or add to PATH: `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.x\bin\Hostx64\x64`

### Qt DLLs not found at runtime

**Problem**: Application crashes with missing DLL errors

**Solution**:
1. Copy Qt DLLs to build output folder:
   ```powershell
   $QtBin = "C:\Qt\6.x.x\msvc2022_64\bin"
   Copy-Item "$QtBin\Qt6Core.dll" "build\Release\"
   Copy-Item "$QtBin\Qt6Gui.dll" "build\Release\"
   Copy-Item "$QtBin\Qt6Widgets.dll" "build\Release\"
   Copy-Item "$QtBin\Qt6Concurrent.dll" "build\Release\"
   Copy-Item "$QtBin\Qt6Network.dll" "build\Release\"
   ```

2. Or add Qt bin folder to PATH

### nlohmann_json not found

**Problem**: `Could not find nlohmann_json`

**Solution**: The library is header-only and bundled. CMakeLists.txt includes it from `extern/` folder. If missing, download:
```powershell
# Create extern folder if it doesn't exist
mkdir src\extern\nlohmann_json\include

# Download single_include version
curl https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp `
  -O -L -o src\extern\nlohmann_json\include\json.hpp
```

## Creating a Distributable Package

### Step 1: Build Release Version

```powershell
cd build
cmake --build . --config Release
```

### Step 2: Gather Dependencies

```powershell
# Create deployment folder
mkdir deploy\ApplicationManager

# Copy executable
Copy-Item "Release\ApplicationManager.exe" "deploy\ApplicationManager\"

# Copy Qt libraries
$QtBin = "C:\Qt\6.x.x\msvc2022_64\bin"
$QtLibs = @(
    "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll",
    "Qt6Concurrent.dll", "Qt6Network.dll",
    "Qt6Core_plugin.dll"  # Plugins if needed
)
foreach ($lib in $QtLibs) {
    if (Test-Path "$QtBin\$lib") {
        Copy-Item "$QtBin\$lib" "deploy\ApplicationManager\"
    }
}

# Copy Qt plugins
Copy-Item "$QtBin\..\plugins\platforms" "deploy\ApplicationManager\plugins\" -Recurse -Force
```

### Step 3: Use Qt Deployment Tool

```powershell
$Qt = "C:\Qt\6.x.x\msvc2022_64\bin"

# Deploy application
& "$Qt\windeployqt.exe" deploy\ApplicationManager\ApplicationManager.exe
```

## Project Structure in Visual Studio

After CMake configuration, Visual Studio will show:

```
CMakeLists.txt
src/
  ├── main.cpp
  ├── MainWindow.h/cpp
  ├── AppListModel.h/cpp
  ├── WingetBackend.h/cpp
  ├── AsyncWorker.h/cpp
  └── models/
      └── AppData.h
resources/
  └── resources.qrc
```

## Debugging Tips

### Enable Qt Creator Integration
- Tools → Options → Debugger → Qt Creator integration
- Improves debugging of Qt objects

### Debugging Signals/Slots
- In Visual Studio, you can set breakpoints in slot functions
- Use the "Debug Output" window to see Qt debug messages

### Profiling
- Use Qt Creator's profiler for performance analysis
- Built-in in Visual Studio: Debug → Performance Profiler

## Transferring to Another Machine

To transfer the compiled project to another computer:

1. **Minimal Setup** (just run exe):
   - Copy entire `deploy\ApplicationManager` folder
   - Recipient needs Windows 10/11 + winget
   - No development tools needed

2. **For Further Development**:
   - Copy entire `ApplicationManager` folder
   - Recipient needs Qt 6 + CMake + Visual Studio
   - Run CMake configuration steps above

3. **Create Installation**:
   - Use NSIS or WiX to create proper installer
   - Include Qt dependencies
   - Register file associations if desired

## Next Steps

1. Open in Visual Studio: `start build\ApplicationManager.sln`
2. Explore the code structure
3. Modify features as needed
4. For debugging: Press F5 in Visual Studio
5. For release build: Build → Batch Build → Select Release

## Useful Visual Studio Commands

| Action | Command |
|--------|---------|
| Build | Ctrl+Shift+B |
| Debug | F5 |
| Run Without Debug | Ctrl+F5 |
| Rebuild | Ctrl+Alt+Z |
| Clean | Edit → Clean |
| Find | Ctrl+H |
| Go to File | Ctrl+T |

---

**Last Updated**: August 2026
**Version**: 1.0
**Status**: Ready for production use
