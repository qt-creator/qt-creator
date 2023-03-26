// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

enum MessageType {
    Error = 1,
    Warning = 2,
    Info = 3,
    Log = 4,
};

class LANGUAGESERVERPROTOCOL_EXPORT ShowMessageParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    int type() const { return typedValue<int>(typeKey); }
    void setType(int type) { insert(typeKey, type); }

    QString message() const { return typedValue<QString>(messageKey); }
    void setMessage(QString message) { insert(messageKey, message); }

    QString toString() const;

    bool isValid() const override
    { return contains(typeKey) && contains(messageKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ShowMessageNotification : public Notification<ShowMessageParams>
{
public:
    explicit ShowMessageNotification(const ShowMessageParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "window/showMessage";
};

class LANGUAGESERVERPROTOCOL_EXPORT MessageActionItem : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString title() const { return typedValue<QString>(titleKey); }
    void setTitle(QString title) { insert(titleKey, title); }

    bool isValid() const override { return contains(titleKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ShowMessageRequestParams : public ShowMessageParams
{
public:
    using ShowMessageParams::ShowMessageParams;

    std::optional<QList<MessageActionItem>> actions() const
    { return optionalArray<MessageActionItem>(actionsKey); }
    void setActions(const QList<MessageActionItem> &actions) { insertArray(actionsKey, actions); }
    void clearActions() { remove(actionsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ShowMessageRequest : public Request<
        LanguageClientValue<MessageActionItem>, std::nullptr_t, ShowMessageRequestParams>
{
public:
    explicit ShowMessageRequest(const ShowMessageRequestParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "window/showMessageRequest";
};

using LogMessageParams = ShowMessageParams;

class LANGUAGESERVERPROTOCOL_EXPORT LogMessageNotification : public Notification<LogMessageParams>
{
public:
    explicit LogMessageNotification(const LogMessageParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "window/logMessage";
};

class LANGUAGESERVERPROTOCOL_EXPORT TelemetryNotification : public Notification<JsonObject>
{
public:
    explicit TelemetryNotification(const JsonObject &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "telemetry/event";

    bool parametersAreValid(QString * /*error*/) const override { return params().has_value(); }
};

} // namespace LanguageClient
