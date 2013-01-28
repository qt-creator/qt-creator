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

#include "informationcontainer.h"

namespace QmlDesigner {

InformationContainer::InformationContainer()
    : m_instanceId(-1),
      m_name(NoName)
{
}

InformationContainer::InformationContainer(qint32 instanceId,
                                           InformationName name,
                                           const QVariant &information,
                                           const QVariant &secondInformation,
                                           const QVariant &thirdInformation)
    : m_instanceId(instanceId),
    m_name(name),
    m_information(information),
    m_secondInformation(secondInformation),
    m_thirdInformation(thirdInformation)
{
}

qint32 InformationContainer::instanceId() const
{
    return m_instanceId;
}

InformationName InformationContainer::name() const
{
    return InformationName(m_name);
}

QString InformationContainer::nameAsString() const
{
    switch (name()) {
        case NoName:
            return QLatin1String("NoName");
        case Size:
            return QLatin1String("Size");
        case BoundingRect:
            return QLatin1String("BoundingRect");
        case Transform:
            return QLatin1String("Transform");
        case HasAnchor:
            return QLatin1String("HasAnchor");
        case Anchor:
            return QLatin1String("Anchor");
        case InstanceTypeForProperty:
            return QLatin1String("InstanceTypeForProperty");
        case PenWidth:
            return QLatin1String("PenWidth");
        case Position:
            return QLatin1String("Position");
        case IsInPositioner:
            return QLatin1String("IsInPositioner");
        case SceneTransform:
            return QLatin1String("SceneTransform");
        case IsResizable:
            return QLatin1String("IsResizable");
        case IsMovable:
            return QLatin1String("IsMovable");
        case IsAnchoredByChildren:
            return QLatin1String("IsAnchoredByChildren");
        case IsAnchoredBySibling:
            return QLatin1String("IsAnchoredBySibling");
        case HasContent:
            return QLatin1String("HasContent");
        case HasBindingForProperty:
            return QLatin1String("HasBindingForProperty");
    }
    return QLatin1String("Unknown");
}

QVariant InformationContainer::information() const
{
    return m_information;
}

QVariant InformationContainer::secondInformation() const
{
    return m_secondInformation;
}

QVariant InformationContainer::thirdInformation() const
{
    return m_thirdInformation;
}

QDataStream &operator<<(QDataStream &out, const InformationContainer &container)
{
    out << container.instanceId();//
    out << container.m_name;
    out << container.information();
    out << container.secondInformation();
    out << container.thirdInformation();

    return out;
}

QDataStream &operator>>(QDataStream &in, InformationContainer &container)
{

    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_information;
    in >> container.m_secondInformation;
    in >> container.m_thirdInformation;

    return in;
}

} // namespace QmlDesigner
