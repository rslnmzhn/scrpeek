tui_cc = meson.get_compiler('c')
tui_c_args = []
if tui_cc.get_id() != 'msvc'
    tui_c_args += ['-Wall', '-Wextra']
endif

tui_src_dir = include_directories('src')
tui_ncurses_dep = dependency('ncurses')

tui_src = [
    'src/adb_list.c',
    'src/config.c',
    'src/launcher.c',
    'src/launch_opts.c',
    'src/main.c',
    'src/ui/device_panel.c',
    'src/ui/log_panel.c',
    'src/ui/options_form.c',
    'src/ui/status_bar.c',
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

tui_opts_argv_test = executable('scrcpy-tui-test-opts-argv',
                                [
                                    'test/test_opts_argv.c',
                                    'src/launch_opts.c',
                                ],
                                include_directories: tui_src_dir,
                                c_args: tui_c_args)

test('scrcpy-tui-test-opts-argv', tui_opts_argv_test)

tui_config_profiles_test = executable('scrcpy-tui-test-config-profiles',
                                      [
                                          'test/test_config_profiles.c',
                                          'src/config.c',
                                          'src/launch_opts.c',
                                      ],
                                      include_directories: tui_src_dir,
                                      c_args: tui_c_args)

test('scrcpy-tui-test-config-profiles', tui_config_profiles_test)
