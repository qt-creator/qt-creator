// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "update3dviewstatecommand.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

Update3dViewStateCommand::Update3dViewStateCommand(const QSize &size)
    : m_size(size)
    , m_type(Update3dViewStateCommand::SizeChange)
{
}

QSize Update3dViewStateCommand::size() const
{
    return m_size;
}

Update3dViewStateCommand::Type Update3dViewStateCommand::type() const
{
    return m_type;
}

QDataStream &operator<<(QDataStream &out, const Update3dViewStateCommand &command)
{
    out << qint32(command.type());
    out << command.size();

    return out;
}

QDataStream &operator>>(QDataStream &in, Update3dViewStateCommand &command)
{
    qint32 type;
    in >> type;
    command.m_type = Update3dViewStateCommand::Type(type);
    in >> command.m_size;

    return in;
}

QDebug operator<<(QDebug debug, const Update3dViewStateCommand &command)
{
    return debug.nospace() << "Update3dViewStateCommand(type: "
                           << command.m_type << ","
                           << command.m_size << ")";
}

} // namespace QmlDesigner
