// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <string.h>

int main()
{
    int *i = new int[10];
    memcpy(i, &i[1], 20);
    delete[] i;
    return 0;
}
