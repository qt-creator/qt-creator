// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertycontainer.h"
#include "propertyparser.h"
#include <QVariant>
#include <QString>
#include <QDebug>

namespace QmlDesigner {

using namespace QmlDesigner::Internal;


// Creates invalid PropertyContainer
PropertyContainer::PropertyContainer() = default;

PropertyContainer::PropertyContainer(const PropertyName &name, const QString &type, const QVariant &value)
        : m_name(name), m_type(type), m_value(value)
{
    Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Name of property cannot be empty");
    Q_ASSERT_X(!type.isEmpty(), Q_FUNC_INFO, "Type of property cannot be empty");
}

void PropertyContainer::set(const PropertyName &name, const QString &type, const QVariant &value)
{
    m_name = name;
    m_type = type;
    m_value = value;
}

bool PropertyContainer::isValid() const
{
    return !m_name.isEmpty() && m_value.isValid();
}

PropertyName PropertyContainer::name() const
{
    return m_name;
}

QVariant PropertyContainer::value() const
{
    if (m_value.typeId() == QVariant::String)
        m_value = PropertyParser::read(m_type, m_value.toString());
    return m_value;
}


void PropertyContainer::setValue(const QVariant &value)
{
    m_value = value;
}

QString PropertyContainer::type() const
{
    return m_type;
}

QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer)
{
    Q_ASSERT(!propertyContainer.name().isEmpty());
    Q_ASSERT(!propertyContainer.type().isEmpty());
    Q_ASSERT(propertyContainer.value().isValid());
    stream << propertyContainer.name();
    stream << propertyContainer.type();
    stream << propertyContainer.value();


    return stream;
}

QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer)
{

    stream >> propertyContainer.m_name;
    stream >> propertyContainer.m_type;
    stream >> propertyContainer.m_value;

    Q_ASSERT(!propertyContainer.name().isEmpty());

    return stream;
}

QDebug operator<<(QDebug debug, const PropertyContainer &propertyContainer)
{
    debug << propertyContainer.m_name;
    debug << propertyContainer.m_type;
    debug << propertyContainer.m_value;

    return debug.space();
}

} //namespace QmlDesigner

