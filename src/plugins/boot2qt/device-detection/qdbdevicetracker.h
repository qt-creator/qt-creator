// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QMap>

namespace Qdb::Internal {

class QdbWatcher;

class QdbDeviceTracker : public QObject
{
    Q_OBJECT
public:
    QdbDeviceTracker(QObject *parent = nullptr);

    enum DeviceEventType
    {
        NewDevice,
        DisconnectedDevice
    };

    void start();
    void stop();

signals:
    void deviceEvent(DeviceEventType eventType, QMap<QString, QString> info);
    void trackerError(QString errorMessage);

private:
    void handleWatchMessage(const QJsonDocument &document);

    QdbWatcher *m_qdbWatcher = nullptr;
};

} // Qdb::Internal
