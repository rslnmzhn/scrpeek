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

#ifndef SC_TUI_DEVICE_PANEL_H
#define SC_TUI_DEVICE_PANEL_H

#include <stdbool.h>

#include "adb_list.h"
#include "ui/layout.h"

enum sc_device_panel_action {
    SC_DEVICE_PANEL_NONE,
    SC_DEVICE_PANEL_SELECTED,
    SC_DEVICE_PANEL_ACTIVATE,
};

struct sc_device_panel {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
    int selected;
    int scroll;
};

bool
sc_device_panel_init(struct sc_device_panel *panel, int y, int x, int rows,
                     int cols);

void
sc_device_panel_destroy(struct sc_device_panel *panel);

bool
sc_device_panel_resize(struct sc_device_panel *panel, int y, int x, int rows,
                       int cols, const struct sc_device_list *devices);

enum sc_device_panel_action
sc_device_panel_handle_key(struct sc_device_panel *panel, int key,
                           const struct sc_device_list *devices);

enum sc_device_panel_action
sc_device_panel_handle_mouse(struct sc_device_panel *panel, const MEVENT *event,
                             const struct sc_device_list *devices);

void
sc_device_panel_draw(struct sc_device_panel *panel,
                     const struct sc_device_list *devices);

const struct sc_device *
sc_device_panel_selected(const struct sc_device_panel *panel,
                         const struct sc_device_list *devices);

#endif
