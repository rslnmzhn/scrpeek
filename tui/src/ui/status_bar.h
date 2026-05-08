#ifndef SC_TUI_STATUS_BAR_H
#define SC_TUI_STATUS_BAR_H

#include <curses.h>
#include <stdbool.h>

#include "launcher.h"

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
