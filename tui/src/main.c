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
#include "config.h"
#include "launcher.h"
#include "launch_opts.h"
#include "ui/device_panel.h"
#include "ui/error_panel.h"
#include "ui/layout.h"
#include "ui/options_form.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
# include <process.h>
# include <windows.h>
#else
# include <pthread.h>
# include <unistd.h>
#endif

#define SC_REFRESH_INTERVAL_MS 2000
#define SC_EVENT_REFRESH (KEY_MAX + 101)
#define SC_CONNECT_INPUT_MAX 63

enum sc_screen {
    SC_SCREEN_DEVICES,
    SC_SCREEN_OPTIONS,
    SC_SCREEN_ERROR,
};

static volatile sig_atomic_t sc_resize_requested;
static volatile sig_atomic_t sc_exit_requested;
static volatile sig_atomic_t sc_refresh_requested;
#ifndef _WIN32
static pthread_t sc_main_thread;
#endif

struct sc_refresh_state {
#ifdef _WIN32
    CRITICAL_SECTION lock;
    HANDLE thread;
#else
    pthread_mutex_t lock;
    pthread_t thread;
#endif
    bool lock_started;
    bool thread_started;
    bool stop;
    bool has_update;
    struct sc_device_list last;
    struct sc_device_list pending;
    char pending_status[80];
};

static void
sc_signal_handler(int sig) {
#ifdef SIGWINCH
    if (sig == SIGWINCH) {
        sc_resize_requested = 1;
        return;
    }
#endif
#ifndef _WIN32
    if (sig == SIGUSR1) {
        sc_refresh_requested = 1;
        return;
    }
#endif
    if (sig == SIGTERM || sig == SIGINT) {
        sc_exit_requested = 1;
    }
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
sc_init_curses_modes(void) {
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(-1);
    mousemask(SC_TUI_MOUSE_MASK, NULL);
    sc_init_colors();
}

static void
sc_draw_header(WINDOW *win, int cols) {
    wattron(win, COLOR_PAIR(PAIR_HEADER));
    mvwprintw(win, 0, 0, "%-*s", cols, "scrpeek");
    wattroff(win, COLOR_PAIR(PAIR_HEADER));
}

static void
sc_draw_footer(WINDOW *win, int rows, int cols, const char *status) {
    wattron(win, COLOR_PAIR(PAIR_STATUS));
    mvwprintw(win, rows - 1, 0, "%-*s", cols,
              "Up/Down: navigate  Enter: options  c: connect  r: refresh  q: quit");
    if (status[0]) {
        int x = cols - (int) strlen(status) - 1;
        if (x > 0) {
            mvwprintw(win, rows - 1, x, "%s", status);
        }
    }
    wattroff(win, COLOR_PAIR(PAIR_STATUS));
}

static bool
sc_connect_input_append(char *text, size_t cap, int key) {
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        size_t len = strlen(text);
        if (!len) {
            return false;
        }
        text[len - 1] = '\0';
        return true;
    }
    if (key < 32 || key > 126) {
        return false;
    }

    const char *allowed = "0123456789abcdefABCDEF.:[]-";
    if (!strchr(allowed, key)) {
        return false;
    }

    size_t len = strlen(text);
    if (len + 1 >= cap) {
        return false;
    }
    text[len] = (char) key;
    text[len + 1] = '\0';
    return true;
}

static void
sc_draw_too_small(WINDOW *win, int rows, int cols) {
    werase(win);
    if (rows > 0 && cols > 0) {
        mvwprintw(win, 0, 0, "Terminal too small: need %dx%d", SC_TUI_MIN_COLS,
                  SC_TUI_MIN_ROWS);
    }
    wnoutrefresh(win);
}

static bool
sc_device_list_same(const struct sc_device_list *a, const struct sc_device_list *b) {
    if (a->count != b->count) {
        return false;
    }

    for (int i = 0; i < a->count; ++i) {
        const struct sc_device *da = &a->devices[i];
        const struct sc_device *db = &b->devices[i];
        if (strcmp(da->serial, db->serial)
                || strcmp(da->model, db->model)
                || strcmp(da->state, db->state)) {
            return false;
        }
    }
    return true;
}

static bool
sc_device_list_clone(struct sc_device_list *dst, const struct sc_device_list *src) {
    dst->devices = NULL;
    dst->count = 0;
    if (src->count <= 0) {
        return true;
    }

    dst->devices = malloc((size_t) src->count * sizeof(*dst->devices));
    if (!dst->devices) {
        return false;
    }
    memcpy(dst->devices, src->devices, (size_t) src->count * sizeof(*dst->devices));
    dst->count = src->count;
    return true;
}

static void
sc_refresh_lock(struct sc_refresh_state *state) {
#ifdef _WIN32
    EnterCriticalSection(&state->lock);
#else
    pthread_mutex_lock(&state->lock);
#endif
}

static void
sc_refresh_unlock(struct sc_refresh_state *state) {
#ifdef _WIN32
    LeaveCriticalSection(&state->lock);
#else
    pthread_mutex_unlock(&state->lock);
#endif
}

static void
sc_sleep_refresh_interval(void) {
#ifdef _WIN32
    Sleep(SC_REFRESH_INTERVAL_MS);
#else
    struct timespec ts;
    ts.tv_sec = SC_REFRESH_INTERVAL_MS / 1000;
    ts.tv_nsec = (SC_REFRESH_INTERVAL_MS % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

static void
sc_refresh_publish(struct sc_refresh_state *state, struct sc_device_list *next,
                   const char *status) {
    sc_refresh_lock(state);
    bool stopped = state->stop;
    if (!stopped) {
        sc_device_list_free(&state->pending);
        if (sc_device_list_clone(&state->pending, next)) {
            snprintf(state->pending_status, sizeof(state->pending_status), "%s", status);
            state->has_update = true;
        }
    }
    sc_refresh_unlock(state);

    if (!stopped) {
#ifdef _WIN32
        /* PDCurses permits cross-thread ungetch(); no drawing is done off-thread. */
        ungetch(SC_EVENT_REFRESH);
#else
        sc_refresh_requested = 1;
        pthread_kill(sc_main_thread, SIGUSR1);
#endif
    }
}

static void
sc_refresh_worker_run(struct sc_refresh_state *state) {
    for (;;) {
        sc_sleep_refresh_interval();

        sc_refresh_lock(state);
        bool stopped = state->stop;
        sc_refresh_unlock(state);
        if (stopped) {
            break;
        }

        struct sc_device_list next;
        char next_status[80];
        int count = sc_device_list_get(&next);
        if (count < 0) {
            snprintf(next_status, sizeof(next_status), "adb refresh failed");
            next.devices = NULL;
            next.count = 0;
        } else {
            snprintf(next_status, sizeof(next_status), "%d device%s", count,
                     count == 1 ? "" : "s");
        }

        sc_refresh_lock(state);
        bool changed = strcmp(state->pending_status, next_status) != 0
                || !sc_device_list_same(&state->last, &next);
        if (changed) {
            sc_device_list_free(&state->last);
            state->last = next;
            next.devices = NULL;
            next.count = 0;
        }
        sc_refresh_unlock(state);

        if (changed) {
            sc_refresh_publish(state, &state->last, next_status);
        }
        sc_device_list_free(&next);
    }
}

#ifdef _WIN32
static unsigned __stdcall
sc_refresh_thread_main(void *data) {
    sc_refresh_worker_run(data);
    return 0;
}
#else
static void *
sc_refresh_thread_main(void *data) {
    sc_refresh_worker_run(data);
    return NULL;
}
#endif

static void
sc_refresh_state_destroy(struct sc_refresh_state *state);

static bool
sc_refresh_state_init(struct sc_refresh_state *state,
                      const struct sc_device_list *initial, const char *status) {
    memset(state, 0, sizeof(*state));
#ifdef _WIN32
    InitializeCriticalSection(&state->lock);
    state->lock_started = true;
#else
    if (pthread_mutex_init(&state->lock, NULL)) {
        return false;
    }
    state->lock_started = true;
#endif
    snprintf(state->pending_status, sizeof(state->pending_status), "%s", status);
    if (!sc_device_list_clone(&state->last, initial)) {
        sc_refresh_state_destroy(state);
        return false;
    }

#ifdef _WIN32
    state->thread = (HANDLE) _beginthreadex(NULL, 0, sc_refresh_thread_main, state, 0, NULL);
    state->thread_started = state->thread != NULL;
#else
    state->thread_started = pthread_create(&state->thread, NULL, sc_refresh_thread_main, state) == 0;
#endif
    if (!state->thread_started) {
        sc_refresh_state_destroy(state);
        return false;
    }
    return true;
}

static void
sc_refresh_state_destroy(struct sc_refresh_state *state) {
    if (state->lock_started) {
        sc_refresh_lock(state);
        state->stop = true;
        sc_refresh_unlock(state);
    }
    if (state->thread_started) {
#ifdef _WIN32
        ungetch(SC_EVENT_REFRESH);
        WaitForSingleObject(state->thread, INFINITE);
        CloseHandle(state->thread);
#else
        pthread_kill(sc_main_thread, SIGUSR1);
        pthread_join(state->thread, NULL);
#endif
    }
    sc_device_list_free(&state->last);
    sc_device_list_free(&state->pending);
    if (state->lock_started) {
#ifdef _WIN32
        DeleteCriticalSection(&state->lock);
#else
        pthread_mutex_destroy(&state->lock);
#endif
    }
}

static bool
sc_refresh_take_update(struct sc_refresh_state *state, struct sc_device_list *devices,
                       char *status, size_t status_len) {
    bool has_update;
    sc_refresh_lock(state);
    has_update = state->has_update;
    if (has_update) {
        sc_device_list_free(devices);
        *devices = state->pending;
        state->pending.devices = NULL;
        state->pending.count = 0;
        snprintf(status, status_len, "%s", state->pending_status);
        state->has_update = false;
    }
    sc_refresh_unlock(state);
    return has_update;
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

static bool
sc_resize_options_form(struct sc_options_form *form, int rows, int cols) {
    int form_rows = rows - SC_TUI_HEADER_HEIGHT - SC_TUI_FOOTER_HEIGHT;
    return sc_options_form_resize(form, SC_TUI_HEADER_HEIGHT, 0, form_rows, cols);
}

static bool
sc_restart_curses(void) {
    WINDOW *std = initscr();
    if (!std) {
        return false;
    }
    sc_init_curses_modes();
    return true;
}

static bool
sc_layout_resize(struct sc_device_panel *panel, struct sc_options_form *form,
                 struct sc_error_panel *error_panel, int rows, int cols,
                 const struct sc_device_list *devices) {
    bool too_small = rows < SC_TUI_MIN_ROWS || cols < SC_TUI_MIN_COLS;
    if (too_small) {
        return true;
    }
    return sc_resize_device_panel(panel, rows, cols, devices)
            && sc_resize_options_form(form, rows, cols)
            && sc_error_panel_resize(error_panel, rows, cols);
}

static void
sc_render(enum sc_screen screen, struct sc_device_panel *panel,
          struct sc_options_form *form, struct sc_error_panel *error_panel,
          const struct sc_device_list *devices, const struct sc_launch_opts *launch_opts,
          const char *status, const char *launch_error, int rows, int cols) {
    bool too_small = rows < SC_TUI_MIN_ROWS || cols < SC_TUI_MIN_COLS;
    if (too_small) {
        sc_draw_too_small(stdscr, rows, cols);
        doupdate();
        return;
    }

    werase(stdscr);
    sc_draw_header(stdscr, cols);
    sc_draw_footer(stdscr, rows, cols, status);
    wnoutrefresh(stdscr);

    if (screen == SC_SCREEN_DEVICES) {
        sc_device_panel_draw(panel, devices);
    } else if (screen == SC_SCREEN_OPTIONS) {
        sc_options_form_draw(form, launch_opts);
    } else {
        sc_error_panel_draw(error_panel, "Launch failed", launch_error);
    }
    doupdate();
}

#ifdef _WIN32
static bool
sc_enable_virtual_terminal(void) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == INVALID_HANDLE_VALUE || out == NULL) {
        return false;
    }

    DWORD mode;
    if (!GetConsoleMode(out, &mode)) {
        return false;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    return SetConsoleMode(out, mode) != 0;
}
#endif

int
main(void) {
    int ret = 1;
    bool curses_started = false;
    bool panel_started = false;
    bool form_started = false;
    bool error_started = false;
    bool refresh_started = false;
    struct sc_device_panel panel;
    struct sc_options_form form;
    struct sc_error_panel error_panel;
    struct sc_refresh_state refresh_state;
    struct sc_device_list devices = {0};
    struct sc_launch_opts launch_opts;
    const char *launch_argv[96];
    enum sc_screen screen = SC_SCREEN_DEVICES;
    char status[80] = "starting";
    char launch_error[512] = "";
    char connect_input[SC_CONNECT_INPUT_MAX + 1] = "";
    int rows = 0;
    int cols = 0;
    bool connect_mode = false;

#ifdef SIGWINCH
    signal(SIGWINCH, sc_signal_handler);
#endif
#ifndef _WIN32
    sc_main_thread = pthread_self();
    signal(SIGUSR1, sc_signal_handler);
#endif
    signal(SIGTERM, sc_signal_handler);
    signal(SIGINT, sc_signal_handler);

#ifdef _WIN32
    if (!sc_enable_virtual_terminal()) {
        fputs("Windows 10 1511+ or Windows Terminal required\n", stderr);
        return 1;
    }
#endif

    WINDOW *std = initscr();
    if (!std) {
        return 1;
    }
    curses_started = true;

    sc_init_curses_modes();

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
    if (!sc_error_panel_init(&error_panel, rows, cols)) {
        goto cleanup;
    }
    error_started = true;

    (void) sc_refresh_devices(&devices, status, sizeof(status));
    if (!sc_refresh_state_init(&refresh_state, &devices, status)) {
        goto cleanup;
    }
    refresh_started = true;
    bool running = true;
    bool first_refresh_rendered = false;

    /* Draw initial state before blocking on getch(). */
    sc_render(screen, &panel, &form, &error_panel, &devices, &launch_opts,
              status, launch_error, rows, cols);

    while (running && !sc_exit_requested) {
        bool dirty = false;

        if (sc_resize_requested) {
            sc_resize_requested = 0;
            endwin();
            refresh();
            clear();
            getmaxyx(stdscr, rows, cols);
            if (!sc_layout_resize(&panel, &form, &error_panel, rows, cols, &devices)) {
                goto cleanup;
            }
            redrawwin(stdscr);
            dirty = true;
        }

        if (dirty) {
            sc_render(screen, &panel, &form, &error_panel, &devices, &launch_opts,
                      status, launch_error, rows, cols);
            continue;
        }

        int key = getch();
        if (key == SC_EVENT_REFRESH || sc_refresh_requested) {
            sc_refresh_requested = 0;
            if (sc_refresh_take_update(&refresh_state, &devices, status, sizeof(status))) {
                if (screen == SC_SCREEN_DEVICES) {
                    dirty = true;
                }
            }
            if (!first_refresh_rendered) {
                dirty = true;
                first_refresh_rendered = true;
            }
            if (dirty) {
                getmaxyx(stdscr, rows, cols);
                if (!sc_layout_resize(&panel, &form, &error_panel, rows, cols, &devices)) {
                    goto cleanup;
                }
                sc_render(screen, &panel, &form, &error_panel, &devices, &launch_opts,
                          status, launch_error, rows, cols);
            }
            continue;
        }

        if (connect_mode) {
            if (key == 27) {
                connect_mode = false;
                connect_input[0] = '\0';
                snprintf(status, sizeof(status), "%d device%s", devices.count,
                         devices.count == 1 ? "" : "s");
                dirty = true;
            } else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
                bool connected = sc_adb_connect(connect_input);
                (void) sc_refresh_devices(&devices, status, sizeof(status));
                if (connected) {
                    snprintf(status, sizeof(status), "connected %s", connect_input);
                } else {
                    snprintf(status, sizeof(status), "connect failed: %s", connect_input);
                }
                connect_mode = false;
                connect_input[0] = '\0';
                dirty = true;
            } else if (sc_connect_input_append(connect_input, sizeof(connect_input), key)) {
                snprintf(status, sizeof(status), "connect %s", connect_input);
                dirty = true;
            }
        } else if (key == 'q' || key == 'Q') {
            running = false;
        } else if (screen == SC_SCREEN_DEVICES) {
            enum sc_device_panel_action action = SC_DEVICE_PANEL_NONE;
            if (key == 'r' || key == 'R') {
                (void) sc_refresh_devices(&devices, status, sizeof(status));
                dirty = true;
            } else if (key == 'c' || key == 'C') {
                connect_mode = true;
                connect_input[0] = '\0';
                snprintf(status, sizeof(status), "connect ip:port");
                dirty = true;
            } else if (key == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    action = sc_device_panel_handle_mouse(&panel, &event, &devices);
                }
            } else if (key != ERR) {
                action = sc_device_panel_handle_key(&panel, key, &devices);
            }

            if (action == SC_DEVICE_PANEL_ACTIVATE && sc_device_panel_selected(&panel, &devices)) {
                const struct sc_device *device = sc_device_panel_selected(&panel, &devices);
                sc_launch_opts_init(&launch_opts, device->serial);
                sc_options_form_focus_launch(&form);
                screen = SC_SCREEN_OPTIONS;
                snprintf(status, sizeof(status), "options for %s", device->serial);
                dirty = true;
            } else if (action == SC_DEVICE_PANEL_SELECTED) {
                dirty = true;
            }
        } else if (screen == SC_SCREEN_OPTIONS) {
            enum sc_options_form_action action = SC_OPTIONS_FORM_NONE;
            if (key == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    action = sc_options_form_handle_mouse(&form, &event, &launch_opts);
                }
            } else {
                action = sc_options_form_handle_key(&form, key, &launch_opts);
            }

            if (action == SC_OPTIONS_FORM_BACK) {
                screen = SC_SCREEN_DEVICES;
                snprintf(status, sizeof(status), "%d device%s", devices.count,
                         devices.count == 1 ? "" : "s");
                dirty = true;
            } else if (action == SC_OPTIONS_FORM_LAUNCH) {
                int argc = sc_launch_opts_to_argv(&launch_opts, launch_argv,
                                                  sizeof(launch_argv) / sizeof(launch_argv[0]));
                if (argc < 0) {
                    snprintf(status, sizeof(status), "argv too small");
                    dirty = true;
                } else {
                    if (sc_launch(launch_argv, launch_error, sizeof(launch_error)) < 0) {
                        if (!sc_restart_curses()) {
                            goto cleanup;
                        }
                        getmaxyx(stdscr, rows, cols);
                        if (!sc_resize_device_panel(&panel, rows, cols, &devices)
                                || !sc_resize_options_form(&form, rows, cols)
                                || !sc_error_panel_resize(&error_panel, rows, cols)) {
                            goto cleanup;
                        }
                        screen = SC_SCREEN_ERROR;
                        snprintf(status, sizeof(status), "launch failed");
                        dirty = true;
                    }
                }
            } else if (action == SC_OPTIONS_FORM_SAVE_PROFILE) {
                const char *name = sc_options_form_profile_name(&form);
                if (!sc_config_profile_name_valid(name)) {
                    sc_options_form_set_message(&form, "Invalid profile name: use a-z A-Z 0-9 _ - max 32");
                } else if (sc_config_save(name, &launch_opts)) {
                    sc_options_form_set_message(&form, "Profile saved");
                    sc_options_form_profile_close(&form);
                    sc_options_form_profiles_reload(&form);
                } else {
                    sc_options_form_set_message(&form, "Could not save profile");
                }
                dirty = true;
            } else if (action == SC_OPTIONS_FORM_LOAD_PROFILE) {
                const char *name = sc_options_form_profile_name(&form);
                char serial[sizeof(launch_opts.serial)];
                snprintf(serial, sizeof(serial), "%s", launch_opts.serial);
                struct sc_launch_opts loaded;
                sc_launch_opts_init(&loaded, serial);
                if (name[0] && sc_config_load(name, &loaded)) {
                    launch_opts = loaded;
                    sc_options_form_set_message(&form, "Profile loaded");
                    sc_options_form_profile_close(&form);
                } else {
                    sc_options_form_set_message(&form, "Could not load profile");
                }
                dirty = true;
            } else if (action == SC_OPTIONS_FORM_DELETE_PROFILE) {
                const char *name = sc_options_form_profile_name(&form);
                if (name[0] && sc_config_delete(name)) {
                    sc_options_form_set_message(&form, "Profile deleted");
                    sc_options_form_profile_close(&form);
                    sc_options_form_profiles_reload(&form);
                } else {
                    sc_options_form_set_message(&form, "Could not delete profile");
                }
                dirty = true;
            } else if (action == SC_OPTIONS_FORM_CHANGED) {
                dirty = true;
            }
        } else if (screen == SC_SCREEN_ERROR) {
            bool dismiss = false;
            if (key == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    dismiss = sc_error_panel_handle_mouse(&error_panel, &event);
                }
            } else if (key != ERR) {
                dismiss = sc_error_panel_handle_key(key);
            }
            if (dismiss) {
                screen = SC_SCREEN_OPTIONS;
                snprintf(status, sizeof(status), "options");
                dirty = true;
            }
        }

        if (dirty) {
            getmaxyx(stdscr, rows, cols);
            if (!sc_layout_resize(&panel, &form, &error_panel, rows, cols, &devices)) {
                goto cleanup;
            }
            sc_render(screen, &panel, &form, &error_panel, &devices, &launch_opts,
                      status, launch_error, rows, cols);
        }
    }

    ret = 0;

cleanup:
    if (refresh_started) {
        sc_refresh_state_destroy(&refresh_state);
    }
    sc_device_list_free(&devices);
    if (panel_started) {
        sc_device_panel_destroy(&panel);
    }
    if (form_started) {
        sc_options_form_destroy(&form);
    }
    if (error_started) {
        sc_error_panel_destroy(&error_panel);
    }
    if (curses_started) {
        endwin();
    }
    return ret;
}
