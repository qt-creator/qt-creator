// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mocktimer.h"

MockTimer::MockTimer()
{
    ON_CALL(*this, start(_)).WillByDefault(Assign(&m_isStarted, true));
}

MockTimer::~MockTimer()
{
    emitTimoutIfStarted();
}

void MockTimer::setSingleShot(bool) {}

void MockTimer::emitTimoutIfStarted()
{
    if (m_isStarted)
        emit timeout();
}
