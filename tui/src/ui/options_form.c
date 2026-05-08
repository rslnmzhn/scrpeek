#include "ui/options_form.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ui/layout.h"

enum sc_options_field {
    SC_FIELD_PROFILE_NAME,
    SC_FIELD_SAVE_PROFILE,
    SC_FIELD_LOAD_PROFILE,
    SC_FIELD_MAX_SIZE,
    SC_FIELD_MAX_FPS,
    SC_FIELD_VIDEO_CODEC,
    SC_FIELD_NO_AUDIO,
    SC_FIELD_RECORD_PATH,
    SC_FIELD_CONNECTION,
    SC_FIELD_TCPIP_ADDR,
    SC_FIELD_KEYBOARD_MODE,
    SC_FIELD_TURN_SCREEN_OFF,
    SC_FIELD_LAUNCH,
    SC_FIELD_COUNT,
};

static const char *const sc_video_codec_names[] = {"h264", "h265", "av1"};
static const char *const sc_connection_names[] = {"USB", "TCPIP"};
static const char *const sc_keyboard_mode_names[] = {"disabled", "sdk", "uhid"};

void
sc_options_form_set_error(struct sc_options_form *form, const char *error) {
    snprintf(form->error, sizeof(form->error), "%s", error ? error : "");
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
sc_text_append(char *text, size_t cap, int ch) {
    size_t len = strlen(text);
    if (len + 1 >= cap) {
        return false;
    }

    text[len] = (char) ch;
    text[len + 1] = '\0';
    return true;
}

static bool
sc_numeric_key(char *text, size_t cap, int key) {
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        (void) sc_text_backspace(text);
        return true;
    }

    if (!isdigit((unsigned char) key)) {
        return false;
    }

    return sc_text_append(text, cap, key);
}

static bool
sc_profile_key(char *text, size_t cap, int key) {
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        (void) sc_text_backspace(text);
        return true;
    }

    unsigned char c = (unsigned char) key;
    if (!isalnum(c) && c != '-' && c != '_') {
        return false;
    }

    return sc_text_append(text, cap, key);
}

static bool
sc_path_key(char *text, size_t cap, int key) {
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        (void) sc_text_backspace(text);
        return true;
    }

    if (key < 32 || key > 126) {
        return false;
    }

    return sc_text_append(text, cap, key);
}

static void
sc_options_form_next(struct sc_options_form *form) {
    form->field = (form->field + 1) % SC_FIELD_COUNT;
}

static void
sc_options_form_prev(struct sc_options_form *form) {
    form->field = form->field ? form->field - 1 : SC_FIELD_COUNT - 1;
}

static void
sc_options_form_toggle(struct sc_options_form *form, struct sc_launch_opts *opts) {
    switch (form->field) {
        case SC_FIELD_VIDEO_CODEC:
            opts->video_codec = (opts->video_codec + 1) % 3;
            break;
        case SC_FIELD_NO_AUDIO:
            opts->no_audio = !opts->no_audio;
            break;
        case SC_FIELD_CONNECTION:
            opts->connection = opts->connection == SC_CONNECTION_USB
                    ? SC_CONNECTION_TCPIP : SC_CONNECTION_USB;
            break;
        case SC_FIELD_KEYBOARD_MODE:
            opts->keyboard_mode = (opts->keyboard_mode + 1) % 3;
            break;
        case SC_FIELD_TURN_SCREEN_OFF:
            opts->turn_screen_off = !opts->turn_screen_off;
            break;
        default:
            break;
    }
}

static void
sc_options_form_draw_field(struct sc_options_form *form, int row,
                           enum sc_options_field field, const char *label,
                           const char *value) {
    bool selected = form->field == (size_t) field;
    int pair = selected ? PAIR_SELECTED : PAIR_NORMAL;
    wattron(form->win, COLOR_PAIR(pair));
    mvwprintw(form->win, row, 2, "%-18s %-*.*s", label, form->cols - 24,
              form->cols - 24, value);
    wattroff(form->win, COLOR_PAIR(pair));
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
    form->field = 0;
    form->error[0] = '\0';
    form->profile_name[0] = '\0';
    keypad(form->win, TRUE);
    return true;
}

void
sc_options_form_destroy(struct sc_options_form *form) {
    if (form->win) {
        delwin(form->win);
        form->win = NULL;
    }
}

bool
sc_options_form_resize(struct sc_options_form *form, int y, int x, int rows,
                       int cols) {
    if (wresize(form->win, rows, cols) == ERR) {
        return false;
    }
    if (mvwin(form->win, y, x) == ERR) {
        return false;
    }

    form->y = y;
    form->x = x;
    form->rows = rows;
    form->cols = cols;
    return true;
}

enum sc_options_form_action
sc_options_form_handle_key(struct sc_options_form *form, int key,
                           struct sc_launch_opts *opts) {
    if (key == ERR) {
        return SC_OPTIONS_FORM_NONE;
    }

    sc_options_form_set_error(form, NULL);

    switch (key) {
        case 27:
            return SC_OPTIONS_FORM_BACK;
        case '\t':
        case KEY_DOWN:
            sc_options_form_next(form);
            return SC_OPTIONS_FORM_NONE;
#ifdef KEY_BTAB
        case KEY_BTAB:
#endif
        case KEY_UP:
            sc_options_form_prev(form);
            return SC_OPTIONS_FORM_NONE;
        case ' ':
            sc_options_form_toggle(form, opts);
            return SC_OPTIONS_FORM_NONE;
        case '\n':
        case '\r':
        case KEY_ENTER:
            if (form->field == SC_FIELD_LAUNCH) {
                return SC_OPTIONS_FORM_LAUNCH;
            }
            if (form->field == SC_FIELD_SAVE_PROFILE) {
                return SC_OPTIONS_FORM_SAVE_PROFILE;
            }
            if (form->field == SC_FIELD_LOAD_PROFILE) {
                return SC_OPTIONS_FORM_LOAD_PROFILE;
            }
            sc_options_form_toggle(form, opts);
            return SC_OPTIONS_FORM_NONE;
        default:
            break;
    }

    bool accepted = true;
    switch (form->field) {
        case SC_FIELD_PROFILE_NAME:
            accepted = sc_profile_key(form->profile_name,
                                      sizeof(form->profile_name), key);
            break;
        case SC_FIELD_MAX_SIZE:
            accepted = sc_numeric_key(opts->max_size, sizeof(opts->max_size), key);
            break;
        case SC_FIELD_MAX_FPS:
            accepted = sc_numeric_key(opts->max_fps, sizeof(opts->max_fps), key);
            break;
        case SC_FIELD_RECORD_PATH:
            accepted = sc_path_key(opts->record_path, sizeof(opts->record_path), key);
            break;
        case SC_FIELD_TCPIP_ADDR:
            accepted = sc_path_key(opts->tcpip_addr, sizeof(opts->tcpip_addr), key);
            break;
        default:
            accepted = true;
            break;
    }

    if (!accepted) {
        sc_options_form_set_error(form, form->field == SC_FIELD_PROFILE_NAME
                                  ? "profile: use letters, numbers, dash or underscore"
                                  : "invalid input for field");
    }
    return SC_OPTIONS_FORM_NONE;
}

void
sc_options_form_draw(struct sc_options_form *form,
                     const struct sc_launch_opts *opts) {
    werase(form->win);
    wattron(form->win, COLOR_PAIR(PAIR_NORMAL));
    box(form->win, 0, 0);
    mvwprintw(form->win, 0, 2, " Options ");

    if (form->cols < 30 || form->rows < 17) {
        mvwprintw(form->win, 1, 2, "Panel too small");
        wattroff(form->win, COLOR_PAIR(PAIR_NORMAL));
        wnoutrefresh(form->win);
        return;
    }

    char value[320];
    sc_options_form_draw_field(form, 2, SC_FIELD_PROFILE_NAME, "Profile",
                               form->profile_name[0] ? form->profile_name : "-");
    sc_options_form_draw_field(form, 3, SC_FIELD_SAVE_PROFILE, "", "[Save As...] ");
    sc_options_form_draw_field(form, 4, SC_FIELD_LOAD_PROFILE, "", "[Load]");
    snprintf(value, sizeof(value), "%s", opts->max_size[0] ? opts->max_size : "0");
    sc_options_form_draw_field(form, 6, SC_FIELD_MAX_SIZE, "Max size", value);
    snprintf(value, sizeof(value), "%s", opts->max_fps[0] ? opts->max_fps : "0");
    sc_options_form_draw_field(form, 7, SC_FIELD_MAX_FPS, "Max fps", value);
    sc_options_form_draw_field(form, 8, SC_FIELD_VIDEO_CODEC, "Video codec",
                               sc_video_codec_names[opts->video_codec]);
    sc_options_form_draw_field(form, 9, SC_FIELD_NO_AUDIO, "No audio",
                               opts->no_audio ? "[x]" : "[ ]");
    sc_options_form_draw_field(form, 10, SC_FIELD_RECORD_PATH, "Record to file",
                               opts->record_path[0] ? opts->record_path : "-");
    sc_options_form_draw_field(form, 11, SC_FIELD_CONNECTION, "Connection",
                               sc_connection_names[opts->connection]);
    sc_options_form_draw_field(form, 12, SC_FIELD_TCPIP_ADDR, "IP:port",
                               opts->tcpip_addr[0] ? opts->tcpip_addr : "-");
    sc_options_form_draw_field(form, 13, SC_FIELD_KEYBOARD_MODE, "Keyboard mode",
                               sc_keyboard_mode_names[opts->keyboard_mode]);
    sc_options_form_draw_field(form, 14, SC_FIELD_TURN_SCREEN_OFF,
                               "Turn screen off", opts->turn_screen_off ? "[x]" : "[ ]");
    sc_options_form_draw_field(form, 16, SC_FIELD_LAUNCH, "", "Launch");

    if (form->error[0]) {
        wattron(form->win, COLOR_PAIR(PAIR_STATUS));
        mvwprintw(form->win, form->rows - 2, 2, "%.*s", form->cols - 4,
                  form->error);
        wattroff(form->win, COLOR_PAIR(PAIR_STATUS));
    }

    wattroff(form->win, COLOR_PAIR(PAIR_NORMAL));
    wnoutrefresh(form->win);
}
