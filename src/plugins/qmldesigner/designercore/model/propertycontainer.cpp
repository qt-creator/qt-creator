/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
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
#include <QDebug>

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

