#include "adb_list.h"

#include <stddef.h>
#include <stdio.h>

int
main(void) {
    struct sc_device_list list;
    int count = sc_device_list_get(&list);
    if (count < 0) {
        fprintf(stderr, "Could not list adb devices\n");
        return 1;
    }

    for (size_t i = 0; i < list.count; ++i) {
        const struct sc_device *device = &list.devices[i];
        printf("%s\t%s\t%s\n",
               device->serial,
               device->model ? device->model : "",
               device->state);
    }

    sc_device_list_free(&list);
    return 0;
}
