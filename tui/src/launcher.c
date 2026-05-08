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

#include "launcher.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <io.h>
#else
# include <fcntl.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

static const char *
sc_find_scrcpy(void) {
    const char *path = getenv("SCRCPY_PATH");
    return path && path[0] ? path : "scrcpy";
}

#ifdef _WIN32
static bool
sc_append_quoted_arg(char *cmd, size_t cap, size_t *pos, const char *arg) {
    if (*pos && *pos + 1 < cap) {
        cmd[(*pos)++] = ' ';
    }
    if (*pos + 1 >= cap) {
        return false;
    }
    cmd[(*pos)++] = '"';

    for (const char *p = arg; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            if (*pos + 1 >= cap) {
                return false;
            }
            cmd[(*pos)++] = '\\';
        }
        if (*pos + 1 >= cap) {
            return false;
        }
        cmd[(*pos)++] = *p;
    }

    if (*pos + 2 > cap) {
        return false;
    }
    cmd[(*pos)++] = '"';
    cmd[*pos] = '\0';
    return true;
}

static bool
sc_build_command(char *cmd, size_t cap, const char *const argv[]) {
    size_t pos = 0;
    cmd[0] = '\0';
    for (size_t i = 0; argv[i]; ++i) {
        const char *arg = i == 0 ? sc_find_scrcpy() : argv[i];
        if (!sc_append_quoted_arg(cmd, cap, &pos, arg)) {
            return false;
        }
    }
    return true;
}

bool
sc_launcher_start(struct sc_launcher *launcher, const char *const argv[],
                  long started_ms) {
    memset(launcher, 0, sizeof(*launcher));
    launcher->exit_code = -1;
    launcher->started_ms = started_ms;

    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE read_pipe;
    HANDLE write_pipe;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        return false;
    }
    if (!SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return false;
    }

    char cmd[8192];
    if (!sc_build_command(cmd, sizeof(cmd), argv)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return false;
    }

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;

    BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si,
                             &pi);
    CloseHandle(write_pipe);
    if (!ok) {
        CloseHandle(read_pipe);
        return false;
    }

    CloseHandle(pi.hThread);
    launcher->pid = pi.dwProcessId;
    launcher->running = true;
    launcher->pipe_open = true;
    launcher->process = pi.hProcess;
    launcher->pipe_read = read_pipe;
    return true;
}

int
sc_launcher_read(struct sc_launcher *launcher, char *buf, size_t len) {
    if (!launcher->pipe_open || !len) {
        return 0;
    }

    DWORD available;
    if (!PeekNamedPipe(launcher->pipe_read, NULL, 0, NULL, &available, NULL)) {
        CloseHandle(launcher->pipe_read);
        launcher->pipe_open = false;
        return 0;
    }
    if (!available) {
        return 0;
    }

    DWORD max_read = len > (size_t) UINT32_MAX ? UINT32_MAX : (DWORD) len;
    DWORD to_read = available < max_read ? available : max_read;
    DWORD read_count;
    if (!ReadFile(launcher->pipe_read, buf, to_read, &read_count, NULL)) {
        CloseHandle(launcher->pipe_read);
        launcher->pipe_open = false;
        return 0;
    }
    return (int) read_count;
}

bool
sc_launcher_poll_exit(struct sc_launcher *launcher) {
    if (!launcher->running) {
        return true;
    }

    DWORD status = WaitForSingleObject(launcher->process, 0);
    if (status != WAIT_OBJECT_0) {
        return false;
    }

    DWORD code;
    launcher->exit_code = GetExitCodeProcess(launcher->process, &code)
            ? (int) code : -1;
    launcher->running = false;
    return true;
}

bool
sc_launcher_terminate(struct sc_launcher *launcher) {
    return launcher->running && TerminateProcess(launcher->process, 1);
}

void
sc_launcher_close(struct sc_launcher *launcher) {
    if (launcher->pipe_open) {
        CloseHandle(launcher->pipe_read);
    }
    if (launcher->process) {
        CloseHandle(launcher->process);
    }
    memset(launcher, 0, sizeof(*launcher));
    launcher->exit_code = -1;
}

#else
bool
sc_launcher_start(struct sc_launcher *launcher, const char *const argv[],
                  long started_ms) {
    memset(launcher, 0, sizeof(*launcher));
    launcher->pipe_fd = -1;
    launcher->exit_code = -1;
    launcher->started_ms = started_ms;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (pipefd[1] != STDOUT_FILENO) {
            dup2(pipefd[1], STDOUT_FILENO);
        }
        if (pipefd[1] != STDERR_FILENO) {
            dup2(pipefd[1], STDERR_FILENO);
        }
        close(pipefd[1]);

        const char *scrcpy = sc_find_scrcpy();
        size_t argc = 0;
        while (argv[argc]) {
            ++argc;
        }
        char **exec_argv = calloc(argc + 1, sizeof(*exec_argv));
        if (!exec_argv) {
            _exit(127);
        }
        exec_argv[0] = (char *) scrcpy;
        for (size_t i = 1; i < argc; ++i) {
            exec_argv[i] = (char *) argv[i];
        }

        execvp(exec_argv[0], exec_argv);
        perror("execvp");
        _exit(errno == ENOENT ? 127 : 126);
    }

    close(pipefd[1]);
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    if (flags == -1 || fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        close(pipefd[0]);
        kill(pid, SIGTERM);
        (void) waitpid(pid, NULL, 0);
        return false;
    }

    launcher->pid = pid;
    launcher->running = true;
    launcher->pipe_open = true;
    launcher->pipe_fd = pipefd[0];
    return true;
}

int
sc_launcher_read(struct sc_launcher *launcher, char *buf, size_t len) {
    if (!launcher->pipe_open || !len) {
        return 0;
    }

    ssize_t r = read(launcher->pipe_fd, buf, len);
    if (r > 0) {
        return (int) r;
    }
    if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }

    close(launcher->pipe_fd);
    launcher->pipe_fd = -1;
    launcher->pipe_open = false;
    return 0;
}

bool
sc_launcher_poll_exit(struct sc_launcher *launcher) {
    if (!launcher->running) {
        return true;
    }

    int status;
    pid_t r = waitpid(launcher->pid, &status, WNOHANG);
    if (r == 0) {
        return false;
    }
    if (r == -1) {
        if (errno == ECHILD) {
            launcher->running = false;
            return true;
        }
        return false;
    }

    if (WIFEXITED(status)) {
        launcher->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        launcher->exit_code = 128 + WTERMSIG(status);
    } else {
        launcher->exit_code = -1;
    }
    launcher->running = false;
    return true;
}

bool
sc_launcher_terminate(struct sc_launcher *launcher) {
    return launcher->running && kill(launcher->pid, SIGTERM) != -1;
}

void
sc_launcher_close(struct sc_launcher *launcher) {
    if (launcher->pipe_open) {
        close(launcher->pipe_fd);
    }
    memset(launcher, 0, sizeof(*launcher));
    launcher->pipe_fd = -1;
    launcher->exit_code = -1;
}
#endif
