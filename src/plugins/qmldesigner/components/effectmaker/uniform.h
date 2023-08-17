// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QVariant>

namespace QmlDesigner {

class Uniform : public QObject
{
    Q_OBJECT

public:
    enum class Type
    {
        Bool,
        Int,
        Float,
        Vec2,
        Vec3,
        Vec4,
        Color,
        Sampler,
        Define
    };

    Uniform(const QJsonObject &props);

    // TODO: setters & getters

private:
    Type m_type;
    QVariant m_value;
    QVariant m_defaultValue;
    QVariant m_minValue;
    QVariant m_maxValue;
    QString m_name;
    QString m_description;
    QString m_customValue;
    bool m_useCustomValue = false;
    bool m_visible = true;
    bool m_enableMipmap = false;

    bool operator==(const Uniform& rhs) const noexcept
    {
        return this->m_name == rhs.m_name;
    }
};

} // namespace QmlDesigner

