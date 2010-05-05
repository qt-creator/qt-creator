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

namespace Debugger {
namespace Internal {

RemoteGdbProcess::RemoteGdbProcess(const Core::SshServerInfo &server,
    RemotePlainGdbAdapter *adapter, QObject *parent)
    : AbstractGdbProcess(parent), m_serverInfo(server), m_adapter(adapter)
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
    m_gdbState = CmdNotYetSent;
    m_gdbConn = Core::InteractiveSshConnection::create(m_serverInfo);
    if (m_gdbConn->hasError())
        return;
    m_appOutputReaderState = CmdNotYetSent;
    m_appOutputConn = Core::InteractiveSshConnection::create(m_serverInfo);
    if (m_appOutputConn->hasError())
        return;
    m_errOutputReaderState = CmdNotYetSent;
    m_errOutputConn = Core::InteractiveSshConnection::create(m_serverInfo);
    if (m_errOutputConn->hasError())
        return;
    m_command = cmd;
    m_cmdArgs = args;
    connect(m_gdbConn.data(), SIGNAL(remoteOutput(QByteArray)),
            this, SLOT(handleGdbOutput(QByteArray)));
    connect(m_appOutputConn.data(), SIGNAL(remoteOutput(QByteArray)),
            this, SLOT(handleAppOutput(QByteArray)));
    connect(m_errOutputConn.data(), SIGNAL(remoteOutput(QByteArray)),
            this, SLOT(handleErrOutput(QByteArray)));
    m_gdbConn->start();
    m_errOutputConn->start();
    m_appOutputConn->start();
}

bool RemoteGdbProcess::waitForStarted()
{
    return true;
}

qint64 RemoteGdbProcess::write(const QByteArray &data)
{
    if (m_gdbState != CmdReceived || !m_inputToSend.isEmpty()
        || !m_lastSeqNr.isEmpty()) {
        m_inputToSend.enqueue(data);
        return data.size();
    } else {
        return sendInput(data);
    }
}

void RemoteGdbProcess::kill()
{
    stopReaders();
    Core::InteractiveSshConnection::Ptr controlConn
        = Core::InteractiveSshConnection::create(m_serverInfo);
    if (!controlConn->hasError()) {
        if (controlConn->start())
            controlConn->sendInput("pkill -x gdb\r\n");
    }

    m_gdbConn->quit();
    emit finished(0, QProcess::CrashExit);
}

QProcess::ProcessState RemoteGdbProcess::state() const
{
    switch (m_gdbState) {
    case CmdNotYetSent:
        return QProcess::NotRunning;
    case CmdSent:
        return QProcess::Starting;
    case CmdReceived:
    default:
        return QProcess::Running;
    }
}

QString RemoteGdbProcess::errorString() const
{
    return m_gdbConn ? m_gdbConn->error() : QString();
}

void RemoteGdbProcess::handleGdbOutput(const QByteArray &output)
{
#if 0
    qDebug("%s: output is '%s'", Q_FUNC_INFO, output.data());
#endif

    if (m_gdbState == CmdNotYetSent)
        return;

    m_currentGdbOutput += removeCarriageReturn(output);
    if (!m_currentGdbOutput.endsWith('\n'))
        return;

    if (m_gdbState == CmdSent) {
        const int index = m_currentGdbOutput.indexOf(m_startCmdLine);
        if (index != -1)
            m_currentGdbOutput.remove(index, m_startCmdLine.size());
        // Note: We can't guarantee that we will match the command line,
        // because the remote terminal sometimes inserts control characters.
        // Otherwise we could change the state to CmdReceived here.
    }

    m_gdbState = CmdReceived;

    checkForGdbExit(m_currentGdbOutput);
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

void RemoteGdbProcess::setProcessEnvironment(const QProcessEnvironment &env)
{
    // TODO: Do something.
}

void RemoteGdbProcess::setEnvironment(const QStringList &env)
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

qint64 RemoteGdbProcess::sendInput(const QByteArray &data)
{
    int pos;
    for (pos = 0; pos < data.size(); ++pos)
        if (!isdigit(data.at(pos)))
            break;
    m_lastSeqNr = data.left(pos);
    return m_gdbConn->sendInput(data) ? data.size() : 0;
}

void RemoteGdbProcess::handleAppOutput(const QByteArray &output)
{
    if (!handleAppOrErrOutput(m_appOutputConn, m_appOutputReaderState,
                              m_initialAppOutput, AppOutputFile, output))
        m_adapter->handleApplicationOutput(output);
}

void RemoteGdbProcess::handleErrOutput(const QByteArray &output)
{
    if (!handleAppOrErrOutput(m_errOutputConn, m_errOutputReaderState,
                              m_initialErrOutput, ErrOutputFile, output)) {
        m_errorOutput += output;
        emit readyReadStandardError();
    }
}

bool RemoteGdbProcess::handleAppOrErrOutput(Core::InteractiveSshConnection::Ptr &conn,
         CmdState &cmdState, QByteArray &initialOutput, const QByteArray &file,
         const QByteArray &output)
{
    const QByteArray cmdLine1 = mkFifoCmdLine(file);
    const QByteArray cmdLine2 = readerCmdLine(file);
    if (cmdState == CmdNotYetSent) {
        conn->sendInput(cmdLine1);
        cmdState = CmdSent;
        return true;
    }

    if (cmdState == CmdSent) {
        initialOutput += output;
        if (initialOutput.endsWith(cmdLine2)) {
            cmdState = CmdReceived;
            if (m_appOutputReaderState == m_errOutputReaderState
                && m_gdbState == CmdNotYetSent)
                startGdb();
        } else if (initialOutput.contains(cmdLine1)
            && !initialOutput.endsWith(cmdLine1)) {
            initialOutput.clear();
            conn->sendInput(cmdLine2);
        }
        return true;
    }

    return false;
}

void RemoteGdbProcess::startGdb()
{
    m_startCmdLine = "stty -echo && " + m_command.toUtf8() + ' '
        + m_cmdArgs.join(QLatin1String(" ")).toUtf8()
        + " -tty=" + AppOutputFile + " 2>" + ErrOutputFile + '\n';
    if (!m_wd.isEmpty())
        m_startCmdLine.prepend("cd " + m_wd.toUtf8() + " && ");
    sendInput(m_startCmdLine);
    m_gdbState = CmdSent;
}

void RemoteGdbProcess::stopReaders()
{
    if (m_appOutputConn) {
        disconnect(m_appOutputConn.data(), SIGNAL(remoteOutput(QByteArray)),
                   this, SLOT(handleAppOutput(QByteArray)));
        m_appOutputConn->sendInput(CtrlC);
        m_appOutputConn->quit();
    }
    if (m_errOutputConn)  {
        disconnect(m_errOutputConn.data(), SIGNAL(remoteOutput(QByteArray)),
                   this, SLOT(handleErrOutput(QByteArray)));
        m_errOutputConn->sendInput(CtrlC);
        m_errOutputConn->quit();
    }
}

QByteArray RemoteGdbProcess::mkFifoCmdLine(const QByteArray &file)
{
    return "rm -f " + file + " && mkfifo " + file + "\r\n";
}

QByteArray RemoteGdbProcess::readerCmdLine(const QByteArray &file)
{
    return  "cat " + file + "\r\n";
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

void RemoteGdbProcess::checkForGdbExit(QByteArray &output)
{
    const QByteArray exitString("^exit");
    const int exitPos = output.indexOf(exitString);
    if (exitPos != -1) {
        disconnect(m_gdbConn.data(), SIGNAL(remoteOutput(QByteArray)),
                   this, SLOT(handleGdbOutput(QByteArray)));
        output.remove(exitPos + exitString.size(), output.size());
        stopReaders();
        emit finished(0, QProcess::NormalExit);
    }
}


const QByteArray RemoteGdbProcess::CtrlC = QByteArray(1, 0x3);
const QByteArray RemoteGdbProcess::AppOutputFile("app_output");
const QByteArray RemoteGdbProcess::ErrOutputFile("err_output");

} // namespace Internal
} // namespace Debugger
