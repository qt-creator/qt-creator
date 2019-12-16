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

#include "enable3dviewcommand.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

// open / close edit view 3D command
Enable3DViewCommand::Enable3DViewCommand(bool enable)
    : m_enable(enable)
{
}

bool Enable3DViewCommand::isEnable() const
{
    return m_enable;
}

QDataStream &operator<<(QDataStream &out, const Enable3DViewCommand &command)
{
    out << qint32(command.isEnable());

    return out;
}

QDataStream &operator>>(QDataStream &in, Enable3DViewCommand &command)
{
    qint32 enable;
    in >> enable;
    command.m_enable = enable;

    return in;
}

QDebug operator<<(QDebug debug, const Enable3DViewCommand &command)
{
    return debug.nospace() << "Enable3DViewCommand(enable: " << command.m_enable << ")";
}

} // namespace QmlDesigner
