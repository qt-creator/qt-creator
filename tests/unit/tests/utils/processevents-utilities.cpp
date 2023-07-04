// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processevents-utilities.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#include <QDebug>

namespace ProcessEventUtilities
{
bool processEventsUntilTrue(std::function<bool ()> condition, int timeOutInMs)
{
    if (condition())
        return true;

    QElapsedTimer time;
    time.start();

    forever {
        if (condition())
            return true;

        if (time.elapsed() > timeOutInMs) {
            qWarning() << "Timeout of" << timeOutInMs
                       << "reached in processEventsAndWaitUntilTrue()";
            return false;
        }

        QCoreApplication::processEvents();
    }
}
}
