// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runextensions.h"

namespace Utils {
namespace Internal {

RunnableThread::RunnableThread(QRunnable *runnable, QObject *parent)
    : QThread(parent),
      m_runnable(runnable)
{
}

void RunnableThread::run()
{
    m_runnable->run();
    if (m_runnable->autoDelete())
        delete m_runnable;
}

} // Internal
} // Utils
