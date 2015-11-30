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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SSHREMOTEPROCESS_P_H
#define SSHREMOTEPROCESS_P_H

#include "sshpseudoterminal.h"

#include "sshchannel_p.h"

#include <QList>
#include <QPair>
#include <QProcess>

namespace QSsh {
class SshRemoteProcess;

namespace Internal {
class SshSendFacility;

class SshRemoteProcessPrivate : public AbstractSshChannel
{
    Q_OBJECT
    friend class QSsh::SshRemoteProcess;
public:
    enum ProcessState {
        NotYetStarted, ExecRequested, StartFailed, Running, Exited
    };

signals:
    void started();
    void readyRead();
    void readyReadStandardOutput();
    void readyReadStandardError();
    void closed(int exitStatus);

private:
    SshRemoteProcessPrivate(const QByteArray &command, quint32 channelId,
        SshSendFacility &sendFacility, SshRemoteProcess *proc);
    SshRemoteProcessPrivate(quint32 channelId, SshSendFacility &sendFacility,
        SshRemoteProcess *proc);

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void handleOpenSuccessInternal();
    virtual void handleOpenFailureInternal(const QString &reason);
    virtual void handleChannelDataInternal(const QByteArray &data);
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data);
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus);
    virtual void handleExitSignal(const SshChannelExitSignal &signal);

    virtual void closeHook();

    void init();
    void setProcState(ProcessState newState);
    QByteArray &data();

    QProcess::ProcessChannel m_readChannel;

    ProcessState m_procState;
    bool m_wasRunning;
    int m_signal;
    int m_exitCode;

    const QByteArray m_command;
    const bool m_isShell;

    typedef QPair<QByteArray, QByteArray> EnvVar;
    QList<EnvVar> m_env;
    bool m_useTerminal;
    SshPseudoTerminal m_terminal;

    QByteArray m_stdout;
    QByteArray m_stderr;

    SshRemoteProcess *m_proc;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHREMOTEPROCESS_P_H
