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

#include "ui/device_panel.h"

#include <stdio.h>

static int
sc_device_panel_visible_rows(const struct sc_device_panel *panel) {
    return panel->rows > 2 ? panel->rows - 2 : 0;
}

static bool
sc_mouse_scroll_up(const MEVENT *event) {
#ifdef BUTTON4_PRESSED
    if (event->bstate & BUTTON4_PRESSED) {
        return true;
    }
#endif
    return false;
}

static bool
sc_mouse_scroll_down(const MEVENT *event) {
#ifdef BUTTON5_PRESSED
    if (event->bstate & BUTTON5_PRESSED) {
        return true;
    }
#endif
    return false;
}

static bool
sc_mouse_middle_click(const MEVENT *event) {
#ifdef BUTTON2_CLICKED
    if (event->bstate & BUTTON2_CLICKED) {
        return true;
    }
#endif
#ifdef BUTTON2_PRESSED
    if (event->bstate & BUTTON2_PRESSED) {
        return true;
    }
#endif
    return false;
}

static void
sc_device_panel_clamp(struct sc_device_panel *panel,
                      const struct sc_device_list *devices) {
    if (devices->count <= 0) {
        panel->selected = 0;
        panel->scroll = 0;
        return;
    }

    if (panel->selected < 0) {
        panel->selected = 0;
    } else if (panel->selected >= devices->count) {
        panel->selected = devices->count - 1;
    }

    int visible = sc_device_panel_visible_rows(panel);
    if (visible <= 0) {
        panel->scroll = panel->selected;
        return;
    }

    if (panel->selected < panel->scroll) {
        panel->scroll = panel->selected;
    } else if (panel->selected >= panel->scroll + visible) {
        panel->scroll = panel->selected - visible + 1;
    }

    if (panel->scroll < 0) {
        panel->scroll = 0;
    } else if (panel->scroll >= devices->count) {
        panel->scroll = devices->count - 1;
    }
}

bool
sc_device_panel_init(struct sc_device_panel *panel, int y, int x, int rows,
                     int cols) {
    panel->win = newwin(rows, cols, y, x);
    if (!panel->win) {
        return false;
    }

    panel->y = y;
    panel->x = x;
    panel->rows = rows;
    panel->cols = cols;
    panel->selected = 0;
    panel->scroll = 0;
    keypad(panel->win, TRUE);
    return true;
}

void
sc_device_panel_destroy(struct sc_device_panel *panel) {
    if (panel->win) {
        delwin(panel->win);
        panel->win = NULL;
    }
}

bool
sc_device_panel_resize(struct sc_device_panel *panel, int y, int x, int rows,
                       int cols, const struct sc_device_list *devices) {
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
    sc_device_panel_clamp(panel, devices);
    redrawwin(panel->win);
    return true;
}

enum sc_device_panel_action
sc_device_panel_handle_key(struct sc_device_panel *panel, int key,
                           const struct sc_device_list *devices) {
    if (devices->count <= 0) {
        panel->selected = 0;
        panel->scroll = 0;
        return SC_DEVICE_PANEL_NONE;
    }

    int old_selected = panel->selected;
    int old_scroll = panel->scroll;
    switch (key) {
        case KEY_UP:
            --panel->selected;
            break;
        case KEY_DOWN:
            ++panel->selected;
            break;
        case KEY_PPAGE:
            panel->selected -= sc_device_panel_visible_rows(panel);
            break;
        case KEY_NPAGE:
            panel->selected += sc_device_panel_visible_rows(panel);
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            return SC_DEVICE_PANEL_ACTIVATE;
        default:
            return SC_DEVICE_PANEL_NONE;
    }

    sc_device_panel_clamp(panel, devices);
    if (panel->selected == old_selected && panel->scroll == old_scroll) {
        return SC_DEVICE_PANEL_NONE;
    }
    return SC_DEVICE_PANEL_SELECTED;
}

enum sc_device_panel_action
sc_device_panel_handle_mouse(struct sc_device_panel *panel, const MEVENT *event,
                             const struct sc_device_list *devices) {
    if (devices->count <= 0) {
        return SC_DEVICE_PANEL_NONE;
    }

    if (event->x <= panel->x || event->x >= panel->x + panel->cols - 1) {
        return SC_DEVICE_PANEL_NONE;
    }

    if (sc_mouse_scroll_up(event)) {
        return sc_device_panel_handle_key(panel, KEY_UP, devices);
    }
    if (sc_mouse_scroll_down(event)) {
        return sc_device_panel_handle_key(panel, KEY_DOWN, devices);
    }
    if (sc_mouse_middle_click(event)) {
        return sc_device_panel_handle_key(panel, KEY_ENTER, devices);
    }

    int first_row_y = panel->y + 1;
    int row = event->y - first_row_y;
    if (row < 0 || row >= sc_device_panel_visible_rows(panel)) {
        return SC_DEVICE_PANEL_NONE;
    }

    int index = panel->scroll + row;
    if (index < 0 || index >= devices->count) {
        return SC_DEVICE_PANEL_NONE;
    }

    int old_selected = panel->selected;
    int old_scroll = panel->scroll;
    panel->selected = index;
    sc_device_panel_clamp(panel, devices);

    if (event->bstate & BUTTON1_DOUBLE_CLICKED) {
        return SC_DEVICE_PANEL_ACTIVATE;
    }
    if (event->bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_RELEASED)) {
        if (panel->selected == old_selected && panel->scroll == old_scroll) {
            return SC_DEVICE_PANEL_NONE;
        }
        return SC_DEVICE_PANEL_SELECTED;
    }

    return SC_DEVICE_PANEL_NONE;
}

void
sc_device_panel_draw(struct sc_device_panel *panel,
                     const struct sc_device_list *devices) {
    werase(panel->win);
    wattron(panel->win, COLOR_PAIR(PAIR_NORMAL));
    SC_BOX(panel->win);
    mvwprintw(panel->win, 0, 2, " Devices ");

    sc_device_panel_clamp(panel, devices);

    int visible = sc_device_panel_visible_rows(panel);
    if (visible <= 0 || panel->cols <= 2) {
        wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
        wnoutrefresh(panel->win);
        return;
    }

    if (devices->count <= 0) {
        mvwprintw(panel->win, 1, 2, "No devices connected");
        wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
        wnoutrefresh(panel->win);
        return;
    }

    mvwprintw(panel->win, 1, 2, "%-28s %-24s %-12s", "Serial", "Model", "State");

    for (int row = 1; row < visible; ++row) {
        int index = panel->scroll + row - 1;
        if (index >= devices->count) {
            break;
        }

        const struct sc_device *device = &devices->devices[index];
        bool selected = index == panel->selected;
        int pair = selected ? PAIR_SELECTED : PAIR_NORMAL;
        wattron(panel->win, COLOR_PAIR(pair));

        char line[256];
        int written = snprintf(line, sizeof(line), "%-28s %-24s %-12s",
                               device->serial, device->model, device->state);
        if (written < 0) {
            line[0] = '\0';
        }

        mvwprintw(panel->win, row + 1, 1, "%-*.*s", panel->cols - 2,
                  panel->cols - 2, line);
        wattroff(panel->win, COLOR_PAIR(pair));
    }

    wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
    wnoutrefresh(panel->win);
}

const struct sc_device *
sc_device_panel_selected(const struct sc_device_panel *panel,
                         const struct sc_device_list *devices) {
    if (devices->count <= 0 || panel->selected < 0 || panel->selected >= devices->count) {
        return NULL;
    }

    return &devices->devices[panel->selected];
}
