// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatcontroller.h"
#include "acpclientobject.h"
#include "acpclienttr.h"
#include "acpfilesystemhandler.h"
#include "acpinspector.h"
#include "acppermissionhandler.h"
#include "acpprotocolv1adapter.h"
#include "acpsettings.h"
#include "acpstdiotransport.h"
#include "acpterminalhandler.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>
#include <coreplugin/mcp/mcpmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/mimeutils.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <QBuffer>
#include <QImage>
#include <QJsonArray>

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
    if (serverId.isEmpty()) {
        disconnectFromServer();
        return;
    }

    const QList<AcpSettings::ServerInfo> servers = AcpSettings::servers();
    const auto it = std::find_if(servers.begin(), servers.end(),
                                  [&serverId](const AcpSettings::ServerInfo &s) {
                                      return s.id == serverId;
                                  });
    if (it == servers.end()) {
        disconnectFromServer();
        return;
    }

    connectToServer(*it);
}

void AcpChatController::connectToServer(const AcpSettings::ServerInfo &serverInfo)
{
    disconnectFromServer();

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

    connect(m_permissionHandler, &AcpPermissionHandler::permissionCancelledByAgent,
            this, &AcpChatController::permissionCancelledByAgent);

    connect(m_client, &AcpClientObject::stateChanged,
            this, &AcpChatController::connectionStateChanged);
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

        SessionConfigOptionsCapabilities configOptionsCaps;
        configOptionsCaps.boolean(BooleanConfigOptionCapabilities());
        ClientSessionCapabilities sessionCaps;
        sessionCaps.configOptions(configOptionsCaps);
        caps.session(sessionCaps);

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

    if (m_adapter) {
        m_adapter->deleteLater();
        m_adapter = nullptr;
    }
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
    m_initialized = false;

    if (wasConnected)
        emit connectionStateChanged(AcpClientObject::State::Disconnected);
}

void AcpChatController::createNewSession(const FilePath &workingDirectory)
{
    if (!m_adapter || !m_initialized)
        return;

    m_workingDirectory = workingDirectory;

    m_adapter->newSession(m_workingDirectory.toFSPathString(), buildMcpServersJson());
}

void AcpChatController::sendPrompt(const QString &text,
                                   const QList<Utils::FilePath> &additionalFiles,
                                   bool includeCurrentEditor,
                                   const QList<TextContext> &textContexts,
                                   const QList<ImageContext> &imageContexts)
{
    using namespace TextEditor;
    if (text.isEmpty() || !m_adapter || m_sessionId.isEmpty())
        return;

    const bool embeddedContext = m_adapter->supportsEmbeddedContext();

    V2::TextContent textContent;
    textContent.text(text);
    QList<V2::ContentBlock> content = {textContent};
    Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
    if (includeCurrentEditor && currentEditor) {
        const Core::IDocument *document = currentEditor->document();
        const QString mimeTypeName = document->mimeType();
        const FilePath filePath = document->filePath();
        if (!filePath.isEmpty()) {
            const QString uri = filePath.toUrl().toString();
            content << V2::ResourceLink()
                           .name(filePath.fileName())
                           .description(QString("Qt Creator's current editor file."))
                           .mimeType(mimeTypeName)
                           .uri(uri);

            if (embeddedContext) {
                if (auto *currentTextEditor = qobject_cast<BaseTextEditor *>(currentEditor)) {
                    TextEditorWidget *widget = currentTextEditor->editorWidget();
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
                    content << V2::EmbeddedResource().resource(
                        V2::TextResourceContents().text(stateString).uri(uri));
                }
            }
        } else if (embeddedContext) {
            auto embeddedResourceResource = [&]() -> V2::EmbeddedResourceResource {
                const MimeType mimeType = Utils::mimeTypeForName(mimeTypeName);
                const QString uri = QStringLiteral("qt_creator://current_editor/%1")
                        .arg(document->displayName());
                if (mimeType.inherits("text/plain")) {
                    V2::TextResourceContents contents;
                    contents.uri(uri);
                    contents.text(TextEncoding::encodingForLocale().decode(document->contents()));
                    contents.mimeType(mimeTypeName);
                    return contents;
                }
                V2::BlobResourceContents contents;
                contents.uri(uri);
                contents.blob(QString::fromLatin1(document->contents().toBase64()));
                contents.mimeType(mimeTypeName);
                return contents;
            };

            content << V2::EmbeddedResource().resource(embeddedResourceResource());
        }
    }

    for (const Utils::FilePath &file : additionalFiles) {
        const QString uri = file.toUrl().toString();
        content << V2::ResourceLink()
                       .name(file.fileName())
                       .description(QString("Manually added context file."))
                       .uri(uri);

        if (embeddedContext) {
            const auto fileContents = file.fileContents();
            if (fileContents) {
                content << V2::EmbeddedResource().resource(
                    V2::TextResourceContents()
                        .text(QString::fromUtf8(*fileContents))
                        .uri(uri));
            }
        }
    }

    for (const TextContext &ctx : textContexts) {
        const QString uri = QStringLiteral("context://%1").arg(ctx.name);
        content << V2::EmbeddedResource().resource(
            V2::TextResourceContents().text(ctx.text).uri(uri));
    }

    // Images are only ever attached when the agent advertises image support
    // (the chat panel blocks pasting otherwise), so send them as image blocks.
    if (m_adapter->supportsImagePrompt()) {
        for (const ImageContext &imageContext : imageContexts) {
            if (imageContext.image.isNull())
                continue;

            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            imageContext.image.save(&buffer, "PNG");
            content << V2::ImageContent()
                           .data(QString::fromLatin1(bytes.toBase64()))
                           .mimeType(QStringLiteral("image/png"));
        }
    }

    QJsonArray contentArray;
    for (const V2::ContentBlock &block : std::as_const(content))
        contentArray.append(V2::toJsonValue(block));

    m_adapter->prompt(m_sessionId, contentArray);
}

void AcpChatController::cancelPrompt()
{
    if (!m_adapter || m_sessionId.isEmpty())
        return;

    m_adapter->cancel(m_sessionId);
}

void AcpChatController::authenticate(const QString &methodId)
{
    if (!m_adapter || !m_initialized)
        return;

    m_adapter->authenticate(methodId);
}

void AcpChatController::setConfigOption(const QString &configId, const QJsonValue &value)
{
    if (m_sessionId.isEmpty() || !m_adapter)
        return;

    m_adapter->setConfigOption(m_sessionId, configId, value);
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

    if (m_inspector) {
        const auto &capabilities = response.agentCapabilities();
        m_inspector->setCapabilities(
            m_serverName, capabilities ? toJson(*capabilities) : QJsonObject());
    }

    m_adapter = new AcpProtocolV1Adapter(m_client, response, this);
    connect(m_adapter, &AcpProtocolAdapter::sessionCreated, this, [this](const QString &sessionId) {
        m_sessionId = sessionId;
        emit sessionCreated(sessionId);
    });
    connect(m_adapter, &AcpProtocolAdapter::sessionResumed, this, [this](const QString &sessionId) {
        m_sessionId = sessionId;
        emit sessionLoaded(sessionId);
    });
    connect(m_adapter, &AcpProtocolAdapter::sessionsListed,
            this, &AcpChatController::sessionsListed);
    connect(m_adapter, &AcpProtocolAdapter::sessionDeleted,
            this, &AcpChatController::sessionDeleted);
    connect(m_adapter, &AcpProtocolAdapter::sessionClosed, this, [this](const QString &sessionId) {
        m_sessionId.clear();
        m_workingDirectory.clear();
        emit sessionClosed(sessionId);
        emit sessionSelectionRequired();
    });
    connect(m_adapter, &AcpProtocolAdapter::configOptionsReceived,
            this, &AcpChatController::configOptionsReceived);
    connect(m_adapter, &AcpProtocolAdapter::sessionUpdate,
            this, &AcpChatController::sessionUpdate);
    connect(m_adapter, &AcpProtocolAdapter::permissionRequested,
            this, &AcpChatController::permissionRequested);
    connect(m_adapter, &AcpProtocolAdapter::authenticationRequired,
            this, &AcpChatController::authenticationRequired);
    connect(m_adapter, &AcpProtocolAdapter::authenticationFailed,
            this, &AcpChatController::authenticationFailed);
    connect(m_adapter, &AcpProtocolAdapter::authenticated,
            this, [this] { createNewSession(); });
    connect(m_adapter, &AcpProtocolAdapter::promptFinished,
            this, [this](const std::optional<V2::StopReason> &) { emit promptFinished(); });
    connect(m_adapter, &AcpProtocolAdapter::errorOccurred,
            this, &AcpChatController::errorOccurred);

    emit agentInfoReceived(m_agentName, m_agentVersion, m_iconUrl);

    emit sessionSelectionRequired();
}

// Agents may insist on the MCP server name charset, so map anything else to '_'.
static QString mcpServerName(const QString &name)
{
    QString result = name;
    for (QChar &c : result) {
        const bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                             || (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!allowed)
            c = '_';
    }
    return result;
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
            auto stdioServer
                = McpServerStdio().name(mcpServerName(info.name)).command(command).args(args);
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
            mcpServers.append(
                McpServerSse().name(mcpServerName(info.name)).url(url).headers(headers));
            break;
        }
        case Core::McpManager::Streamable_Http: {
            QTC_ASSERT(std::holds_alternative<QUrl>(info.launchInfo), continue);
            const QString url = std::get<QUrl>(info.launchInfo).toString();
            const QList<HttpHeader> headers = generateHeaders(info.httpHeaders);
            mcpServers.append(
                McpServerHttp().name(mcpServerName(info.name)).url(url).headers(headers));
            break;
        }
        }
    }
    return mcpServers;
}

// The MCP server list is built with the v1 vocabulary (the SSE variant only
// exists there) and passed to the adapter as an opaque JSON array.
QJsonArray AcpChatController::buildMcpServersJson() const
{
    QJsonArray result;
    const QList<McpServer> mcpServers = buildMcpServers();
    for (const McpServer &server : mcpServers)
        result.append(Acp::toJsonValue(server));
    return result;
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
    return m_adapter && m_adapter->supportsSessionList();
}

bool AcpChatController::supportsSessionDelete() const
{
    return m_adapter && m_adapter->supportsSessionDelete();
}

bool AcpChatController::supportsSessionClose() const
{
    return m_adapter && m_adapter->supportsSessionClose();
}

bool AcpChatController::supportsImagePrompt() const
{
    return m_adapter && m_adapter->supportsImagePrompt();
}

void AcpChatController::closeSession()
{
    if (m_sessionId.isEmpty() || !m_adapter)
        return;

    m_adapter->closeSession(m_sessionId);
}

void AcpChatController::listSessions(const std::optional<QString> &cursor)
{
    if (!m_adapter || !m_initialized)
        return;

    m_adapter->listSessions(cursor);
}

void AcpChatController::deleteSession(const QString &sessionId)
{
    if (!m_adapter || !m_initialized)
        return;

    m_adapter->deleteSession(sessionId);
}

void AcpChatController::loadSession(const QString &sessionId, const FilePath &workingDirectory)
{
    if (!m_adapter || !m_initialized)
        return;

    m_workingDirectory = workingDirectory;

    m_adapter->resumeSession(sessionId, m_workingDirectory.toFSPathString(),
                             buildMcpServersJson());
}

} // namespace AcpClient::Internal
