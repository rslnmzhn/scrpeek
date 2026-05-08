tui_cc = meson.get_compiler('c')
tui_c_args = []
if tui_cc.get_id() != 'msvc'
    tui_c_args += ['-Wall', '-Wextra']
endif

tui_src_dir = include_directories('src')
tui_platform_deps = []
if host_machine.system() == 'windows'
    tui_platform_deps += tui_cc.find_library('shlwapi')
    tui_curses_dep = tui_cc.find_library('pdcurses', static: true, required: false)
    if not tui_curses_dep.found()
        tui_curses_dep = tui_cc.find_library('pdcurses', required: false)
        if tui_curses_dep.found()
            warning('PDCurses linked dynamically - bundle libpdcurses.dll')
        else
            tui_curses_dep = dependency('pdcurses', fallback: ['pdcurses', 'pdcurses_dep'])
        endif
    endif
    tui_platform_deps += tui_cc.find_library('winmm')
else
    tui_curses_dep = dependency('ncurses')
endif

executable('scrcpy-tui',
           [
               'src/adb_list.c',
               'src/main.c',
               'src/ui/device_panel.c',
           ],
           include_directories: tui_src_dir,
           dependencies: [tui_curses_dep, tui_platform_deps],
           c_args: tui_c_args,
           install: true)

tui_adb_test = executable('scrcpy-tui-adb-test',
                          [
                              'src/adb_list.c',
                              'src/adb_list_test.c',
                          ],
                          include_directories: tui_src_dir,
                          dependencies: tui_platform_deps,
                          c_args: tui_c_args)

test('scrcpy-tui-adb-test', tui_adb_test)
