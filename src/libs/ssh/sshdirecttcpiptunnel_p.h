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
#ifndef DIRECTTCPIPCHANNEL_P_H
#define DIRECTTCPIPCHANNEL_P_H

#include "sshchannel_p.h"

#include "sshconnection.h"

namespace QSsh {
class SshDirectTcpIpTunnel;

namespace Internal {

class SshDirectTcpIpTunnelPrivate : public AbstractSshChannel
{
    Q_OBJECT

    friend class QSsh::SshDirectTcpIpTunnel;

public:
    explicit SshDirectTcpIpTunnelPrivate(quint32 channelId, quint16 remotePort,
            const SshConnectionInfo &connectionInfo, SshSendFacility &sendFacility);

signals:
    void initialized();
    void readyRead();
    void error(const QString &reason);
    void closed();

private slots:
    void handleEof();

private:
    void handleChannelSuccess();
    void handleChannelFailure();

    void handleOpenSuccessInternal();
    void handleOpenFailureInternal(const QString &reason);
    void handleChannelDataInternal(const QByteArray &data);
    void handleChannelExtendedDataInternal(quint32 type, const QByteArray &data);
    void handleExitStatus(const SshChannelExitStatus &exitStatus);
    void handleExitSignal(const SshChannelExitSignal &signal);

    void closeHook();

    const quint16 m_remotePort;
    const SshConnectionInfo m_connectionInfo;
    QByteArray m_data;
};

} // namespace Internal
} // namespace QSsh

#endif // DIRECTTCPIPCHANNEL_P_H
