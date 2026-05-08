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
