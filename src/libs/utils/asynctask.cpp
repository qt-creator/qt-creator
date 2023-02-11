// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynctask.h"

#include <QCoreApplication>

namespace Utils {

static int s_maxThreadCount = INT_MAX;

class AsyncThreadPool : public QThreadPool
{
public:
    AsyncThreadPool() {
        setMaxThreadCount(s_maxThreadCount);
        moveToThread(qApp->thread());
    }
};

Q_GLOBAL_STATIC(AsyncThreadPool, s_asyncThreadPool);

QThreadPool *asyncThreadPool()
{
    return s_asyncThreadPool;
}

} // namespace Utils
