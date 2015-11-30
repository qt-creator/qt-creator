/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "propertycontainer.h"
#include "propertyparser.h"
#include <QVariant>
#include <QString>
#include <QDebug>

namespace QmlDesigner {

using namespace QmlDesigner::Internal;


// Creates invalid PropertyContainer
PropertyContainer::PropertyContainer()
{
}

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
    if (m_value.type() == QVariant::String)
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

QDataStream &operator<<(QDataStream &stream, const QList<PropertyContainer> &propertyContainerList)
{
    stream << propertyContainerList.count();
    foreach (const PropertyContainer &propertyContainer, propertyContainerList)
        stream << propertyContainer;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, QList<PropertyContainer> &propertyContainerList)
{
    int count;
    stream >> count;
    Q_ASSERT(count >= 0);
    for ( int i = 0; i < count; i++) {
        PropertyContainer propertyContainer;
        stream >> propertyContainer;
        propertyContainerList.append(propertyContainer);
    }

    return stream;
}

QDebug operator<<(QDebug debug, QList<PropertyContainer> &propertyContainerList)
{
    foreach (const PropertyContainer &propertyContainer, propertyContainerList)
        debug << propertyContainer;

    return debug.space();
}


} //namespace QmlDesigner

