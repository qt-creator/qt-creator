// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpterminalhandler.h"
#include "acpclientobject.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logTerminal, "qtc.acpclient.terminal", QtWarningMsg);

using namespace Acp;
using namespace Utils;

namespace AcpClient::Internal {

AcpTerminalHandler::AcpTerminalHandler(AcpClientObject *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(client, &AcpClientObject::createTerminalRequested,
            this, &AcpTerminalHandler::handleCreate);
    connect(client, &AcpClientObject::terminalOutputRequested,
            this, &AcpTerminalHandler::handleOutput);
    connect(client, &AcpClientObject::waitForTerminalExitRequested,
            this, &AcpTerminalHandler::handleWaitForExit);
    connect(client, &AcpClientObject::killTerminalRequested,
            this, &AcpTerminalHandler::handleKill);
    connect(client, &AcpClientObject::releaseTerminalRequested,
            this, &AcpTerminalHandler::handleRelease);
}

AcpTerminalHandler::~AcpTerminalHandler()
{
    for (auto &info : m_terminals) {
        if (info.process) {
            info.process->kill();
            delete info.process;
        }
    }
}

void AcpTerminalHandler::handleCreate(const QJsonValue &id, const CreateTerminalRequest &request)
{
    const QString terminalId = QStringLiteral("term_%1").arg(m_nextTerminalId++);
    qCDebug(logTerminal) << "Creating terminal:" << terminalId << "command:" << request.command();

    auto *process = new Process(this);
    process->setProcessMode(ProcessMode::Writer);

    CommandLine cmd{FilePath::fromUserInput(request.command())};
    if (const auto args = request.args()) {
        for (const QString &arg : *args)
            cmd.addArg(arg);
    }
    process->setCommand(cmd);

    if (const auto cwd = request.cwd())
        process->setWorkingDirectory(FilePath::fromUserInput(*cwd));

    if (const auto envVars = request.env()) {
        Environment env = Environment::systemEnvironment();
        for (const EnvVariable &var : *envVars)
            env.set(var.name(), var.value());
        process->setEnvironment(env);
    }

    TerminalInfo &info = m_terminals[terminalId];
    info.process = process;

    connect(process, &Process::readyReadStandardOutput, this, [this, terminalId] {
        auto it = m_terminals.find(terminalId);
        if (it != m_terminals.end())
            it->output.append(it->process->readAllRawStandardOutput());
    });
    connect(process, &Process::readyReadStandardError, this, [this, terminalId] {
        auto it = m_terminals.find(terminalId);
        if (it != m_terminals.end())
            it->output.append(it->process->readAllRawStandardError());
    });
    connect(process, &Process::done, this, [this, terminalId] {
        auto it = m_terminals.find(terminalId);
        if (it == m_terminals.end())
            return;
        it->exited = true;
        it->exitCode = it->process->exitCode();
        qCDebug(logTerminal) << "Terminal" << terminalId << "exited with code:" << it->exitCode;

        // Fulfill deferred wait_for_exit if any
        if (!it->waitingId.isUndefined() && !it->waitingId.isNull()) {
            sendWaitResponse(it->waitingId, *it);
            it->waitingId = QJsonValue();
        }
    });

    process->start();

    CreateTerminalResponse response;
    response.terminalId(terminalId);
    m_client->sendResponse(id, Acp::toJson(response));
}

void AcpTerminalHandler::handleOutput(const QJsonValue &id, const TerminalOutputRequest &request)
{
    auto it = m_terminals.find(request.terminalId());
    if (it == m_terminals.end()) {
        m_client->sendErrorResponse(id, ErrorCode::Resource_not_found,
                                    QStringLiteral("Terminal not found: %1").arg(request.terminalId()));
        return;
    }

    TerminalOutputResponse response;
    response.output(QString::fromUtf8(it->output));
    response.truncated(false);

    if (it->exited) {
        TerminalExitStatus exitStatus;
        exitStatus.exitCode(it->exitCode);
        if (!it->signal.isEmpty())
            exitStatus.signal(it->signal);
        response.exitStatus(exitStatus);
    }

    m_client->sendResponse(id, Acp::toJson(response));
}

void AcpTerminalHandler::handleWaitForExit(const QJsonValue &id, const WaitForTerminalExitRequest &request)
{
    auto it = m_terminals.find(request.terminalId());
    if (it == m_terminals.end()) {
        m_client->sendErrorResponse(id, ErrorCode::Resource_not_found,
                                    QStringLiteral("Terminal not found: %1").arg(request.terminalId()));
        return;
    }

    if (it->exited) {
        sendWaitResponse(id, *it);
    } else {
        // Defer the response until the process exits
        it->waitingId = id;
    }
}

void AcpTerminalHandler::handleKill(const QJsonValue &id, const KillTerminalCommandRequest &request)
{
    auto it = m_terminals.find(request.terminalId());
    if (it == m_terminals.end()) {
        m_client->sendErrorResponse(id, ErrorCode::Resource_not_found,
                                    QStringLiteral("Terminal not found: %1").arg(request.terminalId()));
        return;
    }

    if (it->process && it->process->isRunning())
        it->process->kill();

    KillTerminalCommandResponse response;
    m_client->sendResponse(id, Acp::toJson(response));
}

void AcpTerminalHandler::handleRelease(const QJsonValue &id, const ReleaseTerminalRequest &request)
{
    auto it = m_terminals.find(request.terminalId());
    if (it == m_terminals.end()) {
        m_client->sendErrorResponse(id, ErrorCode::Resource_not_found,
                                    QStringLiteral("Terminal not found: %1").arg(request.terminalId()));
        return;
    }

    if (it->process) {
        if (it->process->isRunning())
            it->process->kill();
        delete it->process;
    }
    m_terminals.erase(it);

    ReleaseTerminalResponse response;
    m_client->sendResponse(id, Acp::toJson(response));
}

void AcpTerminalHandler::sendWaitResponse(const QJsonValue &id, const TerminalInfo &info)
{
    WaitForTerminalExitResponse response;
    response.exitCode(info.exitCode);
    if (!info.signal.isEmpty())
        response.signal(info.signal);
    m_client->sendResponse(id, Acp::toJson(response));
}

} // namespace AcpClient::Internal
