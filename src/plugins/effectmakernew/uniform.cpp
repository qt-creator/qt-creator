// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uniform.h"
#include <qmldesignerplugin.h>


#include <QColor>
#include <QJsonObject>
#include <QVector2D>

namespace EffectMaker {

Uniform::Uniform(const QJsonObject &propObj)
{
    QString value, defaultValue, minValue, maxValue;

    m_name = propObj.value("name").toString();
    m_description = propObj.value("description").toString();
    m_type = Uniform::typeFromString(propObj.value("type").toString());
    defaultValue = propObj.value("defaultValue").toString();

    m_displayName = propObj.value("displayName").toString();
    if (m_displayName.isEmpty())
        m_displayName = m_name;

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

    m_backendValue = new QmlDesigner::PropertyEditorValue(this);
    m_backendValue->setValue(value);
}

Uniform::Type Uniform::type() const
{
    return m_type;
}

// String representation of the type for qml
QString Uniform::typeName() const
{
    return Uniform::stringFromType(m_type);
}

QVariant Uniform::value() const
{
    return m_value;
}

QVariant Uniform::backendValue() const
{
    return QVariant::fromValue(m_backendValue);
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
    m_value = value.isEmpty() ? getInitializedVariant(false) : valueStringToVariant(value);
    m_defaultValue = defaultValue.isEmpty() ? getInitializedVariant(false)
                                            : valueStringToVariant(defaultValue);
    m_minValue = minValue.isEmpty() ? getInitializedVariant(false) : valueStringToVariant(minValue);
    m_maxValue = maxValue.isEmpty() ? getInitializedVariant(true) : valueStringToVariant(maxValue);
}

// Initialize the value variant with correct type
QVariant Uniform::getInitializedVariant(bool maxValue)
{
    switch (m_type) {
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

QVariant Uniform::valueStringToVariant(const QString &value)
{
    QVariant variant;
    switch (m_type) {
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

QString Uniform::stringFromType(Uniform::Type type)
{
    if (type == Type::Bool)
        return "bool";
    else if (type == Type::Int)
        return "int";
    else if (type == Type::Float)
        return "float";
    else if (type == Type::Vec2)
        return "vec2";
    else if (type == Type::Vec3)
        return "vec3";
    else if (type == Type::Vec4)
        return "vec4";
    else if (type == Type::Color)
        return "color";
    else if (type == Type::Sampler)
        return "sampler2D";
    else if (type == Type::Define)
        return "define";

    qWarning() << QString("Unknown type");
    return "float";
}

Uniform::Type Uniform::typeFromString(const QString &typeString)
{
    if (typeString == "bool")
        return Uniform::Type::Bool;
    else if (typeString == "int")
        return Uniform::Type::Int;
    else if (typeString == "float")
        return Uniform::Type::Float;
    else if (typeString == "vec2")
        return Uniform::Type::Vec2;
    else if (typeString == "vec3")
        return Uniform::Type::Vec3;
    else if (typeString == "vec4")
        return Uniform::Type::Vec4;
    else if (typeString == "color")
        return Uniform::Type::Color;
    else if (typeString == "sampler2D")
        return Uniform::Type::Sampler;
    else if (typeString == "define")
        return Uniform::Type::Define;

    qWarning() << QString("Unknown type: %1").arg(typeString).toLatin1();
    return Uniform::Type::Float;
}

QString Uniform::typeToProperty(Uniform::Type type)
{
    if (type == Uniform::Type::Bool)
        return "bool";
    else if (type == Uniform::Type::Int)
        return "int";
    else if (type == Uniform::Type::Float)
        return "real";
    else if (type == Uniform::Type::Vec2)
        return "point";
    else if (type == Uniform::Type::Vec3)
        return "vector3d";
    else if (type == Uniform::Type::Vec4)
        return "vector4d";
    else if (type == Uniform::Type::Color)
        return "color";
    else if (type == Uniform::Type::Sampler || type == Uniform::Type::Define)
        return "var";

    qWarning() << QString("Unhandled const variable type: %1").arg(int(type)).toLatin1();
    return QString();
}

} // namespace EffectMaker
