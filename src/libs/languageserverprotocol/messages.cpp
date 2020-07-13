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

#include "messages.h"

#include "icontent.h"

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

bool ShowMessageRequestParams::isValid(ErrorHierarchy *error) const
{
    return ShowMessageParams::isValid(error)
            && checkOptionalArray<MessageActionItem>(error, actionsKey);
}

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
