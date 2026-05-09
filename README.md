# scrcpy-tui

Terminal user interface for selecting Android devices and launching scrcpy with saved profiles.

## Based on

Based on [scrcpy](https://github.com/Genymobile/scrcpy), created by Romain Vimont and Genymobile.

## Requirements

- `adb` on `PATH` for Linux/macOS; Windows bundles `adb.exe` automatically
- `scrcpy` on `PATH` for Linux/macOS; Windows bundles `scrcpy.exe` automatically
- `ncurses` on Linux/macOS or PDCurses on Windows

## Build

```sh
sh tui/scripts/bootstrap.sh
meson setup build
ninja -C build
```

### Windows (native, MSYS2)

1. Install MSYS2: https://www.msys2.org
2. Run from MSYS2 MinGW64 shell:

   ```sh
   bash tui/scripts/bootstrap-msys2.sh
   meson setup build --wipe && ninja -C build
   ```

3. `adb.exe`, `scrcpy.exe`, scrcpy DLLs, and `scrcpy-server` are bundled automatically by `build-windows.bat`.

## Usage

```sh
./build/tui/scrcpy-tui
```

Launches the TUI with no arguments, then lets you select a device and options interactively.

## Features

- Device enumeration panel backed by `adb devices -l`
- Interactive options form for launch settings
- Live scrcpy launch log view
- Named launch profiles saved to a local config file

## License

Apache 2.0 - see LICENSE and NOTICE.
