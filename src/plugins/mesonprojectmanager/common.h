// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

template<typename T>
inline T load(QJsonDocument &&doc);

template<>
inline QJsonArray load<QJsonArray>(QJsonDocument &&doc)
{
    return doc.array();
}

template<>
inline QJsonObject load<QJsonObject>(QJsonDocument &&doc)
{
    return doc.object();
}

template<typename T>
inline T load(const QJsonValueRef &ref);

template<>
inline QJsonArray load<QJsonArray>(const QJsonValueRef &ref)
{
    return ref.toArray();
}

template<>
inline QJsonObject load<QJsonObject>(const QJsonValueRef &ref)
{
    return ref.toObject();
}

template<typename T>
inline std::optional<T> load(const QString &jsonFile)
{
    QFile js(jsonFile);
    js.open(QIODevice::ReadOnly | QIODevice::Text);
    if (js.isOpen()) {
        auto data = js.readAll();
        return load<T>(QJsonDocument::fromJson(data));
    }
    return std::nullopt;
}

template<typename T>
inline std::optional<T> get(const QJsonObject &obj, const QString &name);

template<>
inline std::optional<QJsonArray> get<QJsonArray>(const QJsonObject &obj, const QString &name)
{
    if (obj.contains(name)) {
        auto child = obj[name];
        if (child.isArray())
            return child.toArray();
    }
    return std::nullopt;
}

template<>
inline std::optional<QJsonObject> get<QJsonObject>(const QJsonObject &obj, const QString &name)
{
    if (obj.contains(name)) {
        auto child = obj[name];
        if (child.isObject())
            return child.toObject();
    }
    return std::nullopt;
}

template<typename T, typename... Strings>
inline std::optional<T> get(const QJsonObject &obj, const QString &firstPath, const Strings &...path)
{
    if (obj.contains(firstPath))
        return get<T>(obj[firstPath].toObject(), path...);
    return std::nullopt;
}

} // namespace Internal
} // namespace MesonProjectManager
