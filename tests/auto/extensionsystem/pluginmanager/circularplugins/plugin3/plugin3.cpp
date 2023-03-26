// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugin3.h"

#include <qplugin.h>

using namespace Plugin3;

MyPlugin3::MyPlugin3()
{
}

bool MyPlugin3::initialize(const QStringList &, QString *)
{
    return true;
}

