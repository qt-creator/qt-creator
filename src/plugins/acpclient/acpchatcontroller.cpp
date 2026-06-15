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
#include "acpterminalhandler.h"

#include <coreplugin/mcp/mcpmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <texteditor/texteditor.h>

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

void AcpChatController::showInspector()
{
    if (m_inspector)
        m_inspector->show(m_serverName);
}

void AcpChatController::connectToServer(const QString &serverId)
{
    disconnectFromServer();

    if (serverId.isEmpty())
        return;

    const QList<AcpSettings::ServerInfo> servers = AcpSettings::servers();
    const auto it = std::find_if(servers.begin(), servers.end(),
                                  [&serverId](const AcpSettings::ServerInfo &s) {
                                      return s.id == serverId;
                                  });
    if (it == servers.end())
        return;

    const AcpSettings::ServerInfo &serverInfo = *it;
    m_serverName = serverInfo.name;
    m_iconUrl = serverInfo.iconUrl;

    const CommandLine &cmdLine = serverInfo.launchCommand;
    auto *transport = new AcpStdioTransport(this);
    const FilePath command = cmdLine.executable();
    transport->setCommandLine(CommandLine(command, cmdLine.arguments(), CommandLine::Raw));
    if (serverInfo.envChanges.hasItems()) {
        Environment env = command.deviceEnvironment();
        serverInfo.envChanges.modifyEnvironment(env, nullptr);
        transport->setEnvironment(env);
    }
    m_transport = transport;

    m_client = new AcpClientObject(m_transport, this);
    if (m_inspector)
        m_client->setInspector(m_inspector, m_serverName);
    m_terminalHandler = new AcpTerminalHandler(m_client, this);
    m_filesystemHandler = new AcpFilesystemHandler(m_client, this);
    m_permissionHandler = new AcpPermissionHandler(m_client, this);

    connect(m_permissionHandler, &AcpPermissionHandler::permissionRequested,
            this, &AcpChatController::permissionRequested);

    connect(m_client, &AcpClientObject::stateChanged,
            this, &AcpChatController::connectionStateChanged);
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
        FileSystemCapabilities fsCaps;
        fsCaps.readTextFile(true);
        fsCaps.writeTextFile(true);
        caps.fs(fsCaps);
        initReq.clientCapabilities(caps);

        m_client->initialize(initReq);
    });

    m_transport->start();
}

void AcpChatController::disconnectFromServer()
{
    const bool wasConnected
       = m_client && m_client->state() != AcpClientObject::State::Disconnected;

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
    m_agentCapabilities.reset();
    m_initialized = false;

    if (wasConnected)
        emit connectionStateChanged(AcpClientObject::State::Disconnected);
}

void AcpChatController::createNewSession(const FilePath &workingDirectory)
{
    if (!m_client || !m_initialized)
        return;

    m_workingDirectory = workingDirectory;

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
            emit errorOccurred(Tr::tr("Session error: invalid response."));
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

        if (const auto &modes = response->modes())
            emit sessionModesReceived(modes->availableModes(), modes->currentModeId());

        qCDebug(logController) << "Session created:" << m_sessionId;
    });
}

static bool supportsEmbeddedPromptResources(std::optional<Acp::AgentCapabilities> agentCapabilities)
{
    if (!agentCapabilities)
        return false;
    if (auto promptCapabilities = agentCapabilities->promptCapabilities())
        return promptCapabilities->embeddedContext().value_or(false);
    return false;
}

void AcpChatController::sendPrompt(const QString &text,
                                   const QList<Utils::FilePath> &additionalFiles,
                                   bool includeCurrentEditor)
{
    using namespace TextEditor;
    if (text.isEmpty() || !m_client || m_sessionId.isEmpty())
        return;

    PromptRequest request;
    request.sessionId(m_sessionId);

    TextContent textContent;
    textContent.text(text);
    QList<ContentBlock> content = {textContent};
    BaseTextEditor *currentTextEditor = BaseTextEditor::currentTextEditor();
    if (includeCurrentEditor && currentTextEditor) {
        const FilePath filePath = currentTextEditor->document()->filePath();
        if (!filePath.isEmpty()) {
            const QString uri = filePath.toUrl().toString();
            content << ResourceLink()
                           .name(filePath.fileName())
                           .description("Qt Creators current main Text Editor file.")
                           .uri(uri);

            if (supportsEmbeddedPromptResources(m_agentCapabilities)) {
                TextEditorWidget *widget = currentTextEditor->editorWidget();
                TextResourceContents editorState;
                editorState.uri(uri);
                QString stateString
                    = "This is the state of the current Text Editor in Qt Creator\n";
                QTextCursor tc = currentTextEditor->textCursor();
                const QString cursorString
                    = "Cursor %1: %2, Line(0-based): %3, Column(0-based): %4\n";
                stateString += cursorString.arg("Position")
                                   .arg(tc.position())
                                   .arg(tc.blockNumber())
                                   .arg(tc.positionInBlock());
                tc.setPosition(tc.anchor());
                stateString += cursorString.arg("Anchor")
                                   .arg(tc.position())
                                   .arg(tc.blockNumber())
                                   .arg(tc.positionInBlock());
                stateString += "First Visible Line: "
                               + QString::number(widget->firstVisibleBlockNumber()) + "\n";
                stateString += "Last Visible Line: "
                               + QString::number(widget->lastVisibleBlockNumber()) + "\n";
                content << EmbeddedResource().resource(
                    TextResourceContents().text(stateString).uri(uri));
            }
        }
    }

    for (const Utils::FilePath &file : additionalFiles) {
        const QString uri = file.toUrl().toString();
        content << ResourceLink()
                       .name(file.fileName())
                       .description("Manually added context file.")
                       .uri(uri);

        if (supportsEmbeddedPromptResources(m_agentCapabilities)) {
            const auto fileContents = file.fileContents();
            if (fileContents) {
                content << EmbeddedResource().resource(
                    TextResourceContents()
                        .text(QString::fromUtf8(*fileContents))
                        .uri(uri));
            }
        }
    }

    request.prompt(content);

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
            if (const std::optional<QJsonValue> data = error->data(); data->isString()) {
                if (auto dataString = data->toString(); !dataString.isEmpty())
                    errorMsg.append("\n" + dataString);
            }
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

void AcpChatController::setSessionMode(const QString &modeId)
{
    if (m_sessionId.isEmpty() || !m_client)
        return;

    SetSessionModeRequest req;
    req.sessionId(m_sessionId);
    req.modeId(modeId);
    m_client->setSessionMode(req, [this, modeId](const QJsonObject &result, const std::optional<Error> &error) {
        Q_UNUSED(result)
        if (error) {
            emit errorOccurred(Tr::tr("Failed to set mode: %1").arg(error->message()));
            return;
        }
        emit currentModeChanged(modeId);
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

void AcpChatController::onInitializeResult(const InitializeResponse &response)
{
    if (const auto info = response.agentInfo()) {
        m_agentName = info->title().value_or(info->name());
        m_agentVersion = info->version();
    }

    m_initialized = true;

    m_authMethods = response.authMethods().value_or(QList<AuthMethod>{});
    m_agentCapabilities = response.agentCapabilities();

    if (m_inspector) {
        m_inspector->setCapabilities(
            m_serverName, m_agentCapabilities ? toJson(*m_agentCapabilities) : QJsonObject());
    }

    emit agentInfoReceived(m_agentName, m_agentVersion, m_iconUrl);

    emit sessionSelectionRequired();
}

static QList<McpServer> buildMcpServers()
{
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
            auto stdioServer = McpServerStdio().name(info.name).command(command).args(args);
            if (info.envChanges.hasItems()) {
                QList<EnvVariable> envVars;
                for (const EnvironmentItem &item : info.envChanges.itemsFromUser()) {
                    if (item.operation == EnvironmentItem::SetEnabled)
                        envVars.append(EnvVariable().name(item.name).value(item.value));
                }
                stdioServer.env(envVars);
            }
            mcpServers.append(stdioServer);
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
    return mcpServers;
}

NewSessionRequest AcpChatController::buildNewSessionRequest() const
{
    NewSessionRequest req;
    req.cwd(m_workingDirectory.toFSPathString());
    req.mcpServers(buildMcpServers());
    return req;
}

QString AcpChatController::displayName() const
{
    if (m_serverName.isEmpty())
        return Tr::tr("New Chat");
    if (m_workingDirectory.isEmpty())
        return m_serverName;
    return QString("%1 - %2").arg(m_serverName, m_workingDirectory.fileName());
}

bool AcpChatController::supportsSessionList() const
{
    if (!m_agentCapabilities)
        return false;
    const auto &sessionCaps = m_agentCapabilities->sessionCapabilities();
    return sessionCaps.has_value() && sessionCaps->list().has_value();
}

bool AcpChatController::supportsSessionDelete() const
{
    if (!m_agentCapabilities)
        return false;
    const auto &sessionCaps = m_agentCapabilities->sessionCapabilities();
    return sessionCaps.has_value() && sessionCaps->delete_().has_value();
}

void AcpChatController::listSessions(const std::optional<QString> &cursor)
{
    if (!m_client || !m_initialized)
        return;

    ListSessionsRequest req;
    req.cursor(cursor);

    m_client->listSessions(req, [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to list sessions: %1").arg(error->message()));
            return;
        }
        auto resp = fromJson<ListSessionsResponse>(QJsonValue(result));
        if (!resp) {
            emit errorOccurred(Tr::tr("Failed to list sessions: invalid response."));
            return;
        }
        emit sessionsListed(resp->sessions(), resp->nextCursor());
    });
}

void AcpChatController::deleteSession(const QString &sessionId)
{
    if (!m_client || !m_initialized)
        return;

    DeleteSessionRequest req;
    req.sessionId(sessionId);

    m_client->deleteSession(req, [this, sessionId](const QJsonObject &, const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to delete session: %1").arg(error->message()));
            return;
        }
        emit sessionDeleted(sessionId);
    });
}

void AcpChatController::loadSession(const QString &sessionId, const FilePath &workingDirectory)
{
    if (!m_client || !m_initialized)
        return;

    m_workingDirectory = workingDirectory;

    LoadSessionRequest req;
    req.sessionId(sessionId);
    req.cwd(m_workingDirectory.toFSPathString());
    req.mcpServers(buildMcpServers());

    m_client->loadSession(req, [this, sessionId](const QJsonObject &result,
                                                  const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to load session: %1").arg(error->message()));
            return;
        }
        m_sessionId = sessionId;
        emit sessionLoaded(m_sessionId);

        auto resp = fromJson<LoadSessionResponse>(QJsonValue(result));
        if (resp) {
            if (const auto &opts = resp->configOptions(); opts.has_value() && !opts->isEmpty()) {
                QList<SessionConfigOption> configOptions;
                for (const QJsonValue &item : *opts) {
                    auto opt = fromJson<SessionConfigOption>(item);
                    if (opt)
                        configOptions.append(*opt);
                }
                emit configOptionsReceived(configOptions);
            }
            if (const auto &modes = resp->modes())
                emit sessionModesReceived(modes->availableModes(), modes->currentModeId());
        }
    });
}

} // namespace AcpClient::Internal
