// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonobject.h"

#include <QCoreApplication>

namespace LanguageServerProtocol {

JsonObject &JsonObject::operator=(const JsonObject &other) = default;

JsonObject &JsonObject::operator=(JsonObject &&other)
{
    m_jsonObject.swap(other.m_jsonObject);
    return *this;
}

QJsonObject::iterator JsonObject::insert(const Key key, const JsonObject &object)
{
    return m_jsonObject.insert(key, object.m_jsonObject);
}

QJsonObject::iterator JsonObject::insert(const Key key, const QJsonValue &value)
{
    return m_jsonObject.insert(key, value);
}

} // namespace LanguageServerProtocol
