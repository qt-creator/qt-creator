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

static QString messageTypeName(ShowMessageParams::MessageType messageType)
{
    switch (messageType) {
    case ShowMessageParams::MessageType::Error:
        return QString("Error");
    case ShowMessageParams::MessageType::Warning:
        return QString("Warning");
    case ShowMessageParams::MessageType::Info:
        return QString("Info");
    case ShowMessageParams::MessageType::Log:
        return QString("Log");
    case ShowMessageParams::MessageType::Debug:
        return QString("Debug");
    }
    return QString("");
}

QString ShowMessageParams::toString() const
{
    return messageTypeName(type()) + ": " + message();
}

QtMsgType ShowMessageParams::qtMsgType() const
{
    switch (type()) {
    case ShowMessageParams::MessageType::Error:
        return QtCriticalMsg;
    case ShowMessageParams::MessageType::Warning:
        return QtWarningMsg;
    case ShowMessageParams::MessageType::Info:
        return QtInfoMsg;
    case ShowMessageParams::MessageType::Log:
        return QtInfoMsg;
    case ShowMessageParams::MessageType::Debug:
        return QtDebugMsg;
    }

    return QtCriticalMsg;
}

} // namespace LanguageServerProtocol
