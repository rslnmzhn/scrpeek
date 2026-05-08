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
# include <unistd.h>
# define SC_MKDIR(path) mkdir(path, 0700)
# define SC_PATH_SEP "/"
#endif

#define SC_CONFIG_MAX_PATH 1024
#define SC_CONFIG_MAX_LINE 512

static const char *
sc_video_codec_value(enum sc_video_codec codec) {
    return codec == SC_VIDEO_CODEC_H265 ? "h265"
            : codec == SC_VIDEO_CODEC_AV1 ? "av1" : "h264";
}

static const char *
sc_connection_value(enum sc_connection_type connection) {
    return connection == SC_CONNECTION_TCPIP ? "tcpip" : "usb";
}

static const char *
sc_keyboard_value(enum sc_keyboard_mode mode) {
    return mode == SC_KEYBOARD_DISABLED ? "disabled"
            : mode == SC_KEYBOARD_UHID ? "uhid" : "sdk";
}

static bool
sc_mkdir_if_needed(const char *path) {
    if (SC_MKDIR(path) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static bool
sc_config_paths(char *dir, size_t dir_len, char *file, size_t file_len) {
#ifdef _WIN32
    const char *base = getenv("APPDATA");
    if (!base || !base[0]) {
        return false;
    }
    int d = snprintf(dir, dir_len, "%s%s%s", base, SC_PATH_SEP, "scrcpy-tui");
#else
    const char *base = getenv("XDG_CONFIG_HOME");
    char fallback[SC_CONFIG_MAX_PATH];
    if (!base || !base[0]) {
        const char *home = getenv("HOME");
        if (!home || !home[0]) {
            return false;
        }
        int f = snprintf(fallback, sizeof(fallback), "%s%s.config", home,
                         SC_PATH_SEP);
        if (f < 0 || (size_t) f >= sizeof(fallback)) {
            return false;
        }
        base = fallback;
    }
    int d = snprintf(dir, dir_len, "%s%s%s", base, SC_PATH_SEP, "scrcpy-tui");
#endif
    if (d < 0 || (size_t) d >= dir_len) {
        return false;
    }
    int p = snprintf(file, file_len, "%s%sprofiles.ini", dir, SC_PATH_SEP);
    return p >= 0 && (size_t) p < file_len;
}

bool
sc_config_profile_name_valid(const char *name) {
    if (!name || !name[0]) {
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

static void
sc_apply_key(struct sc_launch_opts *opts, const char *key, const char *value) {
    if (!strcmp(key, "max_size")) {
        snprintf(opts->max_size, sizeof(opts->max_size), "%s", value);
    } else if (!strcmp(key, "max_fps")) {
        snprintf(opts->max_fps, sizeof(opts->max_fps), "%s", value);
    } else if (!strcmp(key, "video_codec")) {
        opts->video_codec = !strcmp(value, "h265") ? SC_VIDEO_CODEC_H265
                : !strcmp(value, "av1") ? SC_VIDEO_CODEC_AV1 : SC_VIDEO_CODEC_H264;
    } else if (!strcmp(key, "no_audio")) {
        opts->no_audio = !strcmp(value, "1") || !strcmp(value, "true");
    } else if (!strcmp(key, "record_path")) {
        snprintf(opts->record_path, sizeof(opts->record_path), "%s", value);
    } else if (!strcmp(key, "connection")) {
        opts->connection = !strcmp(value, "tcpip") ? SC_CONNECTION_TCPIP
                : SC_CONNECTION_USB;
    } else if (!strcmp(key, "tcpip_addr")) {
        snprintf(opts->tcpip_addr, sizeof(opts->tcpip_addr), "%s", value);
    } else if (!strcmp(key, "keyboard_mode")) {
        opts->keyboard_mode = !strcmp(value, "disabled") ? SC_KEYBOARD_DISABLED
                : !strcmp(value, "uhid") ? SC_KEYBOARD_UHID : SC_KEYBOARD_SDK;
    } else if (!strcmp(key, "turn_screen_off")) {
        opts->turn_screen_off = !strcmp(value, "1") || !strcmp(value, "true");
    }
}

bool
sc_config_load(const char *name, struct sc_launch_opts *opts) {
    if (!sc_config_profile_name_valid(name)) {
        return false;
    }

    char dir[SC_CONFIG_MAX_PATH];
    char path[SC_CONFIG_MAX_PATH];
    if (!sc_config_paths(dir, sizeof(dir), path, sizeof(path))) {
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return false;
    }

    bool in_profile = false;
    char line[SC_CONFIG_MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        sc_strip_newline(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') {
            continue;
        }
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (!end) {
                in_profile = false;
                continue;
            }
            *end = '\0';
            in_profile = !strcmp(line + 1, name);
            continue;
        }
        if (!in_profile) {
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        sc_apply_key(opts, line, eq + 1);
    }

    fclose(fp);
    return true;
}

bool
sc_config_profile_exists(const char *name) {
    if (!sc_config_profile_name_valid(name)) {
        return false;
    }

    char dir[SC_CONFIG_MAX_PATH];
    char path[SC_CONFIG_MAX_PATH];
    if (!sc_config_paths(dir, sizeof(dir), path, sizeof(path))) {
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return false;
    }

    bool found = false;
    char line[SC_CONFIG_MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        sc_strip_newline(line);
        if (line[0] != '[') {
            continue;
        }
        char *end = strchr(line, ']');
        if (!end) {
            continue;
        }
        *end = '\0';
        if (!strcmp(line + 1, name)) {
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

static void
sc_write_profile(FILE *fp, const char *name, const struct sc_launch_opts *opts) {
    fprintf(fp, "[%s]\n", name);
    fprintf(fp, "max_size=%s\n", opts->max_size);
    fprintf(fp, "max_fps=%s\n", opts->max_fps);
    fprintf(fp, "video_codec=%s\n", sc_video_codec_value(opts->video_codec));
    fprintf(fp, "no_audio=%d\n", opts->no_audio ? 1 : 0);
    fprintf(fp, "record_path=%s\n", opts->record_path);
    fprintf(fp, "connection=%s\n", sc_connection_value(opts->connection));
    fprintf(fp, "tcpip_addr=%s\n", opts->tcpip_addr);
    fprintf(fp, "keyboard_mode=%s\n", sc_keyboard_value(opts->keyboard_mode));
    fprintf(fp, "turn_screen_off=%d\n\n", opts->turn_screen_off ? 1 : 0);
}

bool
sc_config_save(const char *name, const struct sc_launch_opts *opts) {
    if (!sc_config_profile_name_valid(name)) {
        return false;
    }

    char dir[SC_CONFIG_MAX_PATH];
    char path[SC_CONFIG_MAX_PATH];
    if (!sc_config_paths(dir, sizeof(dir), path, sizeof(path))
            || !sc_mkdir_if_needed(dir)) {
        return false;
    }

    char tmp[SC_CONFIG_MAX_PATH];
    int t = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (t < 0 || (size_t) t >= sizeof(tmp)) {
        return false;
    }

    FILE *out = fopen(tmp, "w");
    if (!out) {
        return false;
    }

    FILE *in = fopen(path, "r");
    bool skipping = false;
    bool wrote = false;
    char line[SC_CONFIG_MAX_LINE];
    if (in) {
        while (fgets(line, sizeof(line), in)) {
            char copy[SC_CONFIG_MAX_LINE];
            snprintf(copy, sizeof(copy), "%s", line);
            sc_strip_newline(copy);
            if (copy[0] == '[') {
                char *end = strchr(copy, ']');
                if (end) {
                    *end = '\0';
                    skipping = !strcmp(copy + 1, name);
                    if (skipping && !wrote) {
                        sc_write_profile(out, name, opts);
                        wrote = true;
                    }
                } else {
                    skipping = false;
                }
            }
            if (!skipping) {
                fputs(line, out);
            }
        }
        fclose(in);
    }

    if (!wrote) {
        sc_write_profile(out, name, opts);
    }

    bool ok = fclose(out) == 0;
    if (ok) {
        ok = rename(tmp, path) == 0;
    }
    if (!ok) {
        remove(tmp);
    }
    return ok;
}
