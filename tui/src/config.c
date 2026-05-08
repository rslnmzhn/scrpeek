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

#ifndef _WIN32
# define _POSIX_C_SOURCE 200809L
#endif

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
# include <direct.h>
# define SC_MKDIR(path) _mkdir(path)
# define SC_PATH_SEP "\\"
#else
# define SC_MKDIR(path) mkdir(path, 0700)
# define SC_PATH_SEP "/"
#endif

#define SC_CONFIG_MAX_PATH 1024
#define SC_CONFIG_MAX_LINE 768

static const char *const codec3_names[] = {"h264", "h265", "av1"};
static const char *const audio_codec_names[] = {"opus", "aac", "flac"};
static const char *const audio_source_names[] = {"output", "playback", "mic"};
static const char *const video_source_names[] = {"display", "camera"};
static const char *const control_names[] = {"disabled", "sdk", "uhid", "aoa"};
static const char *const gamepad_names[] = {"disabled", "uhid", "aoa"};
static const char *const verbosity_names[] = {"info", "verbose", "debug", "warn", "error"};

static char *
sc_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static int
sc_name_index(const char *value, const char *const names[], int count) {
    for (int i = 0; i < count; ++i) {
        if (!strcmp(value, names[i])) {
            return i;
        }
    }
    return 0;
}

static bool
sc_config_paths(char *dir, size_t dir_len, char *file, size_t file_len) {
#ifdef _WIN32
    const char *base = getenv("APPDATA");
#else
    const char *base = getenv("XDG_CONFIG_HOME");
    char fallback[SC_CONFIG_MAX_PATH];
    if (!base || !base[0]) {
        const char *home = getenv("HOME");
        if (!home || !home[0]) {
            return false;
        }
        if (snprintf(fallback, sizeof(fallback), "%s%s.config", home, SC_PATH_SEP) < 0) {
            return false;
        }
        base = fallback;
    }
#endif
    if (!base || !base[0]) {
        return false;
    }
    int d = snprintf(dir, dir_len, "%s%s%s", base, SC_PATH_SEP, "scrcpy-tui");
    if (d < 0 || (size_t) d >= dir_len) {
        return false;
    }
    int p = snprintf(file, file_len, "%s%sprofiles.ini", dir, SC_PATH_SEP);
    return p >= 0 && (size_t) p < file_len;
}

bool
sc_config_profile_name_valid(const char *name) {
    if (!name || !name[0] || strlen(name) > 32) {
        return false;
    }
    for (const char *p = name; *p; ++p) {
        unsigned char c = (unsigned char) *p;
        if (!isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

static void
sc_strip_newline(char *line) {
    size_t len = strlen(line);
    while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

static bool
sc_bool(const char *value) {
    return !strcmp(value, "1") || !strcmp(value, "true");
}

static void
sc_copy(char *dst, size_t dst_len, const char *src) {
    snprintf(dst, dst_len, "%s", src);
}

static void
sc_apply_key(struct sc_launch_opts *opts, const char *key, const char *value) {
    if (!strcmp(key, "max_size")) sc_copy(opts->max_size, sizeof(opts->max_size), value);
    else if (!strcmp(key, "max_fps")) sc_copy(opts->max_fps, sizeof(opts->max_fps), value);
    else if (!strcmp(key, "video_codec")) opts->video_codec = sc_name_index(value, codec3_names, 3);
    else if (!strcmp(key, "video_source")) opts->video_source = sc_name_index(value, video_source_names, 2);
    else if (!strcmp(key, "display_id")) sc_copy(opts->display_id, sizeof(opts->display_id), value);
    else if (!strcmp(key, "new_display")) sc_copy(opts->new_display, sizeof(opts->new_display), value);
    else if (!strcmp(key, "crop")) sc_copy(opts->crop, sizeof(opts->crop), value);
    else if (!strcmp(key, "lock_video_orientation")) sc_copy(opts->lock_video_orientation, sizeof(opts->lock_video_orientation), value);
    else if (!strcmp(key, "record_path")) sc_copy(opts->record_path, sizeof(opts->record_path), value);
    else if (!strcmp(key, "record_format")) sc_copy(opts->record_format, sizeof(opts->record_format), value);
    else if (!strcmp(key, "no_audio")) opts->no_audio = sc_bool(value);
    else if (!strcmp(key, "audio_codec")) opts->audio_codec = sc_name_index(value, audio_codec_names, 3);
    else if (!strcmp(key, "audio_source")) opts->audio_source = sc_name_index(value, audio_source_names, 3);
    else if (!strcmp(key, "audio_buffer")) sc_copy(opts->audio_buffer, sizeof(opts->audio_buffer), value);
    else if (!strcmp(key, "keyboard_mode")) opts->keyboard_mode = sc_name_index(value, control_names, 4);
    else if (!strcmp(key, "mouse_mode")) opts->mouse_mode = sc_name_index(value, control_names, 4);
    else if (!strcmp(key, "gamepad")) opts->gamepad = sc_name_index(value, gamepad_names, 3);
    else if (!strcmp(key, "no_control")) opts->no_control = sc_bool(value);
    else if (!strcmp(key, "tcpip_addr")) sc_copy(opts->tcpip_addr, sizeof(opts->tcpip_addr), value);
    else if (!strcmp(key, "turn_screen_off")) opts->turn_screen_off = sc_bool(value);
    else if (!strcmp(key, "stay_awake")) opts->stay_awake = sc_bool(value);
    else if (!strcmp(key, "show_touches")) opts->show_touches = sc_bool(value);
    else if (!strcmp(key, "power_off_on_close")) opts->power_off_on_close = sc_bool(value);
    else if (!strcmp(key, "fullscreen")) opts->fullscreen = sc_bool(value);
    else if (!strcmp(key, "always_on_top")) opts->always_on_top = sc_bool(value);
    else if (!strcmp(key, "window_title")) sc_copy(opts->window_title, sizeof(opts->window_title), value);
    else if (!strcmp(key, "no_window")) opts->no_window = sc_bool(value);
    else if (!strcmp(key, "otg")) opts->otg = sc_bool(value);
    else if (!strcmp(key, "v4l2_sink")) sc_copy(opts->v4l2_sink, sizeof(opts->v4l2_sink), value);
    else if (!strcmp(key, "no_downsize_on_error")) opts->no_downsize_on_error = sc_bool(value);
    else if (!strcmp(key, "verbosity")) opts->verbosity = sc_name_index(value, verbosity_names, 5);
}

static bool
sc_section_name(char *line, char *name, size_t name_len) {
    if (line[0] != '[') return false;
    char *end = strchr(line, ']');
    if (!end) return false;
    *end = '\0';
    sc_copy(name, name_len, line + 1);
    return true;
}

bool
sc_config_load(const char *name, struct sc_launch_opts *opts) {
    if (!sc_config_profile_name_valid(name)) return false;
    char dir[SC_CONFIG_MAX_PATH], path[SC_CONFIG_MAX_PATH];
    if (!sc_config_paths(dir, sizeof(dir), path, sizeof(path))) return false;
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    bool in_profile = false;
    char line[SC_CONFIG_MAX_LINE], section[SC_CONFIG_PROFILE_NAME_LEN];
    while (fgets(line, sizeof(line), fp)) {
        sc_strip_newline(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') continue;
        if (sc_section_name(line, section, sizeof(section))) {
            in_profile = !strcmp(section, name);
            continue;
        }
        if (!in_profile) continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        sc_apply_key(opts, line, eq + 1);
    }
    fclose(fp);
    return true;
}

int
sc_config_list_profiles(char **names_out, int max) {
    if (max <= 0) return 0;
    char dir[SC_CONFIG_MAX_PATH], path[SC_CONFIG_MAX_PATH];
    if (!sc_config_paths(dir, sizeof(dir), path, sizeof(path))) return 0;
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    int count = 0;
    char line[SC_CONFIG_MAX_LINE], section[SC_CONFIG_PROFILE_NAME_LEN];
    while (count < max && fgets(line, sizeof(line), fp)) {
        sc_strip_newline(line);
        if (sc_section_name(line, section, sizeof(section))
                && sc_config_profile_name_valid(section)) {
            names_out[count] = sc_strdup(section);
            if (names_out[count]) ++count;
        }
    }
    fclose(fp);
    return count;
}

static void
sc_write_profile(FILE *fp, const char *name, const struct sc_launch_opts *opts) {
    fprintf(fp, "[%s]\n", name);
    fprintf(fp, "max_size=%s\nmax_fps=%s\n", opts->max_size, opts->max_fps);
    fprintf(fp, "video_codec=%s\nvideo_source=%s\n", codec3_names[opts->video_codec], video_source_names[opts->video_source]);
    fprintf(fp, "display_id=%s\nnew_display=%s\ncrop=%s\nlock_video_orientation=%s\n", opts->display_id, opts->new_display, opts->crop, opts->lock_video_orientation);
    fprintf(fp, "record_path=%s\nrecord_format=%s\n", opts->record_path, opts->record_format);
    fprintf(fp, "no_audio=%d\naudio_codec=%s\naudio_source=%s\naudio_buffer=%s\n", opts->no_audio, audio_codec_names[opts->audio_codec], audio_source_names[opts->audio_source], opts->audio_buffer);
    fprintf(fp, "keyboard_mode=%s\nmouse_mode=%s\ngamepad=%s\nno_control=%d\n", control_names[opts->keyboard_mode], control_names[opts->mouse_mode], gamepad_names[opts->gamepad], opts->no_control);
    fprintf(fp, "tcpip_addr=%s\nturn_screen_off=%d\nstay_awake=%d\nshow_touches=%d\npower_off_on_close=%d\n", opts->tcpip_addr, opts->turn_screen_off, opts->stay_awake, opts->show_touches, opts->power_off_on_close);
    fprintf(fp, "fullscreen=%d\nalways_on_top=%d\nwindow_title=%s\nno_window=%d\n", opts->fullscreen, opts->always_on_top, opts->window_title, opts->no_window);
    fprintf(fp, "otg=%d\nv4l2_sink=%s\nno_downsize_on_error=%d\nverbosity=%s\n\n", opts->otg, opts->v4l2_sink, opts->no_downsize_on_error, verbosity_names[opts->verbosity]);
}

static bool
sc_rewrite(const char *name, const struct sc_launch_opts *opts, bool delete_only) {
    char dir[SC_CONFIG_MAX_PATH], path[SC_CONFIG_MAX_PATH], tmp[SC_CONFIG_MAX_PATH];
    if (!sc_config_paths(dir, sizeof(dir), path, sizeof(path))) return false;
    if (!delete_only && SC_MKDIR(dir) != 0 && errno != EEXIST) return false;
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) < 0) return false;
    FILE *out = fopen(tmp, "w");
    if (!out) return false;
    FILE *in = fopen(path, "r");
    bool skipping = false, wrote = false;
    char line[SC_CONFIG_MAX_LINE], copy[SC_CONFIG_MAX_LINE], section[SC_CONFIG_PROFILE_NAME_LEN];
    if (in) {
        while (fgets(line, sizeof(line), in)) {
            snprintf(copy, sizeof(copy), "%s", line);
            sc_strip_newline(copy);
            if (sc_section_name(copy, section, sizeof(section))) {
                skipping = !strcmp(section, name);
                if (skipping && !delete_only && !wrote) {
                    sc_write_profile(out, name, opts);
                    wrote = true;
                }
            }
            if (!skipping) fputs(line, out);
        }
        fclose(in);
    }
    if (!delete_only && !wrote) sc_write_profile(out, name, opts);
    bool ok = fclose(out) == 0 && rename(tmp, path) == 0;
    if (!ok) remove(tmp);
    return ok;
}

bool
sc_config_save(const char *name, const struct sc_launch_opts *opts) {
    return sc_config_profile_name_valid(name) && sc_rewrite(name, opts, false);
}

bool
sc_config_delete(const char *name) {
    return sc_config_profile_name_valid(name) && sc_rewrite(name, NULL, true);
}
