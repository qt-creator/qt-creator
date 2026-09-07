// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientsymbolsupport.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientutils.h"

#include "languageclienttr.h"
#include <languageserverprotocol/lsputils.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <projectexplorer/buildconfiguration.h>
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
using namespace ProjectExplorer;
using namespace Utils;

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

/// Whether the server offers this capability, treating a plain false as absent.
template<typename Provider>
static bool providerEnabled(const std::optional<Provider> &provider)
{
    if (!provider)
        return false;
    const auto enabled = std::get_if<bool>(&*provider);
    return !enabled || *enabled;
}

/// Whether \a client answers this method for the document \a uri denotes.
template<typename M>
static bool handlesDocument(Client *client, const QString &uri, bool providerSupported)
{
    const DynamicCapabilities dynamicCapabilities = client->dynamicCapabilities();
    if (const std::optional<bool> registered = dynamicCapabilities.isRegistered(M::method)) {
        if (!*registered)
            return false;
        return registrationApplies(dynamicCapabilities.option(M::method),
                                   client->filePathFor(uri));
    }
    return providerSupported;
}

template<typename Params>
static Params positionParams(const DocumentPosition &position)
{
    return Params().textDocument(position.textDocument).position(position.position);
}

/// The link the first location of \a result points to.
static Utils::Link linkOf(const Definition &definition, const Client *client)
{
    if (const auto location = std::get_if<Location>(&definition))
        return linkFor(client, *location);
    const QList<Location> &locations = std::get<QList<Location>>(definition);
    return locations.isEmpty() ? Utils::Link() : linkFor(client, locations.first());
}

template<typename Result>
static void handleGotoResult(const Utils::Result<Result> &result,
                             const Utils::LinkHandler &callback,
                             const std::optional<Utils::Link> &linkUnderCursor,
                             const Client *client)
{
    if (!result) {
        callback({});
        return;
    }
    if (const auto definition = std::get_if<Definition>(&*result)) {
        const Utils::Link link = linkOf(*definition, client);
        callback(link.hasValidTarget() ? linkUnderCursor.value_or(link) : Utils::Link());
    } else if (const auto links = std::get_if<QList<DefinitionLink>>(&*result)) {
        if (links->isEmpty()) {
            callback({});
        } else {
            const DefinitionLink &target = links->first();
            callback(linkUnderCursor.value_or(
                Utils::Link(client->filePathFor(target.targetUri()),
                            target.targetSelectionRange().start().line() + 1,
                            target.targetSelectionRange().start().character())));
        }
    } else {
        callback({});
    }
}

static DocumentPosition documentPosition(TextEditor::TextDocument *document,
                                         const QTextCursor &cursor,
                                         const Client *client)
{
    return {TextDocumentIdentifier().uri(client->uriFor(document->filePath())), positionOf(cursor)};
}

template<typename M>
static MessageId sendGotoRequest(
    TextEditor::TextDocument *document,
    const QTextCursor &cursor,
    Utils::LinkHandler callback,
    Client *client,
    std::optional<Utils::Link> linkUnderCursor,
    bool providerSupported)
{
    const DocumentPosition position = documentPosition(document, cursor, client);
    if (!handlesDocument<M>(client, position.textDocument.uri(), providerSupported))
        return {};
    return client->sendRequest<M>(
        positionParams<typename M::Params>(position),
        [callback, linkUnderCursor, client](const Utils::Result<typename M::Result> &result) {
            handleGotoResult(result, callback, linkUnderCursor, client);
        });
}

bool SymbolSupport::supportsFindLink(TextEditor::TextDocument *document, LinkTarget target) const
{
    const DynamicCapabilities dynamicCapabilities = m_client->dynamicCapabilities();
    const ServerCapabilities &serverCapability = m_client->capabilities();
    QString methodName;
    // Whether the server announced the capability, and whether it announced it as
    // a plain false.
    bool hasProvider = false;
    bool disabled = false;
    const auto readProvider = [&](const auto &provider) {
        hasProvider = provider.has_value();
        if (hasProvider) {
            if (const auto enabled = std::get_if<bool>(&*provider))
                disabled = !*enabled;
        }
    };
    switch (target) {
    case LinkTarget::SymbolDef:
        methodName = DefinitionRequest::method;
        readProvider(serverCapability.definitionProvider());
        break;
    case LinkTarget::SymbolTypeDef:
        methodName = TypeDefinitionRequest::method;
        readProvider(serverCapability.typeDefinitionProvider());
        break;
    case LinkTarget::SymbolImplementation:
        methodName = ImplementationRequest::method;
        readProvider(serverCapability.implementationProvider());
        break;
    }
    if (methodName.isEmpty())
        return false;
    bool supported = dynamicCapabilities.isRegistered(methodName).value_or(false);
    if (supported) {
        supported = registrationApplies(dynamicCapabilities.option(methodName),
                                        document->filePath(),
                                        document->mimeType());
    } else {
        supported = hasProvider && !disabled;
    }
    return supported;
}

MessageId SymbolSupport::findLinkAt(
    TextEditor::TextDocument *document,
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

    switch (target) {
    case LinkTarget::SymbolDef:
        return sendGotoRequest<DefinitionRequest>(
            document,
            cursor,
            callback,
            m_client,
            linkUnderCursor,
            providerEnabled(m_client->capabilities().definitionProvider()));
    case LinkTarget::SymbolTypeDef:
        return sendGotoRequest<TypeDefinitionRequest>(
            document,
            cursor,
            callback,
            m_client,
            linkUnderCursor,
            providerEnabled(m_client->capabilities().typeDefinitionProvider()));
    case LinkTarget::SymbolImplementation:
        return sendGotoRequest<ImplementationRequest>(
            document,
            cursor,
            callback,
            m_client,
            linkUnderCursor,
            providerEnabled(m_client->capabilities().implementationProvider()));
    }
    return {};
}

bool SymbolSupport::supportsFindUsages(TextEditor::TextDocument *document) const
{
    if (!m_client || !m_client->reachable())
        return false;
    if (m_client->dynamicCapabilities().isRegistered(ReferencesRequest::method)) {
        if (!registrationApplies(
                m_client->dynamicCapabilities().option(ReferencesRequest::method),
                document->filePath(),
                document->mimeType())) {
            return false;
        }
    } else if (auto referencesProvider = m_client->capabilities().referencesProvider()) {
        if (const auto b = std::get_if<bool>(&*referencesProvider)) {
            if (!*b)
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
[[maybe_unused]] static bool operator==(const ItemData &id1, const ItemData &id2)
{
    return id1.range == id2.range && id1.userData == id2.userData;
}

QStringList SymbolSupport::getFileContents(const FilePath &filePath)
{
    QString fileContent;
    if (TextEditor::TextDocument *document = TextEditor::TextDocument::textDocumentForFilePath(
            filePath)) {
        fileContent = document->plainText();
    } else {
        TextFileFormat format;
        format.lineTerminationMode = TextFileFormat::LFLineTerminator;
        const TextEncoding encoding = Core::EditorManager::defaultTextEncoding();
        const TextFileFormat::ReadResult result = format.readFile(filePath, encoding);
        fileContent = result.content;
        if (result.code != TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << result.error;
        }
    }
    return fileContent.split("\n");
}

static Utils::SearchResultItems generateSearchResultItems(
    const QMap<Utils::FilePath, QList<ItemData>> &rangesInDocument,
    Client *client,
    Core::SearchResult *search,
    bool limitToProjects)
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
            const Node * const node = ProjectTree::nodeForFile(filePath);
            if (node) {
                item.setSelectForReplacement(!node->isGenerated());
            } else {
                item.setSelectForReplacement(
                    client->buildConfiguration()
                    && ProjectManager::isInProjectSourceDir(filePath, *client->project()));
            }
            if (item.selectForReplacement()
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
        userData.append(Utils::transform(fileRenameCandidates, &Utils::FilePath::toUrlishString));
        search->setUserData(userData);
        const auto extraWidget = qobject_cast<ReplaceWidget *>(search->additionalReplaceWidget());
        extraWidget->updateCheckBox(fileRenameCandidates);
    }
    return result;
}

using ItemDataPerPath = QMap<Utils::FilePath, QList<ItemData>>;
static void filterFileAliases(ItemDataPerPath &itemDataPerPath)
{
    QSet<Utils::FilePath> canonicalPaths;
    for (auto it = itemDataPerPath.begin(); it != itemDataPerPath.end(); ) {
        const Utils::FilePath canonicalPath = it.key().canonicalPath();
        if (!Utils::insert(canonicalPaths, canonicalPath)
            && it.value() == itemDataPerPath.value(canonicalPath)) { // QTCREATORBUG-30546
            it = itemDataPerPath.erase(it);
        } else {
            ++it;
        }
    }
}

static Utils::SearchResultItems generateSearchResultItems(
    const QList<Location> &locations, Client *client)
{
    ItemDataPerPath rangesInDocument;
    for (const Location &location : locations) {
        rangesInDocument[client->filePathFor(location.uri())]
            << ItemData{SymbolSupport::convertRange(location.range()), {}};
    }
    filterFileAliases(rangesInDocument);
    return generateSearchResultItems(rangesInDocument, client, nullptr, false);
}

void SymbolSupport::handleFindReferencesResult(const QJsonObject &response,
                                               const QString &wordUnderCursor,
                                               const ResultHandler &handler)
{
    const Utils::Result<ReferencesRequestResult> result
        = LanguageServerProtocol::result<ReferencesRequest>(response);
    const QList<Location> *locations = result ? std::get_if<QList<Location>>(&*result) : nullptr;
    if (handler) {
        handler(locations ? *locations : QList<Location>(), response.value("result"));
        return;
    }
    if (locations) {
        Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
            Tr::tr("Find References with %1 for:").arg(m_client->name()), "", wordUnderCursor);
        search->addResults(generateSearchResultItems(*locations, m_client),
                           Core::SearchResult::AddOrdered);
        connect(search, &Core::SearchResult::activated, [](const Utils::SearchResultItem &item) {
            Core::EditorManager::openEditorAtSearchResult(item);
        });
        search->finishSearch(false);
        if (search->isInteractive())
            search->popup();
    }
}

std::optional<MessageId> SymbolSupport::findUsages(
    TextEditor::TextDocument *document, const QTextCursor &cursor, const ResultHandler &handler)
{
    if (!supportsFindUsages(document))
        return {};
    const DocumentPosition position = documentPosition(document, cursor, m_client);
    if (!handlesDocument<ReferencesRequest>(
            m_client,
            position.textDocument.uri(),
            providerEnabled(m_client->capabilities().referencesProvider()))) {
        return {};
    }
    ReferenceParams params = positionParams<ReferenceParams>(position);
    params.context(ReferenceContext().includeDeclaration(true));
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    return m_client->sendRawRequest(
        toJson(params),
        ReferencesRequest::method,
        [this, wordUnderCursor = termCursor.selectedText(), handler](const QJsonObject &response) {
            handleFindReferencesResult(response, wordUnderCursor, handler);
        });
}

static bool supportsRename(Client *client,
                           TextEditor::TextDocument *document,
                           bool &prepareSupported)
{
    if (!client->reachable())
        return false;
    prepareSupported = false;
    if (client->dynamicCapabilities().isRegistered(RenameRequest::method)) {
        QJsonObject options = client->dynamicCapabilities().option(RenameRequest::method).toObject();
        prepareSupported = options.value("prepareProvider").toBool(false);
        if (!registrationApplies(options, document->filePath(), document->mimeType()))
            return false;
    }
    if (auto renameProvider = client->capabilities().renameProvider()) {
        if (const auto b = std::get_if<bool>(&*renameProvider)) {
            if (!*b)
                return false;
        } else if (const auto opt = std::get_if<RenameOptions>(&*renameProvider)) {
            prepareSupported = opt->prepareProvider().value_or(false);
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
    const DocumentPosition position = documentPosition(document, cursor, m_client);
    QTextCursor tc = cursor;
    tc.select(QTextCursor::WordUnderCursor);
    const QString oldSymbolName = tc.selectedText();

    bool prepareSupported;
    if (!LanguageClient::supportsRename(m_client, document, prepareSupported)) {
        const QString error = Tr::tr("Renaming is not supported with %1").arg(m_client->name());
        createSearch(position, derivePlaceholder(oldSymbolName, newSymbolName),
                     {}, callback, {})->finishSearch(true, error);
    } else if (prepareSupported) {
        requestPrepareRename(document,
                             position,
                             newSymbolName,
                             oldSymbolName,
                             callback,
                             preferLowerCaseFileNames);
    } else {
        startRenameSymbol(position,
                          newSymbolName,
                          oldSymbolName,
                          callback,
                          preferLowerCaseFileNames);
    }
}

void SymbolSupport::requestPrepareRename(TextEditor::TextDocument *document,
                                         const DocumentPosition &position,
                                         const QString &placeholder,
                                         const QString &oldSymbolName,
                                         const std::function<void()> &callback,
                                         bool preferLowerCaseFileNames)
{
    m_client->sendRequest<PrepareRenameRequest>(
        positionParams<PrepareRenameParams>(position),
        [this,
         position,
         placeholder,
         oldSymbolName,
         callback,
         preferLowerCaseFileNames,
         document = QPointer<TextEditor::TextDocument>(document)](
            const Utils::Result<PrepareRenameRequestResult> &result) {
            if (!result) {
                m_client->log(QtMsgType::QtCriticalMsg, result.error());
                createSearch(position, placeholder, {}, callback, {})
                    ->finishSearch(true, result.error());
                return;
            }

            const auto renameResult = std::get_if<PrepareRenameResult>(&*result);
            if (!renameResult) {
                startRenameSymbol(position, placeholder, oldSymbolName, callback,
                                  preferLowerCaseFileNames);
                return;
            }
            if (const auto placeHolder = std::get_if<PrepareRenamePlaceholder>(&*renameResult)) {
                startRenameSymbol(position,
                                  placeholder.isEmpty() ? placeHolder->placeholder() : placeholder,
                                  oldSymbolName,
                                  callback,
                                  preferLowerCaseFileNames);
            } else if (const auto range = std::get_if<Range>(&*renameResult)) {
                if (document) {
                    const int start = positionInDocument(range->start(), document->document());
                    const int end = positionInDocument(range->end(), document->document());
                    const QString reportedSymbolName = document->textAt(start, end - start);
                    startRenameSymbol(position,
                                      derivePlaceholder(reportedSymbolName, placeholder),
                                      reportedSymbolName,
                                      callback,
                                      preferLowerCaseFileNames);
                } else {
                    startRenameSymbol(position, placeholder, oldSymbolName, callback,
                                      preferLowerCaseFileNames);
                }
            } else {
                startRenameSymbol(position, placeholder, oldSymbolName, callback,
                                  preferLowerCaseFileNames);
            }
        });
}

void SymbolSupport::requestRename(const DocumentPosition &position, Core::SearchResult *search)
{
    if (m_renameRequestIds[search].isValid())
        m_client->cancelRequest(m_renameRequestIds[search]);
    RenameParams params = positionParams<RenameParams>(position);
    params.newName(search->textToReplace());
    m_renameRequestIds[search] = m_client->sendRequest<RenameRequest>(
        params, [this, search](const Utils::Result<RenameRequestResult> &result) {
            handleRenameResult(search, result);
        });
    if (search->isInteractive())
        search->popup();
}

/// The edits of a document edit, dropping the annotations Qt Creator does not
/// show.
static QList<TextEdit> textEdits(const TextDocumentEdit &documentEdit)
{
    QList<TextEdit> edits;
    for (const TextDocumentEditEditsItem &item : documentEdit.edits()) {
        if (const auto edit = std::get_if<TextEdit>(&item))
            edits << *edit;
        else if (const auto annotated = std::get_if<AnnotatedTextEdit>(&item))
            edits << TextEdit().range(annotated->range()).newText(annotated->newText());
    }
    return edits;
}

static Utils::SearchResultItems generateReplaceItems(
    const WorkspaceEdit &edits, Client *client, Core::SearchResult *search, bool limitToProjects)
{
    Utils::SearchResultItems items;
    auto convertEdits = [](const QList<TextEdit> &edits) {
        return Utils::transform(edits, [](const TextEdit &edit) {
            return ItemData{SymbolSupport::convertRange(edit.range()), QVariant(toJson(edit))};
        });
    };
    ItemDataPerPath rangesInDocument;
    const auto documentChanges = edits.documentChanges().value_or(
        QList<WorkspaceEditDocumentChangesItem>());
    if (!documentChanges.isEmpty()) {
        for (const WorkspaceEditDocumentChangesItem &documentChange : documentChanges) {
            if (const auto edit = std::get_if<TextDocumentEdit>(&documentChange)) {
                rangesInDocument[client->filePathFor(edit->textDocument().uri())]
                    = convertEdits(textEdits(*edit));
            } else {
                Utils::SearchResultItem item;

                if (const auto op = std::get_if<LanguageServerProtocol::CreateFile>(
                        &documentChange)) {
                    item.setLineText(Tr::tr("Create %1").arg(op->uri()));
                    item.setFilePath(client->filePathFor(op->uri()));
                    item.setUserData(QVariant(toJson(*op)));
                } else if (const auto op = std::get_if<RenameFile>(&documentChange)) {
                    item.setLineText(Tr::tr("Rename %1 to %2").arg(op->oldUri(), op->newUri()));
                    item.setFilePath(client->filePathFor(op->oldUri()));
                    item.setUserData(QVariant(toJson(*op)));
                } else if (
                    const auto op = std::get_if<LanguageServerProtocol::DeleteFile>(
                        &documentChange)) {
                    item.setLineText(Tr::tr("Delete %1").arg(op->uri()));
                    item.setFilePath(client->filePathFor(op->uri()));
                    item.setUserData(QVariant(toJson(*op)));
                }

                items << item;
            }
        }
    } else {
        const auto changes = edits.changes().value_or(QMap<QString, QList<TextEdit>>());
        for (auto it = changes.begin(), end = changes.end(); it != end; ++it)
            rangesInDocument[client->filePathFor(it.key())] = convertEdits(it.value());
    }
    filterFileAliases(rangesInDocument);
    items += generateSearchResultItems(rangesInDocument, client, search, limitToProjects);
    return items;
}

Core::SearchResult *SymbolSupport::createSearch(const DocumentPosition &position,
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
    connect(search, &Core::SearchResult::replaceTextChanged, this, [this, search, position]() {
        search->setUserData(search->userData().toList().first(2));
        search->setReplaceEnabled(false);
        search->restart();
        requestRename(position, search);
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

void SymbolSupport::startRenameSymbol(const DocumentPosition &position,
                                      const QString &placeholder,
                                      const QString &oldSymbolName,
                                      const std::function<void()> &callback,
                                      bool preferLowerCaseFileNames)
{
    requestRename(position,
                  createSearch(position, placeholder, oldSymbolName, callback,
                               preferLowerCaseFileNames));
}

void SymbolSupport::handleRenameResult(
    Core::SearchResult *search, const Utils::Result<RenameRequestResult> &result)
{
    m_renameRequestIds.remove(search);
    QString errorMessage;
    if (!result) {
        errorMessage = result.error();
        if (errorMessage.contains("Cannot rename symbol: new name is the same as the old name"))
            errorMessage = Tr::tr("Start typing to see replacements."); // clangd optimization
        else
            m_client->log(QtMsgType::QtCriticalMsg, errorMessage);
    }

    const WorkspaceEdit *edits = result ? std::get_if<WorkspaceEdit>(&*result) : nullptr;
    if (edits) {
        const Utils::SearchResultItems items = generateReplaceItems(
            *edits, m_client, search, m_limitRenamingToProjects);
        search->addResults(items, Core::SearchResult::AddOrdered);
        if (m_renameResultsEnhancer) {
            Utils::SearchResultItems additionalItems = m_renameResultsEnhancer(items);
            for (Utils::SearchResultItem &item : additionalItems) {
                const Utils::Text::Position startPos = item.mainRange().begin;
                const Utils::Text::Position endPos = item.mainRange().end;
                const TextEdit edit
                    = TextEdit()
                          .range(
                              Range()
                                  .start(
                                      Position().line(startPos.line - 1).character(startPos.column))
                                  .end(Position().line(endPos.line - 1).character(endPos.column)))
                          .newText(search->textToReplace());
                item.setUserData(QVariant(toJson(edit)));
            }
            search->addResults(additionalItems, Core::SearchResult::AddSortedByPosition);
        }
        qobject_cast<ReplaceWidget *>(search->additionalReplaceWidget())->showLabel(false);
        search->setReplaceEnabled(true);
        search->finishSearch(false);
    } else {
        search->finishSearch(!result, errorMessage);
    }
}

void SymbolSupport::applyRename(const Utils::SearchResultItems &checkedItems,
                                Core::SearchResult *search)
{
    QMap<Utils::FilePath, QList<TextEdit>> editsForDocuments;
    QList<WorkspaceEditDocumentChangesItem> changes;
    for (const Utils::SearchResultItem &item : checkedItems) {
        const auto filePath = Utils::FilePath::fromUserInput(item.path().value(0));
        const QJsonObject jsonObject = item.userData().toJsonObject();
        // A file operation names itself in its "kind"; everything else is an edit.
        const QString kind = jsonObject.value("kind").toString();
        if (kind.isEmpty()) {
            if (const Utils::Result<TextEdit> edit = fromJson<TextEdit>(jsonObject)) {
                editsForDocuments[filePath] << *edit;
            }
        } else if (kind == "create") {
            if (const Utils::Result<LanguageServerProtocol::CreateFile> op
                = fromJson<LanguageServerProtocol::CreateFile>(jsonObject)) {
                changes << *op;
            }
        } else if (kind == "rename") {
            if (const Utils::Result<RenameFile> op = fromJson<RenameFile>(jsonObject)) {
                changes << *op;
            }
        } else if (kind == "delete") {
            if (const Utils::Result<LanguageServerProtocol::DeleteFile> op
                = fromJson<LanguageServerProtocol::DeleteFile>(jsonObject)) {
                changes << *op;
            }
        }
    }

    for (const WorkspaceEditDocumentChangesItem &change : std::as_const(changes))
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
