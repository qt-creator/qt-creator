// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "informationchangedcommand.h"

#include <QMetaType>
#include <QtDebug>

#include "propertyvaluecontainer.h"

#include <algorithm>

namespace QmlDesigner {

InformationChangedCommand::InformationChangedCommand() = default;

InformationChangedCommand::InformationChangedCommand(const QVector<InformationContainer> &informationVector)
    : m_informationVector(informationVector)
{
}

QVector<InformationContainer> InformationChangedCommand::informations() const
{
    return m_informationVector;
}

void InformationChangedCommand::sort()
{
    std::sort(m_informationVector.begin(), m_informationVector.end());
}

QDataStream &operator<<(QDataStream &out, const InformationChangedCommand &command)
{
    out << command.informations();

    return out;
}

QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command)
{
    in >> command.m_informationVector;

    return in;
}

bool operator ==(const InformationChangedCommand &first, const InformationChangedCommand &second)
{
    return first.m_informationVector == second.m_informationVector;
}

QDebug operator <<(QDebug debug, const InformationChangedCommand &command)
{
    return debug.nospace() << "InformationChangedCommand(" << command.informations() << ")";
}


} // namespace QmlDesigner
