/*
 * Copyright (C) 2026 rslnmzhn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WIN32
# define _POSIX_C_SOURCE 200809L
#endif

#include "adb_list.h"
#include "ui/device_panel.h"
#include "ui/layout.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
# include <windows.h>
#endif

#define SC_REFRESH_INTERVAL_MS 2000
#define SC_INPUT_TIMEOUT_MS 100

enum sc_screen {
    SC_SCREEN_DEVICES,
    SC_SCREEN_OPTIONS_STUB,
};

static volatile sig_atomic_t sc_resize_requested;
static volatile sig_atomic_t sc_exit_requested;

static void
sc_signal_handler(int sig) {
#ifdef SIGWINCH
    if (sig == SIGWINCH) {
        sc_resize_requested = 1;
        return;
    }
#endif
    if (sig == SIGTERM || sig == SIGINT) {
        sc_exit_requested = 1;
    }
}

static long
sc_monotonic_ms(void) {
#ifdef _WIN32
    return (long) GetTickCount64();
#else
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        return 0;
    }

    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
#endif
}

static void
sc_init_colors(void) {
    if (!has_colors()) {
        return;
    }

    start_color();
#ifndef _WIN32
    use_default_colors();
#endif
    init_pair(PAIR_HEADER, COLOR_BLACK, COLOR_CYAN);
    init_pair(PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
    init_pair(PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(PAIR_STATUS, COLOR_YELLOW, COLOR_BLACK);
    init_pair(PAIR_BUTTON, COLOR_BLACK, COLOR_GREEN);
}

static void
sc_draw_header(WINDOW *win, int cols) {
    wattron(win, COLOR_PAIR(PAIR_HEADER));
    mvwprintw(win, 0, 0, "%-*s", cols, "scrcpy-tui");
    wattroff(win, COLOR_PAIR(PAIR_HEADER));
}

static void
sc_draw_footer(WINDOW *win, int rows, int cols, const char *status) {
    wattron(win, COLOR_PAIR(PAIR_STATUS));
    mvwprintw(win, rows - 1, 0, "%-*s", cols,
              "Up/Down: navigate  Mouse: select  Enter/Double-click: options  q: quit");
    if (status[0]) {
        int x = cols - (int) strlen(status) - 1;
        if (x > 0) {
            mvwprintw(win, rows - 1, x, "%s", status);
        }
    }
    wattroff(win, COLOR_PAIR(PAIR_STATUS));
}

static void
sc_draw_too_small(WINDOW *win, int rows, int cols) {
    werase(win);
    if (rows > 0 && cols > 0) {
        mvwprintw(win, 0, 0, "Terminal too small: need %dx%d", SC_TUI_MIN_COLS,
                  SC_TUI_MIN_ROWS);
    }
    wrefresh(win);
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
sc_resize_device_panel(struct sc_device_panel *panel, int rows, int cols,
                       const struct sc_device_list *devices) {
    int panel_rows = rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT;
    return sc_device_panel_resize(panel, SC_TUI_HEADER_HEIGHT, 0, panel_rows,
                                  cols, devices);
}

static void
sc_draw_options_stub(WINDOW *win, const struct sc_device *device, int rows,
                     int cols) {
    (void) cols;
    mvwprintw(win, rows / 2 - 1, 2, "Options form stub");
    if (device) {
        mvwprintw(win, rows / 2, 2, "Selected: %s  %s  %s", device->serial,
                  device->model, device->state);
    }
    mvwprintw(win, rows / 2 + 2, 2, "Press Esc to return, q to quit");
}

int
main(void) {
    int ret = 1;
    bool curses_started = false;
    bool panel_started = false;
    struct sc_device_panel panel;
    struct sc_device_list devices = {0};
    enum sc_screen screen = SC_SCREEN_DEVICES;
    char status[80] = "starting";
    int rows = 0;
    int cols = 0;

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
    mousemask(SC_TUI_MOUSE_MASK, NULL);

    sc_init_colors();

    getmaxyx(stdscr, rows, cols);
    if (!sc_device_panel_init(&panel, SC_TUI_HEADER_HEIGHT, 0,
                              rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT,
                              cols)) {
        goto cleanup;
    }
    panel_started = true;

    (void) sc_refresh_devices(&devices, status, sizeof(status));
    long next_refresh = sc_monotonic_ms() + SC_REFRESH_INTERVAL_MS;
    bool running = true;

    while (running && !sc_exit_requested) {
        getmaxyx(stdscr, rows, cols);
        bool too_small = rows < SC_TUI_MIN_ROWS || cols < SC_TUI_MIN_COLS;

        if (sc_resize_requested) {
            sc_resize_requested = 0;
            endwin();
            refresh();
            clear();
            getmaxyx(stdscr, rows, cols);
            too_small = rows < SC_TUI_MIN_ROWS || cols < SC_TUI_MIN_COLS;
            if (!too_small && !sc_resize_device_panel(&panel, rows, cols, &devices)) {
                goto cleanup;
            }
            redrawwin(stdscr);
            wrefresh(stdscr);
        }

        long now = sc_monotonic_ms();
        if (screen == SC_SCREEN_DEVICES && now >= next_refresh) {
            (void) sc_refresh_devices(&devices, status, sizeof(status));
            next_refresh = now + SC_REFRESH_INTERVAL_MS;
        }

        int key = getch();
        if (key == 'q' || key == 'Q') {
            running = false;
        } else if (screen == SC_SCREEN_DEVICES) {
            enum sc_device_panel_action action = SC_DEVICE_PANEL_NONE;
            if (key == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    action = sc_device_panel_handle_mouse(&panel, &event, &devices);
                }
            } else if (key != ERR) {
                action = sc_device_panel_handle_key(&panel, key, &devices);
            }

            if (action == SC_DEVICE_PANEL_ACTIVATE && sc_device_panel_selected(&panel, &devices)) {
                screen = SC_SCREEN_OPTIONS_STUB;
                snprintf(status, sizeof(status), "options stub");
            }
        } else if (key == 27) {
            screen = SC_SCREEN_DEVICES;
            snprintf(status, sizeof(status), "%d device%s", devices.count,
                     devices.count == 1 ? "" : "s");
        }

        if (too_small) {
            sc_draw_too_small(stdscr, rows, cols);
            continue;
        }

        if (!sc_resize_device_panel(&panel, rows, cols, &devices)) {
            goto cleanup;
        }

        werase(stdscr);
        sc_draw_header(stdscr, cols);
        if (screen == SC_SCREEN_DEVICES) {
            sc_device_panel_draw(&panel, &devices);
        } else {
            sc_draw_options_stub(stdscr, sc_device_panel_selected(&panel, &devices), rows,
                                 cols);
        }
        sc_draw_footer(stdscr, rows, cols, status);
        wnoutrefresh(stdscr);
        doupdate();
    }

    ret = 0;

cleanup:
    sc_device_list_free(&devices);
    if (panel_started) {
        sc_device_panel_destroy(&panel);
    }
    if (curses_started) {
        endwin();
    }
    return ret;
}
