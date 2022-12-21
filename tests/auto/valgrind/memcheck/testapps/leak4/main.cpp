// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qglobal.h>

struct Foo
{
    Foo()
      : num(new qint64)
    {}

    qint64 *num;
};

int main()
{
    new Foo;

    return 0;
}
