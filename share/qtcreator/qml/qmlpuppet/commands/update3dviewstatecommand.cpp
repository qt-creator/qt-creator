/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "update3dviewstatecommand.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

Update3dViewStateCommand::Update3dViewStateCommand(Qt::WindowStates previousStates,
                                                   Qt::WindowStates currentStates)
    : m_previousStates(previousStates)
    , m_currentStates(currentStates)
    , m_type(Update3dViewStateCommand::StateChange)
{
}

Update3dViewStateCommand::Update3dViewStateCommand(bool active, bool hasPopup)
    : m_active(active)
    , m_hasPopup(hasPopup)
    , m_type(Update3dViewStateCommand::ActiveChange)
{
}

Qt::WindowStates Update3dViewStateCommand::previousStates() const
{
    return m_previousStates;
}

Qt::WindowStates Update3dViewStateCommand::currentStates() const
{
    return m_currentStates;
}

bool Update3dViewStateCommand::isActive() const
{
    return m_active;
}

bool Update3dViewStateCommand::hasPopup() const
{
    return m_hasPopup;
}

Update3dViewStateCommand::Type Update3dViewStateCommand::type() const
{
    return m_type;
}

QDataStream &operator<<(QDataStream &out, const Update3dViewStateCommand &command)
{
    out << command.previousStates();
    out << command.currentStates();
    out << qint32(command.isActive());
    out << qint32(command.hasPopup());
    out << qint32(command.type());

    return out;
}

QDataStream &operator>>(QDataStream &in, Update3dViewStateCommand &command)
{
    in >> command.m_previousStates;
    in >> command.m_currentStates;
    qint32 active;
    qint32 hasPopup;
    qint32 type;
    in >> active;
    in >> hasPopup;
    in >> type;
    command.m_active = active;
    command.m_hasPopup = hasPopup;
    command.m_type = Update3dViewStateCommand::Type(type);

    return in;
}

QDebug operator<<(QDebug debug, const Update3dViewStateCommand &command)
{
    return debug.nospace() << "Update3dViewStateCommand(type: " << command.m_type << ")";
}

} // namespace QmlDesigner
