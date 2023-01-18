// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qdbdevicetracker.h"
#include "qdbmessagetracker.h"

namespace Qdb::Internal {

class DeviceDetector : public QObject
{
    Q_OBJECT

public:
    DeviceDetector();
    ~DeviceDetector();

    void start();
    void stop();

private:
    void handleDeviceEvent(QdbDeviceTracker::DeviceEventType eventType,
                           const QMap<QString, QString> &info);
    void handleTrackerError(const QString &errorMessage);
    void resetDevices();

    enum State { Inactive, WaitingForDeviceUpdates };

    State m_state = Inactive;
    QdbDeviceTracker m_deviceTracker;
    QdbMessageTracker m_messageTracker;
};

} // Qdb::Internal
