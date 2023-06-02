// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "port.h"

#include "qtcassert.h"
#include "stringutils.h"

#include <QRegularExpression>

#include <limits>

/*!
  \class Utils::Port
  \inmodule QtCreator

  \brief The Port class implements a wrapper around a 16 bit port number
  to be used in conjunction with IP addresses.
*/

namespace Utils {

Port::Port(int port)
    : m_port((port < 0 || port > std::numeric_limits<quint16>::max()) ? -1 : port)
{
}

Port::Port(uint port)
    : m_port(port > std::numeric_limits<quint16>::max() ? -1 : port)
{
}

quint16 Port::number() const
{
    QTC_ASSERT(isValid(), return -1); return quint16(m_port);
}

QList<Port> Port::parseFromCommandOutput(const QByteArray &output)
{
    QList<Port> ports;
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const Port port(parseUsedPortFromNetstatOutput(line));
        if (port.isValid() && !ports.contains(port))
            ports.append(port);
    }
    return ports;
}

} // Utils
