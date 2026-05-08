#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# define setenv(name, value, overwrite) _putenv_s(name, value)
#endif

static void
test_profile_name_validation(void) {
    assert(sc_config_profile_name_valid("default"));
    assert(sc_config_profile_name_valid("work_1-prod"));
    assert(!sc_config_profile_name_valid(""));
    assert(!sc_config_profile_name_valid("bad name"));
    assert(!sc_config_profile_name_valid("bad/name"));
}

static void
test_save_load_profile(void) {
    struct sc_launch_opts opts;
    sc_launch_opts_init(&opts, "runtime-serial");
    strcpy(opts.max_size, "1080");
    strcpy(opts.max_fps, "60");
    opts.video_codec = SC_VIDEO_CODEC_AV1;
    opts.no_audio = true;
    strcpy(opts.record_path, "record.mkv");
    opts.connection = SC_CONNECTION_TCPIP;
    strcpy(opts.tcpip_addr, "192.168.0.2:5555");
    opts.keyboard_mode = SC_KEYBOARD_UHID;
    opts.turn_screen_off = true;

    assert(sc_config_save("test_profile", &opts));
    assert(sc_config_profile_exists("test_profile"));

    struct sc_launch_opts loaded;
    sc_launch_opts_init(&loaded, "new-runtime-serial");
    assert(sc_config_load("test_profile", &loaded));
    assert(!strcmp(loaded.serial, "new-runtime-serial"));
    assert(!strcmp(loaded.max_size, "1080"));
    assert(!strcmp(loaded.max_fps, "60"));
    assert(loaded.video_codec == SC_VIDEO_CODEC_AV1);
    assert(loaded.no_audio);
    assert(!strcmp(loaded.record_path, "record.mkv"));
    assert(loaded.connection == SC_CONNECTION_TCPIP);
    assert(!strcmp(loaded.tcpip_addr, "192.168.0.2:5555"));
    assert(loaded.keyboard_mode == SC_KEYBOARD_UHID);
    assert(loaded.turn_screen_off);
}

int
main(void) {
#ifdef _WIN32
    setenv("APPDATA", "C:\\Users\\iamze\\AppData\\Local\\Temp\\opencode", 1);
#else
    setenv("XDG_CONFIG_HOME", "/tmp", 1);
#endif
    test_profile_name_validation();
    test_save_load_profile();
    return 0;
}
