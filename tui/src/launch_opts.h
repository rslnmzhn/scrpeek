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

#ifndef SC_TUI_LAUNCH_OPTS_H
#define SC_TUI_LAUNCH_OPTS_H

#include <stdbool.h>
#include <stddef.h>

#define SC_OPTS_SHORT_LEN 32
#define SC_OPTS_TEXT_LEN 256
#define SC_OPTS_SERIAL_LEN 64

enum sc_codec3 {
    SC_CODEC3_FIRST,
    SC_CODEC3_SECOND,
    SC_CODEC3_THIRD,
};

enum sc_binary_choice {
    SC_CHOICE_FIRST,
    SC_CHOICE_SECOND,
};

enum sc_control_mode {
    SC_CONTROL_DISABLED,
    SC_CONTROL_SDK,
    SC_CONTROL_UHID,
    SC_CONTROL_AOA,
};

enum sc_gamepad_mode {
    SC_GAMEPAD_DISABLED,
    SC_GAMEPAD_UHID,
    SC_GAMEPAD_AOA,
};

enum sc_verbosity {
    SC_VERBOSITY_INFO,
    SC_VERBOSITY_VERBOSE,
    SC_VERBOSITY_DEBUG,
    SC_VERBOSITY_WARN,
    SC_VERBOSITY_ERROR,
};

struct sc_launch_opts {
    char serial[SC_OPTS_SERIAL_LEN];
    char max_size[SC_OPTS_SHORT_LEN];
    char max_fps[SC_OPTS_SHORT_LEN];
    enum sc_codec3 video_codec;
    enum sc_binary_choice video_source;
    char display_id[SC_OPTS_SHORT_LEN];
    char new_display[SC_OPTS_TEXT_LEN];
    char crop[SC_OPTS_TEXT_LEN];
    char lock_video_orientation[SC_OPTS_SHORT_LEN];
    char record_path[SC_OPTS_TEXT_LEN];
    char record_format[SC_OPTS_SHORT_LEN];
    bool no_audio;
    enum sc_codec3 audio_codec;
    enum sc_codec3 audio_source;
    char audio_buffer[SC_OPTS_SHORT_LEN];
    enum sc_control_mode keyboard_mode;
    enum sc_control_mode mouse_mode;
    enum sc_gamepad_mode gamepad;
    bool no_control;
    char tcpip_addr[SC_OPTS_TEXT_LEN];
    bool turn_screen_off;
    bool stay_awake;
    bool show_touches;
    bool power_off_on_close;
    bool fullscreen;
    bool always_on_top;
    char window_title[SC_OPTS_TEXT_LEN];
    bool no_window;
    bool otg;
    char v4l2_sink[SC_OPTS_TEXT_LEN];
    bool no_downsize_on_error;
    enum sc_verbosity verbosity;
};

void
sc_launch_opts_init(struct sc_launch_opts *opts, const char *serial);

int
sc_launch_opts_to_argv(const struct sc_launch_opts *opts, const char *argv[],
                       size_t max);

#endif
