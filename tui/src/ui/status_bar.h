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

#ifndef SC_TUI_STATUS_BAR_H
#define SC_TUI_STATUS_BAR_H

#include <stdbool.h>

#include "launcher.h"
#include "ui/layout.h"

struct sc_status_bar {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
};

bool
sc_status_bar_init(struct sc_status_bar *bar, int y, int x, int rows, int cols);

void
sc_status_bar_destroy(struct sc_status_bar *bar);

bool
sc_status_bar_resize(struct sc_status_bar *bar, int y, int x, int rows,
                     int cols);

void
sc_status_bar_draw(struct sc_status_bar *bar, const char *serial,
                   const struct sc_launcher *launcher, long now_ms,
                   const char *message);

#endif
