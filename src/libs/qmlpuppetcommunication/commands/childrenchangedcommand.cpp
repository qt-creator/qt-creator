// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "childrenchangedcommand.h"

#include <QDebug>

#include <algorithm>

namespace QmlDesigner {

ChildrenChangedCommand::ChildrenChangedCommand()
    : m_parentInstanceId(-1)
{
}

ChildrenChangedCommand::ChildrenChangedCommand(qint32 parentInstanceId, const QVector<qint32> &children, const QVector<InformationContainer> &informationVector)
    : m_parentInstanceId(parentInstanceId),
      m_childrenVector(children),
      m_informationVector(informationVector)
{
}

QVector<qint32> ChildrenChangedCommand::childrenInstances() const
{
    return m_childrenVector;
}

qint32 ChildrenChangedCommand::parentInstanceId() const
{
    return m_parentInstanceId;
}

QVector<InformationContainer> ChildrenChangedCommand::informations() const
{
    return m_informationVector;
}

void ChildrenChangedCommand::sort()
{
    std::sort(m_childrenVector.begin(), m_childrenVector.end());
    std::sort(m_informationVector.begin(), m_informationVector.end());
}

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command)
{
    out << command.parentInstanceId();
    out << command.childrenInstances();
    out << command.informations();
    return out;
}

QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command)
{
    in >> command.m_parentInstanceId;
    in >> command.m_childrenVector;
    in >> command.m_informationVector;

    return in;
}

bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second)
{
    return first.m_parentInstanceId == second.m_parentInstanceId
            && first.m_childrenVector == second.m_childrenVector
            && first.m_informationVector == second.m_informationVector;
}

QDebug operator <<(QDebug debug, const ChildrenChangedCommand &command)
{
    return debug.nospace() << "ChildrenChangedCommand("
                           << "parentInstanceId: " << command.parentInstanceId() << ", "
                           << "children: " << command.childrenInstances() << ", "
                           << "informations: " << command.informations() << ")";
}

} // namespace QmlDesigner
