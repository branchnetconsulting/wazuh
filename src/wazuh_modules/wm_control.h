/*
 * Wazuh Module for Agent control
 * Copyright (C) 2015-2019, Wazuh Inc.
 * January, 2019
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */
#ifndef WIN32
#ifndef WM_CONTROL
#define WM_CONTROL

#define WM_CONTROL_LOGTAG ARGV0 ":control"

#include "wmodules.h"

extern const wm_context WM_CONTROL_CONTEXT;

typedef struct wm_control_t {
    unsigned int enabled:1;
    unsigned int run_on_start:1;
} wm_control_t;

wmodule *wm_control_read();
char *getPrimaryIP();
int looking_for_cfgfile(const char *buffer, char *filepath, size_t n);
void msg_to_json(char * output, int peer);

#endif
#endif
