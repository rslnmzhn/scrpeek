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

#include <stdio.h>
#include <string.h>

static const char *const sc_video_codec_names[] = {"h264", "h265", "av1"};
static const char *const sc_video_source_names[] = {"display", "camera"};
static const char *const sc_audio_codec_names[] = {"opus", "aac", "flac"};
static const char *const sc_audio_source_names[] = {"output", "playback", "mic"};
static const char *const sc_control_mode_names[] = {"disabled", "sdk", "uhid", "aoa"};
static const char *const sc_gamepad_mode_names[] = {"disabled", "uhid", "aoa"};
static const char *const sc_verbosity_names[] = {"info", "verbose", "debug", "warn", "error"};

static bool
sc_argv_push(const char *argv[], size_t max, size_t *count, const char *arg) {
    if (*count + 1 >= max) {
        return false;
    }

    argv[(*count)++] = arg;
    argv[*count] = NULL;
    return true;
}

static bool
sc_argv_push_pair(const char *argv[], size_t max, size_t *count,
                  const char *flag, const char *value) {
    return sc_argv_push(argv, max, count, flag)
            && sc_argv_push(argv, max, count, value);
}

void
sc_launch_opts_init(struct sc_launch_opts *opts, const char *serial) {
    memset(opts, 0, sizeof(*opts));
    if (serial) {
        snprintf(opts->serial, sizeof(opts->serial), "%s", serial);
    }
    opts->video_codec = SC_CODEC3_FIRST;
    opts->video_source = SC_CHOICE_FIRST;
    opts->audio_codec = SC_CODEC3_FIRST;
    opts->audio_source = SC_CODEC3_FIRST;
    opts->keyboard_mode = SC_CONTROL_SDK;
    opts->mouse_mode = SC_CONTROL_SDK;
    opts->gamepad = SC_GAMEPAD_DISABLED;
    opts->verbosity = SC_VERBOSITY_INFO;
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
    if (opts->serial[0]
            && !sc_argv_push_pair(argv, max, &count, "--serial", opts->serial)) {
        return -1;
    }
    if (opts->max_size[0]
            && !sc_argv_push_pair(argv, max, &count, "--max-size", opts->max_size)) {
        return -1;
    }
    if (opts->max_fps[0]
            && !sc_argv_push_pair(argv, max, &count, "--max-fps", opts->max_fps)) {
        return -1;
    }
    if (opts->video_codec != SC_CODEC3_FIRST
            && !sc_argv_push_pair(argv, max, &count, "--video-codec",
                                  sc_video_codec_names[opts->video_codec])) {
        return -1;
    }
    if (opts->video_source != SC_CHOICE_FIRST
            && !sc_argv_push_pair(argv, max, &count, "--video-source",
                                  sc_video_source_names[opts->video_source])) {
        return -1;
    }
    if (opts->display_id[0]
            && !sc_argv_push_pair(argv, max, &count, "--display-id", opts->display_id)) {
        return -1;
    }
    if (opts->new_display[0]) {
        if (!strcmp(opts->new_display, "1")) {
            if (!sc_argv_push(argv, max, &count, "--new-display")) {
                return -1;
            }
        } else if (!sc_argv_push_pair(argv, max, &count, "--new-display",
                                      opts->new_display)) {
            return -1;
        }
    }
    if (opts->crop[0]
            && !sc_argv_push_pair(argv, max, &count, "--crop", opts->crop)) {
        return -1;
    }
    if (opts->lock_video_orientation[0]
            && !sc_argv_push_pair(argv, max, &count, "--capture-orientation",
                                  opts->lock_video_orientation)) {
        return -1;
    }
    if (opts->record_path[0]
            && !sc_argv_push_pair(argv, max, &count, "--record", opts->record_path)) {
        return -1;
    }
    if (opts->record_format[0]
            && !sc_argv_push_pair(argv, max, &count, "--record-format",
                                  opts->record_format)) {
        return -1;
    }
    if (opts->no_audio && !sc_argv_push(argv, max, &count, "--no-audio")) {
        return -1;
    }
    if (opts->audio_codec != SC_CODEC3_FIRST
            && !sc_argv_push_pair(argv, max, &count, "--audio-codec",
                                  sc_audio_codec_names[opts->audio_codec])) {
        return -1;
    }
    if (opts->audio_source != SC_CODEC3_FIRST
            && !sc_argv_push_pair(argv, max, &count, "--audio-source",
                                  sc_audio_source_names[opts->audio_source])) {
        return -1;
    }
    if (opts->audio_buffer[0]
            && !sc_argv_push_pair(argv, max, &count, "--audio-buffer",
                                  opts->audio_buffer)) {
        return -1;
    }
    if (opts->keyboard_mode != SC_CONTROL_SDK
            && !sc_argv_push_pair(argv, max, &count, "--keyboard",
                                  sc_control_mode_names[opts->keyboard_mode])) {
        return -1;
    }
    if (opts->mouse_mode != SC_CONTROL_SDK
            && !sc_argv_push_pair(argv, max, &count, "--mouse",
                                  sc_control_mode_names[opts->mouse_mode])) {
        return -1;
    }
    if (opts->gamepad != SC_GAMEPAD_DISABLED
            && !sc_argv_push_pair(argv, max, &count, "--gamepad",
                                  sc_gamepad_mode_names[opts->gamepad])) {
        return -1;
    }
    if (opts->no_control && !sc_argv_push(argv, max, &count, "--no-control")) {
        return -1;
    }
    if (opts->tcpip_addr[0]) {
        if (!sc_argv_push(argv, max, &count, "--tcpip")) {
            return -1;
        }
        if (strcmp(opts->tcpip_addr, "1")
                && !sc_argv_push(argv, max, &count, opts->tcpip_addr)) {
            return -1;
        }
    }
    if (opts->turn_screen_off
            && !sc_argv_push(argv, max, &count, "--turn-screen-off")) {
        return -1;
    }
    if (opts->stay_awake && !sc_argv_push(argv, max, &count, "--stay-awake")) {
        return -1;
    }
    if (opts->show_touches && !sc_argv_push(argv, max, &count, "--show-touches")) {
        return -1;
    }
    if (opts->power_off_on_close
            && !sc_argv_push(argv, max, &count, "--power-off-on-close")) {
        return -1;
    }
    if (opts->fullscreen && !sc_argv_push(argv, max, &count, "--fullscreen")) {
        return -1;
    }
    if (opts->always_on_top && !sc_argv_push(argv, max, &count, "--always-on-top")) {
        return -1;
    }
    if (opts->window_title[0]
            && !sc_argv_push_pair(argv, max, &count, "--window-title",
                                  opts->window_title)) {
        return -1;
    }
    if (opts->no_window && !sc_argv_push(argv, max, &count, "--no-window")) {
        return -1;
    }
    if (opts->otg && !sc_argv_push(argv, max, &count, "--otg")) {
        return -1;
    }
#ifdef __linux__
    if (opts->v4l2_sink[0]
            && !sc_argv_push_pair(argv, max, &count, "--v4l2-sink",
                                  opts->v4l2_sink)) {
        return -1;
    }
#endif
    if (opts->no_downsize_on_error
            && !sc_argv_push(argv, max, &count, "--no-downsize-on-error")) {
        return -1;
    }
    if (opts->verbosity != SC_VERBOSITY_INFO
            && !sc_argv_push_pair(argv, max, &count, "--verbosity",
                                  sc_verbosity_names[opts->verbosity])) {
        return -1;
    }

    return (int) count;
}
