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

#include "changenodesourcecommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

ChangeNodeSourceCommand::ChangeNodeSourceCommand()
{
}

ChangeNodeSourceCommand::ChangeNodeSourceCommand(qint32 newInstanceId, const QString &newNodeSource)
    : m_instanceId(newInstanceId), m_nodeSource(newNodeSource)
{
}

qint32 ChangeNodeSourceCommand::instanceId() const
{
    return m_instanceId;
}

QString ChangeNodeSourceCommand::nodeSource() const
{
    return m_nodeSource;
}

QDataStream &operator<<(QDataStream &out, const ChangeNodeSourceCommand &command)
{
    out << command.instanceId();
    out << command.nodeSource();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeNodeSourceCommand &command)
{
    in >> command.m_instanceId;
    in >> command.m_nodeSource;

    return in;
}

QDebug operator <<(QDebug debug, const ChangeNodeSourceCommand &command)
{
    return debug.nospace() << "ReparentInstancesCommand("
                           << "instanceId: " << command.m_instanceId
                           << "nodeSource: " << command.m_nodeSource << ")";
}

} // namespace QmlDesigner
