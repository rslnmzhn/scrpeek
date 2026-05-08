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

#ifndef SC_TUI_CONFIG_H
#define SC_TUI_CONFIG_H

#include <stdbool.h>

#include "launch_opts.h"

bool
sc_config_profile_name_valid(const char *name);

bool
sc_config_save(const char *name, const struct sc_launch_opts *opts);

bool
sc_config_load(const char *name, struct sc_launch_opts *opts);

bool
sc_config_profile_exists(const char *name);

#endif
