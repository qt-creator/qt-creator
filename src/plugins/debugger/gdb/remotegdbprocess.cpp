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
    m_gdbConn = Core::InteractiveSshConnection::create(m_serverInfo);
    m_appOutputConn = Core::InteractiveSshConnection::create(m_serverInfo);
    m_errOutputConn = Core::InteractiveSshConnection::create(m_serverInfo);
    m_command = cmd;
    m_cmdArgs = args;
    m_errOutputConn->start();
    m_appOutputConn->start();
    m_gdbConn->start();
 }

bool RemoteGdbProcess::waitForStarted()
{
    if (!waitForInputReady(m_appOutputConn))
        return false;
    if (!sendAndWaitForEcho(m_appOutputConn, readerCmdLine(AppOutputFile)))
        return false;
    if (!waitForInputReady(m_errOutputConn))
        return false;
    if (!sendAndWaitForEcho(m_errOutputConn, readerCmdLine(ErrOutputFile)))
        return false;
    if (!waitForInputReady(m_gdbConn))
        return false;
    connect(m_appOutputConn.data(), SIGNAL(remoteOutputAvailable()),
            this, SLOT(handleAppOutput()));
    connect(m_errOutputConn.data(), SIGNAL(remoteOutputAvailable()),
            this, SLOT(handleErrOutput()));
    connect(m_gdbConn.data(), SIGNAL(remoteOutputAvailable()),
            this, SLOT(handleGdbOutput()));
    m_gdbStarted = false;
    m_gdbCmdLine = "stty -echo && DISPLAY=:0.0 " + m_command.toUtf8() + ' '
        + m_cmdArgs.join(QLatin1String(" ")).toUtf8()
        + " -tty=" + AppOutputFile + " 2>" + ErrOutputFile + '\n';
    if (!m_wd.isEmpty())
        m_gdbCmdLine.prepend("cd " + m_wd.toUtf8() + " && ");
    if (sendInput(m_gdbCmdLine) != m_gdbCmdLine.count())
        return false;

    return true;
}

qint64 RemoteGdbProcess::write(const QByteArray &data)
{
    if (!m_gdbStarted || !m_inputToSend.isEmpty() || !m_lastSeqNr.isEmpty()) {
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
    return m_gdbStarted ? QProcess::Running : QProcess::Starting;
}

QString RemoteGdbProcess::errorString() const
{
    return m_gdbConn ? m_gdbConn->error() : QString();
}

void RemoteGdbProcess::handleGdbOutput()
{
    m_currentGdbOutput
        += removeCarriageReturn(m_gdbConn->waitForRemoteOutput(0));
#if 0
    qDebug("%s: complete unread output is '%s'", Q_FUNC_INFO, m_currentGdbOutput.data());
#endif
    if (checkForGdbExit(m_currentGdbOutput)) {
        m_currentGdbOutput.clear();
        return;
    }

    if (!m_currentGdbOutput.endsWith('\n'))
        return;

    if (!m_gdbStarted) {
        const int index = m_currentGdbOutput.indexOf(m_gdbCmdLine);
        if (index != -1)
            m_currentGdbOutput.remove(index, m_gdbCmdLine.size());
        // Note: We can't guarantee that we will match the command line,
        // because the remote terminal sometimes inserts control characters.
        // Otherwise we could set m_gdbStarted here.
    }

    m_gdbStarted = true;

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

void RemoteGdbProcess::handleAppOutput()
{
    m_adapter->handleApplicationOutput(m_appOutputConn->waitForRemoteOutput(0));
}

void RemoteGdbProcess::handleErrOutput()
{
    m_errorOutput += m_errOutputConn->waitForRemoteOutput(0);
    emit readyReadStandardError();
}

void RemoteGdbProcess::stopReaders()
{
    if (m_appOutputConn) {
        disconnect(m_appOutputConn.data(), SIGNAL(remoteOutputAvailable()),
                   this, SLOT(handleAppOutput()));
        m_appOutputConn->sendInput(CtrlC);
        m_appOutputConn->quit();
    }
    if (m_errOutputConn)  {
        disconnect(m_errOutputConn.data(), SIGNAL(remoteOutputAvailable()),
                   this, SLOT(handleErrOutput()));
        m_errOutputConn->sendInput(CtrlC);
        m_errOutputConn->quit();
    }
}

QByteArray RemoteGdbProcess::readerCmdLine(const QByteArray &file)
{
    return "rm -f " + file + " && mkfifo " + file +  " && cat " + file + "\r\n";
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

bool RemoteGdbProcess::checkForGdbExit(QByteArray &output)
{
    const QByteArray exitString("^exit");
    const int exitPos = output.indexOf(exitString);
    if (exitPos == -1)
        return false;

    emit finished(0, QProcess::NormalExit);
    disconnect(m_gdbConn.data(), SIGNAL(remoteOutputAvailable()),
               this, SLOT(handleGdbOutput()));
    output.remove(exitPos + exitString.size(), output.size());
    stopReaders();
    return true;
}

bool RemoteGdbProcess::waitForInputReady(Core::InteractiveSshConnection::Ptr &conn)
{
    if (conn->waitForRemoteOutput(m_serverInfo.timeout).isEmpty())
        return false;
    while (!conn->waitForRemoteOutput(100).isEmpty())
        ;
    return true;
}

bool RemoteGdbProcess::sendAndWaitForEcho(Core::InteractiveSshConnection::Ptr &conn,
    const QByteArray &cmdLine)
{
    conn->sendInput(cmdLine);
    QByteArray allOutput;
    while (!allOutput.endsWith(cmdLine)) {
        const QByteArray curOutput = conn->waitForRemoteOutput(100);
        if (curOutput.isEmpty())
            return false;
        allOutput += curOutput;
    }
    return true;
}


const QByteArray RemoteGdbProcess::CtrlC = QByteArray(1, 0x3);
const QByteArray RemoteGdbProcess::AppOutputFile("app_output");
const QByteArray RemoteGdbProcess::ErrOutputFile("err_output");

} // namespace Internal
} // namespace Debugger
