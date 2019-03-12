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

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/searchresultwindow.h>
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

LanguageClientManager::LanguageClientManager()
{
    JsonRpcMessageHandler::registerMessageProvider<PublishDiagnosticsNotification>();
    JsonRpcMessageHandler::registerMessageProvider<ApplyWorkspaceEditRequest>();
    JsonRpcMessageHandler::registerMessageProvider<LogMessageNotification>();
    JsonRpcMessageHandler::registerMessageProvider<ShowMessageRequest>();
    JsonRpcMessageHandler::registerMessageProvider<ShowMessageNotification>();
    managerInstance = this;
}

LanguageClientManager::~LanguageClientManager()
{
    QTC_ASSERT(m_clients.isEmpty(), qDeleteAll(m_clients));
}

void LanguageClientManager::init()
{
    using namespace Core;
    using namespace ProjectExplorer;
    QTC_ASSERT(managerInstance, return);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            managerInstance, &LanguageClientManager::editorOpened);
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            managerInstance, &LanguageClientManager::editorsClosed);
    connect(EditorManager::instance(), &EditorManager::saved,
            managerInstance, &LanguageClientManager::documentContentsSaved);
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            managerInstance, &LanguageClientManager::documentWillSave);
    connect(SessionManager::instance(), &SessionManager::projectAdded,
            managerInstance, &LanguageClientManager::projectAdded);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            managerInstance, &LanguageClientManager::projectRemoved);
}

void LanguageClientManager::startClient(Client *client)
{
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
}

QVector<Client *> LanguageClientManager::clients()
{
    return managerInstance->m_clients;
}

void LanguageClientManager::addExclusiveRequest(const MessageId &id, Client *client)
{
    managerInstance->m_exclusiveRequests[id] << client;
}

void LanguageClientManager::reportFinished(const MessageId &id, Client *byClient)
{
    for (Client *client : managerInstance->m_exclusiveRequests[id]) {
        if (client != byClient)
            client->cancelRequest(id);
    }
    managerInstance->m_exclusiveRequests.remove(id);
}

void LanguageClientManager::deleteClient(Client *client)
{
    QTC_ASSERT(client, return);
    client->disconnect();
    managerInstance->m_clients.removeAll(client);
    if (managerInstance->m_shuttingDown)
        delete client;
    else
        client->deleteLater();
}

void LanguageClientManager::shutdown()
{
    if (managerInstance->m_shuttingDown)
        return;
    managerInstance->m_shuttingDown = true;
    for (auto interface : managerInstance->m_clients) {
        if (interface->reachable())
            interface->shutdown();
        else
            deleteClient(interface);
    }
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
    QTC_ASSERT(doc, return {};);
    return Utils::filtered(managerInstance->reachableClients(), [doc](Client *client) {
        return client->isSupportedDocument(doc);
    }).toList();
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
        QTimer::singleShot(restartTimeoutS * 1000, client, [client](){ startClient(client); });
    } else {
        if (unexpectedFinish && !m_shuttingDown)
            client->log(tr("Unexpectedly finished."), Core::MessageManager::Flash);
        deleteClient(client);
        if (m_shuttingDown && m_clients.isEmpty())
            emit shutdownFinished();
    }
}

void LanguageClientManager::editorOpened(Core::IEditor *iEditor)
{
    using namespace TextEditor;
    Core::IDocument *document = iEditor->document();
    for (Client *interface : reachableClients())
        interface->openDocument(document);

    if (BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(iEditor)) {
        if (TextEditorWidget *widget = editor->editorWidget()) {
            connect(widget, &TextEditorWidget::requestLinkAt, this,
                    [this, filePath = document->filePath()]
                    (const QTextCursor &cursor, Utils::ProcessLinkCallback &callback) {
                        findLinkAt(filePath, cursor, callback);
                    });
            connect(widget, &TextEditorWidget::requestUsages, this,
                    [this, filePath = document->filePath()]
                    (const QTextCursor &cursor) {
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
        }
    }
}

void LanguageClientManager::editorsClosed(const QList<Core::IEditor *> &editors)
{
    for (auto iEditor : editors) {
        if (auto editor = qobject_cast<TextEditor::BaseTextEditor *>(iEditor)) {
            const DidCloseTextDocumentParams params(TextDocumentIdentifier(
                    DocumentUri::fromFileName(editor->document()->filePath())));
            for (Client *interface : reachableClients())
                interface->closeDocument(params);
        }
    }
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

void LanguageClientManager::findLinkAt(const Utils::FileName &filePath,
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

void LanguageClientManager::findUsages(const Utils::FileName &filePath, const QTextCursor &cursor)
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
    for (Client *interface : reachableClients())
        interface->projectOpened(project);
}

void LanguageClientManager::projectRemoved(ProjectExplorer::Project *project)
{
    for (Client *interface : reachableClients())
        interface->projectClosed(project);
}

} // namespace LanguageClient
