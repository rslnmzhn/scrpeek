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

#include <assert.h>
#include <string.h>

static void
assert_argv(const char *const actual[], const char *const expected[], int count) {
    for (int i = 0; i < count; ++i) {
        assert(actual[i]);
        assert(!strcmp(actual[i], expected[i]));
    }
    assert(actual[count] == NULL);
}

static void
test_default_opts(void) {
    struct sc_launch_opts opts;
    const char *argv[16];

    sc_launch_opts_init(&opts, "abc123");
    int count = sc_launch_opts_to_argv(&opts, argv, 16);

    const char *const expected[] = {"scrcpy", "--serial", "abc123"};
    assert(count == 3);
    assert_argv(argv, expected, count);
}

static void
test_full_opts(void) {
    struct sc_launch_opts opts;
    const char *argv[96];

    sc_launch_opts_init(&opts, "abc123");
    strcpy(opts.max_size, "1080");
    strcpy(opts.max_fps, "60");
    opts.video_codec = SC_CODEC3_SECOND;
    opts.video_source = SC_CHOICE_SECOND;
    strcpy(opts.display_id, "2");
    strcpy(opts.new_display, "1920x1080/420");
    strcpy(opts.crop, "100:200:0:0");
    strcpy(opts.lock_video_orientation, "@90");
    strcpy(opts.record_path, "out.mkv");
    strcpy(opts.record_format, "mkv");
    opts.no_audio = true;
    opts.audio_codec = SC_CODEC3_SECOND;
    opts.audio_source = SC_CODEC3_SECOND;
    strcpy(opts.audio_buffer, "80");
    opts.keyboard_mode = SC_CONTROL_UHID;
    opts.mouse_mode = SC_CONTROL_AOA;
    opts.gamepad = SC_GAMEPAD_UHID;
    opts.no_control = true;
    strcpy(opts.tcpip_addr, "192.168.1.10:5555");
    opts.turn_screen_off = true;
    opts.stay_awake = true;
    opts.show_touches = true;
    opts.power_off_on_close = true;
    opts.fullscreen = true;
    opts.always_on_top = true;
    strcpy(opts.window_title, "Phone");
    opts.no_window = true;
    opts.otg = true;
    strcpy(opts.v4l2_sink, "/dev/video2");
    opts.no_downsize_on_error = true;
    opts.verbosity = SC_VERBOSITY_DEBUG;

    int count = sc_launch_opts_to_argv(&opts, argv, 96);

    const char *const expected_common[] = {
        "scrcpy", "--serial", "abc123",
        "--max-size", "1080",
        "--max-fps", "60",
        "--video-codec", "h265",
        "--video-source", "camera",
        "--display-id", "2",
        "--new-display", "1920x1080/420",
        "--crop", "100:200:0:0",
        "--capture-orientation", "@90",
        "--record", "out.mkv",
        "--record-format", "mkv",
        "--no-audio",
        "--audio-codec", "aac",
        "--audio-source", "playback",
        "--audio-buffer", "80",
        "--keyboard", "uhid",
        "--mouse", "aoa",
        "--gamepad", "uhid",
        "--no-control",
        "--tcpip", "192.168.1.10:5555",
        "--turn-screen-off",
        "--stay-awake",
        "--show-touches",
        "--power-off-on-close",
        "--fullscreen",
        "--always-on-top",
        "--window-title", "Phone",
        "--no-window",
        "--otg",
#ifdef __linux__
        "--v4l2-sink", "/dev/video2",
#endif
        "--no-downsize-on-error",
        "--verbosity", "debug",
    };

    int expected_count = (int) (sizeof(expected_common) / sizeof(expected_common[0]));
    assert(count == expected_count);
    assert_argv(argv, expected_common, count);
}

int
main(void) {
    test_default_opts();
    test_full_opts();
    return 0;
}
