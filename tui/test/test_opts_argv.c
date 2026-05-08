#include "launch_opts.h"

#include <assert.h>
#include <string.h>

static void
test_default_opts(void) {
    struct sc_launch_opts opts;
    const char *argv[16];

    sc_launch_opts_init(&opts, "abc123");
    int count = sc_launch_opts_to_argv(&opts, argv, 16);

    assert(count == 3);
    assert(!strcmp(argv[0], "scrcpy"));
    assert(!strcmp(argv[1], "--serial"));
    assert(!strcmp(argv[2], "abc123"));
    assert(argv[3] == NULL);
}

static void
test_full_opts(void) {
    struct sc_launch_opts opts;
    const char *argv[32];

    sc_launch_opts_init(&opts, "abc123");
    strcpy(opts.max_size, "1080");
    strcpy(opts.max_fps, "60");
    opts.video_codec = SC_VIDEO_CODEC_H265;
    opts.no_audio = true;
    strcpy(opts.record_path, "out.mkv");
    opts.connection = SC_CONNECTION_TCPIP;
    strcpy(opts.tcpip_addr, "192.168.1.10:5555");
    opts.keyboard_mode = SC_KEYBOARD_UHID;
    opts.turn_screen_off = true;

    int count = sc_launch_opts_to_argv(&opts, argv, 32);

    assert(count == 17);
    assert(!strcmp(argv[0], "scrcpy"));
    assert(!strcmp(argv[1], "--serial"));
    assert(!strcmp(argv[2], "abc123"));
    assert(!strcmp(argv[3], "--max-size"));
    assert(!strcmp(argv[4], "1080"));
    assert(!strcmp(argv[5], "--max-fps"));
    assert(!strcmp(argv[6], "60"));
    assert(!strcmp(argv[7], "--video-codec"));
    assert(!strcmp(argv[8], "h265"));
    assert(!strcmp(argv[9], "--no-audio"));
    assert(!strcmp(argv[10], "--record"));
    assert(!strcmp(argv[11], "out.mkv"));
    assert(!strcmp(argv[12], "--tcpip"));
    assert(!strcmp(argv[13], "192.168.1.10:5555"));
    assert(!strcmp(argv[14], "--keyboard"));
    assert(!strcmp(argv[15], "uhid"));
    assert(!strcmp(argv[16], "--turn-screen-off"));
    assert(argv[17] == NULL);
}

int
main(void) {
    test_default_opts();
    test_full_opts();
    return 0;
}
