// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tokencommand.h"

#include <QDataStream>
#include <QDebug>

#include <algorithm>

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
    std::sort(m_instanceIdVector.begin(), m_instanceIdVector.end());
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
