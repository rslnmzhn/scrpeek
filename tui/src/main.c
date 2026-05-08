#ifndef _WIN32
# define _POSIX_C_SOURCE 200809L
#endif

#include <curses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "adb_list.h"
#include "launch_opts.h"
#include "ui/device_panel.h"
#include "ui/layout.h"
#include "ui/options_form.h"

#define SC_REFRESH_INTERVAL_MS 2000
#define SC_INPUT_TIMEOUT_MS 100

enum sc_tui_screen {
    SC_TUI_SCREEN_DEVICES,
    SC_TUI_SCREEN_OPTIONS,
};

static volatile sig_atomic_t sc_sigwinch;
static volatile sig_atomic_t sc_sigterm;

static void
sc_signal_handler(int sig) {
#ifdef SIGWINCH
    if (sig == SIGWINCH) {
        sc_sigwinch = 1;
        return;
    }
#endif
    if (sig == SIGTERM || sig == SIGINT) {
        sc_sigterm = 1;
    }
}

static long
sc_monotonic_ms(void) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        return 0;
    }

    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void
sc_init_colors(void) {
    if (!has_colors()) {
        return;
    }

    start_color();
    use_default_colors();
    init_pair(PAIR_HEADER, COLOR_BLACK, COLOR_CYAN);
    init_pair(PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
    init_pair(PAIR_NORMAL, -1, -1);
    init_pair(PAIR_STATUS, COLOR_YELLOW, -1);
}

static void
sc_draw_header(int cols) {
    attron(COLOR_PAIR(PAIR_HEADER));
    mvprintw(0, 0, "%-*s", cols, "scrcpy-tui");
    attroff(COLOR_PAIR(PAIR_HEADER));
}

static void
sc_draw_footer(int rows, int cols, enum sc_tui_screen screen,
               const char *status) {
    attron(COLOR_PAIR(PAIR_STATUS));
    const char *keys = screen == SC_TUI_SCREEN_DEVICES
            ? "Up/Down: navigate  Enter: select  q: quit"
            : "Tab/Arrows: move  Space/Enter: edit  Esc: devices  q: quit";
    mvprintw(rows - 1, 0, "%-*s", cols, keys);
    if (status && status[0]) {
        int x = cols - (int) strlen(status) - 1;
        if (x > 0) {
            mvprintw(rows - 1, x, "%s", status);
        }
    }
    attroff(COLOR_PAIR(PAIR_STATUS));
}

static void
sc_draw_too_small(int rows, int cols) {
    erase();
    if (rows > 0 && cols > 0) {
        mvprintw(0, 0, "Terminal too small: need %dx%d", SC_TUI_MIN_COLS,
                 SC_TUI_MIN_ROWS);
    }
    refresh();
}

static bool
sc_refresh_devices(struct sc_device_list *devices, char *status,
                   size_t status_len) {
    struct sc_device_list next;
    int count = sc_device_list_get(&next);
    if (count < 0) {
        snprintf(status, status_len, "adb refresh failed");
        return false;
    }

    sc_device_list_free(devices);
    *devices = next;
    snprintf(status, status_len, "%d device%s", count, count == 1 ? "" : "s");
    return true;
}

static bool
sc_resize_panel(struct sc_device_panel *panel, int rows, int cols) {
    int panel_rows = rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT;
    int panel_y = SC_TUI_HEADER_HEIGHT;
    return sc_device_panel_resize(panel, panel_y, 0, panel_rows, cols);
}

static bool
sc_resize_options_form(struct sc_options_form *form, int rows, int cols) {
    int panel_rows = rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT;
    int panel_y = SC_TUI_HEADER_HEIGHT;
    return sc_options_form_resize(form, panel_y, 0, panel_rows, cols);
}

int
main(void) {
    int ret = 1;
    bool curses_started = false;
    bool panel_started = false;
    bool form_started = false;
    struct sc_device_panel panel;
    struct sc_options_form form;
    struct sc_device_list devices = {0};
    struct sc_launch_opts launch_opts;
    const char *launch_argv[32];
    char status[64] = "starting";

#ifdef SIGWINCH
    signal(SIGWINCH, sc_signal_handler);
#endif
    signal(SIGTERM, sc_signal_handler);
    signal(SIGINT, sc_signal_handler);

    WINDOW *std = initscr();
    if (!std) {
        return 1;
    }
    curses_started = true;

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(SC_INPUT_TIMEOUT_MS);
    sc_init_colors();

    int rows;
    int cols;
    getmaxyx(stdscr, rows, cols);
    if (!sc_device_panel_init(&panel, SC_TUI_HEADER_HEIGHT, 0,
                              rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT,
                              cols)) {
        goto cleanup;
    }
    panel_started = true;
    if (!sc_options_form_init(&form, SC_TUI_HEADER_HEIGHT, 0,
                              rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT,
                              cols)) {
        goto cleanup;
    }
    form_started = true;
    sc_launch_opts_init(&launch_opts, NULL);

    (void) sc_refresh_devices(&devices, status, sizeof(status));
    long next_refresh = sc_monotonic_ms() + SC_REFRESH_INTERVAL_MS;
    bool running = true;
    enum sc_tui_screen screen = SC_TUI_SCREEN_DEVICES;

    while (running && !sc_sigterm) {
        getmaxyx(stdscr, rows, cols);
        bool too_small = rows < SC_TUI_MIN_ROWS || cols < SC_TUI_MIN_COLS;

        if (sc_sigwinch) {
            sc_sigwinch = 0;
            endwin();
            refresh();
            clear();
            getmaxyx(stdscr, rows, cols);
            too_small = rows < SC_TUI_MIN_ROWS || cols < SC_TUI_MIN_COLS;
            if (!too_small && !sc_resize_panel(&panel, rows, cols)) {
                goto cleanup;
            }
            if (!too_small && !sc_resize_options_form(&form, rows, cols)) {
                goto cleanup;
            }
        }

        long now = sc_monotonic_ms();
        if (screen == SC_TUI_SCREEN_DEVICES && now >= next_refresh) {
            (void) sc_refresh_devices(&devices, status, sizeof(status));
            next_refresh = now + SC_REFRESH_INTERVAL_MS;
        }

        int key = getch();
        if (key == 'q' || key == 'Q') {
            running = false;
        } else if (screen == SC_TUI_SCREEN_DEVICES) {
            switch (key) {
                case '\n':
                case '\r':
                case KEY_ENTER: {
                    const struct sc_device *device = sc_device_panel_selected(&panel,
                                                                              &devices);
                    if (device) {
                        sc_launch_opts_init(&launch_opts, device->serial);
                        snprintf(status, sizeof(status), "selected %s", device->serial);
                        screen = SC_TUI_SCREEN_OPTIONS;
                    }
                    break;
                }
                case KEY_UP:
                case KEY_DOWN:
                    sc_device_panel_handle_key(&panel, key, &devices);
                    break;
                case ERR:
                default:
                    break;
            }
        } else {
            enum sc_options_form_action action = sc_options_form_handle_key(&form, key,
                                                                            &launch_opts);
            if (action == SC_OPTIONS_FORM_BACK) {
                screen = SC_TUI_SCREEN_DEVICES;
                snprintf(status, sizeof(status), "%zu device%s", devices.count,
                         devices.count == 1 ? "" : "s");
            } else if (action == SC_OPTIONS_FORM_LAUNCH) {
                int count = sc_launch_opts_to_argv(&launch_opts, launch_argv,
                                                   sizeof(launch_argv) / sizeof(launch_argv[0]));
                if (count < 0) {
                    snprintf(status, sizeof(status), "could not build argv");
                } else {
                    snprintf(status, sizeof(status), "launch argv ready (%d args)", count);
                }
            }
        }

        if (too_small) {
            sc_draw_too_small(rows, cols);
            continue;
        }

        if (!sc_resize_panel(&panel, rows, cols)) {
            goto cleanup;
        }
        if (!sc_resize_options_form(&form, rows, cols)) {
            goto cleanup;
        }

        erase();
        sc_draw_header(cols);
        if (screen == SC_TUI_SCREEN_DEVICES) {
            sc_device_panel_draw(&panel, &devices);
        } else {
            sc_options_form_draw(&form, &launch_opts);
        }
        sc_draw_footer(rows, cols, screen, status);
        wnoutrefresh(stdscr);
        doupdate();
    }

    ret = 0;

cleanup:
    sc_device_list_free(&devices);
    if (panel_started) {
        sc_device_panel_destroy(&panel);
    }
    if (form_started) {
        sc_options_form_destroy(&form);
    }
    if (curses_started) {
        endwin();
    }
    return ret;
}
