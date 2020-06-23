/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "languageclientsymbolsupport.h"

#include "client.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <utils/mimetypes/mimedatabase.h>

#include <QFile>

using namespace LanguageServerProtocol;

namespace LanguageClient {

SymbolSupport::SymbolSupport(Client *client) : m_client(client)
{}

template<typename Request>
static void sendTextDocumentPositionParamsRequest(Client *client,
                                                  const Request &request,
                                                  const DynamicCapabilities &dynamicCapabilities,
                                                  const Utils::optional<bool> &serverCapability)
{
    if (!request.isValid(nullptr))
        return;
    const DocumentUri uri = request.params().value().textDocument().uri();
    const bool supportedFile = client->isSupportedUri(uri);
    bool sendMessage = dynamicCapabilities.isRegistered(Request::methodName).value_or(false);
    if (sendMessage) {
        const TextDocumentRegistrationOptions option(
            dynamicCapabilities.option(Request::methodName));
        if (option.isValid(nullptr))
            sendMessage = option.filterApplies(
                Utils::FilePath::fromString(QUrl(uri).adjusted(QUrl::PreferLocalFile).toString()));
        else
            sendMessage = supportedFile;
    } else {
        sendMessage = serverCapability.value_or(sendMessage) && supportedFile;
    }
    if (sendMessage)
        client->sendContent(request);
}

static void handleGotoDefinitionResponse(const GotoDefinitionRequest::Response &response,
                                         Utils::ProcessLinkCallback callback,
                                         Utils::optional<Utils::Link> linkUnderCursor)
{
    if (Utils::optional<GotoResult> _result = response.result()) {
        const GotoResult result = _result.value();
        if (Utils::holds_alternative<std::nullptr_t>(result))
            return;
        if (auto ploc = Utils::get_if<Location>(&result)) {
            callback(linkUnderCursor.value_or(ploc->toLink()));
        } else if (auto plloc = Utils::get_if<QList<Location>>(&result)) {
            if (!plloc->isEmpty())
                callback(linkUnderCursor.value_or(plloc->value(0).toLink()));
        }
    }
}

static TextDocumentPositionParams generateDocPosParams(TextEditor::TextDocument *document,
                                                       const QTextCursor &cursor)
{
    const DocumentUri uri = DocumentUri::fromFilePath(document->filePath());
    const TextDocumentIdentifier documentId(uri);
    const Position pos(cursor);
    return TextDocumentPositionParams(documentId, pos);
}

void SymbolSupport::findLinkAt(TextEditor::TextDocument *document,
                               const QTextCursor &cursor,
                               Utils::ProcessLinkCallback callback,
                               const bool resolveTarget)
{
    if (!m_client->reachable())
        return;
    GotoDefinitionRequest request(generateDocPosParams(document, cursor));
    Utils::optional<Utils::Link> linkUnderCursor;
    if (!resolveTarget) {
        QTextCursor linkCursor = cursor;
        linkCursor.select(QTextCursor::WordUnderCursor);
        Utils::Link link(document->filePath().toString(),
                         linkCursor.blockNumber() + 1,
                         linkCursor.positionInBlock());
        link.linkTextStart = linkCursor.selectionStart();
        link.linkTextEnd = linkCursor.selectionEnd();
        linkUnderCursor = link;
    }
    request.setResponseCallback(
        [callback, linkUnderCursor](const GotoDefinitionRequest::Response &response) {
            handleGotoDefinitionResponse(response, callback, linkUnderCursor);
        });

    sendTextDocumentPositionParamsRequest(m_client,
                                          request,
                                          m_client->dynamicCapabilities(),
                                          m_client->capabilities().referencesProvider());

}

Core::Search::TextRange convertRange(const Range &range)
{
    auto convertPosition = [](const Position &pos) {
        return Core::Search::TextPosition(pos.line() + 1, pos.character());
    };
    return Core::Search::TextRange(convertPosition(range.start()), convertPosition(range.end()));
}

struct ItemData
{
    Core::Search::TextRange range;
    QVariant userData;
};

QList<Core::SearchResultItem> generateSearchResultItems(
    const QMap<QString, QList<ItemData>> &rangesInDocument)
{
    QList<Core::SearchResultItem> result;
    for (auto it = rangesInDocument.begin(); it != rangesInDocument.end(); ++it) {
        const QString &fileName = it.key();
        QFile file(fileName);
        file.open(QFile::ReadOnly);

        Core::SearchResultItem item;
        item.path = QStringList() << fileName;
        item.useTextEditorFont = true;

        QStringList lines = QString::fromLocal8Bit(file.readAll()).split(QChar::LineFeed);
        for (const ItemData &data : it.value()) {
            item.mainRange = data.range;
            if (file.isOpen() && data.range.begin.line > 0 && data.range.begin.line <= lines.size())
                item.text = lines[data.range.begin.line - 1];
            else
                item.text.clear();
            item.userData = data.userData;
            result << item;
        }
    }
    return result;
}

QList<Core::SearchResultItem> generateSearchResultItems(
    const LanguageClientArray<Location> &locations)
{
    if (locations.isNull())
        return {};
    QMap<QString, QList<ItemData>> rangesInDocument;
    for (const Location &location : locations.toList())
        rangesInDocument[location.uri().toFilePath().toString()]
            << ItemData{convertRange(location.range()), {}};
    return generateSearchResultItems(rangesInDocument);
}

void SymbolSupport::handleFindReferencesResponse(const FindReferencesRequest::Response &response,
                                                 const QString &wordUnderCursor)
{
    if (auto result = response.result()) {
        Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
            tr("Find References with %1 for:").arg(m_client->name()), "", wordUnderCursor);
        search->addResults(generateSearchResultItems(result.value()),
                           Core::SearchResult::AddOrdered);
        QObject::connect(search,
                         &Core::SearchResult::activated,
                         [](const Core::SearchResultItem &item) {
                             Core::EditorManager::openEditorAtSearchResult(item);
                         });
        search->finishSearch(false);
        search->popup();
    }
}

void SymbolSupport::findUsages(TextEditor::TextDocument *document, const QTextCursor &cursor)
{
    if (!m_client->reachable())
        return;
    ReferenceParams params(generateDocPosParams(document, cursor));
    params.setContext(ReferenceParams::ReferenceContext(true));
    FindReferencesRequest request(params);
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    request.setResponseCallback([this, wordUnderCursor = termCursor.selectedText()](
                                    const FindReferencesRequest::Response &response) {
        handleFindReferencesResponse(response, wordUnderCursor);
    });

    sendTextDocumentPositionParamsRequest(m_client,
                                          request,
                                          m_client->dynamicCapabilities(),
                                          m_client->capabilities().referencesProvider());
}

static bool supportsRename(Client *client,
                           TextEditor::TextDocument *document,
                           bool &prepareSupported)
{
    if (!client->reachable())
        return false;
    prepareSupported = false;
    if (client->dynamicCapabilities().isRegistered(RenameRequest::methodName)) {
        QJsonObject options
            = client->dynamicCapabilities().option(RenameRequest::methodName).toObject();
        prepareSupported = ServerCapabilities::RenameOptions(options).prepareProvider().value_or(
            false);
        const TextDocumentRegistrationOptions docOps(options);
        if (docOps.isValid(nullptr)
            && !docOps.filterApplies(document->filePath(),
                                     Utils::mimeTypeForName(document->mimeType()))) {
            return false;
        }
    }
    if (auto renameProvider = client->capabilities().renameProvider()) {
        if (Utils::holds_alternative<bool>(*renameProvider)) {
            if (!Utils::get<bool>(*renameProvider))
                return false;
        } else if (Utils::holds_alternative<ServerCapabilities::RenameOptions>(*renameProvider)) {
            prepareSupported = Utils::get<ServerCapabilities::RenameOptions>(*renameProvider)
                                   .prepareProvider()
                                   .value_or(false);
        }
    } else {
        return false;
    }
    return true;
}

bool SymbolSupport::supportsRename(TextEditor::TextDocument *document)
{
    bool prepareSupported;
    return LanguageClient::supportsRename(m_client, document, prepareSupported);
}

void SymbolSupport::renameSymbol(TextEditor::TextDocument *document, const QTextCursor &cursor)
{
    bool prepareSupported;
    if (!LanguageClient::supportsRename(m_client, document, prepareSupported))
        return;

    QTextCursor tc = cursor;
    tc.select(QTextCursor::WordUnderCursor);
    if (prepareSupported)
        requestPrepareRename(generateDocPosParams(document, cursor), tc.selectedText());
    else
        startRenameSymbol(generateDocPosParams(document, cursor), tc.selectedText());
}

void SymbolSupport::requestPrepareRename(const TextDocumentPositionParams &params,
                                         const QString &placeholder)
{
    PrepareRenameRequest request(params);
    request.setResponseCallback([this, params, placeholder](
                                    const PrepareRenameRequest::Response &response) {
        const Utils::optional<PrepareRenameRequest::Response::Error> &error = response.error();
        if (error.has_value())
            m_client->log(*error);

        const Utils::optional<PrepareRenameResult> &result = response.result();
        if (result.has_value()) {
            if (Utils::holds_alternative<PlaceHolderResult>(*result)) {
                auto placeHolderResult = Utils::get<PlaceHolderResult>(*result);
                startRenameSymbol(params, placeHolderResult.placeHolder());
            } else if (Utils::holds_alternative<Range>(*result)) {
                auto range = Utils::get<Range>(*result);
                startRenameSymbol(params, placeholder);
            }
        }
    });
    m_client->sendContent(request);
}

void SymbolSupport::requestRename(const TextDocumentPositionParams &positionParams,
                                  const QString &newName,
                                  Core::SearchResult *search)
{
    RenameParams params(positionParams);
    params.setNewName(newName);
    RenameRequest request(params);
    request.setResponseCallback([this, search](const RenameRequest::Response &response) {
        handleRenameResponse(search, response);
    });
    m_client->sendContent(request);
    search->setTextToReplace(newName);
    search->popup();
}

QList<Core::SearchResultItem> generateReplaceItems(const WorkspaceEdit &edits)
{
    auto convertEdits = [](const QList<TextEdit> &edits) {
        return Utils::transform(edits, [](const TextEdit &edit) {
            return ItemData{convertRange(edit.range()), QVariant(edit)};
        });
    };
    QMap<QString, QList<ItemData>> rangesInDocument;
    auto documentChanges = edits.documentChanges().value_or(QList<TextDocumentEdit>());
    if (!documentChanges.isEmpty()) {
        for (const TextDocumentEdit &documentChange : qAsConst(documentChanges)) {
            rangesInDocument[documentChange.textDocument().uri().toFilePath().toString()] = convertEdits(
                documentChange.edits());
        }
    } else {
        auto changes = edits.changes().value_or(WorkspaceEdit::Changes());
        for (auto it = changes.begin(), end = changes.end(); it != end; ++it)
            rangesInDocument[it.key().toFilePath().toString()] = convertEdits(it.value());
    }
    return generateSearchResultItems(rangesInDocument);
}

void SymbolSupport::startRenameSymbol(const TextDocumentPositionParams &positionParams,
                                      const QString &placeholder)
{
    Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
        tr("Find References with %1 for:").arg(m_client->name()),
        "",
        placeholder,
        Core::SearchResultWindow::SearchAndReplace);
    search->setSearchAgainSupported(true);
    auto label = new QLabel(tr("Search Again to update results and re-enable Replace"));
    label->setVisible(false);
    search->setAdditionalReplaceWidget(label);
    QObject::connect(search, &Core::SearchResult::activated, [](const Core::SearchResultItem &item) {
        Core::EditorManager::openEditorAtSearchResult(item);
    });
    QObject::connect(search, &Core::SearchResult::replaceTextChanged, [search]() {
        search->additionalReplaceWidget()->setVisible(true);
        search->setSearchAgainEnabled(true);
        search->setReplaceEnabled(false);
    });
    QObject::connect(search,
                     &Core::SearchResult::searchAgainRequested,
                     [this, positionParams, search]() {
                         search->restart();
                         requestRename(positionParams, search->textToReplace(), search);
                     });
    QObject::connect(search,
                     &Core::SearchResult::replaceButtonClicked,
                     [this, positionParams](const QString & /*replaceText*/,
                                            const QList<Core::SearchResultItem> &checkedItems) {
                         applyRename(checkedItems);
                     });

    requestRename(positionParams, placeholder, search);
}

void SymbolSupport::handleRenameResponse(Core::SearchResult *search,
                                         const RenameRequest::Response &response)
{
    const Utils::optional<PrepareRenameRequest::Response::Error> &error = response.error();
    if (error.has_value())
        m_client->log(*error);

    const Utils::optional<WorkspaceEdit> &edits = response.result();
    if (edits.has_value()) {
        search->addResults(generateReplaceItems(*edits), Core::SearchResult::AddOrdered);
        search->additionalReplaceWidget()->setVisible(false);
        search->setReplaceEnabled(true);
        search->setSearchAgainEnabled(false);
        search->finishSearch(false);
    } else {
        search->finishSearch(true);
    }
}

void SymbolSupport::applyRename(const QList<Core::SearchResultItem> &checkedItems)
{
    QMap<DocumentUri, QList<TextEdit>> editsForDocuments;
    for (const Core::SearchResultItem &item : checkedItems) {
        auto uri = DocumentUri::fromFilePath(Utils::FilePath::fromString(item.path.value(0)));
        TextEdit edit(item.userData.toJsonObject());
        if (edit.isValid(nullptr))
            editsForDocuments[uri] << edit;
    }

    for (auto it = editsForDocuments.begin(), end = editsForDocuments.end(); it != end; ++it)
        applyTextEdits(it.key(), it.value());
}

} // namespace LanguageClient
