#ifndef SC_TUI_OPTIONS_FORM_H
#define SC_TUI_OPTIONS_FORM_H

#include <curses.h>
#include <stdbool.h>
#include <stddef.h>

#include "launch_opts.h"

enum sc_options_form_action {
    SC_OPTIONS_FORM_NONE,
    SC_OPTIONS_FORM_BACK,
    SC_OPTIONS_FORM_LAUNCH,
};

struct sc_options_form {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
    size_t field;
    char error[80];
};

bool
sc_options_form_init(struct sc_options_form *form, int y, int x, int rows,
                     int cols);

void
sc_options_form_destroy(struct sc_options_form *form);

bool
sc_options_form_resize(struct sc_options_form *form, int y, int x, int rows,
                       int cols);

enum sc_options_form_action
sc_options_form_handle_key(struct sc_options_form *form, int key,
                           struct sc_launch_opts *opts);

void
sc_options_form_draw(struct sc_options_form *form,
                     const struct sc_launch_opts *opts);

#endif
