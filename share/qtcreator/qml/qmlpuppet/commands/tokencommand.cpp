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

#include "tokencommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

TokenCommand::TokenCommand()
    : m_tokenNumber(-1)
{
}

TokenCommand::TokenCommand(const QString &tokenName, qint32 tokenNumber, const QVector<qint32> &instanceIdVector)
    : m_tokenName(tokenName),
      m_tokenNumber(tokenNumber),
      m_instanceIdVector(instanceIdVector)
{
}

QString TokenCommand::tokenName() const
{
    return m_tokenName;
}

qint32 TokenCommand::tokenNumber() const
{
    return m_tokenNumber;
}

QVector<qint32> TokenCommand::instances() const
{
    return m_instanceIdVector;
}

void TokenCommand::sort()
{
    qSort(m_instanceIdVector);
}

QDataStream &operator<<(QDataStream &out, const TokenCommand &command)
{
    out << command.tokenName();
    out << command.tokenNumber();
    out << command.instances();
    return out;
}

QDataStream &operator>>(QDataStream &in, TokenCommand &command)
{
    in >> command.m_tokenName;
    in >> command.m_tokenNumber;
    in >> command.m_instanceIdVector;

    return in;
}

bool operator ==(const TokenCommand &first, const TokenCommand &second)
{
    return first.m_tokenName == second.m_tokenName
            && first.m_tokenNumber == second.m_tokenNumber
            && first.m_instanceIdVector == second.m_instanceIdVector;
}

QDebug operator <<(QDebug debug, const TokenCommand &command)
{
    return debug.nospace() << "TokenCommand("
                           << "tokenName: " << command.tokenName() << ", "
                           << "tokenNumber: " << command.tokenNumber() << ", "
                           << "instances: " << command.instances() << ")";
}

} // namespace QmlDesigner
