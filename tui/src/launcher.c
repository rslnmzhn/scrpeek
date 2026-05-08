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

#include "ui/layout.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <limits.h>
# include <unistd.h>
#endif

#ifdef _WIN32
# define SC_SCRCPY_EXE "scrcpy.exe"
# define SC_PATH_SEP '\\'
#else
# define SC_SCRCPY_EXE "scrcpy"
# define SC_PATH_SEP '/'
#endif

static void
sc_set_error(char *error, size_t error_len, const char *context) {
    if (!error_len) {
        return;
    }
#ifdef _WIN32
    DWORD code = GetLastError();
    char msg[256];
    DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, code, 0, msg, sizeof(msg), NULL);
    if (len > 0) {
        snprintf(error, error_len, "%s: %s", context, msg);
    } else {
        snprintf(error, error_len, "%s: Windows error %lu", context, (unsigned long) code);
    }
#else
    snprintf(error, error_len, "%s: %s", context, strerror(errno));
#endif
}

static char *
sc_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, s, len);
    return copy;
}

#ifdef _WIN32
static char *
sc_wide_to_utf8(const wchar_t *path) {
    int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        return NULL;
    }
    char *utf8 = malloc((size_t) len);
    if (!utf8) {
        return NULL;
    }
    if (!WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8, len, NULL, NULL)) {
        free(utf8);
        return NULL;
    }
    return utf8;
}

static char *
sc_find_exe_dir_scrcpy(void) {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, path, (DWORD) (sizeof(path) / sizeof(path[0])));
    if (!len || len >= (DWORD) (sizeof(path) / sizeof(path[0]))) {
        return NULL;
    }
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash) {
        return NULL;
    }
    slash[1] = L'\0';
    if (wcslen(path) + wcslen(L"scrcpy.exe") + 1 > sizeof(path) / sizeof(path[0])) {
        return NULL;
    }
    wcscat(path, L"scrcpy.exe");
    DWORD attrs = GetFileAttributesW(path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return NULL;
    }
    return sc_wide_to_utf8(path);
}

static char *
sc_find_path_scrcpy(void) {
    wchar_t path[MAX_PATH];
    DWORD len = SearchPathW(NULL, L"scrcpy.exe", NULL,
                           (DWORD) (sizeof(path) / sizeof(path[0])), path, NULL);
    if (!len || len >= (DWORD) (sizeof(path) / sizeof(path[0]))) {
        return NULL;
    }
    return sc_wide_to_utf8(path);
}
#else
static bool
sc_is_executable(const char *path) {
    return access(path, X_OK) == 0;
}

static char *
sc_find_exe_dir_scrcpy(void) {
# if defined(__linux__)
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len < 0) {
        return NULL;
    }
    path[len] = '\0';
    char *slash = strrchr(path, '/');
    if (!slash) {
        return NULL;
    }
    slash[1] = '\0';
    if (strlen(path) + sizeof(SC_SCRCPY_EXE) > sizeof(path)) {
        return NULL;
    }
    strcat(path, SC_SCRCPY_EXE);
    return sc_is_executable(path) ? sc_strdup(path) : NULL;
# else
    return NULL;
# endif
}

static char *
sc_find_path_scrcpy(void) {
    const char *path = getenv("PATH");
    if (!path) {
        return NULL;
    }
    const char *start = path;
    while (*start) {
        const char *end = strchr(start, ':');
        size_t dir_len = end ? (size_t) (end - start) : strlen(start);
        if (dir_len > 0) {
            size_t needed = dir_len + 1 + sizeof(SC_SCRCPY_EXE);
            char *candidate = malloc(needed);
            if (!candidate) {
                return NULL;
            }
            memcpy(candidate, start, dir_len);
            candidate[dir_len] = SC_PATH_SEP;
            memcpy(candidate + dir_len + 1, SC_SCRCPY_EXE, sizeof(SC_SCRCPY_EXE));
            if (sc_is_executable(candidate)) {
                return candidate;
            }
            free(candidate);
        }
        if (!end) {
            break;
        }
        start = end + 1;
    }
    return NULL;
}
#endif

static char *
sc_find_scrcpy(void) {
    const char *env = getenv("SCRCPY_PATH");
    if (env && env[0]) {
        return sc_strdup(env);
    }
    char *beside = sc_find_exe_dir_scrcpy();
    if (beside) {
        return beside;
    }
    return sc_find_path_scrcpy();
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
sc_build_command(char *cmd, size_t cap, const char *scrcpy_path,
                 const char *const argv[]) {
    size_t pos = 0;
    cmd[0] = '\0';
    if (!sc_append_quoted_arg(cmd, cap, &pos, scrcpy_path)) {
        return false;
    }
    for (size_t i = 1; argv[i]; ++i) {
        if (!sc_append_quoted_arg(cmd, cap, &pos, argv[i])) {
            return false;
        }
    }
    return true;
}
#endif

int
sc_launch(const char *const argv[], char *error, size_t error_len) {
    endwin();

    char *scrcpy_path = sc_find_scrcpy();
    if (!scrcpy_path) {
        snprintf(error, error_len, "scrcpy binary not found. Set SCRCPY_PATH or put %s beside scrcpy-tui.",
                 SC_SCRCPY_EXE);
        return -1;
    }

#ifdef _WIN32
    char cmd[8192];
    if (!sc_build_command(cmd, sizeof(cmd), scrcpy_path, argv)) {
        snprintf(error, error_len, "scrcpy command line is too long");
        free(scrcpy_path);
        return -1;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    BOOL ok = CreateProcessA(scrcpy_path, cmd, NULL, NULL, FALSE, 0, NULL, NULL,
                             &si, &pi);
    if (!ok) {
        sc_set_error(error, error_len, "CreateProcess(scrcpy)");
        free(scrcpy_path);
        return -1;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    free(scrcpy_path);
    exit(0);
#else
    size_t argc = 0;
    while (argv[argc]) {
        ++argc;
    }
    char **exec_argv = calloc(argc + 1, sizeof(*exec_argv));
    if (!exec_argv) {
        sc_set_error(error, error_len, "calloc");
        free(scrcpy_path);
        return -1;
    }
    exec_argv[0] = scrcpy_path;
    for (size_t i = 1; i < argc; ++i) {
        exec_argv[i] = (char *) argv[i];
    }

    execv(scrcpy_path, exec_argv);
    sc_set_error(error, error_len, "execv(scrcpy)");
    free(exec_argv);
    free(scrcpy_path);
    return -1;
#endif
}
