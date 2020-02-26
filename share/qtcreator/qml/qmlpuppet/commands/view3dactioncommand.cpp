/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "view3dactioncommand.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

View3DActionCommand::View3DActionCommand(Type type, bool enable)
    : m_type(type)
    , m_enabled(enable)
{
}

bool View3DActionCommand::isEnabled() const
{
    return m_enabled;
}

View3DActionCommand::Type View3DActionCommand::type() const
{
    return m_type;
}

QDataStream &operator<<(QDataStream &out, const View3DActionCommand &command)
{
    out << qint32(command.isEnabled());
    out << qint32(command.type());

    return out;
}

QDataStream &operator>>(QDataStream &in, View3DActionCommand &command)
{
    qint32 enabled;
    qint32 type;
    in >> enabled;
    in >> type;
    command.m_enabled = bool(enabled);
    command.m_type = View3DActionCommand::Type(type);

    return in;
}

QDebug operator<<(QDebug debug, const View3DActionCommand &command)
{
    return debug.nospace() << "View3DActionCommand(type: "
                           << command.m_type << ","
                           << command.m_enabled << ")";
}

} // namespace QmlDesigner
