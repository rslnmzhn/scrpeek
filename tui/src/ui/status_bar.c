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

#include "ui/status_bar.h"

#include <stdio.h>
#include <string.h>

#include "ui/layout.h"

bool
sc_status_bar_init(struct sc_status_bar *bar, int y, int x, int rows, int cols) {
    bar->win = newwin(rows, cols, y, x);
    if (!bar->win) {
        return false;
    }
    bar->y = y;
    bar->x = x;
    bar->rows = rows;
    bar->cols = cols;
    return true;
}

void
sc_status_bar_destroy(struct sc_status_bar *bar) {
    if (bar->win) {
        delwin(bar->win);
        bar->win = NULL;
    }
}

bool
sc_status_bar_resize(struct sc_status_bar *bar, int y, int x, int rows,
                     int cols) {
    if (wresize(bar->win, rows, cols) == ERR) {
        return false;
    }
    if (mvwin(bar->win, y, x) == ERR) {
        return false;
    }
    bar->y = y;
    bar->x = x;
    bar->rows = rows;
    bar->cols = cols;
    return true;
}

void
sc_status_bar_draw(struct sc_status_bar *bar, const char *serial,
                   const struct sc_launcher *launcher, long now_ms,
                   const char *message) {
    long elapsed = now_ms > launcher->started_ms
            ? (now_ms - launcher->started_ms) / 1000 : 0;
    char exit_text[32];
    if (launcher->running) {
        snprintf(exit_text, sizeof(exit_text), "running");
    } else if (launcher->exit_code >= 0) {
        snprintf(exit_text, sizeof(exit_text), "exit=%d", launcher->exit_code);
    } else {
        snprintf(exit_text, sizeof(exit_text), "exited");
    }

    werase(bar->win);
    wattron(bar->win, COLOR_PAIR(PAIR_STATUS));
    mvwprintw(bar->win, 0, 0, "%-*s", bar->cols, "");
    mvwprintw(bar->win, 0, 0, "device=%s pid=%lu elapsed=%lds %s",
              serial ? serial : "-", (unsigned long) launcher->pid, elapsed,
              exit_text);
    if (message && message[0]) {
        int x = bar->cols - (int) strlen(message) - 1;
        if (x > 0) {
            mvwprintw(bar->win, 0, x, "%s", message);
        }
    }
    wattroff(bar->win, COLOR_PAIR(PAIR_STATUS));
    wnoutrefresh(bar->win);
}
