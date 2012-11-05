/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/


#include "debugoutputcommand.h"

namespace QmlDesigner {

DebugOutputCommand::DebugOutputCommand()
{
}

DebugOutputCommand::DebugOutputCommand(const QString &text, DebugOutputCommand::Type type)
    : m_text(text),
      m_type(type)
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

QDataStream &operator<<(QDataStream &out, const DebugOutputCommand &command)
{
    out << command.type();
    out << command.text();

    return out;
}

QDataStream &operator>>(QDataStream &in, DebugOutputCommand &command)
{
    in >> command.m_type;
    in >> command.m_text;

    return in;
}

} // namespace QmlDesigner
