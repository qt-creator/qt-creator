/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

    bool isValid(ErrorHierarchy *error) const override
    { return check<int>(error, typeKey) && check<QString>(error, messageKey); }
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
    MessageActionItem &operator=(const MessageActionItem &) = default;

    QString title() const { return typedValue<QString>(titleKey); }
    void setTitle(QString title) { insert(titleKey, title); }

    bool isValid(ErrorHierarchy *error) const override { return check<QString>(error, titleKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ShowMessageRequestParams : public ShowMessageParams
{
public:
    using ShowMessageParams::ShowMessageParams;

    Utils::optional<QList<MessageActionItem>> actions() const
    { return optionalArray<MessageActionItem>(actionsKey); }
    void setActions(const QList<MessageActionItem> &actions) { insertArray(actionsKey, actions); }
    void clearActions() { remove(actionsKey); }

    bool isValid(ErrorHierarchy *error) const override;
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
