tui_cc = meson.get_compiler('c')
tui_c_args = []
if tui_cc.get_id() != 'msvc'
    tui_c_args += ['-Wall', '-Wextra']
endif

tui_src_dir = include_directories('src')
tui_ncurses_dep = dependency('ncurses')

tui_src = [
    'src/adb_list.c',
    'src/main.c',
    'src/ui/device_panel.c',
]

executable('scrcpy-tui', tui_src,
           include_directories: tui_src_dir,
           dependencies: tui_ncurses_dep,
           c_args: tui_c_args,
           install: true)

tui_adb_test = executable('scrcpy-tui-adb-test',
                          [
                              'src/adb_list.c',
                              'src/adb_list_test.c',
                          ],
                          include_directories: tui_src_dir,
                          c_args: tui_c_args)
