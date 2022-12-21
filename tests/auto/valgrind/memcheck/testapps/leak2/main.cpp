// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <string.h>

char *lower;

int main()
{
    lower = strdup("asdf");

    while (*lower)
        lower++;

    return 0;
}
