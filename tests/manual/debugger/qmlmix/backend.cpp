// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "backend.h"

int Backend::process(int value)
{
    int doubled = value * 2;            // step-into target from QML
    return doubled + offset(value);
}

int Backend::offset(int value) const
{
    return value % 3;
}
