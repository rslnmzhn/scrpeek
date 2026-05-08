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

#ifndef SC_TUI_ERROR_PANEL_H
#define SC_TUI_ERROR_PANEL_H

#include "ui/layout.h"

#include <stdbool.h>

struct sc_error_panel {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
};

bool
sc_error_panel_init(struct sc_error_panel *panel, int term_rows, int term_cols);

void
sc_error_panel_destroy(struct sc_error_panel *panel);

bool
sc_error_panel_resize(struct sc_error_panel *panel, int term_rows, int term_cols);

bool
sc_error_panel_handle_key(int key);

bool
sc_error_panel_handle_mouse(const struct sc_error_panel *panel,
                            const MEVENT *event);

void
sc_error_panel_draw(struct sc_error_panel *panel, const char *title,
                    const char *message);

#endif
