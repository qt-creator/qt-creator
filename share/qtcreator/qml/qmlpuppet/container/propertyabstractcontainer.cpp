/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "propertyabstractcontainer.h"

namespace QmlDesigner {

PropertyAbstractContainer::PropertyAbstractContainer()
    : m_instanceId(-1)
{
}

PropertyAbstractContainer::PropertyAbstractContainer(qint32 instanceId, const QString &name, const QString &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyAbstractContainer::instanceId() const
{
    return m_instanceId;
}

QString PropertyAbstractContainer::name() const
{
    return m_name;
}

bool PropertyAbstractContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

QString PropertyAbstractContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_dynamicTypeName;

    return in;
}

} // namespace QmlDesigner
