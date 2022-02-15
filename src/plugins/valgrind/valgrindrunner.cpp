/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindrunner.h"

#include "xmlprotocol/threadedparser.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>

using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind {

class ValgrindRunner::Private : public QObject
{
public:
    Private(ValgrindRunner *owner) : q(owner) {}

    bool run();

    void processStarted();
    void localProcessStarted();
    void remoteProcessStarted();
    void findPidOutputReceived(const QString &out, Utils::OutputFormat format);

    ValgrindRunner *q;
    Runnable m_debuggee;
    ApplicationLauncher m_valgrindProcess;
    IDevice::ConstPtr m_device;

    ApplicationLauncher m_findPID;

    CommandLine m_valgrindCommand;

    QHostAddress localServerAddress;
    QProcess::ProcessChannelMode channelMode = QProcess::SeparateChannels;
    bool m_finished = false;

    QTcpServer xmlServer;
    XmlProtocol::ThreadedParser parser;
    QTcpServer logServer;
    QTcpSocket *logSocket = nullptr;

    // Workaround for valgrind bug when running vgdb with xml output
    // https://bugs.kde.org/show_bug.cgi?id=343902
    bool disableXml = false;
};

bool ValgrindRunner::Private::run()
{
    CommandLine cmd{m_valgrindCommand.executable(), {}};

    if (!localServerAddress.isNull()) {
        if (!q->startServers())
            return false;

        cmd.addArg("--child-silent-after-fork=yes");

        bool enableXml = !disableXml;

        auto handleSocketParameter = [&enableXml, &cmd](const QString &prefix, const QTcpServer &tcpServer)
        {
            QHostAddress serverAddress = tcpServer.serverAddress();
            if (serverAddress.protocol() != QAbstractSocket::IPv4Protocol) {
                // Report will end up in the Application Output pane, i.e. not have
                // clickable items, but that's better than nothing.
                qWarning("Need IPv4 for valgrind");
                enableXml = false;
            } else {
                cmd.addArg(QString("%1=%2:%3").arg(prefix).arg(serverAddress.toString())
                           .arg(tcpServer.serverPort()));
            }
        };

        handleSocketParameter("--xml-socket", xmlServer);
        handleSocketParameter("--log-socket", logServer);

        if (enableXml)
            cmd.addArg("--xml=yes");
    }
    cmd.addArgs(m_valgrindCommand.arguments(), CommandLine::Raw);

    m_valgrindProcess.setProcessChannelMode(channelMode);
    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual

    connect(&m_valgrindProcess, &ApplicationLauncher::finished,
            q, &ValgrindRunner::processFinished);
    connect(&m_valgrindProcess, &ApplicationLauncher::processStarted,
            this, &ValgrindRunner::Private::processStarted);
    connect(&m_valgrindProcess, &ApplicationLauncher::error,
            q, &ValgrindRunner::processError);
    connect(&m_valgrindProcess, &ApplicationLauncher::appendMessage,
            q, &ValgrindRunner::processOutputReceived);

    if (HostOsInfo::isMacHost())
        // May be slower to start but without it we get no filenames for symbols.
        cmd.addArg("--dsymutil=yes");

    cmd.addCommandLineAsArgs(m_debuggee.command);

    emit q->valgrindExecuted(cmd.toUserOutput());

    Runnable valgrind;
    valgrind.command = cmd;
    valgrind.workingDirectory = m_debuggee.workingDirectory;
    valgrind.environment = m_debuggee.environment;
    valgrind.device = m_device;

    if (m_device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        m_valgrindProcess.setRunnable(valgrind);
        m_valgrindProcess.start();
    } else if (m_device->type() == "DockerDeviceType") {
        valgrind.device = {};
        m_valgrindProcess.setRunnable(valgrind);
        m_valgrindProcess.start();
    } else {
        m_valgrindProcess.setRunnable(valgrind);
        m_valgrindProcess.start(m_device);
    }

    return true;
}

void ValgrindRunner::Private::processStarted()
{
    if (m_valgrindProcess.isLocal())
        localProcessStarted();
    else
        remoteProcessStarted();
}

void ValgrindRunner::Private::localProcessStarted()
{
    qint64 pid = m_valgrindProcess.applicationPID().pid();
    emit q->valgrindStarted(pid);
}

void ValgrindRunner::Private::remoteProcessStarted()
{
    // find out what PID our process has

    // NOTE: valgrind cloaks its name,
    // e.g.: valgrind --tool=memcheck foobar
    // => ps aux, pidof will see valgrind.bin
    // => pkill/killall/top... will see memcheck-amd64-linux or similar
    // hence we need to do something more complex...

    // plain path to exe, m_valgrindExe contains e.g. env vars etc. pp.
    // FIXME: Really?
    const QString proc = m_valgrindCommand.executable().toString().split(' ').last();
    QString procEscaped = proc;
    procEscaped.replace("/", "\\\\/");

    Runnable findPid;
    // sleep required since otherwise we might only match "bash -c..." and not the actual
    // valgrind run
    findPid.command.setExecutable("/bin/sh");
    findPid.command.setArguments(QString("-c \""
           "sleep 1; ps ax"        // list all processes with aliased name
           " | grep '%1.*%2'"      // find valgrind process that runs with our exec
           " | awk '\\$5 ~ /^%3/"  // 5th column must start with valgrind process
           " {print \\$1;}'"       // print 1st then (with PID)
           "\"").arg(proc, m_debuggee.command.executable().fileName(), procEscaped));

//    m_remote.m_findPID = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    connect(&m_findPID, &ApplicationLauncher::appendMessage,
            this, &ValgrindRunner::Private::findPidOutputReceived);
    m_findPID.setRunnable(findPid);
    m_findPID.start(m_device);
}

void ValgrindRunner::Private::findPidOutputReceived(const QString &out, Utils::OutputFormat format)
{
    if (format != Utils::StdOutFormat) {
        emit q->processOutputReceived(out, format);
        return;
    }
    if (out.isEmpty())
        return;
    bool ok;
    const qint64 pid = out.trimmed().toLongLong(&ok);
    if (!ok) {
//        m_remote.m_errorString = tr("Could not determine remote PID.");
//        emit ValgrindRunner::Private::error(QProcess::FailedToStart);
//        close();
    } else {
        emit q->valgrindStarted(pid);
    }
}

ValgrindRunner::ValgrindRunner(QObject *parent)
    : QObject(parent), d(new Private(this))
{
}

ValgrindRunner::~ValgrindRunner()
{
    if (d->m_valgrindProcess.isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    if (d->parser.isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    delete d;
    d = nullptr;
}

void ValgrindRunner::setValgrindCommand(const Utils::CommandLine &command)
{
    d->m_valgrindCommand = command;
}

void ValgrindRunner::setDebuggee(const Runnable &debuggee)
{
    d->m_debuggee = debuggee;
}

void ValgrindRunner::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->channelMode = mode;
}

void ValgrindRunner::setLocalServerAddress(const QHostAddress &localServerAddress)
{
    d->localServerAddress = localServerAddress;
}

void ValgrindRunner::setDevice(const IDevice::ConstPtr &device)
{
    d->m_device = device;
}

void ValgrindRunner::setUseTerminal(bool on)
{
    d->m_valgrindProcess.setUseTerminal(on);
}

void ValgrindRunner::waitForFinished() const
{
    if (d->m_finished)
        return;

    QEventLoop loop;
    connect(this, &ValgrindRunner::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

bool ValgrindRunner::start()
{
    return d->run();
}

void ValgrindRunner::processError(QProcess::ProcessError e)
{
    if (d->m_finished)
        return;

    d->m_finished = true;

    // make sure we don't wait for the connection anymore
    emit processErrorReceived(errorString(), e);
    emit finished();
}

void ValgrindRunner::processFinished()
{
    emit extraProcessFinished();

    if (d->m_finished)
        return;

    d->m_finished = true;

    // make sure we don't wait for the connection anymore
    emit finished();

    if (d->m_valgrindProcess.exitCode() != 0 || d->m_valgrindProcess.exitStatus() == QProcess::CrashExit)
        emit processErrorReceived(errorString(), d->m_valgrindProcess.processError());
}

QString ValgrindRunner::errorString() const
{
    return d->m_valgrindProcess.errorString();
}

void ValgrindRunner::stop()
{
    d->m_valgrindProcess.stop();
}

XmlProtocol::ThreadedParser *ValgrindRunner::parser() const
{
    return &d->parser;
}

void ValgrindRunner::xmlSocketConnected()
{
    QTcpSocket *socket = d->xmlServer.nextPendingConnection();
    QTC_ASSERT(socket, return);
    d->xmlServer.close();
    d->parser.parse(socket);
}

void ValgrindRunner::logSocketConnected()
{
    d->logSocket = d->logServer.nextPendingConnection();
    QTC_ASSERT(d->logSocket, return);
    connect(d->logSocket, &QIODevice::readyRead,
            this, &ValgrindRunner::readLogSocket);
    d->logServer.close();
}

void ValgrindRunner::readLogSocket()
{
    QTC_ASSERT(d->logSocket, return);
    emit logMessageReceived(d->logSocket->readAll());
}

bool ValgrindRunner::startServers()
{
    bool check = d->xmlServer.listen(d->localServerAddress);
    const QString ip = d->localServerAddress.toString();
    if (!check) {
        emit processErrorReceived(tr("XmlServer on %1:").arg(ip) + ' '
                                  + d->xmlServer.errorString(), QProcess::FailedToStart );
        return false;
    }
    d->xmlServer.setMaxPendingConnections(1);
    connect(&d->xmlServer, &QTcpServer::newConnection,
            this, &ValgrindRunner::xmlSocketConnected);
    check = d->logServer.listen(d->localServerAddress);
    if (!check) {
        emit processErrorReceived(tr("LogServer on %1:").arg(ip) + ' '
                                  + d->logServer.errorString(), QProcess::FailedToStart );
        return false;
    }
    d->logServer.setMaxPendingConnections(1);
    connect(&d->logServer, &QTcpServer::newConnection,
            this, &ValgrindRunner::logSocketConnected);
    return true;
}

} // namespace Valgrind
