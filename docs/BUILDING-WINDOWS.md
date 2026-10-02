# Building & Running on Windows (Visual Studio Code)

This guide takes you from a fresh Windows machine to a running maze in
roughly 15 minutes. Two compiler routes are supported — **MSVC (recommended)**
and **MinGW-w64**. Follow one of them, not both.

## What you need before starting

| Tool | Why | Where |
|------|-----|-------|
| **Git** | The build downloads GLFW 3.4 from GitHub at configure time (`FetchContent`) — without Git, configuration fails | [git-scm.com](https://git-scm.com/download/win) |
| **CMake ≥ 3.16** | Build system (CMake 4.x works too; the project already handles it) | Bundled with VS Build Tools, MSYS2, or standalone from [cmake.org](https://cmake.org/download/) — tick "Add CMake to system PATH" |
| **A C++17 compiler** | Compiles the code | MSVC *or* MinGW (see below) |
| **VS Code extensions** | C/C++ IntelliSense + debugging, CMake integration | Install inside VS Code (step 2) |
| **GPU/driver with OpenGL 3.3+** | Renders the maze | Any PC from the last ~12 years; update drivers if unsure |

---

## Route A — MSVC via Visual Studio Build Tools (recommended)

MSVC gives you the best debugger experience on Windows (`cppvsdbg`), the
fastest code, and no PATH fiddling.

### 1. Install the compiler

1. Download **Visual Studio 2022 Build Tools** (free) from
   [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/)
   — scroll to *Tools for Visual Studio → Build Tools for Visual Studio 2022*.
2. Run the installer and check the **"Desktop development with C++"** workload.
3. In the right-hand *Installation details* panel, make sure these are ticked
   (they usually are by default):
   - **MSVC v143 build tools** (x64/x86)
   - **Windows 11 / 10 SDK**
4. Install (~6 GB). No Visual Studio IDE needed — only the compilers.

### 2. Set up VS Code

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Open the Extensions panel (`Ctrl+Shift+X`) and install:
   - **C/C++ Extension Pack** (`ms-vscode.cpptools-extension-pack`) — pulls in
     the C/C++ extension, **CMake Tools**, and IntelliCode in one go.
3. `File → Open Folder…` → select the `procedural-maze` folder.
4. The **CMake Tools** extension will ask *“Select a kit”* — choose
   **`Visual Studio Build Tools ... amd64`** (the amd64/x64 variant, not x86).
   Configuration starts automatically (`cmake.configureOnOpen` is preset in
   `.vscode/settings.json`).
   - Don't see the kit? Press `Ctrl+Shift+P` → **CMake: Select a Kit** → scan.
   - The first configure downloads and builds GLFW — watch the output panel;
     it takes a minute or two and only happens once.

### 3. Build and run

- **Build:** `F7` (CMake Tools) or `Ctrl+Shift+B` (the default *Build maze*
  task). Both produce the same thing.
- **Run:** `Shift+F5` (CMake Tools “run without debugging”), or pick
  **Terminal → Run Task… → Run maze (Visual Studio kit)**, or from a terminal:
  ```powershell
  .\build\Release\maze.exe
  ```
- **Debug:** set a breakpoint (e.g. in `Maze::generate`) and press `F5`
  (configuration *Debug maze (MSVC)*). Switch Debug/Release in the blue
  status-bar button at the bottom if you want an unoptimized run.

### 4. Alternative: pure command line (no VS Code)

Open **“x64 Native Tools Command Prompt for VS 2022”** from the Start menu:

```bat
cd path\to\procedural-maze
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
build\Release\maze.exe
```

---

## Route B — MinGW-w64 via MSYS2

Lighter install (~1 GB) and GCC-identical warnings to the Linux build.

### 1. Install the toolchain

1. Install [MSYS2](https://www.msys2.org/) (default location `C:\msys64`).
2. Open the **UCRT64** shell from the Start menu and run:
   ```bash
   pacman -Syu                                  # may ask to restart the shell
   pacman -S --needed mingw-w64-ucrt-x86_64-toolchain \
                      mingw-w64-ucrt-x86_64-cmake \
                      mingw-w64-ucrt-x86_64-ninja git
   ```
3. Add `C:\msys64\ucrt64\bin` to your **PATH**: Start → “Edit the system
   environment variables” → *Environment Variables…* → edit `Path` → add the
   folder → OK. **Restart VS Code afterwards** so it picks up the new PATH.

### 2. Build in VS Code

1. Open the `procedural-maze` folder.
2. `Ctrl+Shift+P` → **CMake: Select a Kit** → pick
   **`GCC for mingw-w64 (ucrt64)`** (CMake Tools detects it via PATH).
3. Build with `F7` / `Ctrl+Shift+B`. Ninja is a single-config generator, so
   the executable lands directly in `build\maze.exe`.
4. Run via **Terminal → Run Task… → Run maze (MinGW / Ninja kit)**, or:
   ```powershell
   .\build\maze.exe
   ```
5. Debugging: press `F5` with the *Debug maze (MinGW / GDB)* configuration.
   If VS Code can't find the debugger, set `"miDebuggerPath"` in
   `.vscode/launch.json` to `C:\\msys64\\ucrt64\\bin\\gdb.exe`.

---

## Verify it worked

A console window opens, then the game window:

- Dark torch-lit maze, first-person view — **WASD** move, **mouse** look.
- `M` minimap, `R` regenerate, `[` / `]` resize, `Esc` quit.
- Try a deterministic seed for reproducible layouts:
  ```powershell
  .\build\Release\maze.exe --size 16 --seed 42 --minimap
  ```
- Headless smoke test (no window needed, writes a BMP):
  ```powershell
  .\build\Release\maze.exe --shot test.bmp --frames 30 --seed 7
  ```

The build copies the `shaders/` folder next to the executable, so
`maze.exe` is location-independent within the build tree — just don't move
the exe away from its `shaders/` folder.

---

## Troubleshooting

| Symptom | Cause & fix |
|---------|-------------|
| `'cmake' is not recognized` | CMake not on PATH. Restart VS Code after installing; or use the VS Developer Command Prompt (Route A) / UCRT64 shell (Route B). |
| `Failed to get the version of MSVC` or no kits found | VS Build Tools without the *Desktop development with C++* workload. Re-run the installer and tick it. |
| Configure step: `Failed to clone glfw` / network error | Git missing, or a corporate proxy. Install Git; behind a proxy set `$env:HTTPS_PROXY="http://proxy:port"` before configuring. |
| Configure step: `Compatibility with CMake < 3.5 has been removed` | CMake 4.x with an old GLFW — already handled by the `CMAKE_POLICY_VERSION_MINIMUM` guard in `CMakeLists.txt`. If you edited the GLFW tag, restore `3.4`. |
| `LNK1104: cannot open file 'opengl32.lib'` | Wrong kit (x86 toolchain) or missing Windows SDK. Pick the **amd64** kit; reinstall Build Tools with the Windows SDK ticked. |
| `'cl' is not recognized` in a plain PowerShell | Normal — `cl.exe` only exists in the VS environment. Use the kit selection in VS Code or the *x64 Native Tools Command Prompt*. |
| `error C2063` / complaints about `min` or `max` | Shouldn't happen — `NOMINMAX` is defined in CMakeLists for Windows. If you added your own build scripts, define `NOMINMAX` and `WIN32_LEAN_AND_MEAN`. |
| Window opens then closes instantly / shader error printed | The exe was moved away from its `shaders/` folder. Rebuild, or run from the build output directory. |
| Very low FPS | Software renderer active (RDP session or driver issue). Update your GPU driver; run on the physical machine, not over Remote Desktop. |
| SmartScreen blocks the exe | Unsigned locally-built binary — click *More info → Run anyway*. |
| Stale build misbehaves | Delete the `build/` folder and re-run Configure (or run the *Clean rebuild* task). |

---

## Project specifics worth knowing

- **GLFW is fetched, not installed** — nothing to download manually besides Git.
  It's pinned to tag `3.4` in `CMakeLists.txt`.
- **Shader lookup** — the executable checks (1) a `shaders/` dir next to
  itself, then (2) the baked-in source-tree path, so both build-tree and
  editor launches work.
- **Debug vs Release** — the maze generation and physics are fast enough that
  Debug is playable; use it when setting breakpoints, otherwise stay in
  Release.
- **MSVC warnings** — the project builds clean at `/W4 /permissive-`; if you
  see warnings, they're from your own modifications.
