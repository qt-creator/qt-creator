/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "tokencommand.h"

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


} // namespace QmlDesigner
