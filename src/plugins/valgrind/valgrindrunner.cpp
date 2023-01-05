// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindrunner.h"

#include "valgrindtr.h"
#include "xmlprotocol/threadedparser.h"

#include <projectexplorer/runcontrol.h>

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
    Private(ValgrindRunner *owner) : q(owner) {
        connect(&m_process, &QtcProcess::started, this, [this] {
            emit q->valgrindStarted(m_process.processId());
        });
        connect(&m_process, &QtcProcess::done, this, [this] {
            if (m_process.result() != ProcessResult::FinishedWithSuccess)
                emit q->processErrorReceived(m_process.errorString(), m_process.error());
            emit q->finished();
        });
        connect(&m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
            emit q->appendMessage(m_process.readAllStandardOutput(), StdOutFormat);
        });
        connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
            emit q->appendMessage(m_process.readAllStandardError(), StdErrFormat);
        });

        connect(&m_xmlServer, &QTcpServer::newConnection, this, &Private::xmlSocketConnected);
        connect(&m_logServer, &QTcpServer::newConnection, this, &Private::logSocketConnected);
    }

    void xmlSocketConnected();
    void logSocketConnected();
    bool startServers();
    bool run();

    ValgrindRunner *q;
    Runnable m_debuggee;

    CommandLine m_command;
    QtcProcess m_process;

    QHostAddress m_localServerAddress;

    QTcpServer m_xmlServer;
    XmlProtocol::ThreadedParser m_parser;
    QTcpServer m_logServer;
};

void ValgrindRunner::Private::xmlSocketConnected()
{
    QTcpSocket *socket = m_xmlServer.nextPendingConnection();
    QTC_ASSERT(socket, return);
    m_xmlServer.close();
    m_parser.parse(socket);
}

void ValgrindRunner::Private::logSocketConnected()
{
    QTcpSocket *logSocket = m_logServer.nextPendingConnection();
    QTC_ASSERT(logSocket, return);
    connect(logSocket, &QIODevice::readyRead, this, [this, logSocket] {
        emit q->logMessageReceived(logSocket->readAll());
    });
    m_logServer.close();
}

bool ValgrindRunner::Private::startServers()
{
    const bool xmlOK = m_xmlServer.listen(m_localServerAddress);
    const QString ip = m_localServerAddress.toString();
    if (!xmlOK) {
        emit q->processErrorReceived(Tr::tr("XmlServer on %1:").arg(ip) + ' '
                                     + m_xmlServer.errorString(), QProcess::FailedToStart );
        return false;
    }
    m_xmlServer.setMaxPendingConnections(1);
    const bool logOK = m_logServer.listen(m_localServerAddress);
    if (!logOK) {
        emit q->processErrorReceived(Tr::tr("LogServer on %1:").arg(ip) + ' '
                                     + m_logServer.errorString(), QProcess::FailedToStart );
        return false;
    }
    m_logServer.setMaxPendingConnections(1);
    return true;
}

bool ValgrindRunner::Private::run()
{
    CommandLine cmd;
    cmd.setExecutable(m_command.executable());

    if (!m_localServerAddress.isNull()) {
        if (!startServers())
            return false;

        cmd.addArg("--child-silent-after-fork=yes");

        // Workaround for valgrind bug when running vgdb with xml output
        // https://bugs.kde.org/show_bug.cgi?id=343902
        bool enableXml = true;

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

        handleSocketParameter("--xml-socket", m_xmlServer);
        handleSocketParameter("--log-socket", m_logServer);

        if (enableXml)
            cmd.addArg("--xml=yes");
    }
    cmd.addArgs(m_command.arguments(), CommandLine::Raw);

    // consider appending our options last so they override any interfering user-supplied options
    // -q as suggested by valgrind manual

    if (cmd.executable().osType() == OsTypeMac) {
        // May be slower to start but without it we get no filenames for symbols.
        cmd.addArg("--dsymutil=yes");
    }

    cmd.addCommandLineAsArgs(m_debuggee.command);

    emit q->valgrindExecuted(cmd.toUserOutput());

    m_process.setCommand(cmd);
    m_process.setWorkingDirectory(m_debuggee.workingDirectory);
    m_process.setEnvironment(m_debuggee.environment);
    m_process.start();
    return true;
}

ValgrindRunner::ValgrindRunner(QObject *parent)
    : QObject(parent), d(new Private(this))
{
}

ValgrindRunner::~ValgrindRunner()
{
    if (d->m_process.isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    if (d->m_parser.isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    delete d;
    d = nullptr;
}

void ValgrindRunner::setValgrindCommand(const CommandLine &command)
{
    d->m_command = command;
}

void ValgrindRunner::setDebuggee(const Runnable &debuggee)
{
    d->m_debuggee = debuggee;
}

void ValgrindRunner::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_process.setProcessChannelMode(mode);
}

void ValgrindRunner::setLocalServerAddress(const QHostAddress &localServerAddress)
{
    d->m_localServerAddress = localServerAddress;
}

void ValgrindRunner::setUseTerminal(bool on)
{
    d->m_process.setTerminalMode(on ? TerminalMode::On : TerminalMode::Off);
}

void ValgrindRunner::waitForFinished() const
{
    if (d->m_process.state() == QProcess::NotRunning)
        return;

    QEventLoop loop;
    connect(this, &ValgrindRunner::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

QString ValgrindRunner::errorString() const
{
    return d->m_process.errorString();
}

bool ValgrindRunner::start()
{
    return d->run();
}

void ValgrindRunner::stop()
{
    d->m_process.stop();
}

XmlProtocol::ThreadedParser *ValgrindRunner::parser() const
{
    return &d->m_parser;
}

} // namespace Valgrind
