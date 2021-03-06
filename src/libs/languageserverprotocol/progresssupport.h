/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <utils/variant.h>

#include <QJsonValue>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT ProgressToken : public Utils::variant<int, QString>
{
public:
    ProgressToken();
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
    Utils::optional<bool> cancellable() const { return optionalValue<bool>(cancellableKey); }
    void setCancellable(bool cancellable) { insert(cancellableKey, cancellable); }
    void clearCancellable() { remove(cancellableKey); }

    /**
     * Optional, more detailed associated progress message. Contains
     * complementary information to the `title`.
     *
     * Examples: "3/25 files", "project/src/module2", "node_modules/some_dep".
     * If unset, the previous progress message (if any) is still valid.
     */
    Utils::optional<QString> message() const { return optionalValue<QString>(messageKey); }
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
    Utils::optional<int> percentage() const { return optionalValue<int>(percentageKey); }
    void setPercentage(int percentage) { insert(percentageKey, percentage); }
    void clearPercentage() { remove(percentageKey); }

private:
    constexpr static const char cancellableKey[] = "cancellable";
    constexpr static const char messageKey[] = "message";
    constexpr static const char percentageKey[] = "percentage";
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

private:
    constexpr static const char titleKey[] = "title";
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressEnd : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
     * Optional, a final message indicating to for example indicate the outcome
     * of the operation.
     */
    Utils::optional<QString> message() const { return optionalValue<QString>(messageKey); }
    void setMessage(const QString &message) { insert(messageKey, message); }
    void clearMessage() { remove(messageKey); }

private:
    constexpr static const char cancellableKey[] = "cancellable";
    constexpr static const char messageKey[] = "message";
    constexpr static const char percentageKey[] = "percentage";
};

class LANGUAGESERVERPROTOCOL_EXPORT ProgressParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    ProgressToken token() const { return ProgressToken(JsonObject::value(tokenKey)); }
    void setToken(const ProgressToken &token) { insert(tokenKey, token); }

    using ProgressType
        = Utils::variant<WorkDoneProgressBegin, WorkDoneProgressReport, WorkDoneProgressEnd>;
    ProgressType value() const;
    void setValue(const ProgressType &value);

    bool isValid() const override { return contains(tokenKey) && contains(valueKey); }

private:
    static constexpr char tokenKey[] = "token";
    static constexpr char valueKey[] = "value";
};

class LANGUAGESERVERPROTOCOL_EXPORT ProgressNotification : public Notification<ProgressParams>
{
public:
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

private:
    constexpr static const char tokenKey[] = "token";
};

using WorkDoneProgressCreateParams = ProgressTokenParams;

class LANGUAGESERVERPROTOCOL_EXPORT WorkDoneProgressCreateRequest
    : public Request<std::nullptr_t, std::nullptr_t, WorkDoneProgressCreateParams>
{
public:
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
