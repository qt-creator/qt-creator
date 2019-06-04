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

#include "hostmessages.h"

#include <utils/qtcassert.h>

const QString responseField = "response";
const QString requestField = "request";
const QString versionField = "_version";

void setVersionField(QJsonObject &obj)
{
    obj[versionField] = qdbHostMessageVersion;
}

bool checkHostMessageVersion(const QJsonObject &obj)
{
    return obj[versionField].toInt() == qdbHostMessageVersion;
}

QByteArray createRequest(const RequestType &type)
{
    QJsonObject obj;
    setVersionField(obj);
    obj[requestField] = requestTypeString(type);
    return QJsonDocument{obj}.toJson(QJsonDocument::Compact).append('\n');
}

RequestType requestType(const QJsonObject &obj)
{
    const auto fieldValue = obj[requestField];
    if (fieldValue == requestTypeString(RequestType::Devices))
        return RequestType::Devices;
    if (fieldValue == requestTypeString(RequestType::WatchDevices))
        return RequestType::WatchDevices;
    if (fieldValue == requestTypeString(RequestType::StopServer))
        return RequestType::StopServer;
    if (fieldValue == requestTypeString(RequestType::Messages))
        return RequestType::Messages;
    if (fieldValue == requestTypeString(RequestType::WatchMessages))
        return RequestType::WatchMessages;
    if (fieldValue == requestTypeString(RequestType::MessagesAndClear))
        return RequestType::MessagesAndClear;

    return RequestType::Unknown;
}

QString requestTypeString(const RequestType &type)
{
    switch (type) {
    case RequestType::Devices:
        return QStringLiteral("devices");
    case RequestType::WatchDevices:
        return QStringLiteral("watch-devices");
    case RequestType::StopServer:
        return QStringLiteral("stop-server");
    case RequestType::Messages:
        return QStringLiteral("messages");
    case RequestType::WatchMessages:
        return QStringLiteral("watch-messages");
    case RequestType::MessagesAndClear:
        return QStringLiteral("messages-and-clear");
    case RequestType::Unknown:
        break;
    }
    QTC_ASSERT(false, return QString());
}

QJsonObject initializeResponse(const ResponseType &type)
{
    QJsonObject obj;
    setVersionField(obj);
    obj[responseField] = responseTypeString(type);
    return obj;
}

ResponseType responseType(const QJsonObject &obj)
{
    const auto fieldValue = obj[responseField];
    if (fieldValue == responseTypeString(ResponseType::Devices))
        return ResponseType::Devices;
    if (fieldValue == responseTypeString(ResponseType::NewDevice))
        return ResponseType::NewDevice;
    if (fieldValue == responseTypeString(ResponseType::DisconnectedDevice))
        return ResponseType::DisconnectedDevice;
    if (fieldValue == responseTypeString(ResponseType::Stopping))
        return ResponseType::Stopping;
    if (fieldValue == responseTypeString(ResponseType::Messages))
        return ResponseType::Messages;
    if (fieldValue == responseTypeString(ResponseType::InvalidRequest))
        return ResponseType::InvalidRequest;
    if (fieldValue == responseTypeString(ResponseType::UnsupportedVersion))
        return ResponseType::UnsupportedVersion;

    return ResponseType::Unknown;
}

QString responseTypeString(const ResponseType &type)
{
    switch (type) {
    case ResponseType::Devices:
        return QStringLiteral("devices");
    case ResponseType::NewDevice:
        return QStringLiteral("new-device");
    case ResponseType::DisconnectedDevice:
        return QStringLiteral("disconnected-device");
    case ResponseType::Stopping:
        return QStringLiteral("stopping");
    case ResponseType::Messages:
        return QStringLiteral("messages");
    case ResponseType::InvalidRequest:
        return QStringLiteral("invalid-request");
    case ResponseType::UnsupportedVersion:
        return QStringLiteral("unsupported-version");
    case ResponseType::Unknown:
        break;
    }
    QTC_ASSERT(false, return QString());
}

QByteArray serialiseResponse(const QJsonObject &obj)
{
    return QJsonDocument{obj}.toJson(QJsonDocument::Compact).append('\n');
}
