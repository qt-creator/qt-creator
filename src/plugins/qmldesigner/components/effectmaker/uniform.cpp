// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uniform.h"

#include <QJsonObject>

namespace QmlDesigner {

Uniform::Uniform(const QJsonObject &props)
{
    Q_UNUSED(props)
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

void Uniform::setName(const QString &newName)
{
    m_name = newName;
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

} // namespace QmlDesigner
