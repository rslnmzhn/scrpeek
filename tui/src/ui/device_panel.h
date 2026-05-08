#ifndef SC_TUI_DEVICE_PANEL_H
#define SC_TUI_DEVICE_PANEL_H

#include <curses.h>
#include <stdbool.h>
#include <stddef.h>

#include "adb_list.h"

struct sc_device_panel {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
    size_t selected;
    size_t scroll;
};

bool
sc_device_panel_init(struct sc_device_panel *panel, int y, int x, int rows,
                     int cols);

void
sc_device_panel_destroy(struct sc_device_panel *panel);

bool
sc_device_panel_resize(struct sc_device_panel *panel, int y, int x, int rows,
                       int cols);

void
sc_device_panel_handle_key(struct sc_device_panel *panel, int key,
                           const struct sc_device_list *devices);

void
sc_device_panel_draw(struct sc_device_panel *panel,
                     const struct sc_device_list *devices);

#endif
