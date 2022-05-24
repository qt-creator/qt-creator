/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "port.h"

#include "stringutils.h"

/*! \class Utils::Port

  \brief The Port class implements a wrapper around a 16 bit port number
  to be used in conjunction with IP addresses.
*/

namespace Utils {

QList<Port> Port::parseFromSedOutput(const QByteArray &output)
{
    QList<Port> ports;
    const QList<QByteArray> portStrings = output.split('\n');
    for (const QByteArray &portString : portStrings) {
        if (portString.size() != 4)
            continue;
        bool ok;
        const Port port(portString.toInt(&ok, 16));
        if (ok) {
            if (!ports.contains(port))
                ports << port;
        } else {
            qWarning("%s: Unexpected string '%s' is not a port.",
                     Q_FUNC_INFO, portString.data());
        }
    }
    return ports;
}

QList<Port> Port::parseFromNetstatOutput(const QByteArray &output)
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
