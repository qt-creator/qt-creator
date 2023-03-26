// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "threadutils.h"

#include <QCoreApplication>
#include <QThread>

namespace Utils {

bool isMainThread()
{
    return QThread::currentThread() == qApp->thread();
}

} // namespace Utils
