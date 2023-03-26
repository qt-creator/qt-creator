// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qbytearray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>

const int qdbHostMessageVersion = 1;
bool checkHostMessageVersion(const QJsonObject &obj);

enum class RequestType
{
    Unknown = 0,
    Devices,
    WatchDevices,
    StopServer,
    WatchMessages,
    Messages,
    MessagesAndClear,
};

QByteArray createRequest(const RequestType &type);
RequestType requestType(const QJsonObject &obj);
QString requestTypeString(const RequestType &type);

enum class ResponseType
{
    Unknown = 0,
    Devices,
    NewDevice,
    DisconnectedDevice,
    Stopping,
    InvalidRequest,
    UnsupportedVersion,
    Messages,
};

QJsonObject initializeResponse(const ResponseType &type);
ResponseType responseType(const QJsonObject &obj);
QString responseTypeString(const ResponseType &type);
QByteArray serialiseResponse(const QJsonObject &obj);
