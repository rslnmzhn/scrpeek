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

#include "ui/log_panel.h"

#include <stdio.h>
#include <string.h>

#include "ui/layout.h"

static size_t
sc_log_panel_visible(const struct sc_log_panel *panel) {
    return panel->rows > 2 ? (size_t) panel->rows - 2 : 0;
}

static void
sc_log_panel_scroll_bottom(struct sc_log_panel *panel) {
    size_t visible = sc_log_panel_visible(panel);
    if (panel->count > visible) {
        panel->scroll = panel->count - visible;
    } else {
        panel->scroll = 0;
    }
}

static void
sc_log_panel_push_line(struct sc_log_panel *panel, const char *line) {
    if (panel->count == SC_LOG_PANEL_MAX_LINES) {
        memmove(panel->lines, panel->lines[1],
                (SC_LOG_PANEL_MAX_LINES - 1) * sizeof(panel->lines[0]));
        --panel->count;
        if (panel->scroll) {
            --panel->scroll;
        }
    }

    snprintf(panel->lines[panel->count], sizeof(panel->lines[panel->count]),
             "%s", line);
    ++panel->count;

    if (panel->auto_scroll) {
        sc_log_panel_scroll_bottom(panel);
    }
}

bool
sc_log_panel_init(struct sc_log_panel *panel, int y, int x, int rows, int cols) {
    memset(panel, 0, sizeof(*panel));
    panel->win = newwin(rows, cols, y, x);
    if (!panel->win) {
        return false;
    }

    panel->y = y;
    panel->x = x;
    panel->rows = rows;
    panel->cols = cols;
    panel->auto_scroll = true;
    keypad(panel->win, TRUE);
    return true;
}

void
sc_log_panel_destroy(struct sc_log_panel *panel) {
    if (panel->win) {
        delwin(panel->win);
        panel->win = NULL;
    }
}

bool
sc_log_panel_resize(struct sc_log_panel *panel, int y, int x, int rows,
                    int cols) {
    if (wresize(panel->win, rows, cols) == ERR) {
        return false;
    }
    if (mvwin(panel->win, y, x) == ERR) {
        return false;
    }

    panel->y = y;
    panel->x = x;
    panel->rows = rows;
    panel->cols = cols;
    if (panel->auto_scroll) {
        sc_log_panel_scroll_bottom(panel);
    }
    return true;
}

void
sc_log_panel_clear(struct sc_log_panel *panel) {
    panel->count = 0;
    panel->scroll = 0;
    panel->auto_scroll = true;
    panel->partial_len = 0;
    panel->partial[0] = '\0';
}

void
sc_log_panel_append_line(struct sc_log_panel *panel, const char *line) {
    sc_log_panel_push_line(panel, line);
}

void
sc_log_panel_append(struct sc_log_panel *panel, const char *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        char ch = buf[i];
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            panel->partial[panel->partial_len] = '\0';
            sc_log_panel_push_line(panel, panel->partial);
            panel->partial_len = 0;
            panel->partial[0] = '\0';
            continue;
        }

        if (panel->partial_len + 1 >= sizeof(panel->partial)) {
            panel->partial[panel->partial_len] = '\0';
            sc_log_panel_push_line(panel, panel->partial);
            panel->partial_len = 0;
        }
        panel->partial[panel->partial_len++] = ch;
    }
    panel->partial[panel->partial_len] = '\0';
}

void
sc_log_panel_handle_key(struct sc_log_panel *panel, int key) {
    size_t visible = sc_log_panel_visible(panel);
    size_t max_scroll = panel->count > visible ? panel->count - visible : 0;

    switch (key) {
        case KEY_UP:
            if (panel->scroll) {
                --panel->scroll;
            }
            panel->auto_scroll = false;
            break;
        case KEY_DOWN:
            if (panel->scroll < max_scroll) {
                ++panel->scroll;
            }
            panel->auto_scroll = panel->scroll == max_scroll;
            break;
        case KEY_PPAGE:
            panel->scroll = panel->scroll > visible ? panel->scroll - visible : 0;
            panel->auto_scroll = false;
            break;
        case KEY_NPAGE:
            panel->scroll = panel->scroll + visible < max_scroll
                    ? panel->scroll + visible : max_scroll;
            panel->auto_scroll = panel->scroll == max_scroll;
            break;
        case KEY_END:
            panel->scroll = max_scroll;
            panel->auto_scroll = true;
            break;
        default:
            break;
    }
}

void
sc_log_panel_draw(struct sc_log_panel *panel) {
    werase(panel->win);
    wattron(panel->win, COLOR_PAIR(PAIR_NORMAL));
    SC_BOX(panel->win);
    mvwprintw(panel->win, 0, 2, " Log ");

    size_t visible = sc_log_panel_visible(panel);
    if (!visible || panel->cols <= 2) {
        wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
        wnoutrefresh(panel->win);
        return;
    }

    if (panel->auto_scroll) {
        sc_log_panel_scroll_bottom(panel);
    }

    for (size_t row = 0; row < visible; ++row) {
        size_t index = panel->scroll + row;
        if (index >= panel->count) {
            break;
        }
        mvwprintw(panel->win, (int) row + 1, 1, "%-*.*s", panel->cols - 2,
                  panel->cols - 2, panel->lines[index]);
    }

    wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
    wnoutrefresh(panel->win);
}
