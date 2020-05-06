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

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

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

QList<Core::SearchResultItem> generateSearchResultItems(
    const LanguageClientArray<Location> &locations)
{
    auto convertPosition = [](const Position &pos) {
        return Core::Search::TextPosition(pos.line() + 1, pos.character());
    };
    auto convertRange = [convertPosition](const Range &range) {
        return Core::Search::TextRange(convertPosition(range.start()), convertPosition(range.end()));
    };
    QList<Core::SearchResultItem> result;
    if (locations.isNull())
        return result;
    QMap<QString, QList<Core::Search::TextRange>> rangesInDocument;
    for (const Location &location : locations.toList())
        rangesInDocument[location.uri().toFilePath().toString()] << convertRange(location.range());
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

} // namespace LanguageClient
