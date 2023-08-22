// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uniform.h"

#include <QColor>
#include <QJsonObject>
#include <QVector2D>

namespace QmlDesigner {

Uniform::Uniform(const QJsonObject &propObj)
{
    QString value, defaultValue, minValue, maxValue;

    m_name = propObj.value("name").toString();
    m_description = propObj.value("description").toString();
    m_type = typeFromString(propObj.value("type").toString());
    defaultValue = propObj.value("defaultValue").toString();

    if (m_type == Type::Sampler) {
        if (!defaultValue.isEmpty())
            defaultValue = getResourcePath(defaultValue);
        if (propObj.contains("enableMipmap"))
            m_enableMipmap = getBoolValue(propObj.value("enableMipmap"), false);
        // Update the mipmap property
        QString mipmapProperty = mipmapPropertyName(m_name);
    }
    if (propObj.contains("value")) {
        value = propObj.value("value").toString();
        if (m_type == Type::Sampler && !value.isEmpty())
            value = getResourcePath(value);
    } else {
        // QEN files don't store the current value, so with those use default value
        value = defaultValue;
    }
    minValue = propObj.value("minValue").toString();
    maxValue = propObj.value("maxValue").toString();

    setValueData(value, defaultValue, minValue, maxValue);
}

QString Uniform::type() const
{
    if (m_type == Type::Bool)
        return "bool";
    if (m_type == Type::Int)
        return "int";
    if (m_type == Type::Float)
        return "float";
    if (m_type == Type::Vec2)
        return "vec2";
    if (m_type == Type::Vec3)
        return "vec3";
    if (m_type == Type::Vec4)
        return "vec4";
    if (m_type == Type::Color)
        return "color";
    if (m_type == Type::Sampler)
        return "image";
    if (m_type == Type::Define)
        return "define";

    qWarning() << "Unknown type";
    return "float";
}

QVariant Uniform::value() const
{
    return m_value;
}

void Uniform::setValue(const QVariant &newValue)
{
    if (m_value != newValue) {
        m_value = newValue;
        emit uniformValueChanged();
    }
}

QVariant Uniform::defaultValue() const
{
    return m_defaultValue;
}

QVariant Uniform::minValue() const
{
    return m_minValue;
}

QVariant Uniform::maxValue() const
{
    return m_maxValue;
}

QString Uniform::name() const
{
    return m_name;
}

QString Uniform::description() const
{
    return m_description;
}

QString Uniform::customValue() const
{
    return m_customValue;
}

void Uniform::setCustomValue(const QString &newCustomValue)
{
    m_customValue = newCustomValue;
}

bool Uniform::useCustomValue() const
{
    return m_useCustomValue;
}

bool Uniform::enabled() const
{
    return m_enabled;
}

void Uniform::setEnabled(bool newEnabled)
{
    m_enabled = newEnabled;
}

bool Uniform::enableMipmap() const
{
    return m_enableMipmap;
}

// Returns name for image mipmap property.
// e.g. "myImage" -> "myImageMipmap".
QString Uniform::mipmapPropertyName(const QString &name) const
{
    QString simplifiedName = name.simplified();
    simplifiedName = simplifiedName.remove(' ');
    simplifiedName += "Mipmap";
    return simplifiedName;
}

Uniform::Type Uniform::typeFromString(const QString &typeString) const
{
    if (typeString == "bool")
        return Type::Bool;
    if (typeString == "int")
        return Type::Int;
    if (typeString == "float")
        return Type::Float;
    if (typeString == "vec2")
        return Type::Vec2;
    if (typeString == "vec3")
        return Type::Vec3;
    if (typeString == "vec4")
        return Type::Vec4;
    if (typeString == "color")
        return Type::Color;
    if (typeString == "image")
        return Type::Sampler;
    if (typeString == "define")
        return Type::Define;

    qWarning() << QString("Unknown type: %1").arg(typeString).toLatin1();
    return Type::Float;
}

// Returns the boolean value of QJsonValue. It can be either boolean
// (true, false) or string ("true", "false"). Returns the defaultValue
// if QJsonValue is undefined, empty, or some other type.
bool Uniform::getBoolValue(const QJsonValue &jsonValue, bool defaultValue)
{
    if (jsonValue.isBool())
        return jsonValue.toBool();

    if (jsonValue.isString())
        return jsonValue.toString().toLower() == "true";

    return defaultValue;
}

// Returns the path for a shader resource
// Used with sampler types
QString Uniform::getResourcePath(const QString &value) const
{
    Q_UNUSED(value)
    //TODO
    return {};
}

// Validation and setting values
void Uniform::setValueData(const QString &value, const QString &defaultValue,
                           const QString &minValue, const QString &maxValue)
{
    m_value = value.isEmpty() ? getInitializedVariant(m_type, false) : valueStringToVariant(m_type, value);
    m_defaultValue = defaultValue.isEmpty() ? getInitializedVariant(m_type, false)
                                            : valueStringToVariant(m_type, defaultValue);
    m_minValue = minValue.isEmpty() ? getInitializedVariant(m_type, false) : valueStringToVariant(m_type, minValue);
    m_maxValue = maxValue.isEmpty() ? getInitializedVariant(m_type, true) : valueStringToVariant(m_type, maxValue);
}

// Initialize the value variant with correct type
QVariant Uniform::getInitializedVariant(Uniform::Type type, bool maxValue)
{
    switch (type) {
    case Uniform::Type::Bool:
        return maxValue ? true : false;
    case Uniform::Type::Int:
        return maxValue ? 100 : 0;
    case Uniform::Type::Float:
        return maxValue ? 1.0 : 0.0;
    case Uniform::Type::Vec2:
        return maxValue ? QVector2D(1.0, 1.0) : QVector2D(0.0, 0.0);
    case Uniform::Type::Vec3:
        return maxValue ? QVector3D(1.0, 1.0, 1.0) : QVector3D(0.0, 0.0, 0.0);
    case Uniform::Type::Vec4:
        return maxValue ? QVector4D(1.0, 1.0, 1.0, 1.0) : QVector4D(0.0, 0.0, 0.0, 0.0);
    case Uniform::Type::Color:
        return maxValue ? QColor::fromRgbF(1.0f, 1.0f, 1.0f, 1.0f) : QColor::fromRgbF(0.0f, 0.0f, 0.0f, 0.0f);
    default:
        return QVariant();
    }
}

QVariant Uniform::valueStringToVariant(const Uniform::Type type, const QString &value)
{
    QVariant variant;
    switch (type) {
    case Type::Bool:
        variant = (value == "true");
        break;
    case Type::Int:
    case Type::Float:
        variant = value;
        break;
    case Type::Vec2: {
        QStringList list = value.split(QLatin1Char(','));
        if (list.size() >= 2)
            variant = QVector2D(list.at(0).toDouble(), list.at(1).toDouble());
    }
    break;
    case Type::Vec3: {
        QStringList list = value.split(QLatin1Char(','));
        if (list.size() >= 3)
            variant = QVector3D(list.at(0).toDouble(), list.at(1).toDouble(), list.at(2).toDouble());
    }
    break;
    case Type::Vec4: {
        QStringList list = value.split(QLatin1Char(','));
        if (list.size() >= 4)
            variant = QVector4D(list.at(0).toDouble(), list.at(1).toDouble(),
                                list.at(2).toDouble(), list.at(3).toDouble());
    }
    break;
    case Type::Color: {
        QStringList list = value.split(QLatin1Char(','));
        if (list.size() >= 4)
            variant = QColor::fromRgbF(list.at(0).toDouble(), list.at(1).toDouble(),
                                       list.at(2).toDouble(), list.at(3).toDouble());
    }
    break;
    case Type::Sampler:
        variant = value;
        break;
    case Uniform::Type::Define:
        variant = value;
        break;
    }

    return variant;
}

} // namespace QmlDesigner
