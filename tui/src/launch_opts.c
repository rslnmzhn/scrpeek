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

#include "launch_opts.h"

#include <string.h>

static const char *
sc_video_codec_name(enum sc_video_codec codec) {
    switch (codec) {
        case SC_VIDEO_CODEC_H264:
            return "h264";
        case SC_VIDEO_CODEC_H265:
            return "h265";
        case SC_VIDEO_CODEC_AV1:
            return "av1";
        default:
            return NULL;
    }
}

static const char *
sc_keyboard_mode_name(enum sc_keyboard_mode mode) {
    switch (mode) {
        case SC_KEYBOARD_DISABLED:
            return "disabled";
        case SC_KEYBOARD_SDK:
            return "sdk";
        case SC_KEYBOARD_UHID:
            return "uhid";
        default:
            return NULL;
    }
}

static bool
sc_argv_push(const char *argv[], size_t max, size_t *count, const char *arg) {
    if (*count + 1 >= max) {
        return false;
    }

    argv[(*count)++] = arg;
    argv[*count] = NULL;
    return true;
}

void
sc_launch_opts_init(struct sc_launch_opts *opts, const char *serial) {
    memset(opts, 0, sizeof(*opts));
    opts->serial = serial;
    opts->video_codec = SC_VIDEO_CODEC_H264;
    opts->connection = SC_CONNECTION_USB;
    opts->keyboard_mode = SC_KEYBOARD_SDK;
}

int
sc_launch_opts_to_argv(const struct sc_launch_opts *opts, const char *argv[],
                       size_t max) {
    size_t count = 0;

    if (!max) {
        return -1;
    }
    argv[0] = NULL;

    if (!sc_argv_push(argv, max, &count, "scrcpy")) {
        return -1;
    }

    if (opts->serial && opts->serial[0]) {
        if (!sc_argv_push(argv, max, &count, "--serial")
                || !sc_argv_push(argv, max, &count, opts->serial)) {
            return -1;
        }
    }

    if (opts->max_size[0] && strcmp(opts->max_size, "0")) {
        if (!sc_argv_push(argv, max, &count, "--max-size")
                || !sc_argv_push(argv, max, &count, opts->max_size)) {
            return -1;
        }
    }

    if (opts->max_fps[0] && strcmp(opts->max_fps, "0")) {
        if (!sc_argv_push(argv, max, &count, "--max-fps")
                || !sc_argv_push(argv, max, &count, opts->max_fps)) {
            return -1;
        }
    }

    if (opts->video_codec != SC_VIDEO_CODEC_H264) {
        const char *codec = sc_video_codec_name(opts->video_codec);
        if (!codec || !sc_argv_push(argv, max, &count, "--video-codec")
                || !sc_argv_push(argv, max, &count, codec)) {
            return -1;
        }
    }

    if (opts->no_audio && !sc_argv_push(argv, max, &count, "--no-audio")) {
        return -1;
    }

    if (opts->record_path[0]) {
        if (!sc_argv_push(argv, max, &count, "--record")
                || !sc_argv_push(argv, max, &count, opts->record_path)) {
            return -1;
        }
    }

    if (opts->connection == SC_CONNECTION_TCPIP) {
        if (!sc_argv_push(argv, max, &count, "--tcpip")) {
            return -1;
        }
        if (opts->tcpip_addr[0]
                && !sc_argv_push(argv, max, &count, opts->tcpip_addr)) {
            return -1;
        }
    }

    if (opts->keyboard_mode != SC_KEYBOARD_SDK) {
        const char *mode = sc_keyboard_mode_name(opts->keyboard_mode);
        if (!mode || !sc_argv_push(argv, max, &count, "--keyboard")
                || !sc_argv_push(argv, max, &count, mode)) {
            return -1;
        }
    }

    if (opts->turn_screen_off
            && !sc_argv_push(argv, max, &count, "--turn-screen-off")) {
        return -1;
    }

    return (int) count;
}
