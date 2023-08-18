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

    Type type() const;

    QVariant value() const;
    void setValue(const QVariant &newValue);

    QVariant defaultValue() const;

    QVariant minValue() const;

    QVariant maxValue() const;

    QString name() const;
    void setName(const QString &newName);

    QString description() const;

    QString customValue() const;
    void setCustomValue(const QString &newCustomValue);

    bool useCustomValue() const;

    bool enabled() const;
    void setEnabled(bool newEnabled);

    bool enableMipmap() const;

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
    bool m_enabled = true;
    bool m_enableMipmap = false;

    bool operator==(const Uniform& rhs) const noexcept
    {
        return this->m_name == rhs.m_name;
    }
};

} // namespace QmlDesigner

