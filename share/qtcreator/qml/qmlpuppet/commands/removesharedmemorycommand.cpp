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


#include "removesharedmemorycommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

RemoveSharedMemoryCommand::RemoveSharedMemoryCommand()
{
}

RemoveSharedMemoryCommand::RemoveSharedMemoryCommand(const QString &typeName, const QVector<qint32> &keyNumberVector)
    : m_typeName(typeName),
      m_keyNumberVector(keyNumberVector)
{
}

QString RemoveSharedMemoryCommand::typeName() const
{
    return m_typeName;
}

QVector<qint32> RemoveSharedMemoryCommand::keyNumbers() const
{
    return m_keyNumberVector;
}

QDataStream &operator<<(QDataStream &out, const RemoveSharedMemoryCommand &command)
{
    out << command.typeName();
    out << command.keyNumbers();

    return out;
}

QDataStream &operator>>(QDataStream &in, RemoveSharedMemoryCommand &command)
{
    in >> command.m_typeName;
    in >> command.m_keyNumberVector;

    return in;
}

QDebug operator <<(QDebug debug, const RemoveSharedMemoryCommand &command)
{
    return debug.nospace() << "RemoveSharedMemoryCommand("
                           << "typeName: " << command.m_typeName
                           << "keyNumbers: " << command.m_keyNumberVector << ")";
}

} // namespace QmlDesigner
