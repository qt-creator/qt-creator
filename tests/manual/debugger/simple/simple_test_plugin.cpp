// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <stdio.h>
#include <qglobal.h>

class Something {
public:
    Something() {
        int i = 0;
        ++i;
        return;
    }
};

extern "C" Q_DECL_EXPORT int pluginTest()
{
    static Something sst;
    Something st;
    int s = 0;
    for (int i = 1; i != 2000; ++i)
        s += i;
    fprintf(stderr, "in plugin test\n");
    return s;
}
