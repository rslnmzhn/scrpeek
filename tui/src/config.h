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
