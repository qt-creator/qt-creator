/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "languageclientmanager.h"

#include "languageclientutils.h"
#include "languageclientplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/icore.h>
#include <languageserverprotocol/messages.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>
#include <texteditor/textdocument.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QTextBlock>
#include <QTimer>

using namespace LanguageServerProtocol;

namespace LanguageClient {

static LanguageClientManager *managerInstance = nullptr;

LanguageClientManager::LanguageClientManager(QObject *parent)
    : QObject (parent)
{
    using namespace Core;
    using namespace ProjectExplorer;
    JsonRpcMessageHandler::registerMessageProvider<PublishDiagnosticsNotification>();
    JsonRpcMessageHandler::registerMessageProvider<SemanticHighlightNotification>();
    JsonRpcMessageHandler::registerMessageProvider<ApplyWorkspaceEditRequest>();
    JsonRpcMessageHandler::registerMessageProvider<LogMessageNotification>();
    JsonRpcMessageHandler::registerMessageProvider<ShowMessageRequest>();
    JsonRpcMessageHandler::registerMessageProvider<ShowMessageNotification>();
    JsonRpcMessageHandler::registerMessageProvider<WorkSpaceFolderRequest>();
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &LanguageClientManager::editorOpened);
    connect(EditorManager::instance(), &EditorManager::documentOpened,
            this, &LanguageClientManager::documentOpened);
    connect(EditorManager::instance(), &EditorManager::documentClosed,
            this, &LanguageClientManager::documentClosed);
    connect(EditorManager::instance(), &EditorManager::saved,
            this, &LanguageClientManager::documentContentsSaved);
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, &LanguageClientManager::documentWillSave);
    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &LanguageClientManager::projectAdded);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &LanguageClientManager::projectRemoved);
}

LanguageClientManager::~LanguageClientManager()
{
    QTC_ASSERT(m_clients.isEmpty(), qDeleteAll(m_clients));
    qDeleteAll(m_currentSettings);
    managerInstance = nullptr;
}

void LanguageClientManager::init()
{
    if (managerInstance)
        return;
    QTC_ASSERT(LanguageClientPlugin::instance(), return);
    managerInstance = new LanguageClientManager(LanguageClientPlugin::instance());
}

void LanguageClientManager::startClient(Client *client)
{
    QTC_ASSERT(managerInstance, return);
    QTC_ASSERT(client, return);
    if (managerInstance->m_shuttingDown) {
        managerInstance->clientFinished(client);
        return;
    }
    if (!managerInstance->m_clients.contains(client))
        managerInstance->m_clients.append(client);
    connect(client, &Client::finished, managerInstance, [client](){
        managerInstance->clientFinished(client);
    });
    if (client->start())
        client->initialize();
    else
        managerInstance->clientFinished(client);

    connect(client,
            &Client::initialized,
            &managerInstance->m_currentDocumentLocatorFilter,
            &DocumentLocatorFilter::updateCurrentClient);
}

void LanguageClientManager::startClient(BaseSettings *setting, ProjectExplorer::Project *project)
{
    QTC_ASSERT(managerInstance, return);
    QTC_ASSERT(setting, return);
    QTC_ASSERT(setting->isValid(), return);
    Client *client = setting->createClient();
    QTC_ASSERT(client, return);
    client->setCurrentProject(project);
    startClient(client);
    managerInstance->m_clientsForSetting[setting->m_id].append(client);
}

QVector<Client *> LanguageClientManager::clients()
{
    QTC_ASSERT(managerInstance, return {});
    return managerInstance->m_clients;
}

void LanguageClientManager::addExclusiveRequest(const MessageId &id, Client *client)
{
    QTC_ASSERT(managerInstance, return);
    managerInstance->m_exclusiveRequests[id] << client;
}

void LanguageClientManager::reportFinished(const MessageId &id, Client *byClient)
{
    QTC_ASSERT(managerInstance, return);
    for (Client *client : managerInstance->m_exclusiveRequests[id]) {
        if (client != byClient)
            client->cancelRequest(id);
    }
    managerInstance->m_exclusiveRequests.remove(id);
}

void LanguageClientManager::shutdownClient(Client *client)
{
    if (!client)
        return;
    if (client->reachable())
        client->shutdown();
    else if (client->state() != Client::Shutdown && client->state() != Client::ShutdownRequested)
        deleteClient(client);
}

void LanguageClientManager::deleteClient(Client *client)
{
    QTC_ASSERT(managerInstance, return);
    QTC_ASSERT(client, return);
    client->disconnect();
    managerInstance->m_clients.removeAll(client);
    for (QVector<Client *> &clients : managerInstance->m_clientsForSetting)
        clients.removeAll(client);
    if (managerInstance->m_shuttingDown)
        delete client;
    else
        client->deleteLater();
}

void LanguageClientManager::shutdown()
{
    QTC_ASSERT(managerInstance, return);
    if (managerInstance->m_shuttingDown)
        return;
    managerInstance->m_shuttingDown = true;
    for (Client *client : managerInstance->m_clients)
        shutdownClient(client);
    QTimer::singleShot(3000, managerInstance, [](){
        for (auto interface : managerInstance->m_clients)
            deleteClient(interface);
        emit managerInstance->shutdownFinished();
    });
}

LanguageClientManager *LanguageClientManager::instance()
{
    return managerInstance;
}

QList<Client *> LanguageClientManager::clientsSupportingDocument(
    const TextEditor::TextDocument *doc)
{
    QTC_ASSERT(managerInstance, return {});
    QTC_ASSERT(doc, return {};);
    return Utils::filtered(managerInstance->reachableClients(), [doc](Client *client) {
        return client->isSupportedDocument(doc);
    }).toList();
}

void LanguageClientManager::applySettings()
{
    QTC_ASSERT(managerInstance, return);
    qDeleteAll(managerInstance->m_currentSettings);
    managerInstance->m_currentSettings = Utils::transform(LanguageClientSettings::currentPageSettings(),
                                                          [](BaseSettings *settings) {
            return settings->copy();
    });
    LanguageClientSettings::toSettings(Core::ICore::settings(), managerInstance->m_currentSettings);

    const QList<BaseSettings *> restarts = Utils::filtered(managerInstance->m_currentSettings,
                                                           [](BaseSettings *settings) {
            return settings->needsRestart();
    });

    for (BaseSettings *setting : restarts) {
        for (Client *client : clientForSetting(setting))
            shutdownClient(client);
        if (!setting->isValid() || !setting->m_enabled)
            continue;
        switch (setting->m_startBehavior) {
        case BaseSettings::AlwaysOn:
            startClient(setting);
            break;
        case BaseSettings::RequiresFile:
            if (Utils::anyOf(Core::DocumentModel::openedDocuments(),
                             [filter = setting->m_languageFilter](Core::IDocument *doc) {
                                 return filter.isSupported(doc);
                             })) {
                startClient(setting);
            }
            break;
        case BaseSettings::RequiresProject: {
            for (Core::IDocument *doc : Core::DocumentModel::openedDocuments()) {
                if (setting->m_languageFilter.isSupported(doc)) {
                    const Utils::FilePath filePath = doc->filePath();
                    for (ProjectExplorer::Project *project :
                         ProjectExplorer::SessionManager::projects()) {
                        if (project->isKnownFile(filePath))
                            startClient(setting, project);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

QList<BaseSettings *> LanguageClientManager::currentSettings()
{
    QTC_ASSERT(managerInstance, return {});
    return managerInstance->m_currentSettings;
}

QVector<Client *> LanguageClientManager::clientForSetting(const BaseSettings *setting)
{
    QTC_ASSERT(managerInstance, return {});
    auto instance = managerInstance;
    return instance->m_clientsForSetting.value(setting->m_id);
}

const BaseSettings *LanguageClientManager::settingForClient(Client *client)
{
    QTC_ASSERT(managerInstance, return nullptr);
    for (const QString &id : managerInstance->m_clientsForSetting.keys()) {
        for (const Client *settingClient : managerInstance->m_clientsForSetting[id]) {
            if (settingClient == client) {
                return Utils::findOrDefault(managerInstance->m_currentSettings,
                                            [id](BaseSettings *setting) {
                                                return setting->m_id == id;
                                            });
            }
        }
    }
    return nullptr;
}

Client *LanguageClientManager::clientForEditor(Core::IEditor *iEditor)
{
    QTC_ASSERT(managerInstance, return nullptr);

    auto editor = qobject_cast<TextEditor::BaseTextEditor *>(iEditor);
    if (!editor)
        return nullptr;

    return Utils::findOrDefault(managerInstance->reachableClients(),
                                [doc = editor->textDocument()](Client *client) {
                                    return client->documentOpen(doc);
                                });
}

QVector<Client *> LanguageClientManager::reachableClients()
{
    return Utils::filtered(m_clients, &Client::reachable);
}

static void sendToInterfaces(const IContent &content, const QVector<Client *> &interfaces)
{
    for (Client *interface : interfaces)
        interface->sendContent(content);
}

void LanguageClientManager::sendToAllReachableServers(const IContent &content)
{
    sendToInterfaces(content, reachableClients());
}

void LanguageClientManager::clientFinished(Client *client)
{
    constexpr int restartTimeoutS = 5;
    const bool unexpectedFinish = client->state() != Client::Shutdown
            && client->state() != Client::ShutdownRequested;
    if (unexpectedFinish && !m_shuttingDown && client->reset()) {
        client->disconnect(this);
        client->log(tr("Unexpectedly finished. Restarting in %1 seconds.").arg(restartTimeoutS),
                    Core::MessageManager::Flash);
        QTimer::singleShot(restartTimeoutS * 1000, client, [client]() { startClient(client); });
    } else {
        if (unexpectedFinish && !m_shuttingDown)
            client->log(tr("Unexpectedly finished."), Core::MessageManager::Flash);
        deleteClient(client);
        if (m_shuttingDown && m_clients.isEmpty())
            emit shutdownFinished();
    }
}

void LanguageClientManager::editorOpened(Core::IEditor *editor)
{
    using namespace TextEditor;
    if (auto *textEditor = qobject_cast<BaseTextEditor *>(editor)) {
        if (TextEditorWidget *widget = textEditor->editorWidget()) {
            connect(widget, &TextEditorWidget::requestLinkAt, this,
                    [this, filePath = editor->document()->filePath()]
                    (const QTextCursor &cursor, Utils::ProcessLinkCallback &callback) {
                        findLinkAt(filePath, cursor, callback);
                    });
            connect(widget, &TextEditorWidget::requestUsages, this,
                    [this, filePath = editor->document()->filePath()]
                    (const QTextCursor &cursor){
                        findUsages(filePath, cursor);
                    });
            connect(widget, &TextEditorWidget::cursorPositionChanged, this, [this, widget](){
                        // TODO This would better be a compressing timer
                        QTimer::singleShot(50, this,
                                           [this, widget = QPointer<TextEditorWidget>(widget)]() {
                            if (widget) {
                                for (Client *client : this->reachableClients()) {
                                    if (client->isSupportedDocument(widget->textDocument()))
                                        client->cursorPositionChanged(widget);
                                }
                            }
                        });
                    });
            updateEditorToolBar(editor);
            for (auto client : reachableClients())
                widget->addHoverHandler(client->hoverHandler());
        }
    }
}

void LanguageClientManager::documentOpened(Core::IDocument *document)
{
    // check whether we have to start servers for this document
    for (BaseSettings *setting : LanguageClientSettings::currentPageSettings()) {
        const QVector<Client *> clients = clientForSetting(setting);
        if (setting->isValid() && setting->m_enabled
            && setting->m_languageFilter.isSupported(document)) {
            if (setting->m_startBehavior == BaseSettings::RequiresProject) {
                const Utils::FilePath filePath = document->filePath();
                for (ProjectExplorer::Project *project :
                     ProjectExplorer::SessionManager::projects()) {
                    // check whether file is part of this project
                    if (!project->isKnownFile(filePath))
                        continue;

                    // check whether we already have a client running for this project
                    if (Utils::findOrDefault(clients,
                                             [project](QPointer<Client> client) {
                                                 return client->project() == project;
                                             })) {
                        continue;
                    }
                    startClient(setting, project);
                }
            } else if (setting->m_startBehavior == BaseSettings::RequiresFile && clients.isEmpty()) {
                startClient(setting);
            }
        }
    }
    for (Client *interface : reachableClients())
        interface->openDocument(document);
}

void LanguageClientManager::documentClosed(Core::IDocument *document)
{
    const DidCloseTextDocumentParams params(
        TextDocumentIdentifier(DocumentUri::fromFileName(document->filePath())));
    for (Client *interface : reachableClients())
        interface->closeDocument(params);
}

void LanguageClientManager::documentContentsSaved(Core::IDocument *document)
{
    for (Client *interface : reachableClients())
        interface->documentContentsSaved(document);
}

void LanguageClientManager::documentWillSave(Core::IDocument *document)
{
    for (Client *interface : reachableClients())
        interface->documentContentsSaved(document);
}

void LanguageClientManager::findLinkAt(const Utils::FilePath &filePath,
                                       const QTextCursor &cursor,
                                       Utils::ProcessLinkCallback callback)
{
    const DocumentUri uri = DocumentUri::fromFileName(filePath);
    const TextDocumentIdentifier document(uri);
    const Position pos(cursor);
    TextDocumentPositionParams params(document, pos);
    GotoDefinitionRequest request(params);
    request.setResponseCallback([callback](const GotoDefinitionRequest::Response &response){
        if (Utils::optional<GotoResult> _result = response.result()) {
            const GotoResult result = _result.value();
            if (Utils::holds_alternative<std::nullptr_t>(result))
                return;
            if (auto ploc = Utils::get_if<Location>(&result)) {
                callback(ploc->toLink());
            } else if (auto plloc = Utils::get_if<QList<Location>>(&result)) {
                if (!plloc->isEmpty())
                    callback(plloc->value(0).toLink());
            }
        }
    });
    for (Client *interface : reachableClients()) {
        if (interface->findLinkAt(request))
            m_exclusiveRequests[request.id()] << interface;
    }
}

QList<Core::SearchResultItem> generateSearchResultItems(const LanguageClientArray<Location> &locations)
{
    auto convertPosition = [](const Position &pos){
        return Core::Search::TextPosition(pos.line() + 1, pos.character());
    };
    auto convertRange = [convertPosition](const Range &range){
        return Core::Search::TextRange(convertPosition(range.start()), convertPosition(range.end()));
    };
    QList<Core::SearchResultItem> result;
    if (locations.isNull())
        return result;
    QMap<QString, QList<Core::Search::TextRange>> rangesInDocument;
    for (const Location &location : locations.toList())
        rangesInDocument[location.uri().toFileName().toString()] << convertRange(location.range());
    for (auto it = rangesInDocument.begin(); it != rangesInDocument.end(); ++it) {
        const QString &fileName = it.key();
        QFile file(fileName);
        file.open(QFile::ReadOnly);

        Core::SearchResultItem item;
        item.path = QStringList() << fileName;
        item.useTextEditorFont = true;

        QStringList lines = QString::fromLocal8Bit(file.readAll()).split(QChar::LineFeed);
        for (const Core::Search::TextRange &range : it.value()) {
            item.mainRange = range;
            if (file.isOpen() && range.begin.line > 0 && range.begin.line <= lines.size())
                item.text = lines[range.begin.line - 1];
            else
                item.text.clear();
            result << item;
        }
    }
    return result;
}

void LanguageClientManager::findUsages(const Utils::FilePath &filePath, const QTextCursor &cursor)
{
    const DocumentUri uri = DocumentUri::fromFileName(filePath);
    const TextDocumentIdentifier document(uri);
    const Position pos(cursor);
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    ReferenceParams params(TextDocumentPositionParams(document, pos));
    params.setContext(ReferenceParams::ReferenceContext(true));
    FindReferencesRequest request(params);
    auto callback = [wordUnderCursor = termCursor.selectedText()]
            (const QString &clientName, const FindReferencesRequest::Response &response){
        if (auto result = response.result()) {
            Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
                        tr("Find References with %1 for:").arg(clientName), "", wordUnderCursor);
            search->addResults(generateSearchResultItems(result.value()), Core::SearchResult::AddOrdered);
            QObject::connect(search, &Core::SearchResult::activated,
                             [](const Core::SearchResultItem& item) {
                                 Core::EditorManager::openEditorAtSearchResult(item);
                             });
            search->finishSearch(false);
            search->popup();
        }
    };
    for (Client *client : reachableClients()) {
        request.setResponseCallback([callback, clientName = client->name()]
                                    (const FindReferencesRequest::Response &response){
            callback(clientName, response);
        });
        if (client->findUsages(request))
            m_exclusiveRequests[request.id()] << client;
    }
}

void LanguageClientManager::projectAdded(ProjectExplorer::Project *project)
{
    for (BaseSettings *setting : m_currentSettings) {
        if (setting->isValid()
            && setting->m_enabled
            && setting->m_startBehavior == BaseSettings::RequiresProject) {
            if (Utils::findOrDefault(clientForSetting(setting),
                                     [project](QPointer<Client> client) {
                                         return client->project() == project;
                                     })
                    == nullptr) {
                for (Core::IDocument *doc : Core::DocumentModel::openedDocuments()) {
                    if (setting->m_languageFilter.isSupported(doc)) {
                        if (project->isKnownFile(doc->filePath()))
                            startClient(setting, project);
                    }
                }
            }
        }
    }
    for (Client *interface : reachableClients())
        interface->projectOpened(project);
}

void LanguageClientManager::projectRemoved(ProjectExplorer::Project *project)
{
    for (Client *interface : m_clients)
        interface->projectClosed(project);
}

} // namespace LanguageClient
