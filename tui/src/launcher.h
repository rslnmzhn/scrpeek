#ifndef SC_TUI_LAUNCHER_H
#define SC_TUI_LAUNCHER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
# include <windows.h>
typedef DWORD sc_tui_pid;
#else
# include <sys/types.h>
typedef pid_t sc_tui_pid;
#endif

struct sc_launcher {
    sc_tui_pid pid;
    bool running;
    bool pipe_open;
    int exit_code;
    long started_ms;
#ifdef _WIN32
    HANDLE process;
    HANDLE pipe_read;
#else
    int pipe_fd;
#endif
};

bool
sc_launcher_start(struct sc_launcher *launcher, const char *const argv[],
                  long started_ms);

int
sc_launcher_read(struct sc_launcher *launcher, char *buf, size_t len);

bool
sc_launcher_poll_exit(struct sc_launcher *launcher);

bool
sc_launcher_terminate(struct sc_launcher *launcher);

void
sc_launcher_close(struct sc_launcher *launcher);

#endif
