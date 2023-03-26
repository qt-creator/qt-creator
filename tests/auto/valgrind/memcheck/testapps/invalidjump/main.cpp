// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

void foo() { }

int main()
{
    void (*fooPtr)() = 0;
    fooPtr();
    return 0;
}
