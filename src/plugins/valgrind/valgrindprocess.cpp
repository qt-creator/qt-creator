// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindprocess.h"

#include "valgrindtr.h"
#include "xmlprotocol/parser.h"

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/conditional.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/qtcprocess.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

using namespace Tasking;
using namespace Utils;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {

static CommandLine valgrindCommand(const CommandLine &command,
                                   const QTcpServer &xmlServer,
                                   const QTcpServer &logServer)
{
    CommandLine cmd = command;
    cmd.addArg("--child-silent-after-fork=yes");

    // Workaround for valgrind bug when running vgdb with xml output
    // https://bugs.kde.org/show_bug.cgi?id=343902
    bool enableXml = true;

    auto handleSocketParameter = [&enableXml, &cmd](const QString &prefix,
                                                    const QTcpServer &tcpServer)
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
    return cmd;
}

class ValgrindProcessPrivate : public QObject
{
    Q_OBJECT

public:
    ValgrindProcessPrivate(ValgrindProcess *owner)
        : q(owner)
    {
        connect(&m_taskTreeRunner, &TaskTreeRunner::done, this, [this](DoneWith result) {
            emit q->done(toDoneResult(result == DoneWith::Success));
        });
    }

    Group runRecipe() const;

    bool run();

    ValgrindProcess *q = nullptr;

    CommandLine m_valgrindCommand;
    ProcessRunData m_debuggee;
    QProcess::ProcessChannelMode m_channelMode = QProcess::SeparateChannels;
    QHostAddress m_localServerAddress;
    bool m_useTerminal = false;

    TaskTreeRunner m_taskTreeRunner;

signals:
    void stopRequested();
};

Group ValgrindProcessPrivate::runRecipe() const
{
    struct ValgrindStorage {
        CommandLine m_valgrindCommand;
        std::unique_ptr<QTcpServer> m_xmlServer;
        std::unique_ptr<QTcpServer> m_logServer;
        std::unique_ptr<QTcpSocket> m_xmlSocket;
    };

    Storage<ValgrindStorage> storage;
    SingleBarrier xmlBarrier;

    const auto isSetupValid = [this, storage, xmlBarrier] {
        ValgrindStorage *storagePtr = storage.activeStorage();
        storagePtr->m_valgrindCommand.setExecutable(m_valgrindCommand.executable());
        if (!m_localServerAddress.isNull()) {
            Barrier *barrier = xmlBarrier->barrier();
            const QString ip = m_localServerAddress.toString();

            QTcpServer *xmlServer = new QTcpServer;
            storagePtr->m_xmlServer.reset(xmlServer);
            connect(xmlServer, &QTcpServer::newConnection, this, [xmlServer, storagePtr, barrier] {
                QTcpSocket *socket = xmlServer->nextPendingConnection();
                QTC_ASSERT(socket, return);
                xmlServer->close();
                storagePtr->m_xmlSocket.reset(socket);
                barrier->advance(); // Release Parser task
            });
            if (!xmlServer->listen(m_localServerAddress)) {
                emit q->processErrorReceived(Tr::tr("XmlServer on %1:").arg(ip) + ' '
                                             + xmlServer->errorString(), QProcess::FailedToStart);
                return false;
            }
            xmlServer->setMaxPendingConnections(1);

            QTcpServer *logServer = new QTcpServer;
            storagePtr->m_logServer.reset(logServer);
            connect(logServer, &QTcpServer::newConnection, this, [this, logServer] {
                QTcpSocket *socket = logServer->nextPendingConnection();
                QTC_ASSERT(socket, return);
                connect(socket, &QIODevice::readyRead, this, [this, socket] {
                    emit q->logMessageReceived(socket->readAll());
                });
                logServer->close();
            });
            if (!logServer->listen(m_localServerAddress)) {
                emit q->processErrorReceived(Tr::tr("LogServer on %1:").arg(ip) + ' '
                                             + logServer->errorString(), QProcess::FailedToStart);
                return false;
            }
            logServer->setMaxPendingConnections(1);

            storagePtr->m_valgrindCommand = valgrindCommand(storagePtr->m_valgrindCommand,
                                                            *xmlServer, *logServer);
        }
        return true;
    };

    const auto onProcessSetup = [this, storage](Process &process) {
        CommandLine cmd = storage->m_valgrindCommand;
        cmd.addArgs(m_valgrindCommand.arguments(), CommandLine::Raw);

        // consider appending our options last so they override any interfering user-supplied
        // options -q as suggested by valgrind manual

        if (cmd.executable().osType() == OsTypeMac) {
            // May be slower to start but without it we get no filenames for symbols.
            cmd.addArg("--dsymutil=yes");
        }

        cmd.addCommandLineAsArgs(m_debuggee.command);

        emit q->appendMessage(cmd.toUserOutput(), NormalMessageFormat);

        process.setCommand(cmd);
        process.setWorkingDirectory(m_debuggee.workingDirectory);
        process.setEnvironment(m_debuggee.environment);
        process.setProcessChannelMode(m_channelMode);
        process.setTerminalMode(m_useTerminal ? TerminalMode::Run : TerminalMode::Off);

        Process *processPtr = &process;
        connect(processPtr, &Process::started, this, [this, processPtr] {
            emit q->valgrindStarted(processPtr->processId());
        });
        connect(processPtr, &Process::readyReadStandardOutput, this, [this, processPtr] {
            emit q->appendMessage(processPtr->readAllStandardOutput(), StdOutFormat);
        });
        connect(processPtr, &Process::readyReadStandardError, this, [this, processPtr] {
            emit q->appendMessage(processPtr->readAllStandardError(), StdErrFormat);
        });
        connect(this, &ValgrindProcessPrivate::stopRequested, processPtr, &Process::stop);
    };
    const auto onProcessDone = [this, storage](const Process &process) {
        emit q->processErrorReceived(process.errorString(), process.error());
    };

    const auto isAddressValid = [this] { return !m_localServerAddress.isNull(); };

    const auto onParserSetup = [this, storage](Parser &parser) {
        connect(&parser, &Parser::status, q, &ValgrindProcess::status);
        connect(&parser, &Parser::error, q, &ValgrindProcess::error);
        parser.setSocket(storage->m_xmlSocket.release());
    };

    const auto onParserDone = [this](const Parser &parser) {
        emit q->internalError(parser.errorString());
    };

    const Group root {
        storage,
        xmlBarrier,
        If (isSetupValid) >> Then {
            parallel,
            ProcessTask(onProcessSetup, onProcessDone, CallDoneIf::Error),
            If (isAddressValid) >> Then {
                waitForBarrierTask(xmlBarrier),
                ParserTask(onParserSetup, onParserDone, CallDoneIf::Error)
            }
        } >> Else {
            errorItem
        }
    };
    return root;
}

bool ValgrindProcessPrivate::run()
{
    m_taskTreeRunner.start(runRecipe());
    return m_taskTreeRunner.isRunning();
}

ValgrindProcess::ValgrindProcess(QObject *parent)
    : QObject(parent)
    , d(new ValgrindProcessPrivate(this))
{}

ValgrindProcess::~ValgrindProcess() = default;

void ValgrindProcess::setValgrindCommand(const CommandLine &command)
{
    d->m_valgrindCommand = command;
}

void ValgrindProcess::setDebuggee(const ProcessRunData &debuggee)
{
    d->m_debuggee = debuggee;
}

void ValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_channelMode = mode;
}

void ValgrindProcess::setLocalServerAddress(const QHostAddress &localServerAddress)
{
    d->m_localServerAddress = localServerAddress;
}

void ValgrindProcess::setUseTerminal(bool on)
{
    d->m_useTerminal = on;
}

bool ValgrindProcess::start()
{
    return d->run();
}

void ValgrindProcess::stop()
{
    emit d->stopRequested();
}

bool ValgrindProcess::runBlocking()
{
    bool ok = false;
    QEventLoop loop;

    const auto finalize = [&loop, &ok](DoneResult result) {
        ok = result == DoneResult::Success;
        // Refer to the QObject::deleteLater() docs.
        QMetaObject::invokeMethod(&loop, [&loop] { loop.quit(); }, Qt::QueuedConnection);
    };

    connect(this, &ValgrindProcess::done, &loop, finalize);
    QTimer::singleShot(0, this, &ValgrindProcess::start);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    return ok;
}

} // namespace Valgrind

#include "valgrindprocess.moc"
