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

#ifndef TCPPORTSGATHERER_H
#define TCPPORTSGATHERER_H

#include "portlist.h"

namespace Utils {
namespace Internal {
class TcpPortsGathererPrivate;
}

class QTCREATOR_UTILS_EXPORT TcpPortsGatherer
{
public:
    enum NetworkLayerProtocol {
        IPv4Protocol = 0x1,
        IPv6Protocol = 0x2,
        AnyIPProcol = IPv4Protocol | IPv6Protocol
    };
    Q_DECLARE_FLAGS(ProtocolFlags, NetworkLayerProtocol)

    TcpPortsGatherer(ProtocolFlags flags);
    ~TcpPortsGatherer();

    void update();

    PortList usedPorts() const;
    quint16 getNextFreePort(PortList *port);

private:
    Internal::TcpPortsGathererPrivate *d;
};

} // namespace Utils

#endif // TCPPORTSGATHERER_H
