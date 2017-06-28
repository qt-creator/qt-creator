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
#include "valgrindprocess.h"

#include "xmlprotocol/threadedparser.h"

#include <projectexplorer/runnables.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QEventLoop>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>

using namespace ProjectExplorer;

namespace Valgrind {

class ValgrindRunner::Private
{
public:
    QHostAddress localServerAddress;
    ValgrindProcess *process = nullptr;
    QProcess::ProcessChannelMode channelMode = QProcess::SeparateChannels;
    bool finished = false;
    QString valgrindExecutable;
    QStringList valgrindArguments;
    StandardRunnable debuggee;
    IDevice::ConstPtr device;
    QString tool;

    QTcpServer xmlServer;
    XmlProtocol::ThreadedParser parser;
    QTcpServer logServer;
    QTcpSocket *logSocket = nullptr;
    bool disableXml = false;
};

ValgrindRunner::ValgrindRunner(QObject *parent)
    : QObject(parent), d(new Private)
{
    setToolName("memcheck");
}

ValgrindRunner::~ValgrindRunner()
{
    if (d->process && d->process->isRunning()) {
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
    d->valgrindExecutable = executable;
}

QString ValgrindRunner::valgrindExecutable() const
{
    return d->valgrindExecutable;
}

void ValgrindRunner::setValgrindArguments(const QStringList &toolArguments)
{
    d->valgrindArguments = toolArguments;
}

QStringList ValgrindRunner::valgrindArguments() const
{
    return d->valgrindArguments;
}

QStringList ValgrindRunner::fullValgrindArguments() const
{
    QStringList fullArgs = valgrindArguments();
    fullArgs << QString("--tool=%1").arg(d->tool);
    if (Utils::HostOsInfo::isMacHost())
        // May be slower to start but without it we get no filenames for symbols.
        fullArgs << QLatin1String("--dsymutil=yes");
    return fullArgs;
}

void ValgrindRunner::setDebuggee(const StandardRunnable &debuggee)
{
    d->debuggee = debuggee;
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
    d->device = device;
}

IDevice::ConstPtr ValgrindRunner::device() const
{
    return d->device;
}

void ValgrindRunner::waitForFinished() const
{
    if (d->finished || !d->process)
        return;

    QEventLoop loop;
    connect(this, &ValgrindRunner::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void ValgrindRunner::setToolName(const QString &toolName)
{
    d->tool = toolName;
}

bool ValgrindRunner::start()
{
    if (!d->localServerAddress.isNull()) {
        if (!startServers())
            return false;
        setValgrindArguments(memcheckLogArguments() + valgrindArguments());
    }

    d->process = new ValgrindProcess(d->device, this);
    d->process->setProcessChannelMode(d->channelMode);
    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual
    d->process->setValgrindExecutable(d->valgrindExecutable);
    d->process->setValgrindArguments(fullValgrindArguments());
    d->process->setDebuggee(d->debuggee);

    QObject::connect(d->process, &ValgrindProcess::processOutput,
                     this, &ValgrindRunner::processOutputReceived);
    QObject::connect(d->process, &ValgrindProcess::started,
                     this, &ValgrindRunner::started);
    QObject::connect(d->process, &ValgrindProcess::finished,
                     this, &ValgrindRunner::processFinished);
    QObject::connect(d->process, &ValgrindProcess::error,
                     this, &ValgrindRunner::processError);

    d->process->run(d->debuggee.runMode);

    emit extraStart();

    return true;
}

void ValgrindRunner::processError(QProcess::ProcessError e)
{
    if (d->finished)
        return;

    d->finished = true;

    // make sure we don't wait for the connection anymore
    emit processErrorReceived(errorString(), e);
    emit finished();
}

void ValgrindRunner::processFinished(int ret, QProcess::ExitStatus status)
{
    emit extraProcessFinished();

    if (d->finished)
        return;

    d->finished = true;

    // make sure we don't wait for the connection anymore
    emit finished();

    if (ret != 0 || status == QProcess::CrashExit)
        emit processErrorReceived(errorString(), d->process->processError());
}

QString ValgrindRunner::errorString() const
{
    return d->process ? d->process->errorString() : QString();
}

void ValgrindRunner::stop()
{
    QTC_ASSERT(d->process, finished(); return);
    d->process->close();
}

ValgrindProcess *ValgrindRunner::valgrindProcess() const
{
    return d->process;
}

XmlProtocol::ThreadedParser *ValgrindRunner::parser() const
{
    return &d->parser;
}


// Workaround for valgrind bug when running vgdb with xml output
// https://bugs.kde.org/show_bug.cgi?id=343902
void ValgrindRunner::disableXml()
{
    d->disableXml = true;
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

QStringList ValgrindRunner::memcheckLogArguments() const
{
    bool enableXml = !d->disableXml;

    QStringList arguments = {"--child-silent-after-fork=yes"};

    handleSocketParameter("--xml-socket", d->xmlServer, &enableXml, &arguments);
    handleSocketParameter("--log-socket", d->logServer, &enableXml, &arguments);

    if (enableXml)
        arguments << "--xml=yes";

    return arguments;
}

} // namespace Valgrind
