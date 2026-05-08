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

#ifndef SC_TUI_LOG_PANEL_H
#define SC_TUI_LOG_PANEL_H

#include <curses.h>
#include <stdbool.h>
#include <stddef.h>

#define SC_LOG_PANEL_MAX_LINES 1000
#define SC_LOG_PANEL_LINE_LEN 512

struct sc_log_panel {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
    char lines[SC_LOG_PANEL_MAX_LINES][SC_LOG_PANEL_LINE_LEN];
    size_t count;
    size_t scroll;
    bool auto_scroll;
    char partial[SC_LOG_PANEL_LINE_LEN];
    size_t partial_len;
};

bool
sc_log_panel_init(struct sc_log_panel *panel, int y, int x, int rows, int cols);

void
sc_log_panel_destroy(struct sc_log_panel *panel);

bool
sc_log_panel_resize(struct sc_log_panel *panel, int y, int x, int rows,
                    int cols);

void
sc_log_panel_clear(struct sc_log_panel *panel);

void
sc_log_panel_append(struct sc_log_panel *panel, const char *buf, size_t len);

void
sc_log_panel_append_line(struct sc_log_panel *panel, const char *line);

void
sc_log_panel_handle_key(struct sc_log_panel *panel, int key);

void
sc_log_panel_draw(struct sc_log_panel *panel);

#endif
