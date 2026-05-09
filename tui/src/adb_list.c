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

#include "adb_list.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <windows.h>
# define SC_ADB_EXE "adb.exe"
# define SC_PATH_SEP '\\'
#else
# include <unistd.h>
# define SC_ADB_EXE "adb"
# define SC_PATH_SEP '/'
#endif

#define SC_ADB_HEADER "List of devices attached"
#define SC_ADB_HEADER_LEN (sizeof(SC_ADB_HEADER) - 1)

static bool
sc_copy_field(char *dst, size_t dst_size, const char *src) {
    size_t len = strlen(src);
    if (len >= dst_size) {
        errno = ENAMETOOLONG;
        return false;
    }
    memcpy(dst, src, len + 1);
    return true;
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
        errno = ENOENT;
        return NULL;
    }

    char *utf8 = malloc((size_t) len);
    if (!utf8) {
        return NULL;
    }

    if (!WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8, len, NULL, NULL)) {
        free(utf8);
        errno = ENOENT;
        return NULL;
    }

    return utf8;
}

static char *
sc_find_exe_dir_adb(void) {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, path, (DWORD) (sizeof(path) / sizeof(path[0])));
    if (!len || len >= (DWORD) (sizeof(path) / sizeof(path[0]))) {
        errno = ENOENT;
        return NULL;
    }

    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash) {
        errno = ENOENT;
        return NULL;
    }
    slash[1] = L'\0';

    if (wcslen(path) + wcslen(L"adb.exe") + 1 > sizeof(path) / sizeof(path[0])) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    wcscat(path, L"adb.exe");

    DWORD attrs = GetFileAttributesW(path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        errno = ENOENT;
        return NULL;
    }

    return sc_wide_to_utf8(path);
}

static char *
sc_find_path_adb(void) {
    wchar_t path[MAX_PATH];
    DWORD len = SearchPathW(NULL, L"adb.exe", NULL,
                           (DWORD) (sizeof(path) / sizeof(path[0])), path, NULL);
    if (!len || len >= (DWORD) (sizeof(path) / sizeof(path[0]))) {
        errno = ENOENT;
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
sc_find_exe_dir_adb(void) {
# if defined(__APPLE__)
    errno = ENOENT;
    return NULL;
# elif defined(__linux__)
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len < 0) {
        return NULL;
    }
    exe_path[len] = '\0';

    char *slash = strrchr(exe_path, '/');
    if (!slash) {
        errno = ENOENT;
        return NULL;
    }
    slash[1] = '\0';

    if (strlen(exe_path) + sizeof(SC_ADB_EXE) > sizeof(exe_path)) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    strcat(exe_path, SC_ADB_EXE);

    if (!sc_is_executable(exe_path)) {
        errno = ENOENT;
        return NULL;
    }

    return sc_strdup(exe_path);
# else
    errno = ENOENT;
    return NULL;
# endif
}

static char *
sc_find_path_adb(void) {
    const char *path = getenv("PATH");
    if (!path) {
        errno = ENOENT;
        return NULL;
    }

    const char *start = path;
    while (*start) {
        const char *end = strchr(start, ':');
        size_t dir_len = end ? (size_t) (end - start) : strlen(start);
        if (dir_len > 0) {
            size_t needed = dir_len + 1 + sizeof(SC_ADB_EXE);
            char *candidate = malloc(needed);
            if (!candidate) {
                return NULL;
            }
            memcpy(candidate, start, dir_len);
            candidate[dir_len] = SC_PATH_SEP;
            memcpy(candidate + dir_len + 1, SC_ADB_EXE, sizeof(SC_ADB_EXE));
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

    errno = ENOENT;
    return NULL;
}
#endif

static char *
sc_find_adb(void) {
    const char *adb = getenv("SCRCPY_ADB");
    if (adb && adb[0]) {
        return sc_strdup(adb);
    }

    char *exe_dir_adb = sc_find_exe_dir_adb();
    if (exe_dir_adb) {
        return exe_dir_adb;
    }

    char *path_adb = sc_find_path_adb();
    if (path_adb) {
        return path_adb;
    }

    errno = ENOENT;
    return NULL;
}

static bool
sc_quote_command_arg(const char *arg, char *dst, size_t dst_size) {
    size_t pos = 0;
    if (pos + 1 >= dst_size) {
        errno = ENAMETOOLONG;
        return false;
    }
    dst[pos++] = '"';

    for (const char *p = arg; *p; ++p) {
        if (*p == '"') {
            if (pos + 2 >= dst_size) {
                errno = ENAMETOOLONG;
                return false;
            }
            dst[pos++] = '\\';
        } else if (pos + 1 >= dst_size) {
            errno = ENAMETOOLONG;
            return false;
        }
        dst[pos++] = *p;
    }

    if (pos + 2 > dst_size) {
        errno = ENAMETOOLONG;
        return false;
    }
    dst[pos++] = '"';
    dst[pos] = '\0';
    return true;
}

#ifdef _WIN32
int
sc_popen_silent(const char *cmd, char *buf, unsigned int buf_size) {
    if (!cmd || !buf || buf_size == 0) {
        errno = EINVAL;
        return -1;
    }

    buf[0] = '\0';

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE read_pipe = NULL;
    HANDLE write_pipe = NULL;
    HANDLE nul = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));

    char *cmdline = sc_strdup(cmd);
    if (!cmdline) {
        return -1;
    }

    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        free(cmdline);
        errno = EIO;
        return -1;
    }

    if (!SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        free(cmdline);
        errno = EIO;
        return -1;
    }

    nul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (nul == INVALID_HANDLE_VALUE) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        free(cmdline);
        errno = EIO;
        return -1;
    }

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_pipe;
    si.hStdError = nul;

    BOOL created = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                                  NULL, NULL, &si, &pi);
    CloseHandle(write_pipe);
    CloseHandle(nul);
    free(cmdline);

    if (!created) {
        CloseHandle(read_pipe);
        errno = EIO;
        return -1;
    }

    unsigned int used = 0;
    bool read_ok = true;
    for (;;) {
        DWORD n = 0;
        DWORD space = buf_size - used - 1;
        char scratch[512];
        char *dst = space > 0 ? buf + used : scratch;
        DWORD want = space > 0 && space < sizeof(scratch) ? space : sizeof(scratch);
        if (!ReadFile(read_pipe, dst, want, &n, NULL)) {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE) {
                break;
            }
            read_ok = false;
            break;
        }
        if (n == 0) {
            break;
        }
        if (space > 0) {
            used += n;
        }
    }
    buf[used] = '\0';

    DWORD wait_result = WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    bool exit_ok = wait_result == WAIT_OBJECT_0 && GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(read_pipe);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (!read_ok || !exit_ok || exit_code != 0) {
        errno = EIO;
        return -1;
    }
    return (int) used;
}
#else
int
sc_popen_silent(const char *cmd, char *buf, unsigned int buf_size) {
    if (!cmd || !buf || buf_size == 0) {
        errno = EINVAL;
        return -1;
    }

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }

    size_t len = fread(buf, 1, buf_size - 1, pipe);
    buf[len] = '\0';
    if (ferror(pipe)) {
        (void) pclose(pipe);
        errno = EIO;
        return -1;
    }

    int status = pclose(pipe);
    if (status != 0) {
        errno = EIO;
        return -1;
    }
    return (int) len;
}
#endif

static char *
sc_read_adb_devices(void) {
    char *adb = sc_find_adb();
    if (!adb) {
        return NULL;
    }

    char quoted_adb[1024];
    if (!sc_quote_command_arg(adb, quoted_adb, sizeof(quoted_adb))) {
        free(adb);
        return NULL;
    }

    char command[1100];
    int written = snprintf(command, sizeof(command), "%s devices -l", quoted_adb);
    free(adb);
    if (written < 0 || (size_t) written >= sizeof(command)) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    size_t cap = 4096;
    char *buf = malloc(cap);
    if (!buf) {
        return NULL;
    }

    if (sc_popen_silent(command, buf, (unsigned int) cap) < 0) {
        free(buf);
        return NULL;
    }

    return buf;
}

static bool
sc_parse_device_line(char *line, struct sc_device *device) {
    if (!line[0] || line[0] == '*') {
        return false;
    }
    if (!strncmp(line, "adb server", sizeof("adb server") - 1)) {
        return false;
    }

    char *s = line;
    size_t serial_len = strcspn(s, " \t");
    if (!serial_len || !s[serial_len]) {
        return false;
    }
    s[serial_len] = '\0';

    if (!sc_copy_field(device->serial, sizeof(device->serial), s)) {
        return false;
    }

    s += serial_len + 1;
    s += strspn(s, " \t");

    size_t state_len = strcspn(s, " \t");
    if (!state_len) {
        return false;
    }
    bool eol = s[state_len] == '\0';
    s[state_len] = '\0';

    if (!sc_copy_field(device->state, sizeof(device->state), s)) {
        return false;
    }
    device->model[0] = '\0';

    if (eol) {
        return true;
    }

    s += state_len + 1;
    while (*s) {
        s += strspn(s, " \t");
        if (!*s) {
            break;
        }

        size_t token_len = strcspn(s, " \t");
        eol = s[token_len] == '\0';
        s[token_len] = '\0';

        const char prefix[] = "model:";
        if (!strncmp(s, prefix, sizeof(prefix) - 1)) {
            if (!sc_copy_field(device->model, sizeof(device->model),
                               s + sizeof(prefix) - 1)) {
                return false;
            }
        }

        if (eol) {
            break;
        }
        s += token_len + 1;
    }

    return true;
}

static bool
sc_device_list_push(struct sc_device_list *list, const struct sc_device *device) {
    if (list->count == INT_MAX) {
        errno = EOVERFLOW;
        return false;
    }

    int new_count = list->count + 1;
    struct sc_device *new_devices = realloc(list->devices,
            (size_t) new_count * sizeof(*new_devices));
    if (!new_devices) {
        return false;
    }

    list->devices = new_devices;
    list->devices[list->count] = *device;
    list->count = new_count;
    return true;
}

static bool
sc_parse_devices(char *str, struct sc_device_list *out) {
    bool header_found = false;
    size_t idx = 0;

    while (str[idx]) {
        char *line = &str[idx];
        size_t len = strcspn(line, "\n");
        bool is_last_line = line[len] == '\0';
        if (!is_last_line) {
            line[len] = '\0';
        }
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
        }

        if (!header_found) {
            if (!strncmp(line, SC_ADB_HEADER, SC_ADB_HEADER_LEN)) {
                header_found = true;
            }
        } else {
            struct sc_device device;
            if (sc_parse_device_line(line, &device)) {
                if (!sc_device_list_push(out, &device)) {
                    return false;
                }
            }
        }

        if (is_last_line) {
            break;
        }
        idx += len + 1;
    }

    if (!header_found) {
        errno = EPROTO;
    }
    return header_found;
}

int
sc_device_list_get(struct sc_device_list *out) {
    if (!out) {
        errno = EINVAL;
        return -1;
    }

    out->devices = NULL;
    out->count = 0;

    char *adb_output = sc_read_adb_devices();
    if (!adb_output) {
        return -1;
    }

    bool ok = sc_parse_devices(adb_output, out);
    free(adb_output);
    if (!ok) {
        sc_device_list_free(out);
        return -1;
    }

    return out->count;
}

void
sc_device_list_free(struct sc_device_list *list) {
    if (!list) {
        return;
    }

    free(list->devices);
    list->devices = NULL;
    list->count = 0;
}
