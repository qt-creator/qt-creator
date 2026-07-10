// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlentrypoint.h"

int QmlEntryPoint::process(int value)
{
    int doubled = value * 2;
    return doubled + offset(value);
}

int QmlEntryPoint::offset(int value) const
{
    return value % 3;
}
