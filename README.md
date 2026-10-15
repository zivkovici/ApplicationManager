# Application Manager - Microsoft Store-Like Desktop Application

This is my submission for the C++ technical assessment - a Windows desktop app that works a bit like the Microsoft Store: you can search for apps, install them, see what's already on your machine, and manage everything from one window. Under the hood it drives the Windows Package Manager (`winget`) instead of talking to any Store API directly.

## Overview

A few things I wanted this project to show:
- A working C++20 desktop app built against the v143 toolset in Visual Studio 2022
- An async design where the UI never freezes, even while winget is doing something slow (like installing an app)
- A backend integration with `winget` that's actually reliable, not just "works on my machine"
- Live status updates for every app in the list (Available, Installing, Installed, Error, etc.)

## How it's put together

### Stack I picked and why

I went with **Qt 6** for the GUI. It has a mature widget set, the signal/slot mechanism makes the async plumbing straightforward, and it plays nicely with C++20. I used **CMake** for the build instead of a hand-written `.vcxproj`, mostly because it's easier to keep the Qt path and other settings out of the repo and let each machine configure itself (through `CMakePresets.json`). Opening the folder in Visual Studio 2022 generates the actual `.sln`/`.vcxproj` on the fly using the v143 toolset.

The part that took the most trial and error was the **winget integration**, so I want to explain it properly instead of glossing over it:

- **winget doesn't have a real JSON output mode.** I originally assumed `--output json` existed (a lot of blog posts imply it does), but the installed CLI rejected it outright. So instead of parsing structured data, `WingetBackend` reads winget's normal console table output and reconstructs the columns by finding the header row (the one right before the row of dashes) and using the position of each header word to slice up the data rows underneath.
- **winget also draws a progress spinner using bare `\r`** (carriage return, no newline) to redraw the same line - the classic terminal trick. If you split output naively on `\n`, the spinner frames and the real header can end up glued together into one giant line. I handle this by collapsing each physical line down to whatever comes after its *last* `\r`, which is what a real terminal would be showing at that point.
- **Calling `winget.exe` directly and redirecting its output hangs forever.** This is apparently a known quirk with how Windows "App Execution Alias" stubs (which is what `winget` actually is under `%LOCALAPPDATA%\Microsoft\WindowsApps`) behave when their stdio is piped instead of attached to a real console. Routing the call through `cmd.exe` fixes the hang.
- **...but `cmd.exe /c "<command with quotes>"` isn't safe either.** cmd has its own, different rules for stripping quotes from whatever follows `/c`, and those rules don't match how a normal C/C++ program would escape arguments. If a search query or the combined command string contains embedded quotes, cmd can mangle it in ways that are hard to predict. My fix was to write the winget command into a small temporary `.bat` file and run `cmd.exe /c <path-to-script>` - cmd just reads and executes the file line by line like you'd typed it yourself, so none of that inline quoting weirdness applies.
- **winget can block waiting for a Y/N answer** the first time it touches a source (like the Microsoft Store source) that has agreements to accept. Since the app never feeds anything into winget's stdin, that prompt would just hang forever, so I always pass `--accept-source-agreements` (and `--accept-package-agreements` for installs/updates) and also explicitly bind winget's stdin to `NUL`, just in case something else tries to read from it.
- One smaller gotcha: the silent-install flag is `--silent`, not `-q` - I initially used `-q` and winget complained that it "requires a value" for it.

### Roughly how a request flows through the app

```
MainWindow (UI)
   │  user clicks Search / Install / Uninstall / etc.
   ▼
AsyncWorker (QObject living on its own QThread)
   │  does the actual work, never touches the UI directly
   ▼
WingetBackend
   │  writes a temp .bat, runs it through cmd.exe, parses the text table
   ▼
winget CLI  →  Microsoft Store / installed package list
```

Results come back the same way in reverse, via Qt signals, so the UI thread only ever reacts to finished results - it never blocks waiting on a process to exit.

### What's in each file

- **`MainWindow.h/cpp`** - the actual window: search box, the table of apps, the action buttons (Install/Uninstall/Launch/Update), and the status bar at the bottom. It owns the worker thread and just reacts to signals coming back from it.
- **`AsyncWorker.h/cpp`** - lives on a separate `QThread`. Every slow operation (search, install, uninstall, list, update-check) happens here so the UI thread is free. Emits signals back with the results.
- **`WingetBackend.h/cpp`** - all the winget-specific logic described above: building the command, running it safely through `cmd.exe`, and parsing the resulting table into `AppData` objects.
- **`AppListModel.h/cpp`** - a `QAbstractTableModel` that backs the table view. Color-codes rows by status (yellow while installing/updating, green once installed, red on error).
- **`models/AppData.h`** - the plain data struct for a single app (id, name, version, publisher, status, etc.) plus the `AppStatus` enum.

## Features

Core stuff from the assessment:
- Search the Microsoft Store from inside the app
- See what's currently installed
- Install apps with live status feedback (Available → Installing → Installed)
- Everything above runs asynchronously - the window stays responsive the whole time

Bonus stuff I also implemented:
- Launch an installed app straight from the list
- Uninstall an installed app
- Check for updates and apply them

One thing I'll flag honestly: search results always show status "Available", even if that exact package is already installed - I didn't cross-reference the search results against the installed list. It's not incorrect behavior (installing an already-installed package is handled gracefully and just reports success), just something I'd polish further given more time.

## Requirements

- Windows 10 or 11
- `winget` (Windows Package Manager) available on PATH
- Visual Studio 2022 with the Desktop C++ workload (v143 toolset)
- Qt 6 (Core, Gui, Widgets, Concurrent, Network) - built with the MSVC 2022 64-bit kit
- CMake 3.21+

If `winget` isn't installed, get "App Installer" from the Microsoft Store, or grab a release from https://github.com/microsoft/winget-cli/releases.

## Building it

The easiest path is opening the folder directly in Visual Studio 2022 (File → Open → Folder) and letting its built-in CMake support configure everything - it'll generate the `.sln`/`.vcxproj` for you targeting v143 automatically. If your Qt install isn't at the default path, set `CMAKE_PREFIX_PATH` to your Qt kit folder (see `CMakePresets.json` - I've got mine pointed at `C:/Qt/6.11.2/msvc2022_64`, change that to match your own install).

If you'd rather drive it from a terminal:

```powershell
cd ApplicationManager
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64"
cmake --build . --config Release
Release\ApplicationManager.exe
```

(`build.bat` / `build.ps1` do roughly the same thing with some extra dependency checks.)

One note if you're running from inside Visual Studio's debugger: the app needs the Qt DLLs to be reachable at runtime. `launch.vs.json` in this repo already adds the Qt `bin` folder to `PATH` for debug sessions - update the path in there if your Qt install lives somewhere else. If you're running the built `.exe` outside of Visual Studio, run `windeployqt.exe` against it once to copy the Qt DLLs next to it.

## Using the app

1. Type a name into the search box and hit **Search** - this hits the Microsoft Store source through winget.
2. Click **Refresh Installed** to see what's currently installed on the machine (also loads automatically on startup).
3. Select a row and use the buttons below the table: **Install**, **Uninstall**, **Launch**, **Update**. Buttons enable/disable depending on whether the selected app is installed.
4. Status colors: yellow while something's in progress, green once installed, red if something went wrong. Hover over the status cell for more detail if there's a message.

## Project layout

```
ApplicationManager/
├── CMakeLists.txt
├── CMakePresets.json
├── launch.vs.json
├── README.md
├── src/
│   ├── main.cpp
│   ├── MainWindow.h/cpp
│   ├── AppListModel.h/cpp
│   ├── WingetBackend.h/cpp
│   ├── AsyncWorker.h/cpp
│   └── models/AppData.h
└── resources/
    └── resources.qrc
```

## Troubleshooting

**"winget is not installed or not in PATH"** - install App Installer from the Microsoft Store, then confirm `winget --version` works from a plain Command Prompt.

**CMake can't find Qt6 / `Qt6Config.cmake` not found** - set `CMAKE_PREFIX_PATH` (in `CMakePresets.json`, or pass `-DCMAKE_PREFIX_PATH=...` on the command line) to your Qt kit folder, e.g. `C:/Qt/6.x.x/msvc2022_64`. Make sure you installed the MSVC 2022 64-bit Qt component, not MinGW.

**App builds but crashes/fails to start with a missing `Qt6*.dll`** - either run through the debugger (which picks up `launch.vs.json`'s PATH addition), or run `windeployqt.exe path\to\ApplicationManager.exe` once to copy the Qt runtime DLLs next to the executable.

**Search/Install/Uninstall silently return nothing** - this used to happen to me when winget was hanging waiting on a source-agreement prompt. If you hit this, double check the winget commands in `WingetBackend.cpp` still include `--accept-source-agreements`.

**Install/uninstall need admin rights for some packages** - if an operation fails for what looks like a permissions reason, try running the app as Administrator.

## A couple of design notes

- Everything long-running goes through `AsyncWorker` on its own thread; the UI thread only ever reads finished results via signals, never blocks on a process.
- `AppListModel` follows Qt's model/view separation - the table view has no idea how the data got there.
- Smart pointers (`std::unique_ptr`) manage the model and worker thread lifetime; the destructor stops and joins the worker thread cleanly.
- Errors are surfaced through message boxes and the status bar rather than failing silently (with the one exception noted above around search-result status not reflecting install state).

## License
Built for a technical assessment - not intended for distribution.

