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

#ifndef SC_TUI_ADB_LIST_H
#define SC_TUI_ADB_LIST_H

#include <stdbool.h>

struct sc_device {
    char serial[64];
    char model[64];
    char state[16];
};

struct sc_device_list {
    struct sc_device *devices;
    int count;
};

int
sc_popen_silent(const char *cmd, char *buf, unsigned int buf_size);

int
sc_device_list_get(struct sc_device_list *out);

bool
sc_adb_connect(const char *endpoint);

void
sc_device_list_free(struct sc_device_list *list);

#endif
