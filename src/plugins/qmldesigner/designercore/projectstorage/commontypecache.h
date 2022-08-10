/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "projectstoragetypes.h"

#include <tuple>
#include <type_traits>

QT_BEGIN_NAMESPACE
class QColor;
class QDateTime;
class QString;
class QUrl;
class QVariant;
class QVector2D;
class QVector3D;
class QVector4D;
QT_END_NAMESPACE

namespace QmlDesigner::Storage::Info {

inline constexpr char QtQuick[] = "QtQuick";
inline constexpr char QML[] = "QML";
inline constexpr char QMLNative[] = "QML-cppnative";
inline constexpr char Item[] = "Item";
inline constexpr char DoubleType[] = "double";
inline constexpr char IntType[] = "int";
inline constexpr char BoolType[] = "bool";
inline constexpr char FloatType[] = "float";
inline constexpr char var[] = "var";
inline constexpr char string[] = "string";
inline constexpr char date[] = "date";
inline constexpr char url[] = "url";
inline constexpr char color[] = "color";
inline constexpr char vector2d[] = "vector2d";
inline constexpr char vector3d[] = "vector3d";
inline constexpr char vector4d[] = "vector4d";

template<const auto &moduleName_, const auto &typeName_>
struct CacheType
{
    QmlDesigner::ModuleId moduleId;
    QmlDesigner::TypeId typeId;
};

template<typename ProjectStorage>
class CommonTypeCache
{
    using CommonTypes = std::tuple<CacheType<QtQuick, Item>,
                                   CacheType<QtQuick, color>,
                                   CacheType<QtQuick, vector2d>,
                                   CacheType<QtQuick, vector3d>,
                                   CacheType<QtQuick, vector4d>,
                                   CacheType<QML, DoubleType>,
                                   CacheType<QML, var>,
                                   CacheType<QML, IntType>,
                                   CacheType<QML, BoolType>,
                                   CacheType<QML, string>,
                                   CacheType<QML, date>,
                                   CacheType<QML, date>,
                                   CacheType<QML, url>,
                                   CacheType<QMLNative, FloatType>>;

public:
    CommonTypeCache(const ProjectStorage &projectStorage)
        : m_projectStorage{projectStorage}
    {}

    void resetTypeIds()
    {
        std::apply([](auto &...type) { ((type.typeId = QmlDesigner::TypeId{}), ...); }, m_types);
    }

    template<const auto &moduleName, const auto &typeName>
    auto typeId() const
    {
        auto &type = std::get<CacheType<moduleName, typeName>>(m_types);
        if (type.typeId)
            return type.typeId;

        if (!type.moduleId)
            type.moduleId = m_projectStorage.moduleId(moduleName);

        type.typeId = m_projectStorage.typeId(type.moduleId,
                                              typeName,
                                              QmlDesigner::Storage::Synchronization::Version{});

        return type.typeId;
    }

    template<const auto &typeName>
    auto builtinTypeId() const
    {
        return typeId<QML, typeName>();
    }

    template<typename Type>
    auto builtinTypeId() const
    {
        if constexpr (std::is_same_v<Type, double>)
            return typeId<QML, DoubleType>();
        if constexpr (std::is_same_v<Type, int>)
            return typeId<QML, IntType>();
        if constexpr (std::is_same_v<Type, bool>)
            return typeId<QML, BoolType>();
        if constexpr (std::is_same_v<Type, float>)
            return typeId<QMLNative, FloatType>();
        if constexpr (std::is_same_v<Type, QString>)
            return typeId<QML, string>();
        if constexpr (std::is_same_v<Type, QDateTime>)
            return typeId<QML, date>();
        if constexpr (std::is_same_v<Type, QUrl>)
            return typeId<QML, url>();
        if constexpr (std::is_same_v<Type, QVariant>)
            return typeId<QML, var>();
        if constexpr (std::is_same_v<Type, QColor>)
            return typeId<QtQuick, color>();
        if constexpr (std::is_same_v<Type, QVector2D>)
            return typeId<QtQuick, vector2d>();
        if constexpr (std::is_same_v<Type, QVector3D>)
            return typeId<QtQuick, vector3d>();
        if constexpr (std::is_same_v<Type, QVector4D>)
            return typeId<QtQuick, vector4d>();
        else
            return TypeId{};
    }

private:
    const ProjectStorage &m_projectStorage;
    mutable CommonTypes m_types;
};

} // namespace QmlDesigner::Storage::Info
