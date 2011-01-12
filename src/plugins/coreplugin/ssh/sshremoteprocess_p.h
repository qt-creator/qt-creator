/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SSHREMOTEPROCESS_P_H
#define SSHREMOTEPROCESS_P_H

#include "sshchannel_p.h"

#include <QtCore/QList>
#include <QtCore/QPair>

namespace Core {
class SshRemoteProcess;

namespace Internal {
class SshSendFacility;

class SshRemoteProcessPrivate : public AbstractSshChannel
{
    Q_OBJECT
    friend class Core::SshRemoteProcess;
public:
    enum ProcessState {
        NotYetStarted, ExecRequested, StartFailed,Running, Exited
    };

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void closeHook();

signals:
    void started();
    void outputAvailable(const QByteArray &output);
    void errorOutputAvailable(const QByteArray &output);
    void closed(int exitStatus);

private:
    SshRemoteProcessPrivate(const QByteArray &command, quint32 channelId,
        SshSendFacility &sendFacility, SshRemoteProcess *proc);

    virtual void handleOpenSuccessInternal();
    virtual void handleOpenFailureInternal();
    virtual void handleChannelDataInternal(const QByteArray &data);
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data);
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus);
    virtual void handleExitSignal(const SshChannelExitSignal &signal);

    void setProcState(ProcessState newState);

    ProcessState m_procState;
    bool m_wasRunning;
    QByteArray m_signal;
    int m_exitCode;

    const QByteArray m_command;

    typedef QPair<QByteArray, QByteArray> EnvVar;
    QList<EnvVar> m_env;

    SshRemoteProcess *m_proc;
};

} // namespace Internal
} // namespace Core

#endif // SSHREMOTEPROCESS_P_H
