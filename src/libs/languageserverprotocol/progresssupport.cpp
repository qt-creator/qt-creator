// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progresssupport.h"

#include <utils/qtcassert.h>

#include <QUuid>

namespace LanguageServerProtocol {

ProgressToken::ProgressToken(const QJsonValue &value)
{
    if (!QTC_GUARD(value.isDouble() || value.isString()))
        emplace<QString>(QUuid::createUuid().toString());
    else if (value.isDouble())
        emplace<int>(value.toInt());
    else
        emplace<QString>(value.toString());
}

ProgressToken::operator QJsonValue() const
{
    if (std::holds_alternative<QString>(*this))
        return QJsonValue(std::get<QString>(*this));
    return QJsonValue(std::get<int>(*this));
}

ProgressParams::ProgressType ProgressParams::value() const
{
    QJsonObject paramsValue = JsonObject::value(valueKey).toObject();
    if (paramsValue[kindKey] == "begin")
        return ProgressParams::ProgressType(WorkDoneProgressBegin(paramsValue));
    if (paramsValue[kindKey] == "report")
        return ProgressParams::ProgressType(WorkDoneProgressReport(paramsValue));
    return ProgressParams::ProgressType(WorkDoneProgressEnd(paramsValue));
}

void ProgressParams::setValue(const ProgressParams::ProgressType &value)
{
    insertVariant<WorkDoneProgressBegin, WorkDoneProgressReport, WorkDoneProgressEnd>(valueKey, value);
}

ProgressNotification::ProgressNotification(const ProgressParams &params)
    : Notification(methodName, params)
{

}

WorkDoneProgressCreateRequest::WorkDoneProgressCreateRequest(const WorkDoneProgressCreateParams &params)
    : Request(methodName, params)
{

}

} // namespace LanguageServerProtocol
