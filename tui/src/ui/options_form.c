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

#include "ui/options_form.h"

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
sc_text_backspace(char *text);

enum sc_field_type {
    SC_FIELD_SECTION,
    SC_FIELD_TEXT,
    SC_FIELD_NUMERIC,
    SC_FIELD_SELECT,
    SC_FIELD_CHECKBOX,
    SC_FIELD_BUTTON,
};

enum sc_field_id {
    F_PROFILE,
    F_VIDEO,
    F_MAX_SIZE,
    F_MAX_FPS,
    F_VIDEO_CODEC,
    F_VIDEO_SOURCE,
    F_DISPLAY_ID,
    F_NEW_DISPLAY,
    F_CROP,
    F_ORIENTATION,
    F_RECORD_PATH,
    F_RECORD_FORMAT,
    F_AUDIO,
    F_NO_AUDIO,
    F_AUDIO_CODEC,
    F_AUDIO_SOURCE,
    F_AUDIO_BUFFER,
    F_CONTROL,
    F_KEYBOARD,
    F_MOUSE,
    F_GAMEPAD,
    F_NO_CONTROL,
    F_DEVICE,
    F_TCPIP,
    F_TURN_SCREEN_OFF,
    F_STAY_AWAKE,
    F_SHOW_TOUCHES,
    F_POWER_OFF,
    F_WINDOW,
    F_FULLSCREEN,
    F_ALWAYS_ON_TOP,
    F_WINDOW_TITLE,
    F_NO_WINDOW,
    F_MISC,
    F_OTG,
    F_V4L2,
    F_NO_DOWNSIZE,
    F_VERBOSITY,
    F_BACK,
    F_LAUNCH,
    F_COUNT,
};

struct sc_field_def {
    enum sc_field_id id;
    enum sc_field_type type;
    const char *label;
    const char *help;
};

static const struct sc_field_def sc_fields[] = {
    {F_PROFILE, SC_FIELD_BUTTON, "Profile", "Load, save, or delete named launch profiles"},
    {F_VIDEO, SC_FIELD_SECTION, "Video", "Video capture and recording options"},
    {F_MAX_SIZE, SC_FIELD_NUMERIC, "Max size", "--max-size value: limit width and height"},
    {F_MAX_FPS, SC_FIELD_NUMERIC, "Max FPS", "--max-fps value: limit capture framerate"},
    {F_VIDEO_CODEC, SC_FIELD_SELECT, "Video codec", "--video-codec h264|h265|av1"},
    {F_VIDEO_SOURCE, SC_FIELD_SELECT, "Video source", "--video-source display|camera"},
    {F_DISPLAY_ID, SC_FIELD_NUMERIC, "Display ID", "--display-id id"},
    {F_NEW_DISPLAY, SC_FIELD_TEXT, "New display", "--new-display[=WxH[/DPI]]; enter 1 for no value"},
    {F_CROP, SC_FIELD_TEXT, "Crop", "--crop width:height:x:y"},
    {F_ORIENTATION, SC_FIELD_TEXT, "Lock orientation", "--capture-orientation value"},
    {F_RECORD_PATH, SC_FIELD_TEXT, "Record path", "--record file"},
    {F_RECORD_FORMAT, SC_FIELD_TEXT, "Record format", "--record-format format"},
    {F_AUDIO, SC_FIELD_SECTION, "Audio", "Audio capture options"},
    {F_NO_AUDIO, SC_FIELD_CHECKBOX, "No audio", "--no-audio"},
    {F_AUDIO_CODEC, SC_FIELD_SELECT, "Audio codec", "--audio-codec opus|aac|flac"},
    {F_AUDIO_SOURCE, SC_FIELD_SELECT, "Audio source", "--audio-source output|playback|mic"},
    {F_AUDIO_BUFFER, SC_FIELD_NUMERIC, "Audio buffer", "--audio-buffer ms"},
    {F_CONTROL, SC_FIELD_SECTION, "Control", "Input/control options"},
    {F_KEYBOARD, SC_FIELD_SELECT, "Keyboard", "--keyboard disabled|sdk|uhid|aoa"},
    {F_MOUSE, SC_FIELD_SELECT, "Mouse", "--mouse disabled|sdk|uhid|aoa"},
    {F_GAMEPAD, SC_FIELD_SELECT, "Gamepad", "--gamepad disabled|uhid|aoa"},
    {F_NO_CONTROL, SC_FIELD_CHECKBOX, "No control", "--no-control"},
    {F_DEVICE, SC_FIELD_SECTION, "Device", "Device connection and device-state options"},
    {F_TCPIP, SC_FIELD_TEXT, "TCP/IP", "--tcpip[=ip[:port]]; enter 1 for no value"},
    {F_TURN_SCREEN_OFF, SC_FIELD_CHECKBOX, "Turn screen off", "--turn-screen-off"},
    {F_STAY_AWAKE, SC_FIELD_CHECKBOX, "Stay awake", "--stay-awake"},
    {F_SHOW_TOUCHES, SC_FIELD_CHECKBOX, "Show touches", "--show-touches"},
    {F_POWER_OFF, SC_FIELD_CHECKBOX, "Power off close", "--power-off-on-close"},
    {F_WINDOW, SC_FIELD_SECTION, "Window", "Desktop window options"},
    {F_FULLSCREEN, SC_FIELD_CHECKBOX, "Fullscreen", "--fullscreen"},
    {F_ALWAYS_ON_TOP, SC_FIELD_CHECKBOX, "Always on top", "--always-on-top"},
    {F_WINDOW_TITLE, SC_FIELD_TEXT, "Window title", "--window-title text"},
    {F_NO_WINDOW, SC_FIELD_CHECKBOX, "No window", "--no-window"},
    {F_MISC, SC_FIELD_SECTION, "Misc", "Miscellaneous options"},
    {F_OTG, SC_FIELD_CHECKBOX, "OTG", "--otg"},
    {F_V4L2, SC_FIELD_TEXT, "V4L2 sink", "--v4l2-sink /dev/videoN"},
    {F_NO_DOWNSIZE, SC_FIELD_CHECKBOX, "No downsize", "--no-downsize-on-error"},
    {F_VERBOSITY, SC_FIELD_SELECT, "Verbosity", "--verbosity verbose|debug|info|warn|error"},
    {F_BACK, SC_FIELD_BUTTON, "Back", "Return to device list"},
    {F_LAUNCH, SC_FIELD_BUTTON, "Launch", "Build argv and launch scrcpy"},
};

static const char *const video_codecs[] = {"h264", "h265", "av1"};
static const char *const video_sources[] = {"display", "camera"};
static const char *const audio_codecs[] = {"opus", "aac", "flac"};
static const char *const audio_sources[] = {"output", "playback", "mic"};
static const char *const control_modes[] = {"disabled", "sdk", "uhid", "aoa"};
static const char *const gamepad_modes[] = {"disabled", "uhid", "aoa"};
static const char *const verbosity_names[] = {"info", "verbose", "debug", "warn", "error"};

enum sc_profile_mode {
    SC_PROFILE_IDLE,
    SC_PROFILE_LOAD,
    SC_PROFILE_SAVE_AS,
    SC_PROFILE_DELETE_CONFIRM,
};

static void
sc_free_profiles(struct sc_options_form *form) {
    for (int i = 0; i < form->profile_count; ++i) {
        free(form->profiles[i]);
        form->profiles[i] = NULL;
    }
    form->profile_count = 0;
    form->profile_selected = 0;
}

void
sc_options_form_profiles_reload(struct sc_options_form *form) {
    sc_free_profiles(form);
    form->profile_count = sc_config_list_profiles(form->profiles,
            (int) (sizeof(form->profiles) / sizeof(form->profiles[0])));
    if (form->profile_count < 0) {
        form->profile_count = 0;
    }
}

const char *
sc_options_form_profile_name(const struct sc_options_form *form) {
    if (form->profile_mode == SC_PROFILE_SAVE_AS) {
        return form->profile_name;
    }
    if (form->profile_count > 0 && form->profile_selected >= 0
            && form->profile_selected < form->profile_count) {
        return form->profiles[form->profile_selected];
    }
    return form->profile_name;
}

void
sc_options_form_set_message(struct sc_options_form *form, const char *message) {
    snprintf(form->help, sizeof(form->help), "%s", message ? message : "");
}

void
sc_options_form_profile_close(struct sc_options_form *form) {
    form->profile_mode = SC_PROFILE_IDLE;
}

static bool
sc_field_visible(enum sc_field_id id) {
    if (id == F_PROFILE) {
        return false;
    }
#ifndef __linux__
    if (id == F_V4L2) {
        return false;
    }
#else
    (void) id;
#endif
    return true;
}

static bool
sc_field_focusable(enum sc_field_id id) {
    return sc_field_visible(id) && sc_fields[id].type != SC_FIELD_SECTION;
}

static bool
sc_profile_name_append(char *text, size_t cap, int key) {
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        return sc_text_backspace(text);
    }
    if (key < 32 || key > 126) {
        return false;
    }
    unsigned char c = (unsigned char) key;
    if (!isalnum(c) && c != '_' && c != '-') {
        return false;
    }
    size_t len = strlen(text);
    if (len + 1 >= cap) {
        return false;
    }
    text[len] = (char) key;
    text[len + 1] = '\0';
    return true;
}

static int
sc_next_focus(int focus, int dir) {
    int next = focus;
    do {
        next += dir;
        if (next < 0) {
            next = F_COUNT - 1;
        } else if (next >= F_COUNT) {
            next = 0;
        }
    } while (!sc_field_focusable((enum sc_field_id) next));
    return next;
}

static void
sc_form_clamp_scroll(struct sc_options_form *form) {
    int row = 0;
    int focus_row = 0;
    for (int i = 0; i < F_COUNT; ++i) {
        if (!sc_field_visible((enum sc_field_id) i)) {
            continue;
        }
        if (i == form->focus) {
            focus_row = row;
            break;
        }
        ++row;
    }

    int visible = form->rows > 4 ? form->rows - 4 : 1;
    if (focus_row < form->scroll) {
        form->scroll = focus_row;
    } else if (focus_row >= form->scroll + visible) {
        form->scroll = focus_row - visible + 1;
    }
    if (form->scroll < 0) {
        form->scroll = 0;
    }
}

static char *
sc_field_text(struct sc_launch_opts *opts, enum sc_field_id id, size_t *cap) {
    switch (id) {
        case F_MAX_SIZE: *cap = sizeof(opts->max_size); return opts->max_size;
        case F_MAX_FPS: *cap = sizeof(opts->max_fps); return opts->max_fps;
        case F_DISPLAY_ID: *cap = sizeof(opts->display_id); return opts->display_id;
        case F_NEW_DISPLAY: *cap = sizeof(opts->new_display); return opts->new_display;
        case F_CROP: *cap = sizeof(opts->crop); return opts->crop;
        case F_ORIENTATION: *cap = sizeof(opts->lock_video_orientation); return opts->lock_video_orientation;
        case F_RECORD_PATH: *cap = sizeof(opts->record_path); return opts->record_path;
        case F_RECORD_FORMAT: *cap = sizeof(opts->record_format); return opts->record_format;
        case F_AUDIO_BUFFER: *cap = sizeof(opts->audio_buffer); return opts->audio_buffer;
        case F_TCPIP: *cap = sizeof(opts->tcpip_addr); return opts->tcpip_addr;
        case F_WINDOW_TITLE: *cap = sizeof(opts->window_title); return opts->window_title;
        case F_V4L2: *cap = sizeof(opts->v4l2_sink); return opts->v4l2_sink;
        default: *cap = 0; return NULL;
    }
}

static bool
sc_text_backspace(char *text) {
    size_t len = strlen(text);
    if (!len) {
        return false;
    }
    text[len - 1] = '\0';
    return true;
}

static bool
sc_text_append(char *text, size_t cap, int key, bool numeric) {
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        return sc_text_backspace(text);
    }
    if (key < 32 || key > 126) {
        return false;
    }
    if (numeric && !isdigit((unsigned char) key)) {
        return false;
    }
    size_t len = strlen(text);
    if (len + 1 >= cap) {
        return false;
    }
    text[len] = (char) key;
    text[len + 1] = '\0';
    return true;
}

static void
sc_cycle_select(struct sc_launch_opts *opts, enum sc_field_id id, int dir) {
    switch (id) {
        case F_VIDEO_CODEC: opts->video_codec = (opts->video_codec + dir + 3) % 3; break;
        case F_VIDEO_SOURCE: opts->video_source = (opts->video_source + dir + 2) % 2; break;
        case F_AUDIO_CODEC: opts->audio_codec = (opts->audio_codec + dir + 3) % 3; break;
        case F_AUDIO_SOURCE: opts->audio_source = (opts->audio_source + dir + 3) % 3; break;
        case F_KEYBOARD: opts->keyboard_mode = (opts->keyboard_mode + dir + 4) % 4; break;
        case F_MOUSE: opts->mouse_mode = (opts->mouse_mode + dir + 4) % 4; break;
        case F_GAMEPAD: opts->gamepad = (opts->gamepad + dir + 3) % 3; break;
        case F_VERBOSITY: opts->verbosity = (opts->verbosity + dir + 5) % 5; break;
        default: break;
    }
}

static void
sc_toggle_checkbox(struct sc_launch_opts *opts, enum sc_field_id id) {
    switch (id) {
        case F_NO_AUDIO: opts->no_audio = !opts->no_audio; break;
        case F_NO_CONTROL: opts->no_control = !opts->no_control; break;
        case F_TURN_SCREEN_OFF: opts->turn_screen_off = !opts->turn_screen_off; break;
        case F_STAY_AWAKE: opts->stay_awake = !opts->stay_awake; break;
        case F_SHOW_TOUCHES: opts->show_touches = !opts->show_touches; break;
        case F_POWER_OFF: opts->power_off_on_close = !opts->power_off_on_close; break;
        case F_FULLSCREEN: opts->fullscreen = !opts->fullscreen; break;
        case F_ALWAYS_ON_TOP: opts->always_on_top = !opts->always_on_top; break;
        case F_NO_WINDOW: opts->no_window = !opts->no_window; break;
        case F_OTG: opts->otg = !opts->otg; break;
        case F_NO_DOWNSIZE: opts->no_downsize_on_error = !opts->no_downsize_on_error; break;
        default: break;
    }
}

static bool
sc_checkbox_value(const struct sc_launch_opts *opts, enum sc_field_id id) {
    switch (id) {
        case F_NO_AUDIO: return opts->no_audio;
        case F_NO_CONTROL: return opts->no_control;
        case F_TURN_SCREEN_OFF: return opts->turn_screen_off;
        case F_STAY_AWAKE: return opts->stay_awake;
        case F_SHOW_TOUCHES: return opts->show_touches;
        case F_POWER_OFF: return opts->power_off_on_close;
        case F_FULLSCREEN: return opts->fullscreen;
        case F_ALWAYS_ON_TOP: return opts->always_on_top;
        case F_NO_WINDOW: return opts->no_window;
        case F_OTG: return opts->otg;
        case F_NO_DOWNSIZE: return opts->no_downsize_on_error;
        default: return false;
    }
}

static const char *
sc_select_value(const struct sc_launch_opts *opts, enum sc_field_id id) {
    switch (id) {
        case F_VIDEO_CODEC: return video_codecs[opts->video_codec];
        case F_VIDEO_SOURCE: return video_sources[opts->video_source];
        case F_AUDIO_CODEC: return audio_codecs[opts->audio_codec];
        case F_AUDIO_SOURCE: return audio_sources[opts->audio_source];
        case F_KEYBOARD: return control_modes[opts->keyboard_mode];
        case F_MOUSE: return control_modes[opts->mouse_mode];
        case F_GAMEPAD: return gamepad_modes[opts->gamepad];
        case F_VERBOSITY: return verbosity_names[opts->verbosity];
        default: return "";
    }
}

static enum sc_options_form_action
sc_activate(struct sc_options_form *form, struct sc_launch_opts *opts) {
    enum sc_field_id id = (enum sc_field_id) form->focus;
    if (id == F_PROFILE) {
        form->profile_mode = SC_PROFILE_LOAD;
        sc_options_form_profiles_reload(form);
        return SC_OPTIONS_FORM_NONE;
    }
    switch (sc_fields[id].type) {
        case SC_FIELD_CHECKBOX:
            sc_toggle_checkbox(opts, id);
            break;
        case SC_FIELD_SELECT:
            sc_cycle_select(opts, id, 1);
            break;
        case SC_FIELD_BUTTON:
            return id == F_LAUNCH ? SC_OPTIONS_FORM_LAUNCH : SC_OPTIONS_FORM_BACK;
        default:
            break;
    }
    return SC_OPTIONS_FORM_NONE;
}

bool
sc_options_form_init(struct sc_options_form *form, int y, int x, int rows,
                     int cols) {
    form->win = newwin(rows, cols, y, x);
    if (!form->win) {
        return false;
    }
    form->y = y;
    form->x = x;
    form->rows = rows;
    form->cols = cols;
    form->focus = F_MAX_SIZE;
    form->scroll = 0;
    form->profile_mode = SC_PROFILE_IDLE;
    form->profile_count = 0;
    form->profile_selected = 0;
    form->profile_name[0] = '\0';
    form->help[0] = '\0';
    keypad(form->win, TRUE);
    return true;
}

void
sc_options_form_destroy(struct sc_options_form *form) {
    if (form->win) {
        delwin(form->win);
        form->win = NULL;
    }
    sc_free_profiles(form);
}

static enum sc_options_form_action
sc_profile_key(struct sc_options_form *form, int key) {
    if (form->profile_mode == SC_PROFILE_IDLE) {
        return SC_OPTIONS_FORM_NONE;
    }
    if (key == 27) {
        form->profile_mode = SC_PROFILE_IDLE;
        return SC_OPTIONS_FORM_CHANGED;
    }
    if (form->profile_mode == SC_PROFILE_LOAD) {
        if (key == KEY_UP && form->profile_selected > 0) {
            --form->profile_selected;
            return SC_OPTIONS_FORM_CHANGED;
        }
        if (key == KEY_DOWN && form->profile_selected + 1 < form->profile_count) {
            ++form->profile_selected;
            return SC_OPTIONS_FORM_CHANGED;
        }
        else if (key == '\n' || key == '\r' || key == KEY_ENTER) return SC_OPTIONS_FORM_LOAD_PROFILE;
        return SC_OPTIONS_FORM_NONE;
    }
    if (form->profile_mode == SC_PROFILE_SAVE_AS) {
        if (key == '\n' || key == '\r' || key == KEY_ENTER) return SC_OPTIONS_FORM_SAVE_PROFILE;
        (void) sc_profile_name_append(form->profile_name, sizeof(form->profile_name), key);
        return SC_OPTIONS_FORM_CHANGED;
    }
    if (form->profile_mode == SC_PROFILE_DELETE_CONFIRM) {
        if (key == 'y' || key == 'Y') return SC_OPTIONS_FORM_DELETE_PROFILE;
        if (key != ERR) {
            form->profile_mode = SC_PROFILE_IDLE;
            return SC_OPTIONS_FORM_CHANGED;
        }
    }
    return SC_OPTIONS_FORM_NONE;
}

bool
sc_options_form_resize(struct sc_options_form *form, int y, int x, int rows,
                       int cols) {
    if (wresize(form->win, rows, cols) == ERR || mvwin(form->win, y, x) == ERR) {
        return false;
    }
    form->y = y;
    form->x = x;
    form->rows = rows;
    form->cols = cols;
    sc_form_clamp_scroll(form);
    redrawwin(form->win);
    return true;
}

enum sc_options_form_action
sc_options_form_handle_key(struct sc_options_form *form, int key,
                           struct sc_launch_opts *opts) {
    if (key == ERR) {
        return SC_OPTIONS_FORM_NONE;
    }
    form->help[0] = '\0';
    if (form->profile_mode != SC_PROFILE_IDLE) {
        return sc_profile_key(form, key);
    }

    switch (key) {
        case 27:
            return SC_OPTIONS_FORM_BACK;
        case KEY_F(1):
            snprintf(form->help, sizeof(form->help), "%s", sc_fields[form->focus].help);
            return SC_OPTIONS_FORM_CHANGED;
        case '\t':
        case KEY_DOWN:
            form->focus = sc_next_focus(form->focus, 1);
            sc_form_clamp_scroll(form);
            return SC_OPTIONS_FORM_CHANGED;
#ifdef KEY_BTAB
        case KEY_BTAB:
#endif
        case KEY_UP:
            form->focus = sc_next_focus(form->focus, -1);
            sc_form_clamp_scroll(form);
            return SC_OPTIONS_FORM_CHANGED;
        case KEY_LEFT:
            sc_cycle_select(opts, (enum sc_field_id) form->focus, -1);
            return SC_OPTIONS_FORM_CHANGED;
        case KEY_RIGHT:
            sc_cycle_select(opts, (enum sc_field_id) form->focus, 1);
            return SC_OPTIONS_FORM_CHANGED;
        case ' ':
        case '\n':
        case '\r':
        case KEY_ENTER:
            return sc_activate(form, opts);
        default:
            break;
    }

    enum sc_field_id id = (enum sc_field_id) form->focus;
    if (sc_fields[id].type == SC_FIELD_TEXT || sc_fields[id].type == SC_FIELD_NUMERIC) {
        size_t cap;
        char *text = sc_field_text(opts, id, &cap);
        if (text) {
            (void) sc_text_append(text, cap, key, sc_fields[id].type == SC_FIELD_NUMERIC);
            return SC_OPTIONS_FORM_CHANGED;
        }
    }
    return SC_OPTIONS_FORM_NONE;
}

enum sc_options_form_action
sc_options_form_handle_mouse(struct sc_options_form *form, const MEVENT *event,
                             struct sc_launch_opts *opts) {
    if (event->x <= form->x || event->x >= form->x + form->cols - 1) {
        return SC_OPTIONS_FORM_NONE;
    }
    if (event->y == form->y + 1 && event->bstate & (BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | BUTTON1_PRESSED)) {
        int rel = event->x - form->x;
        if (rel >= 22 && rel < 32) {
            form->profile_mode = SC_PROFILE_LOAD;
            sc_options_form_profiles_reload(form);
            return SC_OPTIONS_FORM_CHANGED;
        }
        if (rel >= 34 && rel < 47) {
            form->profile_mode = SC_PROFILE_SAVE_AS;
            form->profile_name[0] = '\0';
            return SC_OPTIONS_FORM_CHANGED;
        }
        if (rel >= 49 && rel < 59) {
            form->profile_mode = SC_PROFILE_DELETE_CONFIRM;
            return SC_OPTIONS_FORM_CHANGED;
        }
    }
    if (form->profile_mode == SC_PROFILE_LOAD && event->y > form->y + 1
            && event->y < form->y + 2 + form->profile_count) {
        form->profile_selected = event->y - form->y - 2;
        return SC_OPTIONS_FORM_LOAD_PROFILE;
    }
    int visible_row = event->y - form->y - 4;
    if (visible_row < 0 || visible_row >= form->rows - 5) {
        return SC_OPTIONS_FORM_NONE;
    }
    int logical_row = form->scroll + visible_row;
    int row = 0;
    for (int i = 0; i < F_COUNT; ++i) {
        if (!sc_field_visible((enum sc_field_id) i)) {
            continue;
        }
        if (row == logical_row) {
            if (!sc_field_focusable((enum sc_field_id) i)) {
                return SC_OPTIONS_FORM_NONE;
            }
            form->focus = i;
            if (event->bstate & (BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | BUTTON1_PRESSED)) {
                return sc_activate(form, opts);
            }
            return SC_OPTIONS_FORM_CHANGED;
        }
        ++row;
    }
    return SC_OPTIONS_FORM_NONE;
}

void
sc_options_form_draw(struct sc_options_form *form,
                     const struct sc_launch_opts *opts) {
    werase(form->win);
    SC_BOX(form->win);
    mvwprintw(form->win, 0, 2, " Options ");
    sc_form_clamp_scroll(form);

    mvwprintw(form->win, 1, 2, "%-18s", "Profile");
    wattron(form->win, COLOR_PAIR(PAIR_BUTTON));
    mvwprintw(form->win, 1, 22, "[Load v]");
    mvwprintw(form->win, 1, 34, "[Save As]");
    mvwprintw(form->win, 1, 49, "[Delete]");
    wattroff(form->win, COLOR_PAIR(PAIR_BUTTON));
    if (form->profile_mode == SC_PROFILE_SAVE_AS) {
        mvwprintw(form->win, 2, 2, "Save profile name: %-32s", form->profile_name);
    } else if (form->profile_mode == SC_PROFILE_DELETE_CONFIRM) {
        mvwprintw(form->win, 2, 2, "Delete '%s'? [y/N]", sc_options_form_profile_name(form));
    } else if (form->profile_mode == SC_PROFILE_LOAD) {
        if (!form->profile_count) {
            mvwprintw(form->win, 2, 2, "No saved profiles");
        }
        for (int i = 0; i < form->profile_count && i < 6; ++i) {
            int pair = i == form->profile_selected ? PAIR_SELECTED : PAIR_NORMAL;
            wattron(form->win, COLOR_PAIR(pair));
            mvwprintw(form->win, 2 + i, 2, "%-32s", form->profiles[i]);
            wattroff(form->win, COLOR_PAIR(pair));
        }
    }

    int visible = form->rows - 3;
    int logical_row = 0;
    int drawn = 0;
    for (int i = 0; i < F_COUNT && drawn < visible; ++i) {
        enum sc_field_id id = (enum sc_field_id) i;
        if (!sc_field_visible(id)) {
            continue;
        }
        if (logical_row++ < form->scroll) {
            continue;
        }

        const struct sc_field_def *field = &sc_fields[i];
        int y = drawn + 4;
        if (y >= form->rows - 1) {
            break;
        }
        if (field->type == SC_FIELD_SECTION) {
            wattron(form->win, COLOR_PAIR(PAIR_HEADER));
            mvwprintw(form->win, y, 2, "%-*s", form->cols - 4, field->label);
            wattroff(form->win, COLOR_PAIR(PAIR_HEADER));
        } else {
            bool focused = form->focus == i;
            int pair = focused ? PAIR_SELECTED : PAIR_NORMAL;
            wattron(form->win, COLOR_PAIR(pair));
            char value[320] = "";
            if (field->type == SC_FIELD_CHECKBOX) {
                snprintf(value, sizeof(value), "[%c]", sc_checkbox_value(opts, id) ? 'x' : ' ');
            } else if (field->type == SC_FIELD_SELECT) {
                snprintf(value, sizeof(value), "< %s >", sc_select_value(opts, id));
            } else if (field->type == SC_FIELD_BUTTON) {
                snprintf(value, sizeof(value), "[ %s ]", field->label);
            } else {
                size_t cap;
                char *text = sc_field_text((struct sc_launch_opts *) opts, id, &cap);
                (void) cap;
                snprintf(value, sizeof(value), "%s", text ? text : "");
            }
            if (field->type == SC_FIELD_BUTTON) {
                mvwprintw(form->win, y, 2, "%-*s", form->cols - 4, value);
            } else {
                mvwprintw(form->win, y, 2, "%-18s %-*.*s", field->label,
                          form->cols - 24, form->cols - 24, value);
            }
            wattroff(form->win, COLOR_PAIR(pair));
        }
        ++drawn;
    }

    if (form->help[0]) {
        wattron(form->win, COLOR_PAIR(PAIR_STATUS));
        mvwprintw(form->win, form->rows - 2, 2, "%-*.*s", form->cols - 4,
                  form->cols - 4, form->help);
        wattroff(form->win, COLOR_PAIR(PAIR_STATUS));
    }
    wnoutrefresh(form->win);
}
