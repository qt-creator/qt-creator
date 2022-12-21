// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#define _POSIX_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>

int ptracepreload_init(void) __attribute__((constructor,visibility("hidden")));

int ptracepreload_init(void)
{
    prctl(0x59616d61, getppid(), 0, 0, 0);
    puts("eeks\n");
    return 0;
}

