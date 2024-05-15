// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "threadutils.h"

#include <QCoreApplication>
#include <QThread>

namespace Utils {

bool isMainThread()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // TODO: Remove threadutils.h and replace all usages with:
    return QThread::isMainThread();
#else
    return QThread::currentThread() == qApp->thread();
#endif
}

} // namespace Utils
