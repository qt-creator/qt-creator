/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "informationchangedcommand.h"

#include <QMetaType>
#include <QtDebug>

#include "propertyvaluecontainer.h"

#include <algorithm>

namespace QmlDesigner {

InformationChangedCommand::InformationChangedCommand()
{
}

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
