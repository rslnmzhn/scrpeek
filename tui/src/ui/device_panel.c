#include "ui/device_panel.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ui/layout.h"

static void
sc_device_panel_clamp(struct sc_device_panel *panel,
                      const struct sc_device_list *devices) {
    if (!devices->count) {
        panel->selected = 0;
        panel->scroll = 0;
        return;
    }

    if (panel->selected >= devices->count) {
        panel->selected = devices->count - 1;
    }

    size_t visible = panel->rows > 2 ? (size_t) panel->rows - 2 : 0;
    if (!visible) {
        panel->scroll = panel->selected;
        return;
    }

    if (panel->selected < panel->scroll) {
        panel->scroll = panel->selected;
    } else if (panel->selected >= panel->scroll + visible) {
        panel->scroll = panel->selected - visible + 1;
    }

    if (panel->scroll >= devices->count) {
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
    return true;
}

void
sc_device_panel_handle_key(struct sc_device_panel *panel, int key,
                           const struct sc_device_list *devices) {
    if (!devices->count) {
        panel->selected = 0;
        panel->scroll = 0;
        return;
    }

    switch (key) {
        case KEY_UP:
            if (panel->selected > 0) {
                --panel->selected;
            }
            break;
        case KEY_DOWN:
            if (panel->selected + 1 < devices->count) {
                ++panel->selected;
            }
            break;
        default:
            break;
    }

    sc_device_panel_clamp(panel, devices);
}

void
sc_device_panel_draw(struct sc_device_panel *panel,
                     const struct sc_device_list *devices) {
    werase(panel->win);
    wattron(panel->win, COLOR_PAIR(PAIR_NORMAL));
    box(panel->win, 0, 0);
    mvwprintw(panel->win, 0, 2, " Devices ");

    sc_device_panel_clamp(panel, devices);

    int visible = panel->rows - 2;
    if (visible <= 0 || panel->cols <= 2) {
        wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
        wnoutrefresh(panel->win);
        return;
    }

    if (!devices->count) {
        mvwprintw(panel->win, 1, 2, "No devices connected");
        wattroff(panel->win, COLOR_PAIR(PAIR_NORMAL));
        wnoutrefresh(panel->win);
        return;
    }

    for (int row = 0; row < visible; ++row) {
        size_t index = panel->scroll + (size_t) row;
        if (index >= devices->count) {
            break;
        }

        const struct sc_device *device = &devices->devices[index];
        bool selected = index == panel->selected;
        int pair = selected ? PAIR_SELECTED : PAIR_NORMAL;
        wattron(panel->win, COLOR_PAIR(pair));

        char line[512];
        int written = snprintf(line, sizeof(line), "%-28s %-24s %-12s",
                               device->serial,
                               device->model ? device->model : "",
                               device->state);
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
    if (!devices->count || panel->selected >= devices->count) {
        return NULL;
    }

    return &devices->devices[panel->selected];
}
