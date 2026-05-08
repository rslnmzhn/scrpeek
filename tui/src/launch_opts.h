#ifndef SC_TUI_LAUNCH_OPTS_H
#define SC_TUI_LAUNCH_OPTS_H

#include <stdbool.h>
#include <stddef.h>

#define SC_LAUNCH_OPTS_NUMERIC_LEN 16
#define SC_LAUNCH_OPTS_PATH_LEN 256
#define SC_LAUNCH_OPTS_TCPIP_LEN 64

enum sc_video_codec {
    SC_VIDEO_CODEC_H264,
    SC_VIDEO_CODEC_H265,
    SC_VIDEO_CODEC_AV1,
};

enum sc_connection_type {
    SC_CONNECTION_USB,
    SC_CONNECTION_TCPIP,
};

enum sc_keyboard_mode {
    SC_KEYBOARD_DISABLED,
    SC_KEYBOARD_SDK,
    SC_KEYBOARD_UHID,
};

struct sc_launch_opts {
    const char *serial;
    char max_size[SC_LAUNCH_OPTS_NUMERIC_LEN];
    char max_fps[SC_LAUNCH_OPTS_NUMERIC_LEN];
    enum sc_video_codec video_codec;
    bool no_audio;
    char record_path[SC_LAUNCH_OPTS_PATH_LEN];
    enum sc_connection_type connection;
    char tcpip_addr[SC_LAUNCH_OPTS_TCPIP_LEN];
    enum sc_keyboard_mode keyboard_mode;
    bool turn_screen_off;
};

void
sc_launch_opts_init(struct sc_launch_opts *opts, const char *serial);

int
sc_launch_opts_to_argv(const struct sc_launch_opts *opts, const char *argv[],
                       size_t max);

#endif
