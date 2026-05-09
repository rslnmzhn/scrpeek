tui_cc = meson.get_compiler('c')
tui_c_args = []
tui_link_args = []
if tui_cc.get_id() != 'msvc'
    tui_c_args += ['-Wall', '-Wextra']
endif

tui_src_dir = include_directories('src')
tui_platform_deps = []
if host_machine.system() == 'windows'
    tui_link_args += ['-Wl,--subsystem,console']
    tui_c_args += ['-DPDC_WIDE=1', '-DPDC_FORCE_UTF8=1', '-DPDC_VT=1']
    tui_platform_deps += tui_cc.find_library('shlwapi')
    pdcurses_sp = subproject('pdcurses')
    tui_curses_dep = pdcurses_sp.get_variable('pdcurses_dep')
else
    tui_curses_dep = dependency('ncurses')
    tui_platform_deps += dependency('threads')
endif

executable('scrcpy-tui',
           [
               'src/adb_list.c',
               'src/config.c',
               'src/launcher.c',
               'src/launch_opts.c',
               'src/main.c',
               'src/ui/device_panel.c',
               'src/ui/error_panel.c',
               'src/ui/options_form.c',
           ],
           include_directories: tui_src_dir,
           dependencies: [tui_curses_dep, tui_platform_deps],
           c_args: tui_c_args,
           link_args: tui_link_args,
           install: true)

tui_adb_test = executable('scrcpy-tui-adb-test',
                          [
                              'src/adb_list.c',
                              'src/adb_list_test.c',
                          ],
                          include_directories: tui_src_dir,
                          dependencies: tui_platform_deps,
                          c_args: tui_c_args)

tui_opts_argv_test = executable('scrcpy-tui-test-opts-argv',
                                [
                                    'test/test_opts_argv.c',
                                    'src/launch_opts.c',
                                ],
                                include_directories: tui_src_dir,
                                c_args: tui_c_args)

test('scrcpy-tui-test-opts-argv', tui_opts_argv_test)
