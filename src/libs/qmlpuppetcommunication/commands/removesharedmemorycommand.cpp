// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "removesharedmemorycommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

RemoveSharedMemoryCommand::RemoveSharedMemoryCommand() = default;

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
