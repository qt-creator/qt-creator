/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbdevicetracker.h"
#include "qdbwatcher.h"

#include "../qdbutils.h"
#include "hostmessages.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace Qdb {
namespace Internal {

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
                tr("Shutting down device discovery due to unexpected response: %1");
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

} // namespace Internal
} // namespace Qdb
