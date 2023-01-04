// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lsputils.h"

#include <QHash>
#include <QLoggingCategory>
#include <QVector>

namespace LanguageServerProtocol {

Q_LOGGING_CATEGORY(conversionLog, "qtc.languageserverprotocol.conversion", QtWarningMsg)

template<>
QString fromJsonValue<QString>(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isString())
        qCDebug(conversionLog) << "Expected String in json value but got: " << value;
    return value.toString();
}

template<>
int fromJsonValue<int>(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isDouble())
        qCDebug(conversionLog) << "Expected double in json value but got: " << value;
    return value.toInt();
}

template<>
double fromJsonValue<double>(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isDouble())
        qCDebug(conversionLog) << "Expected double in json value but got: " << value;
    return value.toDouble();
}

template<>
bool fromJsonValue<bool>(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isBool())
        qCDebug(conversionLog) << "Expected bool in json value but got: " << value;
    return value.toBool();
}

template<>
QJsonArray fromJsonValue<QJsonArray>(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isArray())
        qCDebug(conversionLog) << "Expected Array in json value but got: " << value;
    return value.toArray();
}

template<>
QJsonObject fromJsonValue<QJsonObject>(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isObject())
        qCDebug(conversionLog) << "Expected Object in json value but got: " << value;
    return value.toObject();
}

template<>
QJsonValue fromJsonValue<QJsonValue>(const QJsonValue &value)
{
    return value;
}

} // namespace LanguageServerProtocol
