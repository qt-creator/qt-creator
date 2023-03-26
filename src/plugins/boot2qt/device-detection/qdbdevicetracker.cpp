// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbdevicetracker.h"

#include "hostmessages.h"
#include "qdbwatcher.h"
#include "../qdbtr.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace Qdb::Internal {

QdbDeviceTracker::QdbDeviceTracker(QObject *parent)
    : QObject(parent)
{
    m_qdbWatcher = new QdbWatcher(this);
    connect(m_qdbWatcher, &QdbWatcher::incomingMessage, this, &QdbDeviceTracker::handleWatchMessage);
    connect(m_qdbWatcher, &QdbWatcher::watcherError, this, &QdbDeviceTracker::trackerError);
}
void QdbDeviceTracker::start()
{
    m_qdbWatcher->start(RequestType::WatchDevices);
}

void QdbDeviceTracker::stop()
{
    m_qdbWatcher->stop();
}

void QdbDeviceTracker::handleWatchMessage(const QJsonDocument &document)
{
    const auto type = responseType(document.object());
    if (type != ResponseType::NewDevice && type != ResponseType::DisconnectedDevice) {
        stop();
        const QString message =
                Tr::tr("Shutting down device discovery due to unexpected response: %1");
        emit trackerError(message.arg(QString::fromUtf8(document.toJson())));
        return;
    }

    DeviceEventType eventType = type == ResponseType::NewDevice ? NewDevice
                                                                : DisconnectedDevice;
    QVariantMap variantInfo = document.object().toVariantMap();
    QMap<QString, QString> info;

    switch (eventType) {
    case NewDevice:
    {
        const QVariantMap deviceInfo = variantInfo["device"].value<QVariantMap>();
        for (auto iter = deviceInfo.begin(); iter != deviceInfo.end(); ++iter)
            info[iter.key()] = iter.value().toString();
    }
        break;
    case DisconnectedDevice:
        info["serial"] = variantInfo["serial"].toString();
        break;
    }
    emit deviceEvent(eventType, info);
}

} // Qdb::Internal
