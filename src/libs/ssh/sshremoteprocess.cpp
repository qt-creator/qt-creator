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

#include "sshremoteprocess.h"
#include "sshremoteprocess_p.h"

#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <botan/botan.h>

#include <QTimer>

#include <cstring>
#include <cstdlib>

/*!
    \class QSsh::SshRemoteProcess

    \brief This class implements an SSH channel for running a remote process.

    Objects are created via SshConnection::createRemoteProcess.
    The process is started via the start() member function.
    If the process needs a pseudo terminal, you can request one
    via requestTerminal() before calling start().
    Note that this class does not support QIODevice's waitFor*() functions, i.e. it has
    no synchronous mode.
 */

namespace QSsh {

const struct {
    SshRemoteProcess::Signal signalEnum;
    const char * const signalString;
} signalMap[] = {
    { SshRemoteProcess::AbrtSignal, "ABRT" }, { SshRemoteProcess::AlrmSignal, "ALRM" },
    { SshRemoteProcess::FpeSignal, "FPE" }, { SshRemoteProcess::HupSignal, "HUP" },
    { SshRemoteProcess::IllSignal, "ILL" }, { SshRemoteProcess::IntSignal, "INT" },
    { SshRemoteProcess::KillSignal, "KILL" }, { SshRemoteProcess::PipeSignal, "PIPE" },
    { SshRemoteProcess::QuitSignal, "QUIT" }, { SshRemoteProcess::SegvSignal, "SEGV" },
    { SshRemoteProcess::TermSignal, "TERM" }, { SshRemoteProcess::Usr1Signal, "USR1" },
    { SshRemoteProcess::Usr2Signal, "USR2" }
};

SshRemoteProcess::SshRemoteProcess(const QByteArray &command, quint32 channelId,
    Internal::SshSendFacility &sendFacility)
    : d(new Internal::SshRemoteProcessPrivate(command, channelId, sendFacility, this))
{
    init();
}

SshRemoteProcess::SshRemoteProcess(quint32 channelId, Internal::SshSendFacility &sendFacility)
    : d(new Internal::SshRemoteProcessPrivate(channelId, sendFacility, this))
{
    init();
}

SshRemoteProcess::~SshRemoteProcess()
{
    Q_ASSERT(d->channelState() == Internal::SshRemoteProcessPrivate::Inactive
        || d->channelState() == Internal::SshRemoteProcessPrivate::CloseRequested
        || d->channelState() == Internal::SshRemoteProcessPrivate::Closed);
    delete d;
}

bool SshRemoteProcess::atEnd() const
{
    return QIODevice::atEnd() && d->data().isEmpty();
}

qint64 SshRemoteProcess::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + d->data().count();
}

bool SshRemoteProcess::canReadLine() const
{
    return QIODevice::canReadLine() || d->data().contains('\n');
}

QByteArray SshRemoteProcess::readAllStandardOutput()
{
    return readAllFromChannel(QProcess::StandardOutput);
}

QByteArray SshRemoteProcess::readAllStandardError()
{
    return readAllFromChannel(QProcess::StandardError);
}

QByteArray SshRemoteProcess::readAllFromChannel(QProcess::ProcessChannel channel)
{
    const QProcess::ProcessChannel currentReadChannel = readChannel();
    setReadChannel(channel);
    const QByteArray &data = readAll();
    setReadChannel(currentReadChannel);
    return data;
}

void SshRemoteProcess::close()
{
    d->closeChannel();
    QIODevice::close();
}

qint64 SshRemoteProcess::readData(char *data, qint64 maxlen)
{
    const qint64 bytesRead = qMin(qint64(d->data().count()), maxlen);
    memcpy(data, d->data().constData(), bytesRead);
    d->data().remove(0, bytesRead);
    return bytesRead;
}

qint64 SshRemoteProcess::writeData(const char *data, qint64 len)
{
    if (isRunning()) {
        d->sendData(QByteArray(data, len));
        return len;
    }
    return 0;
}

QProcess::ProcessChannel SshRemoteProcess::readChannel() const
{
    return d->m_readChannel;
}

void SshRemoteProcess::setReadChannel(QProcess::ProcessChannel channel)
{
    d->m_readChannel = channel;
}

void SshRemoteProcess::init()
{
    connect(d, SIGNAL(started()), this, SIGNAL(started()),
        Qt::QueuedConnection);
    connect(d, SIGNAL(readyReadStandardOutput()), this, SIGNAL(readyReadStandardOutput()),
        Qt::QueuedConnection);
    connect(d, SIGNAL(readyRead()), this, SIGNAL(readyRead()), Qt::QueuedConnection);
    connect(d, SIGNAL(readyReadStandardError()), this,
        SIGNAL(readyReadStandardError()), Qt::QueuedConnection);
    connect(d, SIGNAL(closed(int)), this, SIGNAL(closed(int)), Qt::QueuedConnection);
    connect(d, SIGNAL(eof()), SIGNAL(readChannelFinished()), Qt::QueuedConnection);
}

void SshRemoteProcess::addToEnvironment(const QByteArray &var, const QByteArray &value)
{
    if (d->channelState() == Internal::SshRemoteProcessPrivate::Inactive)
        d->m_env << qMakePair(var, value); // Cached locally and sent on start()
}

void SshRemoteProcess::requestTerminal(const SshPseudoTerminal &terminal)
{
    QSSH_ASSERT_AND_RETURN(d->channelState() == Internal::SshRemoteProcessPrivate::Inactive);
    d->m_useTerminal = true;
    d->m_terminal = terminal;
}

void SshRemoteProcess::start()
{
    if (d->channelState() == Internal::SshRemoteProcessPrivate::Inactive) {
#ifdef CREATOR_SSH_DEBUG
        qDebug("process start requested, channel id = %u", d->localChannelId());
#endif
        QIODevice::open(QIODevice::ReadWrite);
        d->requestSessionStart();
    }
}

void SshRemoteProcess::sendSignal(Signal signal)
{
    try {
        if (isRunning()) {
            const char *signalString = 0;
            for (size_t i = 0; i < sizeof signalMap/sizeof *signalMap && !signalString; ++i) {
                if (signalMap[i].signalEnum == signal)
                    signalString = signalMap[i].signalString;
            }
            QSSH_ASSERT_AND_RETURN(signalString);
            d->m_sendFacility.sendChannelSignalPacket(d->remoteChannel(), signalString);
        }
    }  catch (Botan::Exception &e) {
        setErrorString(QString::fromLatin1(e.what()));
        d->closeChannel();
    }
}

bool SshRemoteProcess::isRunning() const
{
    return d->m_procState == Internal::SshRemoteProcessPrivate::Running;
}

int SshRemoteProcess::exitCode() const { return d->m_exitCode; }

SshRemoteProcess::Signal SshRemoteProcess::exitSignal() const
{
    return static_cast<SshRemoteProcess::Signal>(d->m_signal);
}

namespace Internal {

SshRemoteProcessPrivate::SshRemoteProcessPrivate(const QByteArray &command,
        quint32 channelId, SshSendFacility &sendFacility, SshRemoteProcess *proc)
    : AbstractSshChannel(channelId, sendFacility),
      m_command(command),
      m_isShell(false),
      m_useTerminal(false),
      m_proc(proc)
{
    init();
}

SshRemoteProcessPrivate::SshRemoteProcessPrivate(quint32 channelId, SshSendFacility &sendFacility,
            SshRemoteProcess *proc)
    : AbstractSshChannel(channelId, sendFacility),
      m_isShell(true),
      m_useTerminal(true),
      m_proc(proc)
{
    init();
}

void SshRemoteProcessPrivate::init()
{
    m_procState = NotYetStarted;
    m_wasRunning = false;
    m_exitCode = 0;
    m_readChannel = QProcess::StandardOutput;
    m_signal = SshRemoteProcess::NoSignal;
}

void SshRemoteProcessPrivate::setProcState(ProcessState newState)
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("channel: old state = %d,new state = %d", m_procState, newState);
#endif
    m_procState = newState;
    if (newState == StartFailed) {
        emit closed(SshRemoteProcess::FailedToStart);
    } else if (newState == Running) {
        m_wasRunning = true;
        emit started();
    }
}

QByteArray &SshRemoteProcessPrivate::data()
{
    return m_readChannel == QProcess::StandardOutput ? m_stdout : m_stderr;
}

void SshRemoteProcessPrivate::closeHook()
{
    if (m_wasRunning) {
        if (m_signal != SshRemoteProcess::NoSignal)
            emit closed(SshRemoteProcess::CrashExit);
        else
            emit closed(SshRemoteProcess::NormalExit);
    }
}

void SshRemoteProcessPrivate::handleOpenSuccessInternal()
{
   foreach (const EnvVar &envVar, m_env) {
       m_sendFacility.sendEnvPacket(remoteChannel(), envVar.first,
           envVar.second);
   }

   if (m_useTerminal)
       m_sendFacility.sendPtyRequestPacket(remoteChannel(), m_terminal);

   if (m_isShell)
       m_sendFacility.sendShellPacket(remoteChannel());
   else
       m_sendFacility.sendExecPacket(remoteChannel(), m_command);
   setProcState(ExecRequested);
   m_timeoutTimer->start(ReplyTimeout);
}

void SshRemoteProcessPrivate::handleOpenFailureInternal(const QString &reason)
{
   setProcState(StartFailed);
   m_proc->setErrorString(reason);
}

void SshRemoteProcessPrivate::handleChannelSuccess()
{
    if (m_procState != ExecRequested)  {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_SUCCESS message.");
    }
    m_timeoutTimer->stop();
    setProcState(Running);
}

void SshRemoteProcessPrivate::handleChannelFailure()
{
    if (m_procState != ExecRequested)  {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_FAILURE message.");
    }
    m_timeoutTimer->stop();
    setProcState(StartFailed);
    closeChannel();
}

void SshRemoteProcessPrivate::handleChannelDataInternal(const QByteArray &data)
{
    m_stdout += data;
    emit readyReadStandardOutput();
    if (m_readChannel == QProcess::StandardOutput)
        emit readyRead();
}

void SshRemoteProcessPrivate::handleChannelExtendedDataInternal(quint32 type,
    const QByteArray &data)
{
    if (type != SSH_EXTENDED_DATA_STDERR) {
        qWarning("Unknown extended data type %u", type);
    } else {
        m_stderr += data;
        emit readyReadStandardError();
        if (m_readChannel == QProcess::StandardError)
            emit readyRead();
    }
}

void SshRemoteProcessPrivate::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("Process exiting with exit code %d", exitStatus.exitStatus);
#endif
    m_exitCode = exitStatus.exitStatus;
    m_procState = Exited;
}

void SshRemoteProcessPrivate::handleExitSignal(const SshChannelExitSignal &signal)
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("Exit due to signal %s", signal.signal.data());
#endif

    for (size_t i = 0; i < sizeof signalMap/sizeof *signalMap; ++i) {
        if (signalMap[i].signalString == signal.signal) {
            m_signal = signalMap[i].signalEnum;
            m_procState = Exited;
            m_proc->setErrorString(tr("Process killed by signal"));
            return;
        }
    }

    throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR, "Invalid signal",
        tr("Server sent invalid signal '%1'").arg(QString::fromUtf8(signal.signal)));
}

} // namespace Internal
} // namespace QSsh
