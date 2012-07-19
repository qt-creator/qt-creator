/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "propertybindingcontainer.h"


namespace QmlDesigner {

PropertyBindingContainer::PropertyBindingContainer()
    : m_instanceId(-1)
{
}

PropertyBindingContainer::PropertyBindingContainer(qint32 instanceId, const QString &name, const QString &expression, const QString &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_expression(expression),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyBindingContainer::instanceId() const
{
    return m_instanceId;
}

QString PropertyBindingContainer::name() const
{
    return m_name;
}

QString PropertyBindingContainer::expression() const
{
    return m_expression;
}

bool PropertyBindingContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

QString PropertyBindingContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyBindingContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.expression();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_expression;
    in >> container.m_dynamicTypeName;

    return in;
}

} // namespace QmlDesigner
