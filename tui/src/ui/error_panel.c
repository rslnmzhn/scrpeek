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

#include "ui/error_panel.h"

#include <string.h>

static void
sc_error_panel_geometry(int term_rows, int term_cols, int *y, int *x, int *rows,
                        int *cols) {
    *rows = term_rows < 10 ? term_rows : 10;
    *cols = term_cols < 70 ? term_cols : 70;
    *y = (term_rows - *rows) / 2;
    *x = (term_cols - *cols) / 2;
    if (*y < 0) {
        *y = 0;
    }
    if (*x < 0) {
        *x = 0;
    }
}

bool
sc_error_panel_init(struct sc_error_panel *panel, int term_rows, int term_cols) {
    sc_error_panel_geometry(term_rows, term_cols, &panel->y, &panel->x,
                            &panel->rows, &panel->cols);
    panel->win = newwin(panel->rows, panel->cols, panel->y, panel->x);
    return panel->win != NULL;
}

void
sc_error_panel_destroy(struct sc_error_panel *panel) {
    if (panel->win) {
        delwin(panel->win);
        panel->win = NULL;
    }
}

bool
sc_error_panel_resize(struct sc_error_panel *panel, int term_rows, int term_cols) {
    sc_error_panel_geometry(term_rows, term_cols, &panel->y, &panel->x,
                            &panel->rows, &panel->cols);
    if (wresize(panel->win, panel->rows, panel->cols) == ERR
            || mvwin(panel->win, panel->y, panel->x) == ERR) {
        return false;
    }
    redrawwin(panel->win);
    return true;
}

bool
sc_error_panel_handle_key(int key) {
    return key == '\n' || key == '\r' || key == KEY_ENTER || key == 27;
}

bool
sc_error_panel_handle_mouse(const struct sc_error_panel *panel,
                            const MEVENT *event) {
    int by = panel->y + panel->rows - 3;
    int bx = panel->x + panel->cols / 2 - 4;
    return event->y == by && event->x >= bx && event->x < bx + 8
            && (event->bstate & (BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | BUTTON1_PRESSED));
}

void
sc_error_panel_draw(struct sc_error_panel *panel, const char *title,
                    const char *message) {
    werase(panel->win);
    SC_BOX(panel->win);
    wattron(panel->win, COLOR_PAIR(PAIR_HEADER));
    mvwprintw(panel->win, 0, 2, " %s ", title);
    wattroff(panel->win, COLOR_PAIR(PAIR_HEADER));

    int max = panel->cols - 4;
    mvwprintw(panel->win, 2, 2, "%-*.*s", max, max, message ? message : "Unknown error");

    const char *button = "[ Back ]";
    int bx = panel->cols / 2 - (int) strlen(button) / 2;
    wattron(panel->win, COLOR_PAIR(PAIR_BUTTON));
    mvwprintw(panel->win, panel->rows - 3, bx, "%s", button);
    wattroff(panel->win, COLOR_PAIR(PAIR_BUTTON));
    wnoutrefresh(panel->win);
}
