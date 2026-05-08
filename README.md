# scrcpy-tui

Terminal user interface for selecting Android devices and launching scrcpy with saved profiles.

## Based on

Based on [scrcpy](https://github.com/Genymobile/scrcpy), created by Romain Vimont and Genymobile.

## Requirements

- `adb` on `PATH`
- `scrcpy` installed separately
- `ncurses`

## Build

```sh
sh tui/scripts/bootstrap.sh
meson setup build
ninja -C build
```

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
