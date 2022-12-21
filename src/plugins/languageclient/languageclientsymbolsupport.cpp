// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientsymbolsupport.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientutils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

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
        m_infoLabel.setText(tr("Search Again to update results and re-enable Replace"));
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
        m_renameFilesCheckBox.setText(tr("Re&name %n files", nullptr, filesToRename.size()));
        const auto filesForUser = Utils::transform<QStringList>(filesToRename,
                                                                [](const Utils::FilePath &fp) {
                                                                    return fp.toUserOutput();
                                                                });
        m_renameFilesCheckBox.setToolTip(tr("Files:\n%1").arg(filesForUser.join('\n')));
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

void SymbolSupport::findLinkAt(TextEditor::TextDocument *document,
                               const QTextCursor &cursor,
                               Utils::LinkHandler callback,
                               const bool resolveTarget)
{
    if (!m_client->reachable())
        return;
    GotoDefinitionRequest request(generateDocPosParams(document, cursor, m_client));
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
    request.setResponseCallback([callback, linkUnderCursor, client = m_client](
                                    const GotoDefinitionRequest::Response &response) {
        handleGotoDefinitionResponse(response, callback, linkUnderCursor, client);
    });

    sendTextDocumentPositionParamsRequest(m_client,
                                          request,
                                          m_client->dynamicCapabilities(),
                                          m_client->capabilities());
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
    const QMap<Utils::FilePath, QList<ItemData>> &rangesInDocument,
    Core::SearchResult *search = nullptr,
    bool limitToProjects = false)
{
    QList<Core::SearchResultItem> result;
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

        Core::SearchResultItem item;
        item.setFilePath(filePath);
        item.setUseTextEditorFont(true);
        if (renaming && limitToProjects) {
            const bool fileBelongsToProject = ProjectExplorer::SessionManager::projectForFile(
                filePath);
            item.setSelectForReplacement(fileBelongsToProject);
            if (fileBelongsToProject
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

QList<Core::SearchResultItem> generateSearchResultItems(
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
            tr("Find References with %1 for:").arg(m_client->name()), "", wordUnderCursor);
        search->addResults(generateSearchResultItems(*result, m_client->hostPathMapper()),
                           Core::SearchResult::AddOrdered);
        connect(search, &Core::SearchResult::activated, [](const Core::SearchResultItem &item) {
            Core::EditorManager::openEditorAtSearchResult(item);
        });
        search->finishSearch(false);
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

void SymbolSupport::renameSymbol(TextEditor::TextDocument *document,
                                 const QTextCursor &cursor,
                                 const QString &newSymbolName,
                                 bool preferLowerCaseFileNames)
{
    const TextDocumentPositionParams params = generateDocPosParams(document, cursor, m_client);
    QTextCursor tc = cursor;
    tc.select(QTextCursor::WordUnderCursor);
    const QString oldSymbolName = tc.selectedText();
    QString placeholder = newSymbolName;
    if (placeholder.isEmpty())
        placeholder = m_defaultSymbolMapper ? m_defaultSymbolMapper(oldSymbolName) : oldSymbolName;

    bool prepareSupported;
    if (!LanguageClient::supportsRename(m_client, document, prepareSupported)) {
        const QString error = tr("Renaming is not supported with %1").arg(m_client->name());
        createSearch(params, placeholder, {}, {})->finishSearch(true, error);
    } else if (prepareSupported) {
        requestPrepareRename(document,
                             generateDocPosParams(document, cursor, m_client),
                             placeholder,
                             oldSymbolName,
                             preferLowerCaseFileNames);
    } else {
        startRenameSymbol(generateDocPosParams(document, cursor, m_client),
                          placeholder,
                          oldSymbolName,
                          preferLowerCaseFileNames);
    }
}

void SymbolSupport::requestPrepareRename(TextEditor::TextDocument *document,
                                         const TextDocumentPositionParams &params,
                                         const QString &placeholder,
                                         const QString &oldSymbolName,
                                         bool preferLowerCaseFileNames)
{
    PrepareRenameRequest request(params);
    request.setResponseCallback([this,
                                 params,
                                 placeholder,
                                 oldSymbolName,
                                 preferLowerCaseFileNames,
                                 document = QPointer<TextEditor::TextDocument>(document)](
                                    const PrepareRenameRequest::Response &response) {
        const std::optional<PrepareRenameRequest::Response::Error> &error = response.error();
        if (error.has_value()) {
            m_client->log(*error);
            createSearch(params, placeholder, {}, {})->finishSearch(true, error->toString());
        }

        const std::optional<PrepareRenameResult> &result = response.result();
        if (result.has_value()) {
            if (std::holds_alternative<PlaceHolderResult>(*result)) {
                auto placeHolderResult = std::get<PlaceHolderResult>(*result);
                startRenameSymbol(params,
                                  placeHolderResult.placeHolder(),
                                  oldSymbolName,
                                  preferLowerCaseFileNames);
            } else if (std::holds_alternative<Range>(*result)) {
                auto range = std::get<Range>(*result);
                if (document) {
                    const int start = range.start().toPositionInDocument(document->document());
                    const int end = range.end().toPositionInDocument(document->document());
                    const QString reportedSymbolName = document->textAt(start, end - start);
                    const QString newPlaceholder = m_defaultSymbolMapper
                                                       ? m_defaultSymbolMapper(reportedSymbolName)
                                                       : reportedSymbolName;
                    startRenameSymbol(params,
                                      newPlaceholder,
                                      reportedSymbolName,
                                      preferLowerCaseFileNames);
                } else {
                    startRenameSymbol(params, placeholder, oldSymbolName, preferLowerCaseFileNames);
                }
            }
        }
    });
    m_client->sendMessage(request);
}

void SymbolSupport::requestRename(const TextDocumentPositionParams &positionParams,
                                  Core::SearchResult *search)
{
    RenameParams params(positionParams);
    params.setNewName(search->textToReplace());
    RenameRequest request(params);
    request.setResponseCallback([this, search](const RenameRequest::Response &response) {
        handleRenameResponse(search, response);
    });
    m_client->sendMessage(request);
    search->popup();
}

QList<Core::SearchResultItem> generateReplaceItems(const WorkspaceEdit &edits,
                                                   Core::SearchResult *search,
                                                   bool limitToProjects,
                                                   const DocumentUri::PathMapper &pathMapper)
{
    auto convertEdits = [](const QList<TextEdit> &edits) {
        return Utils::transform(edits, [](const TextEdit &edit) {
            return ItemData{SymbolSupport::convertRange(edit.range()), QVariant(edit)};
        });
    };
    QMap<Utils::FilePath, QList<ItemData>> rangesInDocument;
    auto documentChanges = edits.documentChanges().value_or(QList<TextDocumentEdit>());
    if (!documentChanges.isEmpty()) {
        for (const TextDocumentEdit &documentChange : std::as_const(documentChanges)) {
            rangesInDocument[documentChange.textDocument().uri().toFilePath(pathMapper)]
                = convertEdits(documentChange.edits());
        }
    } else {
        auto changes = edits.changes().value_or(WorkspaceEdit::Changes());
        for (auto it = changes.begin(), end = changes.end(); it != end; ++it)
            rangesInDocument[it.key().toFilePath(pathMapper)] = convertEdits(it.value());
    }
    return generateSearchResultItems(rangesInDocument, search, limitToProjects);
}

Core::SearchResult *SymbolSupport::createSearch(const TextDocumentPositionParams &positionParams,
                                                const QString &placeholder,
                                                const QString &oldSymbolName,
                                                bool preferLowerCaseFileNames)
{
    Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
        tr("Find References with %1 for:").arg(m_client->name()),
        "",
        placeholder,
        Core::SearchResultWindow::SearchAndReplace);
    search->setUserData(QVariantList{oldSymbolName, preferLowerCaseFileNames});
    const auto extraWidget = new ReplaceWidget;
    search->setAdditionalReplaceWidget(extraWidget);
    search->setTextToReplace(placeholder);

    connect(search, &Core::SearchResult::activated, [](const Core::SearchResultItem &item) {
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
              search->finishSearch(true, tr("%1 is not reachable anymore.").arg(clientName));
          });

    connect(search, &Core::SearchResult::replaceButtonClicked, this,
            [this, search, resetConnection](const QString & /*replaceText*/,
                                            const QList<Core::SearchResultItem> &checkedItems) {
                applyRename(checkedItems, search);
                disconnect(resetConnection);
            });

    return search;
}

void SymbolSupport::startRenameSymbol(const TextDocumentPositionParams &positionParams,
                                      const QString &placeholder,
                                      const QString &oldSymbolName,
                                      bool preferLowerCaseFileNames)
{
    requestRename(positionParams,
                  createSearch(positionParams, placeholder, oldSymbolName, preferLowerCaseFileNames));
}

void SymbolSupport::handleRenameResponse(Core::SearchResult *search,
                                         const RenameRequest::Response &response)
{
    const std::optional<PrepareRenameRequest::Response::Error> &error = response.error();
    QString errorMessage;
    if (error.has_value()) {
        m_client->log(*error);
        errorMessage = error->toString();
    }

    const std::optional<WorkspaceEdit> &edits = response.result();
    if (edits.has_value()) {
        search->addResults(generateReplaceItems(*edits,
                                                search,
                                                m_limitRenamingToProjects,
                                                m_client->hostPathMapper()),
                           Core::SearchResult::AddOrdered);
        qobject_cast<ReplaceWidget *>(search->additionalReplaceWidget())->showLabel(false);
        search->setReplaceEnabled(true);
        search->finishSearch(false);
    } else {
        search->finishSearch(error.has_value(), errorMessage);
    }
}

void SymbolSupport::applyRename(const QList<Core::SearchResultItem> &checkedItems,
                                Core::SearchResult *search)
{
    QSet<Utils::FilePath> affectedNonOpenFilePaths;
    QMap<Utils::FilePath, QList<TextEdit>> editsForDocuments;
    for (const Core::SearchResultItem &item : checkedItems) {
        const auto filePath = Utils::FilePath::fromString(item.path().value(0));
        if (!m_client->documentForFilePath(filePath))
            affectedNonOpenFilePaths << filePath;
        TextEdit edit(item.userData().toJsonObject());
        if (edit.isValid())
            editsForDocuments[filePath] << edit;
    }

    for (auto it = editsForDocuments.begin(), end = editsForDocuments.end(); it != end; ++it)
        applyTextEdits(m_client, it.key(), it.value());

    if (!affectedNonOpenFilePaths.isEmpty()) {
        Core::DocumentManager::notifyFilesChangedInternally(Utils::toList(affectedNonOpenFilePaths));
    }

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

Core::Search::TextRange SymbolSupport::convertRange(const Range &range)
{
    auto convertPosition = [](const Position &pos) {
        return Core::Search::TextPosition(pos.line() + 1, pos.character());
    };
    return Core::Search::TextRange(convertPosition(range.start()), convertPosition(range.end()));
}

void SymbolSupport::setDefaultRenamingSymbolMapper(const SymbolMapper &mapper)
{
    m_defaultSymbolMapper = mapper;
}

} // namespace LanguageClient

#include <languageclientsymbolsupport.moc>
