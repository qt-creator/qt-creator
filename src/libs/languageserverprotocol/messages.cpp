// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messages.h"

namespace LanguageServerProtocol {

constexpr const char ShowMessageNotification::methodName[];
constexpr const char ShowMessageRequest::methodName[];
constexpr const char LogMessageNotification::methodName[];
constexpr const char TelemetryNotification::methodName[];

ShowMessageNotification::ShowMessageNotification(const ShowMessageParams &params)
    : Notification(methodName, params)
{ }

ShowMessageRequest::ShowMessageRequest(const ShowMessageRequestParams &params)
    : Request(methodName, params)
{ }

LogMessageNotification::LogMessageNotification(const LogMessageParams &params)
    : Notification(methodName, params)
{ }

TelemetryNotification::TelemetryNotification(const JsonObject &params)
    : Notification(methodName, params)
{ }

static QString messageTypeName(int messageType)
{
    switch (messageType) {
    case Error: return QString("Error");
    case Warning: return QString("Warning");
    case Info: return QString("Info");
    case Log: return QString("Log");
    }
    return QString("");
}

QString ShowMessageParams::toString() const
{
    return messageTypeName(type()) + ": " + message();
}

} // namespace LanguageServerProtocol
