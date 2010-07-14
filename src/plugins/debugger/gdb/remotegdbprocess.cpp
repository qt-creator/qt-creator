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

#include "remotegdbprocess.h"

#include "remoteplaingdbadapter.h"

#include <ctype.h>

using namespace Core;

namespace Debugger {
namespace Internal {

RemoteGdbProcess::RemoteGdbProcess(const Core::SshConnectionParameters &connParams,
    RemotePlainGdbAdapter *adapter, QObject *parent)
    : AbstractGdbProcess(parent), m_connParams(connParams), m_adapter(adapter)
{
}

QByteArray RemoteGdbProcess::readAllStandardOutput()
{
    QByteArray output = m_gdbOutput;
    m_gdbOutput.clear();
    return output;
}

QByteArray RemoteGdbProcess::readAllStandardError()
{
    QByteArray errorOutput = m_errorOutput;
    m_errorOutput.clear();
    return errorOutput;
}

void RemoteGdbProcess::start(const QString &cmd, const QStringList &args)
{
    m_command = cmd;
    m_cmdArgs = args;
    m_gdbStarted = false;
    m_error.clear();
    m_conn = SshConnection::create();
    connect(m_conn.data(), SIGNAL(connected()), this, SLOT(handleConnected()));
    connect(m_conn.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionError()));
    m_conn->connectToHost(m_connParams);
}

void RemoteGdbProcess::handleConnected()
{
    m_fifoCreator = m_conn->createRemoteProcess( "rm -f "
        + AppOutputFile + " && mkfifo " + AppOutputFile);
    connect(m_fifoCreator.data(), SIGNAL(closed(int)), this,
        SLOT(handleFifoCreationFinished(int)));
    m_fifoCreator->start();
}

void RemoteGdbProcess::handleConnectionError()
{
    emitErrorExit(tr("Connection could not be established."));
}

void RemoteGdbProcess::handleFifoCreationFinished(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emitErrorExit(tr("Could not create FIFO."));
    } else {
        m_appOutputReader = m_conn->createRemoteProcess("cat " + AppOutputFile);
        connect(m_appOutputReader.data(), SIGNAL(started()), this,
            SLOT(handleAppOutputReaderStarted()));
        connect(m_appOutputReader.data(), SIGNAL(closed(int)), this,
            SLOT(handleAppOutputReaderFinished(int)));
        m_appOutputReader->start();
    }
}

void RemoteGdbProcess::handleAppOutputReaderStarted()
{
    connect(m_appOutputReader.data(), SIGNAL(outputAvailable(QByteArray)),
        this, SLOT(handleAppOutput(QByteArray)));
    QByteArray cmdLine = "DISPLAY=:0.0 " + m_command.toUtf8() + ' '
        + m_cmdArgs.join(QLatin1String(" ")).toUtf8()
        + " -tty=" + AppOutputFile;
    if (!m_wd.isEmpty())
        cmdLine.prepend("cd " + m_wd.toUtf8() + " && ");
    m_gdbProc = m_conn->createRemoteProcess(cmdLine);
    connect(m_gdbProc.data(), SIGNAL(started()), this,
        SLOT(handleGdbStarted()));
    connect(m_gdbProc.data(), SIGNAL(closed(int)), this,
        SLOT(handleGdbFinished(int)));
    connect(m_gdbProc.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SLOT(handleGdbOutput(QByteArray)));
    connect(m_gdbProc.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SLOT(handleErrOutput(QByteArray)));
    m_gdbProc->start();
}

void RemoteGdbProcess::handleAppOutputReaderFinished(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally)
        emitErrorExit(tr("Application output reader unexpectedly finished."));
}

void RemoteGdbProcess::handleGdbStarted()
{
    m_gdbStarted = true;
}

void RemoteGdbProcess::handleGdbFinished(int exitStatus)
{
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        emitErrorExit(tr("Remote gdb failed to start."));
        break;
    case SshRemoteProcess::KilledBySignal:
        emitErrorExit(tr("Remote gdb crashed."));
        break;
    case SshRemoteProcess::ExitedNormally:
        emit finished(m_gdbProc->exitCode(), QProcess::NormalExit);
        break;
    }
    disconnect(m_conn.data(), 0, this, 0);
    m_gdbProc = SshRemoteProcess::Ptr();
    m_appOutputReader = SshRemoteProcess::Ptr();
    m_conn->disconnectFromHost();
}

bool RemoteGdbProcess::waitForStarted()
{
    return m_error.isEmpty();
}

qint64 RemoteGdbProcess::write(const QByteArray &data)
{
    if (!m_gdbStarted || !m_inputToSend.isEmpty() || !m_lastSeqNr.isEmpty())
        m_inputToSend.enqueue(data);
    else
        sendInput(data);
    return data.size();
}

void RemoteGdbProcess::kill()
{
    SshRemoteProcess::Ptr killProc
        = m_conn->createRemoteProcess("pkill -SIGKILL -x gdb");
    killProc->start();
}

void RemoteGdbProcess::interruptInferior()
{
    SshRemoteProcess::Ptr intProc
        = m_conn->createRemoteProcess("pkill -x -SIGINT gdb");
    intProc->start();
}

QProcess::ProcessState RemoteGdbProcess::state() const
{
    return m_gdbStarted ? QProcess::Running : QProcess::Starting;
}

QString RemoteGdbProcess::errorString() const
{
    return m_error;
}

void RemoteGdbProcess::handleGdbOutput(const QByteArray &output)
{
    // TODO: Carriage return removal still necessary?
    m_currentGdbOutput += removeCarriageReturn(output);
#if 0
    qDebug("%s: complete unread output is '%s'", Q_FUNC_INFO, m_currentGdbOutput.data());
#endif
    if (!m_currentGdbOutput.endsWith('\n'))
        return;

    if (m_currentGdbOutput.contains(m_lastSeqNr + '^'))
        m_lastSeqNr.clear();

    if (m_lastSeqNr.isEmpty() && !m_inputToSend.isEmpty()) {
#if 0
        qDebug("Sending queued command: %s", m_inputToSend.head().data());
#endif
        sendInput(m_inputToSend.dequeue());
    }

    if (!m_currentGdbOutput.isEmpty()) {
        const int startPos
            = m_gdbOutput.isEmpty() ? findAnchor(m_currentGdbOutput) : 0;
        if (startPos != -1) {
            m_gdbOutput += m_currentGdbOutput.mid(startPos);
            m_currentGdbOutput.clear();
            emit readyReadStandardOutput();
        }
    }
}

QProcessEnvironment RemoteGdbProcess::processEnvironment() const
{
    return QProcessEnvironment(); // TODO: Provide actual environment.
}

void RemoteGdbProcess::setProcessEnvironment(const QProcessEnvironment & /* env */)
{
    // TODO: Do something. (if remote process exists: set, otherwise queue)
}

void RemoteGdbProcess::setEnvironment(const QStringList & /* env */)
{
    // TODO: Do something.
}

void RemoteGdbProcess::setWorkingDirectory(const QString &dir)
{
    m_wd = dir;
}

int RemoteGdbProcess::findAnchor(const QByteArray &data) const
{
    for (int pos = 0; pos < data.count(); ++pos) {
        const char c = data.at(pos);
        if (isdigit(c) || c == '*' || c == '+' || c == '=' || c == '~'
            || c == '@' || c == '&' || c == '^')
            return pos;
    }
    return -1;
}

void RemoteGdbProcess::sendInput(const QByteArray &data)
{
    int pos;
    for (pos = 0; pos < data.size(); ++pos)
        if (!isdigit(data.at(pos)))
            break;
    m_lastSeqNr = data.left(pos);
    m_gdbProc->sendInput(data);
}

void RemoteGdbProcess::handleAppOutput(const QByteArray &output)
{
    m_adapter->handleApplicationOutput(output);
}

void RemoteGdbProcess::handleErrOutput(const QByteArray &output)
{
    m_errorOutput += output;
    emit readyReadStandardError();
}

QByteArray RemoteGdbProcess::removeCarriageReturn(const QByteArray &data)
{
    QByteArray output;
    for (int i = 0; i < data.size(); ++i) {
        const char c = data.at(i);
        if (c != '\r')
            output += c;
    }
    return output;
}

void RemoteGdbProcess::emitErrorExit(const QString &error)
{
    if (m_error.isEmpty()) {
        m_error = error;
        emit finished(-1, QProcess::CrashExit);
    }
}

const QByteArray RemoteGdbProcess::CtrlC = QByteArray(1, 0x3);
const QByteArray RemoteGdbProcess::AppOutputFile("app_output");

} // namespace Internal
} // namespace Debugger
