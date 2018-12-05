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

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <languageserverprotocol/messages.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>
#include <texteditor/textdocument.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QTimer>

using namespace LanguageServerProtocol;

namespace LanguageClient {

static LanguageClientManager *managerInstance = nullptr;

class LanguageClientMark : public TextEditor::TextMark
{
public:
    LanguageClientMark(const Utils::FileName &fileName, const Diagnostic &diag)
        : TextEditor::TextMark(fileName, diag.range().start().line() + 1, "lspmark")
    {
        using namespace Utils;
        setLineAnnotation(diag.message());
        setToolTip(diag.message());
        const bool isError
                = diag.severity().value_or(DiagnosticSeverity::Hint) == DiagnosticSeverity::Error;
        setColor(isError ? Theme::CodeModel_Error_TextMarkColor
                         : Theme::CodeModel_Warning_TextMarkColor);

        setIcon(isError ? Icons::CODEMODEL_ERROR.icon()
                        : Icons::CODEMODEL_WARNING.icon());
    }

    void removedFromEditor() override
    {
        LanguageClientManager::removeMark(this);
    }
};

LanguageClientManager::LanguageClientManager()
{
    JsonRpcMessageHandler::registerMessageProvider<PublishDiagnosticsNotification>();
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

void LanguageClientManager::publishDiagnostics(const Core::Id &id,
                                               const PublishDiagnosticsParams &params)
{
    const Utils::FileName filePath = params.uri().toFileName();
    auto doc = qobject_cast<TextEditor::TextDocument *>(
                    Core::DocumentModel::documentForFilePath(filePath.toString()));
    if (!doc)
        return;

    removeMarks(filePath, id);
    managerInstance->m_marks[filePath][id].reserve(params.diagnostics().size());
    for (const Diagnostic& diagnostic : params.diagnostics()) {
        auto mark = new LanguageClientMark(filePath, diagnostic);
        managerInstance->m_marks[filePath][id].append(mark);
        doc->addMark(mark);
    }
}

void LanguageClientManager::removeMark(LanguageClientMark *mark)
{
    for (auto &marks : managerInstance->m_marks[mark->fileName()])
        marks.removeAll(mark);
    delete mark;
}

void LanguageClientManager::removeMarks(const Utils::FileName &fileName)
{
    auto doc = qobject_cast<TextEditor::TextDocument *>(
                    Core::DocumentModel::documentForFilePath(fileName.toString()));
    if (!doc)
        return;

    for (auto marks : managerInstance->m_marks[fileName]) {
        for (TextEditor::TextMark *mark : marks) {
            doc->removeMark(mark);
            delete mark;
        }
    }
    managerInstance->m_marks[fileName].clear();
}

void LanguageClientManager::removeMarks(const Utils::FileName &fileName, const Core::Id &id)
{
    auto doc = qobject_cast<TextEditor::TextDocument *>(
                    Core::DocumentModel::documentForFilePath(fileName.toString()));
    if (!doc)
        return;

    for (TextEditor::TextMark *mark : managerInstance->m_marks[fileName][id]) {
        doc->removeMark(mark);
        delete mark;
    }
    managerInstance->m_marks[fileName][id].clear();
}

void LanguageClientManager::removeMarks(const Core::Id &id)
{
    for (const Utils::FileName &fileName : managerInstance->m_marks.keys())
        removeMarks(fileName, id);
}

void LanguageClientManager::startClient(BaseClient *client)
{
    QTC_ASSERT(client, return);
    if (managerInstance->m_shuttingDown) {
        managerInstance->clientFinished(client);
        return;
    }
    if (!managerInstance->m_clients.contains(client))
        managerInstance->m_clients.append(client);
    connect(client, &BaseClient::finished, managerInstance, [client](){
        managerInstance->clientFinished(client);
    });
    if (client->start())
        client->initialize();
    else
        managerInstance->clientFinished(client);
}

QVector<BaseClient *> LanguageClientManager::clients()
{
    return managerInstance->m_clients;
}

void LanguageClientManager::addExclusiveRequest(const MessageId &id, BaseClient *client)
{
    managerInstance->m_exclusiveRequests[id] << client;
}

void LanguageClientManager::reportFinished(const MessageId &id, BaseClient *byClient)
{
    for (BaseClient *client : managerInstance->m_exclusiveRequests[id]) {
        if (client != byClient)
            client->cancelRequest(id);
    }
    managerInstance->m_exclusiveRequests.remove(id);
}

void LanguageClientManager::deleteClient(BaseClient *client)
{
    QTC_ASSERT(client, return);
    client->disconnect();
    managerInstance->removeMarks(client->id());
    managerInstance->m_clients.removeAll(client);
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

QVector<BaseClient *> LanguageClientManager::reachableClients()
{
    return Utils::filtered(m_clients, &BaseClient::reachable);
}

static void sendToInterfaces(const IContent &content, const QVector<BaseClient *> &interfaces)
{
    for (BaseClient *interface : interfaces)
        interface->sendContent(content);
}

void LanguageClientManager::sendToAllReachableServers(const IContent &content)
{
    sendToInterfaces(content, reachableClients());
}

void LanguageClientManager::clientFinished(BaseClient *client)
{
    constexpr int restartTimeoutS = 5;
    const bool unexpectedFinish = client->state() != BaseClient::Shutdown
            && client->state() != BaseClient::ShutdownRequested;
    if (unexpectedFinish && !m_shuttingDown && client->reset()) {
        removeMarks(client->id());
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
    for (BaseClient *interface : reachableClients())
        interface->openDocument(document);

    if (auto textDocument = qobject_cast<TextDocument *>(document)) {
        if (BaseTextEditor *editor = BaseTextEditor::textEditorForDocument(textDocument)) {
            if (TextEditorWidget *widget = editor->editorWidget()) {
                connect(widget, &TextEditorWidget::requestLinkAt, this,
                        [this, filePath = document->filePath()]
                        (const QTextCursor &cursor, Utils::ProcessLinkCallback &callback){
                    findLinkAt(filePath, cursor, callback);
                });
            }
        }
    }
}

void LanguageClientManager::editorsClosed(const QList<Core::IEditor *> editors)
{
    for (auto iEditor : editors) {
        if (auto editor = qobject_cast<TextEditor::BaseTextEditor *>(iEditor)) {
            removeMarks(editor->document()->filePath());
            const DidCloseTextDocumentParams params(TextDocumentIdentifier(
                    DocumentUri::fromFileName(editor->document()->filePath())));
            for (BaseClient *interface : reachableClients())
                interface->closeDocument(params);
        }
    }
}

void LanguageClientManager::documentContentsSaved(Core::IDocument *document)
{
    for (BaseClient *interface : reachableClients())
        interface->documentContentsSaved(document);
}

void LanguageClientManager::documentWillSave(Core::IDocument *document)
{
    for (BaseClient *interface : reachableClients())
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
    request.setResponseCallback([callback](const Response<GotoResult, LanguageClientNull> &response){
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
    for (BaseClient *interface : reachableClients()) {
        if (interface->findLinkAt(request))
            m_exclusiveRequests[request.id()] << interface;
    }
}

void LanguageClientManager::projectAdded(ProjectExplorer::Project *project)
{
    for (BaseClient *interface : reachableClients())
        interface->projectOpened(project);
}

void LanguageClientManager::projectRemoved(ProjectExplorer::Project *project)
{
    for (BaseClient *interface : reachableClients())
        interface->projectClosed(project);
}

} // namespace LanguageClient
