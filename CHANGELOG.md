# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-05-10

### Added

- Added the standalone `scrpeek` terminal UI for selecting Android devices and launching scrcpy.
- Added an interactive device list backed by `adb devices -l`.
- Added an interactive options form covering video, audio, control, device, window, and misc scrcpy flags.
- Added named launch profiles with save, load, and delete support.
- Added profile validation for names containing only letters, digits, `_`, and `-`.
- Added platform-specific config storage under XDG config paths on Unix-like systems and `%APPDATA%` on Windows.
- Added a launch error screen with a back action when scrcpy cannot be started.
- Added silent ADB process execution on Windows to avoid visible command windows during device refresh.
- Added bundled Windows ADB support in build and release packaging.
- Added bundled Windows scrcpy support in build and release packaging, including `scrcpy.exe`, `scrcpy-server`, and required DLLs.
- Added Meson/Ninja build support for the TUI.
- Added a static PDCursesMod VT backend for Windows Terminal/PowerShell rendering.
- Added ncurses support for Linux/macOS builds.
- Added POSIX version increment script `tui/scripts/next-version.sh`.
- Added tests for launch argv generation and version rollover behavior.
- Added PR CI for Linux and Windows TUI builds/tests.
- Added release CI for automatic versioning, artifact packaging, tag creation, and GitHub Releases.

### Changed

- Renamed the shipped TUI executable from `scrcpy-tui` to `scrpeek`.
- Renamed release artifacts to `scrpeek_X.Y.Z_linux.tar.gz` and `scrpeek_X.Y.Z_windows.zip`.
- Switched Windows rendering from PDCurses wincon to PDCursesMod VT to render inline in modern terminals.
- Switched the TUI event loop to blocking, dirty-state redraws instead of timer-driven polling.
- Moved periodic ADB refresh work to a background thread without curses drawing off the UI thread.
- Centralized screen updates through `wnoutrefresh()` and `doupdate()` to reduce flicker.
- Updated Windows launch behavior to start scrcpy with `CreateProcessA()` and exit the TUI immediately.
- Updated Unix launch behavior to replace the TUI process with scrcpy via `execv()`.
- Updated Windows build flow to use native MSYS2 MinGW64.
- Updated Windows build checks to verify the console subsystem and static PDCurses linkage.

### Fixed

- Fixed the initial blank screen after switching to event-driven rendering.
- Fixed panel rendering order so `stdscr` no longer overwrites child windows before `doupdate()`.
- Fixed mouse hit-testing in the options form, which previously selected items several rows below the cursor.
- Fixed mouse wheel navigation so scrolling acts like Up/Down in device and options screens.
- Fixed middle mouse click handling so it acts like Enter in device and options screens.
- Fixed options form scrolling so the final `Launch` button is visible when focused.
- Fixed Windows console encoding and VT setup before curses initialization.
- Fixed Windows ADB refresh so it does not spawn visible console windows.
- Fixed generated Meson subproject cache and local IDE files being accidentally staged.

### Removed

- Removed WSL-specific build scripts and documentation.
- Removed the old non-standard GitHub Actions workflows.
- Removed reliance on a runtime `libpdcurses.dll` for the Windows TUI.

scrcpy upstream: https://github.com/Genymobile/scrcpy
