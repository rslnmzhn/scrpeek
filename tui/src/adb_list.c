#ifndef _WIN32
# define _POSIX_C_SOURCE 200809L
#endif

#include "adb_list.h"

#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# define SC_POPEN _popen
# define SC_PCLOSE _pclose
# define SC_ADB_EXE "adb.exe"
#else
# define SC_POPEN popen
# define SC_PCLOSE pclose
# define SC_ADB_EXE "adb"
#endif

#define SC_ADB_HEADER "List of devices attached"
#define SC_ADB_HEADER_LEN (sizeof(SC_ADB_HEADER) - 1)

static char *
sc_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static char *
sc_find_adb(void) {
    const char *adb = getenv("ADB");
    if (adb && adb[0]) {
        return sc_strdup(adb);
    }

    return sc_strdup(SC_ADB_EXE);
}

static bool
sc_quote_command_arg(const char *arg, char *dst, size_t dst_size) {
    size_t pos = 0;

    if (pos + 1 >= dst_size) {
        return false;
    }
    dst[pos++] = '"';

    for (const char *p = arg; *p; ++p) {
        if (*p == '"') {
            if (pos + 2 >= dst_size) {
                return false;
            }
            dst[pos++] = '\\';
        } else if (pos + 1 >= dst_size) {
            return false;
        }
        dst[pos++] = *p;
    }

    if (pos + 2 > dst_size) {
        return false;
    }
    dst[pos++] = '"';
    dst[pos] = '\0';
    return true;
}

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
        return NULL;
    }

    FILE *pipe = SC_POPEN(command, "r");
    if (!pipe) {
        return NULL;
    }

    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        (void) SC_PCLOSE(pipe);
        return NULL;
    }

    for (;;) {
        if (len + 1 == cap) {
            size_t new_cap = cap * 2;
            char *new_buf = realloc(buf, new_cap);
            if (!new_buf) {
                free(buf);
                (void) SC_PCLOSE(pipe);
                return NULL;
            }
            buf = new_buf;
            cap = new_cap;
        }

        size_t read = fread(buf + len, 1, cap - len - 1, pipe);
        len += read;

        if (read == 0) {
            if (ferror(pipe)) {
                free(buf);
                (void) SC_PCLOSE(pipe);
                return NULL;
            }
            break;
        }
    }

    buf[len] = '\0';

    int status = SC_PCLOSE(pipe);
    if (status != 0) {
        free(buf);
        return NULL;
    }

    return buf;
}

static void
sc_device_destroy(struct sc_device *device) {
    free(device->serial);
    free(device->transport);
    free(device->model);
    free(device->state);
}

static void
sc_device_init_empty(struct sc_device *device) {
    device->serial = NULL;
    device->transport = NULL;
    device->model = NULL;
    device->state = NULL;
}

static bool
sc_device_copy_token(char **dst, const char *token, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    if (strncmp(token, prefix, prefix_len)) {
        return true;
    }

    free(*dst);
    *dst = sc_strdup(token + prefix_len);
    return *dst != NULL;
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
    if (!serial_len || s[serial_len] == '\0') {
        return false;
    }
    s[serial_len] = '\0';
    char *serial = s;

    s += serial_len + 1;
    s += strspn(s, " \t");

    size_t state_len = strcspn(s, " \t");
    if (!state_len) {
        return false;
    }
    bool eol = s[state_len] == '\0';
    s[state_len] = '\0';
    char *state = s;

    sc_device_init_empty(device);
    device->serial = sc_strdup(serial);
    device->state = sc_strdup(state);
    if (!device->serial || !device->state) {
        sc_device_destroy(device);
        return false;
    }

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

        bool ok = sc_device_copy_token(&device->model, s, "model:");
        ok = ok && sc_device_copy_token(&device->transport, s, "transport_id:");
        if (ok && !device->transport && !strncmp(s, "usb:", sizeof("usb:") - 1)) {
            device->transport = sc_strdup(s + sizeof("usb:") - 1);
            ok = device->transport != NULL;
        }
        if (!ok) {
            sc_device_destroy(device);
            return false;
        }

        if (eol) {
            break;
        }
        s += token_len + 1;
    }

    return true;
}

static bool
sc_device_list_push(struct sc_device_list *list, struct sc_device *device) {
    size_t new_count = list->count + 1;
    struct sc_device *new_devices = realloc(list->devices,
            new_count * sizeof(*new_devices));
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
                    sc_device_destroy(&device);
                    return false;
                }
            }
        }

        if (is_last_line) {
            break;
        }
        idx += len + 1;
    }

    return header_found;
}

int
sc_device_list_get(struct sc_device_list *out) {
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

    if (out->count > (size_t) INT_MAX) {
        sc_device_list_free(out);
        return -1;
    }

    return (int) out->count;
}

void
sc_device_list_free(struct sc_device_list *list) {
    if (!list) {
        return;
    }

    for (size_t i = 0; i < list->count; ++i) {
        sc_device_destroy(&list->devices[i]);
    }
    free(list->devices);
    list->devices = NULL;
    list->count = 0;
}
