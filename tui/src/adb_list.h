#ifndef SC_TUI_ADB_LIST_H
#define SC_TUI_ADB_LIST_H

#include <stddef.h>

struct sc_device {
    char *serial;
    char *transport;
    char *model;
    char *state;
};

struct sc_device_list {
    struct sc_device *devices;
    size_t count;
};

int
sc_device_list_get(struct sc_device_list *out);

void
sc_device_list_free(struct sc_device_list *list);

#endif
