@echo off
setlocal

set "REPO_ROOT=%~dp0..\.."
set "MSYS2_ROOT=%MSYS2_ROOT%"

if "%MSYS2_ROOT%"=="" set "MSYS2_ROOT=C:\msys64"

if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
    echo MSYS2 shell not found at %MSYS2_ROOT%\msys2_shell.cmd
    echo Install MSYS2 from https://www.msys2.org or set MSYS2_ROOT.
    exit /b 1
)

"%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -no-start -where "%REPO_ROOT%" -c "bash tui/scripts/bootstrap-msys2.sh && meson setup build --wipe -Dcompile_app=false -Dcompile_server=false -Dcompile_tui=true && ninja -C build && curl -L -o build/platform-tools-latest-windows.zip https://dl.google.com/android/repository/platform-tools-latest-windows.zip && rm -rf build/platform-tools && unzip -q -o build/platform-tools-latest-windows.zip -d build && cp build/platform-tools/adb.exe build/platform-tools/AdbWinApi.dll build/platform-tools/AdbWinUsbApi.dll build/tui/ && if objdump -p build/tui/scrcpy-tui.exe | grep -i libpdcurses; then echo 'PDCurses linked dynamically - build is not self-contained' >&2; exit 1; fi && if objdump -p build/tui/scrcpy-tui.exe | grep -i adb; then echo 'adb must remain a subprocess, not a linked dependency' >&2; exit 1; fi"
set "EXIT_CODE=%ERRORLEVEL%"

exit /b %EXIT_CODE%
