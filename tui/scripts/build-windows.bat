@echo off
setlocal

set "REPO_ROOT=%~dp0..\.."
set "MSYS2_ROOT=%MSYS2_ROOT%"
set "SCRCPY_VERSION=3.3.4"

if "%MSYS2_ROOT%"=="" set "MSYS2_ROOT=C:\msys64"

if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
    echo MSYS2 shell not found at %MSYS2_ROOT%\msys2_shell.cmd
    echo Install MSYS2 from https://www.msys2.org or set MSYS2_ROOT.
    exit /b 1
)

"%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -no-start -where "%REPO_ROOT%" -c "bash tui/scripts/bootstrap-msys2.sh && if [ -d build/meson-info ]; then meson setup build --reconfigure -Dcompile_app=false -Dcompile_server=false -Dcompile_tui=true; else meson setup build -Dcompile_app=false -Dcompile_server=false -Dcompile_tui=true; fi && ninja -C build && { objdump -p build/tui/scrpeek.exe | grep -i 'Subsystem.*00000003' || { echo 'scrpeek.exe must be a console subsystem binary' >&2; exit 1; }; } && curl -L -o build/platform-tools-latest-windows.zip https://dl.google.com/android/repository/platform-tools-latest-windows.zip && rm -rf build/platform-tools && unzip -q -o build/platform-tools-latest-windows.zip -d build && powershell.exe -NoProfile -Command Stop-Process -Name adb -Force -ErrorAction SilentlyContinue && cp build/platform-tools/adb.exe build/platform-tools/AdbWinApi.dll build/platform-tools/AdbWinUsbApi.dll build/tui/ && curl -L -o build/scrcpy-win64.zip https://github.com/Genymobile/scrcpy/releases/download/v%SCRCPY_VERSION%/scrcpy-win64-v%SCRCPY_VERSION%.zip && rm -rf build/scrcpy-win64 && unzip -q -o build/scrcpy-win64.zip -d build/scrcpy-win64 && cp build/scrcpy-win64/scrcpy-win64-v%SCRCPY_VERSION%/scrcpy.exe build/scrcpy-win64/scrcpy-win64-v%SCRCPY_VERSION%/*.dll build/scrcpy-win64/scrcpy-win64-v%SCRCPY_VERSION%/scrcpy-server build/tui/ && objdump -p build/tui/scrpeek.exe | grep -i 'DLL Name' > build/tui/scrpeek.imports && if grep -i libpdcurses build/tui/scrpeek.imports; then echo 'PDCurses linked dynamically - build is not self-contained' >&2; exit 1; fi && if grep -i adb build/tui/scrpeek.imports; then echo 'adb must remain a subprocess, not a linked dependency' >&2; exit 1; fi"
set "EXIT_CODE=%ERRORLEVEL%"

exit /b %EXIT_CODE%
