// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hostmessages.h"

#include <utils/qtcassert.h>

const int qdbHostMessageVersion = 1;

const QString responseField = "response";
const QString requestField = "request";
const QString versionField = "_version";

void setVersionField(QJsonObject &obj)
{
    obj[versionField] = qdbHostMessageVersion;
}

static QString requestTypeString(const RequestType &type)
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
    QTC_ASSERT(false, return {});
}

QByteArray createRequest(const RequestType &type)
{
    QJsonObject obj;
    setVersionField(obj);
    obj[requestField] = requestTypeString(type);
    return QJsonDocument{obj}.toJson(QJsonDocument::Compact).append('\n');
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
