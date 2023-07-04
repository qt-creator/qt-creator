// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "eventspy.h"

#include <QCoreApplication>
#include <QEvent>

using namespace std::literals::chrono_literals;

EventSpy::EventSpy(uint eventType)
    : startTime(std::chrono::steady_clock::now()),
      eventType(eventType)
{
}

bool EventSpy::waitForEvent()
{
    while (shouldRun())
        QCoreApplication::processEvents();

    return eventHappened;
}

bool EventSpy::event(QEvent *event)
{
    if (event->type() == eventType) {
        eventHappened = true;

        return true;
    }

    return false;
}

bool EventSpy::shouldRun() const
{
    return !eventHappened
        && (std::chrono::steady_clock::now() - startTime) < 1s;
}
