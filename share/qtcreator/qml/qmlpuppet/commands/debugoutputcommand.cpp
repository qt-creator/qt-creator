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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#include "debugoutputcommand.h"

#include <QtDebug>

namespace QmlDesigner {

DebugOutputCommand::DebugOutputCommand()
{
}

DebugOutputCommand::DebugOutputCommand(const QString &text, DebugOutputCommand::Type type, const QVector<qint32> &instanceIds)
    : m_instanceIds(instanceIds)
    , m_text(text)
    , m_type(type)
{
}

qint32 DebugOutputCommand::type() const
{
    return m_type;
}

QString DebugOutputCommand::text() const
{
    return m_text;
}

QVector<qint32> DebugOutputCommand::instanceIds() const
{
    return m_instanceIds;
}

QDataStream &operator<<(QDataStream &out, const DebugOutputCommand &command)
{
    out << command.type();
    out << command.text();
    out << command.instanceIds();

    return out;
}

QDataStream &operator>>(QDataStream &in, DebugOutputCommand &command)
{
    in >> command.m_type;
    in >> command.m_text;
    in >> command.m_instanceIds;

    return in;
}

bool operator ==(const DebugOutputCommand &first, const DebugOutputCommand &second)
{
    return first.m_type == second.m_type
            && first.m_text == second.m_text;
}

QDebug operator <<(QDebug debug, const DebugOutputCommand &command)
{
        return debug.nospace() << "DebugOutputCommand("
                               << "type: " << command.type() << ", "
                               << "text: " << command.text() << ")";
}

} // namespace QmlDesigner
