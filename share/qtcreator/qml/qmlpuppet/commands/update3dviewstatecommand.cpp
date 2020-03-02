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
