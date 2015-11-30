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

#include "synchronizecommand.h"

#include <QDebug>

namespace QmlDesigner {

SynchronizeCommand::SynchronizeCommand()
    : m_synchronizeId(-1)
{
}

SynchronizeCommand::SynchronizeCommand(int synchronizeId)
    : m_synchronizeId (synchronizeId)
{
}

int SynchronizeCommand::synchronizeId() const
{
    return m_synchronizeId;
}


QDataStream &operator<<(QDataStream &out, const SynchronizeCommand &command)
{
    out << command.synchronizeId();

    return out;
}

QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command)
{
    in >> command.m_synchronizeId;

    return in;
}

bool operator ==(const SynchronizeCommand &first, const SynchronizeCommand &second)
{
    return first.m_synchronizeId == second.m_synchronizeId;
}

QDebug operator <<(QDebug debug, const SynchronizeCommand &command)
{
    return debug.nospace() << "SynchronizeCommand(synchronizeId: " << command.synchronizeId() << ")";
}

} // namespace QmlDesigner
