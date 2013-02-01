/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef  SSHDIRECTTCPIPTUNNEL_H
#define  SSHDIRECTTCPIPTUNNEL_H

#include "ssh_global.h"

#include <QIODevice>
#include <QSharedPointer>

namespace QSsh {
class SshConnectionInfo;

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
    SshDirectTcpIpTunnel(quint32 channelId, quint16 remotePort,
            const SshConnectionInfo &connectionInfo, Internal::SshSendFacility &sendFacility);

    // QIODevice stuff
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    Q_SLOT void handleError(const QString &reason);

    Internal::SshDirectTcpIpTunnelPrivate * const d;
};

} // namespace QSsh

#endif // SSHDIRECTTCPIPTUNNEL_H
