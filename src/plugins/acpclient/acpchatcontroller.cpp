// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatcontroller.h"
#include "acpclientobject.h"
#include "acpclienttr.h"
#include "acpfilesystemhandler.h"
#include "acpinspector.h"
#include "acppermissionhandler.h"
#include "acpsettings.h"
#include "acpstdiotransport.h"
#include "acptcptransport.h"
#include "acpterminalhandler.h"

#include <coreplugin/mcpmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logController, "qtc.acpclient.controller", QtWarningMsg);

using namespace Acp;
using namespace Utils;

namespace AcpClient::Internal {

AcpChatController::AcpChatController(QObject *parent)
    : QObject(parent)
{
}

AcpChatController::~AcpChatController()
{
    disconnectFromServer();
}

void AcpChatController::setInspector(AcpInspector *inspector)
{
    m_inspector = inspector;
}

void AcpChatController::connectToServer(const QString &serverId, const FilePath &workingDirectory)
{
    disconnectFromServer();

    if (serverId.isEmpty())
        return;

    m_workingDirectory = workingDirectory;

    const QList<AcpSettings::ServerInfo> servers = AcpSettings::servers();
    const auto it = std::find_if(servers.begin(), servers.end(),
                                  [&serverId](const AcpSettings::ServerInfo &s) {
                                      return s.id == serverId;
                                  });
    if (it == servers.end())
        return;

    const AcpSettings::ServerInfo &serverInfo = *it;
    m_serverName = serverInfo.name;

    if (serverInfo.connectionType == AcpSettings::Stdio) {
        QTC_ASSERT(std::holds_alternative<CommandLine>(serverInfo.launchInfo), return);
        const CommandLine &cmdLine = std::get<CommandLine>(serverInfo.launchInfo);
        auto *transport = new AcpStdioTransport(this);
        const FilePath command = cmdLine.executable();
        transport->setCommandLine(CommandLine(command, cmdLine.arguments(), CommandLine::Raw));
        if (!workingDirectory.isEmpty())
            transport->setWorkingDirectory(workingDirectory);
        if (serverInfo.envChanges.hasItems()) {
            Environment env = command.deviceEnvironment();
            serverInfo.envChanges.modifyEnvironment(env, nullptr);
            transport->setEnvironment(env);
        }
        m_transport = transport;
    } else {
        QTC_ASSERT((std::holds_alternative<QPair<QString, quint16>>(serverInfo.launchInfo)), return);
        const auto &[host, port] = std::get<QPair<QString, quint16>>(serverInfo.launchInfo);
        auto *transport = new AcpTcpTransport(this);
        transport->setHost(host);
        transport->setPort(port);
        m_transport = transport;
    }

    m_client = new AcpClientObject(m_transport, this);
    if (m_inspector)
        m_client->setInspector(m_inspector, m_serverName);
    m_terminalHandler = new AcpTerminalHandler(m_client, this);
    m_filesystemHandler = new AcpFilesystemHandler(m_client, this);
    m_permissionHandler = new AcpPermissionHandler(m_client, this);

    connect(m_permissionHandler, &AcpPermissionHandler::permissionRequested,
            this, &AcpChatController::permissionRequested);

    connect(m_client, &AcpClientObject::stateChanged,
            this, [this](AcpClientObject::State state) { onStateChanged(static_cast<int>(state)); });
    connect(m_client, &AcpClientObject::sessionUpdate,
            this, &AcpChatController::sessionUpdate);
    connect(m_client, &AcpClientObject::initializeResult,
            this, &AcpChatController::onInitializeResult);
    connect(m_client, &AcpClientObject::errorOccurred,
            this, &AcpChatController::errorOccurred);

    connect(m_transport, &AcpTransport::started, this, [this] {
        InitializeRequest initReq;
        initReq.protocolVersion(1);
        Implementation clientInfoImpl;
        clientInfoImpl.name(QStringLiteral("QtCreator"));
        clientInfoImpl.version(QStringLiteral("1.0"));
        initReq.clientInfo(clientInfoImpl);

        ClientCapabilities caps;
        caps.terminal(true);
        FileSystemCapability fsCaps;
        fsCaps.readTextFile(true);
        fsCaps.writeTextFile(true);
        caps.fs(fsCaps);
        initReq.clientCapabilities(caps);

        m_client->initialize(initReq);
    });

    m_connected = true;
    emit connectionStateChanged(true);
    m_transport->start();
}

void AcpChatController::disconnectFromServer()
{
    if (m_transport)
        m_transport->stop();

    if (m_permissionHandler) {
        m_permissionHandler->deleteLater();
        m_permissionHandler = nullptr;
    }
    if (m_filesystemHandler) {
        m_filesystemHandler->deleteLater();
        m_filesystemHandler = nullptr;
    }
    if (m_terminalHandler) {
        m_terminalHandler->deleteLater();
        m_terminalHandler = nullptr;
    }
    if (m_client) {
        m_client->deleteLater();
        m_client = nullptr;
    }
    if (m_transport) {
        m_transport->deleteLater();
        m_transport = nullptr;
    }

    m_workingDirectory.clear();
    m_sessionId.clear();
    m_agentName.clear();
    m_agentVersion.clear();
    m_serverName.clear();
    m_authMethods.clear();
    m_initialized = false;

    if (m_connected) {
        m_connected = false;
        emit connectionStateChanged(false);
    }
}

void AcpChatController::createNewSession()
{
    if (!m_client || !m_initialized)
        return;


    m_client->newSession(buildNewSessionRequest(),
                         [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            if (error->code() == ErrorCode::Authentication_required && !m_authMethods.isEmpty()) {
                emit authenticationRequired(m_authMethods);
                return;
            }
            emit errorOccurred(Tr::tr("Session error: %1").arg(error->message()));
            return;
        }
        auto response = fromJson<NewSessionResponse>(QJsonValue(result));
        if (!response) {
            emit errorOccurred(Tr::tr("Session error: invalid response"));
            return;
        }

        m_sessionId = response->sessionId();
        emit sessionCreated(m_sessionId);

        if (const auto &opts = response->configOptions(); opts.has_value() && !opts->isEmpty()) {
            qCDebug(logController) << "Session configOptions raw array size:" << opts->size();
            QList<SessionConfigOption> configOptions;
            for (const QJsonValue &item : *opts) {
                auto opt = fromJson<SessionConfigOption>(item);
                if (opt)
                    configOptions.append(*opt);
            }
            emit configOptionsReceived(configOptions);
        } else {
            qCDebug(logController) << "Session response has no configOptions";
        }

        qCDebug(logController) << "Session created:" << m_sessionId;
    });
}

void AcpChatController::sendPrompt(const QString &text)
{
    if (text.isEmpty() || !m_client || m_sessionId.isEmpty())
        return;

    PromptRequest request;
    request.sessionId(m_sessionId);

    TextContent textContent;
    textContent.text(text);
    request.prompt({ContentBlock(textContent)});

    m_client->prompt(request, [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Prompt error: %1").arg(error->message()));
        } else {
            auto response = fromJson<PromptResponse>(QJsonValue(result));
            if (response)
                qCDebug(logController) << "Prompt completed. Stop reason:" << toString(response->stopReason());
        }
        emit promptFinished();
    });
}

void AcpChatController::cancelPrompt()
{
    if (!m_client || m_sessionId.isEmpty())
        return;

    CancelNotification cancel;
    cancel.sessionId(m_sessionId);
    m_client->cancelSession(cancel);
}

void AcpChatController::authenticate(const QString &methodId)
{
    if (!m_client || !m_initialized)
        return;

    AuthenticateRequest req;
    req.methodId(methodId);
    m_client->authenticate(req, [this](const QJsonObject &result, const std::optional<Error> &error) {
        Q_UNUSED(result)
        if (error) {
            QString errorMsg = error->message();
            if (const std::optional<QString> data = error->data())
                errorMsg.append("\n" + *data);
            emit authenticationFailed(errorMsg);
            return;
        }
        createNewSession();
    });
}

void AcpChatController::setConfigOption(const QString &configId, const QString &value)
{
    if (m_sessionId.isEmpty() || !m_client)
        return;

    SetSessionConfigOptionRequest req;
    req.sessionId(m_sessionId);
    req.configId(configId);
    req.value(value);
    m_client->setSessionConfigOption(req, [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to set config option: %1").arg(error->message()));
            return;
        }
        auto resp = fromJson<SetSessionConfigOptionResponse>(QJsonValue(result));
        if (resp)
            emit configOptionsReceived(resp->configOptions());
    });
}

void AcpChatController::sendPermissionResponse(const QJsonValue &id, const QString &optionId)
{
    if (m_permissionHandler)
        m_permissionHandler->sendPermissionResponse(id, optionId);
}

void AcpChatController::sendPermissionCancelled(const QJsonValue &id)
{
    if (m_permissionHandler)
        m_permissionHandler->sendPermissionCancelled(id);
}

void AcpChatController::onStateChanged(int state)
{
    const auto s = static_cast<AcpClientObject::State>(state);
    switch (s) {
    case AcpClientObject::State::Disconnected:
        m_initialized = false;
        if (m_connected) {
            m_connected = false;
            emit connectionStateChanged(false);
        }
        break;
    case AcpClientObject::State::Connecting:
        if (!m_connected) {
            m_connected = true;
            emit connectionStateChanged(true);
        }
        break;
    case AcpClientObject::State::Initialized:
        break;
    default:
        break;
    }
}

void AcpChatController::onInitializeResult(const InitializeResponse &response)
{
    if (const auto info = response.agentInfo()) {
        m_agentName = info->title().value_or(info->name());
        m_agentVersion = info->version();
    }

    emit agentInfoReceived(m_agentName, m_agentVersion);

    m_initialized = true;

    m_authMethods = response.authMethods().value_or(QList<AuthMethod>{});
    createNewSession();
}

NewSessionRequest AcpChatController::buildNewSessionRequest() const
{
    NewSessionRequest req;
    req.cwd(m_workingDirectory.toFSPathString());

    auto generateHeaders = [](const QStringList &headers) {
        return Utils::transform(headers, [](const QString &h) -> HttpHeader {
            HttpHeader header;
            int split = h.indexOf(':');
            header.name(h.left(split).trimmed());
            header.value(h.mid(split + 1).trimmed());
            return header;
        });
    };

    QList<McpServer> mcpServers;
    for (const Core::McpManager::ServerInfo &info : Core::McpManager::mcpServers()) {
        switch (info.connectionType) {
        case Core::McpManager::Stdio: {
            QTC_ASSERT(std::holds_alternative<CommandLine>(info.launchInfo), continue);
            const CommandLine commandLine = std::get<CommandLine>(info.launchInfo);
            const QString command = commandLine.executable().toUserOutput();
            const QStringList args = ProcessArgs::splitArgs(
                commandLine.arguments(), HostOsInfo::hostOs());
            mcpServers.append(McpServerStdio().name(info.name).command(command).args(args));
            break;
        }
        case Core::McpManager::Sse: {
            QTC_ASSERT(std::holds_alternative<QUrl>(info.launchInfo), continue);
            const QString url = std::get<QUrl>(info.launchInfo).toString();
            const QList<HttpHeader> headers = generateHeaders(info.httpHeaders);
            mcpServers.append(McpServerSse().name(info.name).url(url).headers(headers));
            break;
        }
        case Core::McpManager::Streamable_Http: {
            QTC_ASSERT(std::holds_alternative<QUrl>(info.launchInfo), continue);
            const QString url = std::get<QUrl>(info.launchInfo).toString();
            const QList<HttpHeader> headers = generateHeaders(info.httpHeaders);
            mcpServers.append(McpServerHttp().name(info.name).url(url).headers(headers));
            break;
        }
        }
    }
    req.mcpServers(mcpServers);

    return req;
}

} // namespace AcpClient::Internal
