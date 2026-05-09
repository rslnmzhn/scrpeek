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

#ifndef SC_TUI_OPTIONS_FORM_H
#define SC_TUI_OPTIONS_FORM_H

#include "launch_opts.h"
#include "ui/layout.h"

#include <stdbool.h>

enum sc_options_form_action {
    SC_OPTIONS_FORM_NONE,
    SC_OPTIONS_FORM_CHANGED,
    SC_OPTIONS_FORM_BACK,
    SC_OPTIONS_FORM_LAUNCH,
    SC_OPTIONS_FORM_LOAD_PROFILE,
    SC_OPTIONS_FORM_SAVE_PROFILE,
    SC_OPTIONS_FORM_DELETE_PROFILE,
};

struct sc_options_form {
    WINDOW *win;
    int y;
    int x;
    int rows;
    int cols;
    int focus;
    int scroll;
    int profile_mode;
    int profile_count;
    int profile_selected;
    char *profiles[32];
    char profile_name[33];
    char help[160];
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

enum sc_options_form_action
sc_options_form_handle_mouse(struct sc_options_form *form, const MEVENT *event,
                             struct sc_launch_opts *opts);

void
sc_options_form_profiles_reload(struct sc_options_form *form);

const char *
sc_options_form_profile_name(const struct sc_options_form *form);

void
sc_options_form_set_message(struct sc_options_form *form, const char *message);

void
sc_options_form_profile_close(struct sc_options_form *form);

void
sc_options_form_draw(struct sc_options_form *form,
                     const struct sc_launch_opts *opts);

#endif
