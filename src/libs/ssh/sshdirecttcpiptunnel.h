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

#ifndef  SSHDIRECTTCPIPTUNNEL_H
#define  SSHDIRECTTCPIPTUNNEL_H

#include "ssh_global.h"

#include <QIODevice>
#include <QSharedPointer>

namespace QSsh {

namespace Internal {
class SshChannelManager;
class SshDirectTcpIpTunnelPrivate;
class SshSendFacility;
} // namespace Internal

class QSSH_EXPORT SshDirectTcpIpTunnel : public QIODevice
{
    Q_OBJECT

    friend class Internal::SshChannelManager;

public:
    typedef QSharedPointer<SshDirectTcpIpTunnel> Ptr;

    ~SshDirectTcpIpTunnel();

    // QIODevice stuff
    bool atEnd() const;
    qint64 bytesAvailable() const;
    bool canReadLine() const;
    void close();
    bool isSequential() const { return true; }

    void initialize();

signals:
    void initialized();
    void error(const QString &reason);
    void tunnelClosed();

private:
    SshDirectTcpIpTunnel(quint32 channelId, const QString &originatingHost,
            quint16 originatingPort, const QString &remoteHost, quint16 remotePort,
            Internal::SshSendFacility &sendFacility);

    // QIODevice stuff
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    Q_SLOT void handleError(const QString &reason);

    Internal::SshDirectTcpIpTunnelPrivate * const d;
};

} // namespace QSsh

#endif // SSHDIRECTTCPIPTUNNEL_H
