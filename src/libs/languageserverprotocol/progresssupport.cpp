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
    if (Utils::holds_alternative<QString>(*this))
        return QJsonValue(Utils::get<QString>(*this));
    return QJsonValue(Utils::get<int>(*this));
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
