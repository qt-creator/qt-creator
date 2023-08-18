// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uniform.h"

#include <QJsonObject>

namespace QmlDesigner {

Uniform::Uniform(const QJsonObject &propObj)
{
    //TODO: some cases such as missing values or default values not yet implemented

    QString value, defaultValue;

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
    m_customValue = propObj.value("customValue").toString();
    m_useCustomValue = getBoolValue(propObj.value("useCustomValue"), false);
    m_minValue = propObj.value("minValue").toString();
    m_maxValue = propObj.value("maxValue").toString();
    //TODO: set uniform value data after validating, for now just set to current value
    m_defaultValue = defaultValue;
    m_value = value;
}

Uniform::Type Uniform::type() const
{
    return m_type;
}

QVariant Uniform::value() const
{
    return m_value;
}

void Uniform::setValue(const QVariant &newValue)
{
    m_value = newValue;
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

} // namespace QmlDesigner
