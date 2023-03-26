// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "header.h"
#include "dummy.h"

#include <QString>

int xi = 10;

int freefunc2(int a) { return a; }

int freefunc2(double)
{ return 1; }

int freefunc2(const QString &)
{ return 1; }

void here() {
    test::Dummy d;
    d;
}
