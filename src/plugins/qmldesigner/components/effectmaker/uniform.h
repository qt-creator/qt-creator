// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QVariant>

QT_FORWARD_DECLARE_CLASS(QJsonObject)

namespace QmlDesigner {

class Uniform : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString uniformName MEMBER m_name CONSTANT)
    Q_PROPERTY(Type uniformType MEMBER m_type CONSTANT)
    Q_PROPERTY(QVariant uniformValue MEMBER m_value CONSTANT)
    Q_PROPERTY(QVariant uniformMinValue MEMBER m_minValue CONSTANT)
    Q_PROPERTY(QVariant uniformMaxValue MEMBER m_maxValue CONSTANT)

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
    QString description() const;

    QString customValue() const;
    void setCustomValue(const QString &newCustomValue);
    bool useCustomValue() const;

    bool enabled() const;
    void setEnabled(bool newEnabled);

    bool enableMipmap() const;

private:
    QString mipmapPropertyName(const QString &name) const;

    Uniform::Type typeFromString(const QString &typeString) const;
    bool getBoolValue(const QJsonValue &jsonValue, bool defaultValue);
    QString getResourcePath(const QString &value) const;

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

    bool operator==(const Uniform &rhs) const noexcept
    {
        return this->m_name == rhs.m_name;
    }
};

} // namespace QmlDesigner
