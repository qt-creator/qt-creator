/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
    friend class Core::SshRemoteProcess;
public:
    enum ProcessState {
        NotYetStarted, ExecRequested, StartFailed,Running, Exited
    };

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void closeHook();

    void emitStartedSignal();
    void emitOutputAvailableSignal(const QByteArray &output);
    void emitErrorOutputAvailableSignal(const QByteArray &output);
    void emitClosedSignal(int exitStatus);

private:
    SshRemoteProcessPrivate(const QByteArray &command, quint32 channelId,
        SshSendFacility &sendFacility, SshRemoteProcess *proc);

    virtual void handleOpenSuccessInternal();
    virtual void handleOpenFailureInternal();
    virtual void handleChannelDataInternal(const QByteArray &data);
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data);
    virtual void handleChannelRequest(const SshIncomingPacket &packet);

    void setProcState(ProcessState newState);

    void createStartedSignal();
    void createOutputAvailableSignal(const QByteArray &output);
    void createErrorOutputAvailableSignal(const QByteArray &output);
    void createClosedSignal(int exitStatus);

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
