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

    void run();

    void handleRemoteStderr(const QString &b);
    void handleRemoteStdout(const QString &b);

    void closed(bool success);
    void localProcessStarted();
    void remoteProcessStarted();
    void findPidOutputReceived(const QString &out);

    ValgrindRunner *q;
    StandardRunnable m_debuggee;
    ApplicationLauncher m_valgrindProcess;
    IDevice::ConstPtr m_device;

    ApplicationLauncher m_findPID;

    QString m_valgrindExecutable;
    QStringList m_valgrindArguments;

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

void ValgrindRunner::Private::run()
{
    connect(&m_valgrindProcess, &ApplicationLauncher::processExited,
            this, &ValgrindRunner::Private::closed);
    connect(&m_valgrindProcess, &ApplicationLauncher::processStarted,
            this, &ValgrindRunner::Private::localProcessStarted);
    connect(&m_valgrindProcess, &ApplicationLauncher::error,
            q, &ValgrindRunner::processError);
    connect(&m_valgrindProcess, &ApplicationLauncher::appendMessage,
            q, &ValgrindRunner::processOutputReceived);
    connect(&m_valgrindProcess, &ApplicationLauncher::finished,
            q, &ValgrindRunner::finished);

    connect(&m_valgrindProcess, &ApplicationLauncher::remoteStderr,
            this, &ValgrindRunner::Private::handleRemoteStderr);
    connect(&m_valgrindProcess, &ApplicationLauncher::remoteStdout,
            this, &ValgrindRunner::Private::handleRemoteStdout);
    connect(&m_valgrindProcess, &ApplicationLauncher::remoteProcessStarted,
            this, &ValgrindRunner::Private::remoteProcessStarted);

    QStringList fullArgs = m_valgrindArguments;
    if (HostOsInfo::isMacHost())
        // May be slower to start but without it we get no filenames for symbols.
        fullArgs << "--dsymutil=yes";
    fullArgs << m_debuggee.executable;

    StandardRunnable valgrind;
    valgrind.executable = m_valgrindExecutable;
    valgrind.workingDirectory = m_debuggee.workingDirectory;
    valgrind.environment = m_debuggee.environment;
    valgrind.runMode = m_debuggee.runMode;
    valgrind.device = m_device;
    valgrind.commandLineArguments = QtcProcess::joinArgs(fullArgs, m_device->osType());
    Utils::QtcProcess::addArgs(&valgrind.commandLineArguments, m_debuggee.commandLineArguments);

    if (m_device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        m_valgrindProcess.start(valgrind);
    else
        m_valgrindProcess.start(valgrind, m_device);
}

void ValgrindRunner::Private::handleRemoteStderr(const QString &b)
{
    if (!b.isEmpty())
        q->processOutputReceived(b, Utils::StdErrFormat);
}

void ValgrindRunner::Private::handleRemoteStdout(const QString &b)
{
    if (!b.isEmpty())
        q->processOutputReceived(b, Utils::StdOutFormat);
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
    const QString proc = m_valgrindExecutable.split(' ').last();

    StandardRunnable findPid;
    findPid.executable = "/bin/sh";
    // sleep required since otherwise we might only match "bash -c..."
    //  and not the actual valgrind run
    findPid.commandLineArguments = QString("-c \""
                                           "sleep 1; ps ax" // list all processes with aliased name
                                           " | grep '\\b%1.*%2'" // find valgrind process
                                           " | tail -n 1" // limit to single process
                                           // we pick the last one, first would be "bash -c ..."
                                           " | awk '{print $1;}'" // get pid
                                           "\""
                                           ).arg(proc, Utils::FileName::fromString(m_debuggee.executable).fileName());

//    m_remote.m_findPID = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    connect(&m_findPID, &ApplicationLauncher::remoteStderr,
            this, &ValgrindRunner::Private::handleRemoteStderr);
    connect(&m_findPID, &ApplicationLauncher::remoteStdout,
            this, &ValgrindRunner::Private::findPidOutputReceived);
    m_findPID.start(findPid, m_device);
}

void ValgrindRunner::Private::findPidOutputReceived(const QString &out)
{
    if (out.isEmpty())
        return;
    bool ok;
    qint64 pid = out.trimmed().toLongLong(&ok);
    if (!ok) {
//        m_remote.m_errorString = tr("Could not determine remote PID.");
//        emit ValgrindRunner::Private::error(QProcess::FailedToStart);
//        close();
    } else {
        emit q->valgrindStarted(pid);
    }
}

void ValgrindRunner::Private::closed(bool success)
{
    Q_UNUSED(success);
//    QTC_ASSERT(m_remote.m_process, return);

//    m_remote.m_errorString = m_remote.m_process->errorString();
//    if (status == QSsh::SshRemoteProcess::FailedToStart) {
//        m_remote.m_error = QProcess::FailedToStart;
//        q->processError(QProcess::FailedToStart);
//    } else if (status == QSsh::SshRemoteProcess::NormalExit) {
//        q->processFinished(m_remote.m_process->exitCode(), QProcess::NormalExit);
//    } else if (status == QSsh::SshRemoteProcess::CrashExit) {
//        m_remote.m_error = QProcess::Crashed;
//        q->processFinished(m_remote.m_process->exitCode(), QProcess::CrashExit);
//    }
     q->processFinished(0, QProcess::NormalExit);
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
    d = 0;
}

void ValgrindRunner::setValgrindExecutable(const QString &executable)
{
    d->m_valgrindExecutable = executable;
}

void ValgrindRunner::setValgrindArguments(const QStringList &toolArguments)
{
    d->m_valgrindArguments = toolArguments;
}

void ValgrindRunner::setDebuggee(const StandardRunnable &debuggee)
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

void ValgrindRunner::waitForFinished() const
{
    if (d->m_finished)
        return;

    QEventLoop loop;
    connect(this, &ValgrindRunner::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

static void handleSocketParameter(const QString &prefix, const QTcpServer &tcpServer,
                            bool *useXml, QStringList *arguments)
{
    QHostAddress serverAddress = tcpServer.serverAddress();
    if (serverAddress.protocol() != QAbstractSocket::IPv4Protocol) {
        // Report will end up in the Application Output pane, i.e. not have
        // clickable items, but that's better than nothing.
        qWarning("Need IPv4 for valgrind");
        *useXml = false;
    } else {
        *arguments << QString("%1=%2:%3").arg(prefix).arg(serverAddress.toString())
                                         .arg(tcpServer.serverPort());
    }
}

bool ValgrindRunner::start()
{
    if (!d->localServerAddress.isNull()) {
        if (!startServers())
            return false;

        bool enableXml = !d->disableXml;

        QStringList arguments = {"--child-silent-after-fork=yes"};

        handleSocketParameter("--xml-socket", d->xmlServer, &enableXml, &arguments);
        handleSocketParameter("--log-socket", d->logServer, &enableXml, &arguments);

        if (enableXml)
            arguments << "--xml=yes";

        d->m_valgrindArguments = arguments + d->m_valgrindArguments;
    }

    d->m_valgrindProcess.setProcessChannelMode(d->channelMode);
    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual
    d->m_valgrindExecutable = d->m_valgrindExecutable;
    d->run();

    return true;
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

void ValgrindRunner::processFinished(int ret, QProcess::ExitStatus status)
{
    emit extraProcessFinished();

    if (d->m_finished)
        return;

    d->m_finished = true;

    // make sure we don't wait for the connection anymore
    emit finished();

    if (ret != 0 || status == QProcess::CrashExit)
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
