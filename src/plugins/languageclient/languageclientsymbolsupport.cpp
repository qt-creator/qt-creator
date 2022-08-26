// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "languageclientsymbolsupport.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <utils/mimeutils.h>

#include <QFile>
#include <QLabel>

using namespace LanguageServerProtocol;

namespace LanguageClient {

SymbolSupport::SymbolSupport(Client *client) : m_client(client)
{}

template<typename Request>
static void sendTextDocumentPositionParamsRequest(Client *client,
                                                  const Request &request,
                                                  const DynamicCapabilities &dynamicCapabilities,
                                                  const ServerCapabilities &serverCapability)
{
    if (!request.isValid(nullptr))
        return;
    const DocumentUri uri = request.params().value().textDocument().uri();
    const bool supportedFile = client->isSupportedUri(uri);
    bool sendMessage = dynamicCapabilities.isRegistered(Request::methodName).value_or(false);
    if (sendMessage) {
        const TextDocumentRegistrationOptions option(
            dynamicCapabilities.option(Request::methodName));
        if (option.isValid())
            sendMessage = option.filterApplies(
                Utils::FilePath::fromString(QUrl(uri).adjusted(QUrl::PreferLocalFile).toString()));
        else
            sendMessage = supportedFile;
    } else {
        const std::optional<std::variant<bool, WorkDoneProgressOptions>> &provider
            = serverCapability.referencesProvider();
        sendMessage = provider.has_value();
        if (sendMessage && std::holds_alternative<bool>(*provider))
            sendMessage = std::get<bool>(*provider);
    }
    if (sendMessage)
        client->sendMessage(request);
}

static void handleGotoDefinitionResponse(const GotoDefinitionRequest::Response &response,
                                         Utils::LinkHandler callback,
                                         std::optional<Utils::Link> linkUnderCursor)
{
    if (std::optional<GotoResult> result = response.result()) {
        if (std::holds_alternative<std::nullptr_t>(*result)) {
            callback({});
        } else if (auto ploc = std::get_if<Location>(&*result)) {
            callback(linkUnderCursor.value_or(ploc->toLink()));
        } else if (auto plloc = std::get_if<QList<Location>>(&*result)) {
            if (!plloc->isEmpty())
                callback(linkUnderCursor.value_or(plloc->value(0).toLink()));
            else
                callback({});
        }
    } else {
        callback({});
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
                               Utils::LinkHandler callback,
                               const bool resolveTarget)
{
    if (!m_client->reachable())
        return;
    GotoDefinitionRequest request(generateDocPosParams(document, cursor));
    std::optional<Utils::Link> linkUnderCursor;
    if (!resolveTarget) {
        QTextCursor linkCursor = cursor;
        linkCursor.select(QTextCursor::WordUnderCursor);
        Utils::Link link(document->filePath(),
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
                                          m_client->capabilities());

}

struct ItemData
{
    Core::Search::TextRange range;
    QVariant userData;
};

QStringList SymbolSupport::getFileContents(const Utils::FilePath &filePath)
{
    QString fileContent;
    if (TextEditor::TextDocument *document = TextEditor::TextDocument::textDocumentForFilePath(
            filePath)) {
        fileContent = document->plainText();
    } else {
        Utils::TextFileFormat format;
        format.lineTerminationMode = Utils::TextFileFormat::LFLineTerminator;
        QString error;
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        if (Utils::TextFileFormat::readFile(filePath, codec, &fileContent, &format, &error)
            != Utils::TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << error;
        }
    }
    return fileContent.split("\n");
}

QList<Core::SearchResultItem> generateSearchResultItems(
    const QMap<Utils::FilePath, QList<ItemData>> &rangesInDocument)
{
    QList<Core::SearchResultItem> result;
    for (auto it = rangesInDocument.begin(); it != rangesInDocument.end(); ++it) {
        const Utils::FilePath &filePath = it.key();

        Core::SearchResultItem item;
        item.setFilePath(filePath);
        item.setUseTextEditorFont(true);

        QStringList lines = SymbolSupport::getFileContents(filePath);
        for (const ItemData &data : it.value()) {
            item.setMainRange(data.range);
            if (data.range.begin.line > 0 && data.range.begin.line <= lines.size())
                item.setLineText(lines[data.range.begin.line - 1]);
            item.setUserData(data.userData);
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
    QMap<Utils::FilePath, QList<ItemData>> rangesInDocument;
    for (const Location &location : locations.toList())
        rangesInDocument[location.uri().toFilePath()]
            << ItemData{SymbolSupport::convertRange(location.range()), {}};
    return generateSearchResultItems(rangesInDocument);
}

void SymbolSupport::handleFindReferencesResponse(const FindReferencesRequest::Response &response,
                                                 const QString &wordUnderCursor,
                                                 const ResultHandler &handler)
{
    const auto result = response.result();
    if (handler) {
        const LanguageClientArray<Location> locations = result.value_or(nullptr);
        handler(locations.isNull() ? QList<Location>() : locations.toList());
        return;
    }
    if (result) {
        Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
            tr("Find References with %1 for:").arg(m_client->name()), "", wordUnderCursor);
        search->addResults(generateSearchResultItems(*result), Core::SearchResult::AddOrdered);
        QObject::connect(search,
                         &Core::SearchResult::activated,
                         [](const Core::SearchResultItem &item) {
                             Core::EditorManager::openEditorAtSearchResult(item);
                         });
        search->finishSearch(false);
        search->popup();
    }
}

std::optional<MessageId> SymbolSupport::findUsages(
        TextEditor::TextDocument *document, const QTextCursor &cursor, const ResultHandler &handler)
{
    if (!m_client->reachable())
        return {};
    ReferenceParams params(generateDocPosParams(document, cursor));
    params.setContext(ReferenceParams::ReferenceContext(true));
    FindReferencesRequest request(params);
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    request.setResponseCallback([this, wordUnderCursor = termCursor.selectedText(), handler](
                                const FindReferencesRequest::Response &response) {
        handleFindReferencesResponse(response, wordUnderCursor, handler);
    });

    sendTextDocumentPositionParamsRequest(m_client,
                                          request,
                                          m_client->dynamicCapabilities(),
                                          m_client->capabilities());
    return request.id();
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
        if (docOps.isValid()
            && !docOps.filterApplies(document->filePath(),
                                     Utils::mimeTypeForName(document->mimeType()))) {
            return false;
        }
    }
    if (auto renameProvider = client->capabilities().renameProvider()) {
        if (std::holds_alternative<bool>(*renameProvider)) {
            if (!std::get<bool>(*renameProvider))
                return false;
        } else if (std::holds_alternative<ServerCapabilities::RenameOptions>(*renameProvider)) {
            prepareSupported = std::get<ServerCapabilities::RenameOptions>(*renameProvider)
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
        const std::optional<PrepareRenameRequest::Response::Error> &error = response.error();
        if (error.has_value())
            m_client->log(*error);

        const std::optional<PrepareRenameResult> &result = response.result();
        if (result.has_value()) {
            if (std::holds_alternative<PlaceHolderResult>(*result)) {
                auto placeHolderResult = std::get<PlaceHolderResult>(*result);
                startRenameSymbol(params, placeHolderResult.placeHolder());
            } else if (std::holds_alternative<Range>(*result)) {
                auto range = std::get<Range>(*result);
                startRenameSymbol(params, placeholder);
            }
        }
    });
    m_client->sendMessage(request);
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
    m_client->sendMessage(request);
    search->setTextToReplace(newName);
    search->popup();
}

QList<Core::SearchResultItem> generateReplaceItems(const WorkspaceEdit &edits)
{
    auto convertEdits = [](const QList<TextEdit> &edits) {
        return Utils::transform(edits, [](const TextEdit &edit) {
            return ItemData{SymbolSupport::convertRange(edit.range()), QVariant(edit)};
        });
    };
    QMap<Utils::FilePath, QList<ItemData>> rangesInDocument;
    auto documentChanges = edits.documentChanges().value_or(QList<TextDocumentEdit>());
    if (!documentChanges.isEmpty()) {
        for (const TextDocumentEdit &documentChange : qAsConst(documentChanges)) {
            rangesInDocument[documentChange.textDocument().uri().toFilePath()] = convertEdits(
                documentChange.edits());
        }
    } else {
        auto changes = edits.changes().value_or(WorkspaceEdit::Changes());
        for (auto it = changes.begin(), end = changes.end(); it != end; ++it)
            rangesInDocument[it.key().toFilePath()] = convertEdits(it.value());
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
    const std::optional<PrepareRenameRequest::Response::Error> &error = response.error();
    if (error.has_value())
        m_client->log(*error);

    const std::optional<WorkspaceEdit> &edits = response.result();
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
        auto uri = DocumentUri::fromFilePath(Utils::FilePath::fromString(item.path().value(0)));
        TextEdit edit(item.userData().toJsonObject());
        if (edit.isValid())
            editsForDocuments[uri] << edit;
    }

    for (auto it = editsForDocuments.begin(), end = editsForDocuments.end(); it != end; ++it)
        applyTextEdits(m_client, it.key(), it.value());
}

Core::Search::TextRange SymbolSupport::convertRange(const Range &range)
{
    auto convertPosition = [](const Position &pos) {
        return Core::Search::TextPosition(pos.line() + 1, pos.character());
    };
    return Core::Search::TextRange(convertPosition(range.start()), convertPosition(range.end()));
}

} // namespace LanguageClient
