/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

/* log: stderr logging information. */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "log.h"

void log_die(char *msg, ...)
{
    va_list argp;

    //log_null(msg); // causes an infinte loop in the macro

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);

    fprintf(stderr, "\n");
    abort();
}

void log_info(char *msg, ...)
{
    va_list argp;

    //log_null(msg); // causes an infinte loop in the macro

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);

    fprintf(stderr, "\n");
}
