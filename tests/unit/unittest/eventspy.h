// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <chrono>

class EventSpy : public QObject
{
    Q_OBJECT

public:
    EventSpy(uint eventType);

    bool waitForEvent();

protected:
    bool event(QEvent *event) override;

private:
    bool shouldRun() const;

private:
    std::chrono::steady_clock::time_point startTime;
    uint eventType;
    bool eventHappened = false;
};
