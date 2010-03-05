/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "propertycontainer.h"
#include "propertyparser.h"
#include <QVariant>
#include <QString>
#include <QRegExp>
#include <QSize>
#include <QSizeF>
#include <QPoint>
#include <QPointF>
#include <QtDebug>

namespace QmlDesigner {

using namespace QmlDesigner::Internal;


// Creates invalid PropertyContainer
PropertyContainer::PropertyContainer()
{
}

PropertyContainer::PropertyContainer(const QString &name, const QString &type, const QVariant &value)
        : m_name(name), m_type(type), m_value(value)
{
    Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Name of property cannot be empty");
    Q_ASSERT_X(!type.isEmpty(), Q_FUNC_INFO, "Type of property cannot be empty");
}

void PropertyContainer::set(const QString &name, const QString &type, const QVariant &value)
{
    m_name = name;
    m_type = type;
    m_value = value;
}

bool PropertyContainer::isValid() const
{
    return !m_name.isEmpty() && m_value.isValid();
}

QString PropertyContainer::name() const
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
    for( int i = 0; i < count; i++) {
        PropertyContainer propertyContainer;
        stream >> propertyContainer;
        propertyContainerList.append(propertyContainer);
    }

    return stream;
}


} //namespace QmlDesigner

