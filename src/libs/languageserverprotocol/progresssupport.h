// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

#include <QJsonValue>

#include <variant>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT ProgressToken : public std::variant<int, QString>
{
public:
    using variant::variant;
    explicit ProgressToken(const QJsonValue &value);

    bool isValid() { return true; }
    operator QJsonValue() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressReport : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
     * Controls if a cancel button should be shown to allow the user to cancel the
     * long running operation.
     * Clients that don't support cancellation can ignore the setting.
     */
    std::optional<bool> cancellable() const { return optionalValue<bool>(cancellableKey); }
    void setCancellable(bool cancellable) { insert(cancellableKey, cancellable); }
    void clearCancellable() { remove(cancellableKey); }

    /**
     * Optional, more detailed associated progress message. Contains
     * complementary information to the `title`.
     *
     * Examples: "3/25 files", "project/src/module2", "node_modules/some_dep".
     * If unset, the previous progress message (if any) is still valid.
     */
    std::optional<QString> message() const { return optionalValue<QString>(messageKey); }
    void setMessage(const QString &message) { insert(messageKey, message); }
    void clearMessage() { remove(messageKey); }

    /**
     * Optional progress percentage to display (value 100 is considered 100%).
     * If not provided infinite progress is assumed and clients are allowed
     * to ignore the `percentage` value in subsequent in report notifications.
     *
     * The value should be steadily rising. Clients are free to ignore values
     * that are not following this rule.
     */

    // Allthough percentage is defined as an uint by the protocol some server
    // return a double here. Be less strict and also use a double.
    // CAUTION: the range is still 0 - 100 and not 0 - 1
    std::optional<double> percentage() const { return optionalValue<double>(percentageKey); }
    void setPercentage(double percentage) { insert(percentageKey, percentage); }
    void clearPercentage() { remove(percentageKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressBegin : public WorkDoneProgressReport
{
public:
    using WorkDoneProgressReport::WorkDoneProgressReport;

    /**
     * Mandatory title of the progress operation. Used to briefly inform about
     * the kind of operation being performed.
     *
     * Examples: "Indexing" or "Linking dependencies".
     */

    QString title() const { return typedValue<QString>(titleKey); }
    void setTitle(const QString &title) { insert(titleKey, title); }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressEnd : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
     * Optional, a final message indicating to for example indicate the outcome
     * of the operation.
     */
    std::optional<QString> message() const { return optionalValue<QString>(messageKey); }
    void setMessage(const QString &message) { insert(messageKey, message); }
    void clearMessage() { remove(messageKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ProgressParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    ProgressToken token() const { return ProgressToken(JsonObject::value(tokenKey)); }
    void setToken(const ProgressToken &token) { insert(tokenKey, token); }

    using ProgressType
        = std::variant<WorkDoneProgressBegin, WorkDoneProgressReport, WorkDoneProgressEnd>;
    ProgressType value() const;
    void setValue(const ProgressType &value);

    bool isValid() const override { return contains(tokenKey) && contains(valueKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ProgressNotification : public Notification<ProgressParams>
{
public:
    ProgressNotification(const ProgressParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "$/progress";
};

class LANGUAGESERVERPROTOCOL_EXPORT ProgressTokenParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The token to be used to report progress.
    ProgressToken token() const { return typedValue<ProgressToken>(tokenKey); }
    void setToken(const ProgressToken &token) { insert(tokenKey, token); }
};

using WorkDoneProgressCreateParams = ProgressTokenParams;

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressCreateRequest
    : public Request<std::nullptr_t, std::nullptr_t, WorkDoneProgressCreateParams>
{
public:
    WorkDoneProgressCreateRequest(const WorkDoneProgressCreateParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "window/workDoneProgress/create";
};

using WorkDoneProgressCancelParams = ProgressTokenParams;

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressCancelRequest
    : public Request<std::nullptr_t, std::nullptr_t, WorkDoneProgressCancelParams>
{
public:
    using Request::Request;
    constexpr static const char methodName[] = "window/workDoneProgress/cancel";
};


} // namespace LanguageServerProtocol
