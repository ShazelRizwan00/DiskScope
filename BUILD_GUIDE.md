# DiskScope Build Guide (Windows)

This guide assumes you have never built a C++ project before. Follow the steps in order. These instructions use Windows, PowerShell, Git for Windows, and the MSYS2 UCRT64 GCC compiler.

## What you will get

DiskScope is a native Windows disk-space analyzer. Building the repository creates:

- `build/DiskScope.exe` — the graphical application.
- `build/diskscope_tests.exe` — the automated test executable.

The project uses C++17, GCC, CMake and Ninja. CMake downloads the pinned Dear ImGui, ImPlot and GLFW dependencies automatically during the first build. You do not need to download those libraries yourself.

## 1. Install Git

Git is used to download the source code.

1. Download Git for Windows from https://git-scm.com/download/win
2. Run the installer. The default options are suitable.
3. Open PowerShell and verify the installation:

```powershell
git --version
```

A version number should appear. If PowerShell says Git is not recognized, close and reopen PowerShell. If it still fails, reinstall Git for Windows.

## 2. Install the C++ compiler

**Important:** The repository's setup script downloads CMake and Ninja, but it does not install GCC. You must install a C++ compiler separately.

1. Download and install MSYS2 from https://www.msys2.org/
2. Open **MSYS2 UCRT64** from the Windows Start menu. Make sure it is UCRT64, not the plain MSYS terminal.
3. Update MSYS2:

```bash
pacman -Syu
```

If it asks you to close the terminal, close it, reopen MSYS2 UCRT64, and repeat the update if instructed.

4. Install GCC and the download/extraction utilities:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc curl unzip
```

Enter `Y` if asked to confirm.

5. Verify GCC in the MSYS2 UCRT64 terminal:

```bash
gcc --version
g++ --version
```

Both commands should print version information.

### Point DiskScope to your compiler

For a default MSYS2 installation, the compiler executables are in:

```text
C:\msys64\ucrt64\bin
```

Open **PowerShell** and set this variable:

```powershell
$env:DISKSCOPE_TOOLCHAIN_BIN = "C:\msys64\ucrt64\bin"
```

Keep using this same PowerShell window. This variable applies only to the current window; if you close it and open another, set the variable again.

If MSYS2 was installed in a different location, use the folder that contains both `gcc.exe` and `g++.exe`. Do not include the executable names in the path.

Check that the path is correct:

```powershell
Test-Path "$env:DISKSCOPE_TOOLCHAIN_BIN\gcc.exe"
Test-Path "$env:DISKSCOPE_TOOLCHAIN_BIN\g++.exe"
```

Both should return `True`.

## 3. Download DiskScope

In PowerShell, go to a folder where you keep projects. For example:

```powershell
cd $HOME\Documents
```

Download the repository:

```powershell
git clone https://github.com/ShazelRizwan00/DiskScope.git
```

Enter the project folder:

```powershell
cd DiskScope
```

You should now be in the folder containing `CMakeLists.txt`, `README.md`, `src`, `scripts` and `tests`.

**Run all remaining commands from this DiskScope folder.** Do not run them from the parent folder or from inside `scripts`.

## 4. Download the build tools

DiskScope stores portable copies of CMake and Ninja inside the repository's `.tools` folder. They do not need to be installed system-wide.

Run in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\setup_toolchain.ps1
```

This script downloads:

- CMake 4.5.1 into `.tools\cmake\`.
- Ninja 1.13.2 into `.tools\ninja\`.

It then prints their versions. The first run needs an internet connection. Later runs reuse the downloaded tools. The generated `.tools` folder is ignored by Git and stays on your computer.

## 5. Build the application

Make sure you are still in the DiskScope folder and that `DISKSCOPE_TOOLCHAIN_BIN` is set in this PowerShell window.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

The build script will:

1. Add your GCC folder to the current process PATH.
2. Check that CMake and Ninja have been downloaded.
3. Configure the project with CMake.
4. Download the pinned GUI libraries (on the first build).
5. Compile the application and tests.

The first build takes longer because dependencies must be downloaded and compiled. Keep the terminal open until the command completes.

When it succeeds, verify that the output files exist:

```powershell
Test-Path build\DiskScope.exe
Test-Path build\diskscope_tests.exe
```

Both should return `True`.

## 6. Run DiskScope

From the DiskScope project folder:

```powershell
.\build\DiskScope.exe
```

The graphical application should open.

To scan a particular folder immediately, pass its path:

```powershell
.\build\DiskScope.exe --root "C:\Users\YourName\Documents"
```

Replace the example with a real folder path on your computer.

## 7. Run the tests

A successful compile does not guarantee that the program behaves correctly. Run the automated tests:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test
```

This rebuilds the project if needed and runs the CTest test suite. A successful run should report:

```text
100% tests passed, 0 tests failed
```

A few tests rely on Windows permissions or directory-junction creation. On a system that blocks those operations, the test harness may report the affected test as skipped.

## 8. Optional: run the GUI smoke test

To build, run the unit tests, and then run the automated off-screen GUI capture:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test -Smoke
```

Generated BMP captures are saved under `docs\captures\`. This is an optional verification step; it is not required to launch DiskScope normally.

## Common commands

Run these from the repository root in PowerShell:

| What you want to do | Command |
|---|---|
| Download CMake and Ninja | `powershell -ExecutionPolicy Bypass -File scripts\setup_toolchain.ps1` |
| Build the project | `powershell -ExecutionPolicy Bypass -File scripts\build.ps1` |
| Build and run tests | `powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test` |
| Build, test and capture GUI | `powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test -Smoke` |
| Clean and rebuild | `powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Clean -Test` |
| Launch the app | `.\build\DiskScope.exe` |
| Show app options | `.\build\DiskScope.exe --help` |

## Troubleshooting

### CMake or Ninja is missing

Run the setup script again from the DiskScope folder:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\setup_toolchain.ps1
```

Then retry the build.

### GCC or G++ is missing

Set the compiler folder again:

```powershell
$env:DISKSCOPE_TOOLCHAIN_BIN = "C:\msys64\ucrt64\bin"
Test-Path "$env:DISKSCOPE_TOOLCHAIN_BIN\gcc.exe"
Test-Path "$env:DISKSCOPE_TOOLCHAIN_BIN\g++.exe"
```

Both checks must return `True`. If not, find the actual folder containing those executables.

### Compiler runtime DLL or cc1plus errors

The compiler's own `bin` folder may not be on PATH, or the compiler installation may be incomplete. Confirm `DISKSCOPE_TOOLCHAIN_BIN` is correct and that you installed the UCRT64 GCC package. Do not mix executables and runtime DLLs from different compiler installations.

### CMake cannot download a dependency

The first build needs internet access to fetch GLFW, Dear ImGui and ImPlot. Check your connection, VPN or firewall, then run the build command again. You do not need to download these dependencies manually.

### You changed your compiler installation

CMake remembers the compiler path in the build folder. Clean the old build and configure again:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Clean
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test
```

### Git is not recognized

Install Git for Windows, close PowerShell, open a new PowerShell window, and verify with `git --version`.

### The app reports a missing DLL

Use the same MSYS2 UCRT64 installation that you configured in `DISKSCOPE_TOOLCHAIN_BIN`. Its `bin` folder contains the matching runtime DLLs. Set the variable and launch the app from that PowerShell window.

## When asking someone for help

Include the exact command you ran, the complete error message, your Windows version, and the output of:

```powershell
$env:DISKSCOPE_TOOLCHAIN_BIN
Test-Path "$env:DISKSCOPE_TOOLCHAIN_BIN\gcc.exe"
Test-Path "$env:DISKSCOPE_TOOLCHAIN_BIN\g++.exe"
git --version
```

A screenshot of only the last error line is usually not enough to diagnose a build failure.

## Final checklist

- [ ] Git for Windows installed.
- [ ] MSYS2 installed and UCRT64 GCC package installed.
- [ ] `DISKSCOPE_TOOLCHAIN_BIN` points to the folder containing `gcc.exe` and `g++.exe`.
- [ ] Repository cloned and PowerShell opened in the DiskScope folder.
- [ ] Toolchain setup script completed.
- [ ] Build script completed successfully.
- [ ] `build\DiskScope.exe` launches.
- [ ] Unit tests pass.

**You are finished when DiskScope opens and the unit tests pass.**
