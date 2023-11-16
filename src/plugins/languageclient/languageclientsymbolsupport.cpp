// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientsymbolsupport.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientutils.h"
#include "languageclienttr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>

#include <QCheckBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>

using namespace LanguageServerProtocol;

namespace LanguageClient {

namespace {
class ReplaceWidget : public QWidget
{
    Q_OBJECT
public:
    ReplaceWidget()
    {
        m_infoLabel.setText(Tr::tr("Search Again to update results and re-enable Replace"));
        m_infoLabel.setVisible(false);
        m_renameFilesCheckBox.setVisible(false);
        const auto layout = new QHBoxLayout(this);
        layout->addWidget(&m_infoLabel);
        layout->addWidget(&m_renameFilesCheckBox);
    }

    void showLabel(bool show)
    {
        m_infoLabel.setVisible(show);
        if (show)
            updateCheckBox({});
    }

    void updateCheckBox(const Utils::FilePaths &filesToRename)
    {
        if (filesToRename.isEmpty()) {
            m_renameFilesCheckBox.hide();
            return;
        }
        m_renameFilesCheckBox.setText(Tr::tr("Re&name %n files", nullptr, filesToRename.size()));
        const auto filesForUser = Utils::transform<QStringList>(filesToRename,
                                                                [](const Utils::FilePath &fp) {
                                                                    return fp.toUserOutput();
                                                                });
        m_renameFilesCheckBox.setToolTip(Tr::tr("Files:\n%1").arg(filesForUser.join('\n')));
        m_renameFilesCheckBox.setVisible(true);
    }

    bool shouldRenameFiles() const { return m_renameFilesCheckBox.isChecked(); }

private:
    QLabel m_infoLabel;
    QCheckBox m_renameFilesCheckBox;
};
} // anonymous namespace

SymbolSupport::SymbolSupport(Client *client)
    : m_client(client)
{}

template<typename Request, typename R>
static MessageId sendTextDocumentPositionParamsRequest(Client *client,
                                                       const Request &request,
                                                       R ServerCapabilities::*member)
{
    if (!request.isValid(nullptr))
        return {};
    const DocumentUri uri = request.params().value().textDocument().uri();
    const bool supportedFile = client->isSupportedUri(uri);
    const DynamicCapabilities dynamicCapabilities = client->dynamicCapabilities();
    const ServerCapabilities serverCapability = client->capabilities();
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
        const auto provider = std::mem_fn(member)(serverCapability);
        sendMessage = provider.has_value();
        if (sendMessage && std::holds_alternative<bool>(*provider))
            sendMessage = std::get<bool>(*provider);
    }
    if (sendMessage) {
        client->sendMessage(request);
        return request.id();
    }
    return {};
}

template<typename Request>
static void handleGotoResponse(const typename Request::Response &response,
                               Utils::LinkHandler callback,
                               std::optional<Utils::Link> linkUnderCursor,
                               const Client *client)
{
    if (std::optional<GotoResult> result = response.result()) {
        if (std::holds_alternative<std::nullptr_t>(*result)) {
            callback({});
        } else if (auto ploc = std::get_if<Location>(&*result)) {
            callback(linkUnderCursor.value_or(ploc->toLink(client->hostPathMapper())));
        } else if (auto plloc = std::get_if<QList<Location>>(&*result)) {
            if (!plloc->isEmpty())
                callback(linkUnderCursor.value_or(plloc->value(0).toLink(client->hostPathMapper())));
            else
                callback({});
        }
    } else {
        callback({});
    }
}

static TextDocumentPositionParams generateDocPosParams(TextEditor::TextDocument *document,
                                                       const QTextCursor &cursor,
                                                       const Client *client)
{
    const DocumentUri uri = client->hostPathToServerUri(document->filePath());
    const TextDocumentIdentifier documentId(uri);
    const Position pos(cursor);
    return TextDocumentPositionParams(documentId, pos);
}

template<typename Request, typename R>
static MessageId sendGotoRequest(TextEditor::TextDocument *document,
                                 const QTextCursor &cursor,
                                 Utils::LinkHandler callback,
                                 Client *client,
                                 std::optional<Utils::Link> linkUnderCursor,
                                 R ServerCapabilities::*member)
{
    Request request(generateDocPosParams(document, cursor, client));
    request.setResponseCallback([callback, linkUnderCursor, client](
                                    const typename Request::Response &response) {
        handleGotoResponse<Request>(response, callback, linkUnderCursor, client);
    });
    return sendTextDocumentPositionParamsRequest(client, request, member);
    return request.id();
}

bool SymbolSupport::supportsFindLink(TextEditor::TextDocument *document, LinkTarget target) const
{
    const DocumentUri uri = m_client->hostPathToServerUri(document->filePath());
    const DynamicCapabilities dynamicCapabilities = m_client->dynamicCapabilities();
    const ServerCapabilities serverCapability = m_client->capabilities();
    QString methodName;
    std::optional<std::variant<bool, ServerCapabilities::RegistrationOptions>> provider;
    switch (target) {
    case LinkTarget::SymbolDef:
        methodName = GotoDefinitionRequest::methodName;
        provider = serverCapability.definitionProvider();
        break;
    case LinkTarget::SymbolTypeDef:
        methodName = GotoTypeDefinitionRequest::methodName;
        provider = serverCapability.typeDefinitionProvider();
        break;
    case LinkTarget::SymbolImplementation:
        methodName = GotoImplementationRequest::methodName;
        provider = serverCapability.implementationProvider();
        break;
    }
    if (methodName.isEmpty())
        return false;
    bool supported = dynamicCapabilities.isRegistered(methodName).value_or(false);
    if (supported) {
        const TextDocumentRegistrationOptions option(dynamicCapabilities.option(methodName));
        if (option.isValid())
            supported = option.filterApplies(
                Utils::FilePath::fromString(QUrl(uri).adjusted(QUrl::PreferLocalFile).toString()));
        else
            supported = m_client->isSupportedUri(uri);
    } else {
        supported = provider.has_value();
        if (supported && std::holds_alternative<bool>(*provider))
            supported = std::get<bool>(*provider);
    }
    return supported;
}

MessageId SymbolSupport::findLinkAt(TextEditor::TextDocument *document,
                                    const QTextCursor &cursor,
                                    Utils::LinkHandler callback,
                                    const bool resolveTarget,
                                    const LinkTarget target)
{
    if (!m_client->reachable())
        return {};
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

    const TextDocumentPositionParams params = generateDocPosParams(document, cursor, m_client);
    switch (target) {
    case LinkTarget::SymbolDef:
        return sendGotoRequest<GotoDefinitionRequest>(document,
                                                      cursor,
                                                      callback,
                                                      m_client,
                                                      linkUnderCursor,
                                                      &ServerCapabilities::definitionProvider);
    case LinkTarget::SymbolTypeDef:
        return sendGotoRequest<GotoTypeDefinitionRequest>(document,
                                                          cursor,
                                                          callback,
                                                          m_client,
                                                          linkUnderCursor,
                                                          &ServerCapabilities::typeDefinitionProvider);
    case LinkTarget::SymbolImplementation:
        return sendGotoRequest<GotoImplementationRequest>(document,
                                                          cursor,
                                                          callback,
                                                          m_client,
                                                          linkUnderCursor,
                                                          &ServerCapabilities::implementationProvider);
    }
    return {};
}

bool SymbolSupport::supportsFindUsages(TextEditor::TextDocument *document) const
{
    if (!m_client || !m_client->reachable())
        return false;
    if (m_client->dynamicCapabilities().isRegistered(FindReferencesRequest::methodName)) {
        QJsonObject options
            = m_client->dynamicCapabilities().option(FindReferencesRequest::methodName).toObject();
        const TextDocumentRegistrationOptions docOps(options);
        if (docOps.isValid()
            && !docOps.filterApplies(document->filePath(),
                                     Utils::mimeTypeForName(document->mimeType()))) {
            return false;
        }
    } else if (auto referencesProvider = m_client->capabilities().referencesProvider()) {
        if (std::holds_alternative<bool>(*referencesProvider)) {
            if (!std::get<bool>(*referencesProvider))
                return false;
        }
    } else {
        return false;
    }
    return true;
}

struct ItemData
{
    Utils::Text::Range range;
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

Utils::SearchResultItems generateSearchResultItems(
    const QMap<Utils::FilePath, QList<ItemData>> &rangesInDocument,
    Core::SearchResult *search = nullptr,
    bool limitToProjects = false)
{
    Utils::SearchResultItems result;
    const bool renaming = search && search->supportsReplace();
    QString oldSymbolName;
    QVariantList userData;
    if (renaming) {
        userData = search->userData().toList();
        oldSymbolName = userData.first().toString();
    }
    Utils::FilePaths fileRenameCandidates;
    for (auto it = rangesInDocument.begin(); it != rangesInDocument.end(); ++it) {
        const Utils::FilePath &filePath = it.key();

        Utils::SearchResultItem item;
        item.setFilePath(filePath);
        item.setUseTextEditorFont(true);
        if (renaming && limitToProjects) {
            const ProjectExplorer::Node * const node
                = ProjectExplorer::ProjectTree::nodeForFile(filePath);
            item.setSelectForReplacement(node && !node->isGenerated());
            if (node
                && filePath.baseName().compare(oldSymbolName, Qt::CaseInsensitive) == 0) {
                fileRenameCandidates << filePath;
            }
        }

        QStringList lines = SymbolSupport::getFileContents(filePath);
        for (const ItemData &data : it.value()) {
            item.setMainRange(data.range);
            if (data.range.begin.line > 0 && data.range.begin.line <= lines.size())
                item.setLineText(lines[data.range.begin.line - 1]);
            item.setUserData(data.userData);
            result << item;
        }
    }
    if (renaming) {
        userData.append(Utils::transform(fileRenameCandidates, &Utils::FilePath::toString));
        search->setUserData(userData);
        const auto extraWidget = qobject_cast<ReplaceWidget *>(search->additionalReplaceWidget());
        extraWidget->updateCheckBox(fileRenameCandidates);
    }
    return result;
}

Utils::SearchResultItems generateSearchResultItems(
    const LanguageClientArray<Location> &locations, const DocumentUri::PathMapper &pathMapper)
{
    if (locations.isNull())
        return {};
    QMap<Utils::FilePath, QList<ItemData>> rangesInDocument;
    for (const Location &location : locations.toList())
        rangesInDocument[location.uri().toFilePath(pathMapper)]
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
            Tr::tr("Find References with %1 for:").arg(m_client->name()), "", wordUnderCursor);
        search->addResults(generateSearchResultItems(*result, m_client->hostPathMapper()),
                           Core::SearchResult::AddOrdered);
        connect(search, &Core::SearchResult::activated, [](const Utils::SearchResultItem &item) {
            Core::EditorManager::openEditorAtSearchResult(item);
        });
        search->finishSearch(false);
        if (search->isInteractive())
            search->popup();
    }
}

std::optional<MessageId> SymbolSupport::findUsages(TextEditor::TextDocument *document,
                                                   const QTextCursor &cursor,
                                                   const ResultHandler &handler)
{
    if (!supportsFindUsages(document))
        return {};
    ReferenceParams params(generateDocPosParams(document, cursor, m_client));
    params.setContext(ReferenceParams::ReferenceContext(true));
    FindReferencesRequest request(params);
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    request.setResponseCallback([this, wordUnderCursor = termCursor.selectedText(), handler](
                                    const FindReferencesRequest::Response &response) {
        handleFindReferencesResponse(response, wordUnderCursor, handler);
    });

    sendTextDocumentPositionParamsRequest(m_client, request, &ServerCapabilities::referencesProvider);
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

void SymbolSupport::renameSymbol(TextEditor::TextDocument *document,
                                 const QTextCursor &cursor,
                                 const QString &newSymbolName,
                                 const std::function<void ()> &callback,
                                 bool preferLowerCaseFileNames)
{
    const TextDocumentPositionParams params = generateDocPosParams(document, cursor, m_client);
    QTextCursor tc = cursor;
    tc.select(QTextCursor::WordUnderCursor);
    const QString oldSymbolName = tc.selectedText();

    bool prepareSupported;
    if (!LanguageClient::supportsRename(m_client, document, prepareSupported)) {
        const QString error = Tr::tr("Renaming is not supported with %1").arg(m_client->name());
        createSearch(params, derivePlaceholder(oldSymbolName, newSymbolName),
                     {}, callback, {})->finishSearch(true, error);
    } else if (prepareSupported) {
        requestPrepareRename(document,
                             generateDocPosParams(document, cursor, m_client),
                             newSymbolName,
                             oldSymbolName,
                             callback,
                             preferLowerCaseFileNames);
    } else {
        startRenameSymbol(generateDocPosParams(document, cursor, m_client),
                          newSymbolName,
                          oldSymbolName,
                          callback,
                          preferLowerCaseFileNames);
    }
}

void SymbolSupport::requestPrepareRename(TextEditor::TextDocument *document,
                                         const TextDocumentPositionParams &params,
                                         const QString &placeholder,
                                         const QString &oldSymbolName,
                                         const std::function<void()> &callback,
                                         bool preferLowerCaseFileNames)
{
    PrepareRenameRequest request(params);
    request.setResponseCallback([this,
                                 params,
                                 placeholder,
                                 oldSymbolName,
                                 callback,
                                 preferLowerCaseFileNames,
                                 document = QPointer<TextEditor::TextDocument>(document)](
                                    const PrepareRenameRequest::Response &response) {
        const std::optional<PrepareRenameRequest::Response::Error> &error = response.error();
        if (error.has_value()) {
            m_client->log(*error);
            createSearch(params, placeholder, {}, callback, {})
                    ->finishSearch(true, error->toString());
        }

        const std::optional<PrepareRenameResult> &result = response.result();
        if (result.has_value()) {
            if (std::holds_alternative<PlaceHolderResult>(*result)) {
                auto placeHolderResult = std::get<PlaceHolderResult>(*result);
                startRenameSymbol(params,
                                  placeholder.isEmpty() ? placeHolderResult.placeHolder()
                                                        : placeholder,
                                  oldSymbolName,
                                  callback,
                                  preferLowerCaseFileNames);
            } else if (std::holds_alternative<Range>(*result)) {
                auto range = std::get<Range>(*result);
                if (document) {
                    const int start = range.start().toPositionInDocument(document->document());
                    const int end = range.end().toPositionInDocument(document->document());
                    const QString reportedSymbolName = document->textAt(start, end - start);
                    startRenameSymbol(params,
                                      derivePlaceholder(reportedSymbolName, placeholder),
                                      reportedSymbolName,
                                      callback,
                                      preferLowerCaseFileNames);
                } else {
                    startRenameSymbol(params, placeholder, oldSymbolName, callback,
                                      preferLowerCaseFileNames);
                }
            }
        }
    });
    m_client->sendMessage(request);
}

void SymbolSupport::requestRename(const TextDocumentPositionParams &positionParams,
                                  Core::SearchResult *search)
{
    if (m_renameRequestIds[search].isValid())
        m_client->cancelRequest(m_renameRequestIds[search]);
    RenameParams params(positionParams);
    params.setNewName(search->textToReplace());
    RenameRequest request(params);
    request.setResponseCallback([this, search](const RenameRequest::Response &response) {
        handleRenameResponse(search, response);
    });
    m_renameRequestIds[search] = request.id();
    m_client->sendMessage(request);
    if (search->isInteractive())
        search->popup();
}

Utils::SearchResultItems generateReplaceItems(const WorkspaceEdit &edits,
                                              Core::SearchResult *search,
                                              bool limitToProjects,
                                              const DocumentUri::PathMapper &pathMapper)
{
    Utils::SearchResultItems items;
    auto convertEdits = [](const QList<TextEdit> &edits) {
        return Utils::transform(edits, [](const TextEdit &edit) {
            return ItemData{SymbolSupport::convertRange(edit.range()), QVariant(edit)};
        });
    };
    QMap<Utils::FilePath, QList<ItemData>> rangesInDocument;
    auto documentChanges = edits.documentChanges().value_or(QList<DocumentChange>());
    if (!documentChanges.isEmpty()) {
        for (const DocumentChange &documentChange : std::as_const(documentChanges)) {
            if (std::holds_alternative<TextDocumentEdit>(documentChange)) {
                const TextDocumentEdit edit = std::get<TextDocumentEdit>(documentChange);
                rangesInDocument[edit.textDocument().uri().toFilePath(pathMapper)] = convertEdits(
                    edit.edits());
            } else {
                Utils::SearchResultItem item;

                if (std::holds_alternative<CreateFileOperation>(documentChange)) {
                    auto op = std::get<CreateFileOperation>(documentChange);
                    item.setLineText(op.message(pathMapper));
                    item.setFilePath(op.uri().toFilePath(pathMapper));
                    item.setUserData(QVariant(op));
                } else if (std::holds_alternative<RenameFileOperation>(documentChange)) {
                    auto op = std::get<RenameFileOperation>(documentChange);
                    item.setLineText(op.message(pathMapper));
                    item.setFilePath(op.oldUri().toFilePath(pathMapper));
                    item.setUserData(QVariant(op));
                } else if (std::holds_alternative<DeleteFileOperation>(documentChange)) {
                    auto op = std::get<DeleteFileOperation>(documentChange);
                    item.setLineText(op.message(pathMapper));
                    item.setFilePath(op.uri().toFilePath(pathMapper));
                    item.setUserData(QVariant(op));
                }

                items << item;
            }
        }
    } else {
        auto changes = edits.changes().value_or(WorkspaceEdit::Changes());
        for (auto it = changes.begin(), end = changes.end(); it != end; ++it)
            rangesInDocument[it.key().toFilePath(pathMapper)] = convertEdits(it.value());
    }
    items += generateSearchResultItems(rangesInDocument, search, limitToProjects);
    return items;
}

Core::SearchResult *SymbolSupport::createSearch(const TextDocumentPositionParams &positionParams,
                                                const QString &placeholder,
                                                const QString &oldSymbolName,
                                                const std::function<void()> &callback,
                                                bool preferLowerCaseFileNames)
{
    Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
        Tr::tr("Find References with %1 for:").arg(m_client->name()),
        "",
        placeholder,
        Core::SearchResultWindow::SearchAndReplace);
    search->setUserData(QVariantList{oldSymbolName, preferLowerCaseFileNames});
    const auto extraWidget = new ReplaceWidget;
    search->setAdditionalReplaceWidget(extraWidget);
    search->setTextToReplace(placeholder);
    if (callback)
        search->makeNonInteractive(callback);

    connect(search, &Core::SearchResult::activated, [](const Utils::SearchResultItem &item) {
        Core::EditorManager::openEditorAtSearchResult(item);
    });
    connect(search, &Core::SearchResult::replaceTextChanged, this, [this, search, positionParams]() {
        search->setUserData(search->userData().toList().first(2));
        search->setReplaceEnabled(false);
        search->restart();
        requestRename(positionParams, search);
    });

    auto resetConnection
        = connect(this, &QObject::destroyed, search, [search, clientName = m_client->name()]() {
              search->restart(); // clears potential current results
              search->finishSearch(true, Tr::tr("%1 is not reachable anymore.").arg(clientName));
          });

    connect(search, &Core::SearchResult::replaceButtonClicked, this,
            [this, search, resetConnection](const QString & /*replaceText*/,
                                            const Utils::SearchResultItems &checkedItems) {
                applyRename(checkedItems, search);
                disconnect(resetConnection);
            });

    return search;
}

void SymbolSupport::startRenameSymbol(const TextDocumentPositionParams &positionParams,
                                      const QString &placeholder,
                                      const QString &oldSymbolName,
                                      const std::function<void()> &callback,
                                      bool preferLowerCaseFileNames)
{
    requestRename(positionParams,
                  createSearch(positionParams, placeholder, oldSymbolName, callback,
                               preferLowerCaseFileNames));
}

void SymbolSupport::handleRenameResponse(Core::SearchResult *search,
                                         const RenameRequest::Response &response)
{
    m_renameRequestIds.remove(search);
    const std::optional<PrepareRenameRequest::Response::Error> &error = response.error();
    QString errorMessage;
    if (error.has_value()) {
        errorMessage = error->toString();
        if (errorMessage.contains("Cannot rename symbol: new name is the same as the old name"))
            errorMessage = Tr::tr("Start typing to see replacements."); // clangd optimization
        else
            m_client->log(*error);
    }

    const std::optional<WorkspaceEdit> &edits = response.result();
    if (edits.has_value()) {
        const Utils::SearchResultItems items = generateReplaceItems(
            *edits, search, m_limitRenamingToProjects, m_client->hostPathMapper());
        search->addResults(items, Core::SearchResult::AddOrdered);
        if (m_renameResultsEnhancer) {
            Utils::SearchResultItems additionalItems = m_renameResultsEnhancer(items);
            for (Utils::SearchResultItem &item : additionalItems) {
                TextEdit edit;
                const Utils::Text::Position startPos = item.mainRange().begin;
                const Utils::Text::Position endPos = item.mainRange().end;
                edit.setRange({{startPos.line - 1, startPos.column},
                               {endPos.line - 1, endPos.column}});
                edit.setNewText(search->textToReplace());
                item.setUserData(QVariant(edit));
            }
            search->addResults(additionalItems, Core::SearchResult::AddSortedByPosition);
        }
        qobject_cast<ReplaceWidget *>(search->additionalReplaceWidget())->showLabel(false);
        search->setReplaceEnabled(true);
        search->finishSearch(false);
    } else {
        search->finishSearch(error.has_value(), errorMessage);
    }
}

void SymbolSupport::applyRename(const Utils::SearchResultItems &checkedItems,
                                Core::SearchResult *search)
{
    QMap<Utils::FilePath, QList<TextEdit>> editsForDocuments;
    QList<DocumentChange> changes;
    for (const Utils::SearchResultItem &item : checkedItems) {
        const auto filePath = Utils::FilePath::fromUserInput(item.path().value(0));
        const QJsonObject jsonObject = item.userData().toJsonObject();
        if (const TextEdit edit(jsonObject); edit.isValid())
            editsForDocuments[filePath] << edit;
        else if (const CreateFileOperation createFile(jsonObject); createFile.isValid())
            changes << createFile;
        else if (const RenameFileOperation renameFile(jsonObject); renameFile.isValid())
            changes << renameFile;
        else if (const DeleteFileOperation deleteFile(jsonObject); deleteFile.isValid())
            changes << deleteFile;
    }

    for (const DocumentChange &change : changes)
        applyDocumentChange(m_client, change);

    for (auto it = editsForDocuments.begin(), end = editsForDocuments.end(); it != end; ++it)
        applyTextEdits(m_client, it.key(), it.value());

    const auto extraWidget = qobject_cast<ReplaceWidget *>(search->additionalReplaceWidget());
    QTC_ASSERT(extraWidget, return);
    if (!extraWidget->shouldRenameFiles())
        return;
    const QVariantList userData = search->userData().toList();
    QTC_ASSERT(userData.size() == 3, return);
    const Utils::FilePaths filesToRename = Utils::transform(userData.at(2).toStringList(),
                                                            [](const QString &f) {
                                                                return Utils::FilePath::fromString(
                                                                    f);
                                                            });
    ProjectExplorer::ProjectExplorerPlugin::renameFilesForSymbol(userData.at(0).toString(),
                                                                 search->textToReplace(),
                                                                 filesToRename,
                                                                 userData.at(1).toBool());
}

QString SymbolSupport::derivePlaceholder(const QString &oldSymbol, const QString &newSymbol)
{
    if (!newSymbol.isEmpty())
        return newSymbol;
    return m_defaultSymbolMapper ? m_defaultSymbolMapper(oldSymbol) : oldSymbol;
}

Utils::Text::Range SymbolSupport::convertRange(const Range &range)
{
    const auto convertPosition = [](const Position &pos) {
        return Utils::Text::Position{pos.line() + 1, pos.character()};
    };
    return {convertPosition(range.start()), convertPosition(range.end())};
}

void SymbolSupport::setDefaultRenamingSymbolMapper(const SymbolMapper &mapper)
{
    m_defaultSymbolMapper = mapper;
}

void SymbolSupport::setRenameResultsEnhancer(const RenameResultsEnhancer &enhancer)
{
    m_renameResultsEnhancer = enhancer;
}

} // namespace LanguageClient

#include <languageclientsymbolsupport.moc>
