// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugin2.h"

using namespace Plugin2;

bool MyPlugin2::initialize(const QStringList &, QString *)
{
    return true;
}
