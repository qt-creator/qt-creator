// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QVariant>

#include <qmldesigner/components/propertyeditor/propertyeditorvalue.h>

QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QVector2D)

namespace EffectMaker {



class Uniform : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString uniformName MEMBER m_displayName CONSTANT)
    Q_PROPERTY(QString uniformType READ typeName CONSTANT)
    Q_PROPERTY(QString uniformDescription READ description CONSTANT)
    Q_PROPERTY(QVariant uniformValue READ value WRITE setValue NOTIFY uniformValueChanged)
    Q_PROPERTY(QVariant uniformBackendValue READ backendValue NOTIFY uniformBackendValueChanged)
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
    QString typeName() const;

    QVariant value() const;
    void setValue(const QVariant &newValue);

    QVariant backendValue() const;

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

    static QString stringFromType(Uniform::Type type);
    static Uniform::Type typeFromString(const QString &typeString);
    static QString typeToProperty(Uniform::Type type);

signals:
    void uniformValueChanged();
    void uniformBackendValueChanged();

private:
    QString mipmapPropertyName(const QString &name) const;
    bool getBoolValue(const QJsonValue &jsonValue, bool defaultValue);
    QString getResourcePath(const QString &value) const;
    void setValueData(const QString &value, const QString &defaultValue,
                      const QString &minValue, const QString &maxValue);

    QVariant getInitializedVariant(bool maxValue);
    QVariant valueStringToVariant(const QString &value);

    Type m_type;
    QVariant m_value;
    QVariant m_defaultValue;
    QVariant m_minValue;
    QVariant m_maxValue;
    QString m_name;
    QString m_displayName;
    QString m_description;
    QString m_customValue;
    bool m_useCustomValue = false;
    bool m_enabled = true;
    bool m_enableMipmap = false;
    QmlDesigner::PropertyEditorValue *m_backendValue = nullptr;

    bool operator==(const Uniform &rhs) const noexcept
    {
        return this->m_name == rhs.m_name;
    }
};

} // namespace EffectMaker
