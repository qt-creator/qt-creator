// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdfindreferences.h"

#include "clangcodemodeltr.h"
#include "clangdast.h"
#include "clangdclient.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <cplusplus/FindUsages.h>

#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppfindreferences.h>
#include <cppeditor/cpptoolsreuse.h>

#include <languageclient/languageclientsymbolsupport.h>
#include <languageserverprotocol/lsptypes.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/basefilefind.h>

#include <QCheckBox>
#include <QFile>
#include <QMap>
#include <QSet>

using namespace Core;
using namespace CppEditor;
using namespace CPlusPlus;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel::Internal {

class ReferencesFileData {
public:
    QList<QPair<Range, QString>> rangesAndLineText;
    QString fileContent;
    ClangdAstNode ast;
};

class ReplacementData {
public:
    QString oldSymbolName;
    QString newSymbolName;
    QSet<Utils::FilePath> fileRenameCandidates;
};

class ClangdFindReferences::CheckUnusedData
{
public:
    CheckUnusedData(ClangdFindReferences *q, const Link &link, SearchResult *search,
                    const LinkHandler &callback)
        : q(q), link(link), linkAsPosition(link.targetLine, link.targetColumn), search(search),
          callback(callback) {}
    ~CheckUnusedData();

    ClangdFindReferences * const q;
    const Link link;
    const Position linkAsPosition;
    const QPointer<SearchResult> search;
    const LinkHandler callback;
    SearchResultItems declDefItems;
    bool openedExtraFileForLink = false;
    bool declHasUsedTag = false;
    bool recursiveCallDetected = false;
    bool serverRestarted = false;
};

class ClangdFindReferences::Private
{
public:
    Private(ClangdFindReferences *q) : q(q) {}

    ClangdClient *client() const { return qobject_cast<ClangdClient *>(q->parent()); }
    static void handleRenameRequest(
            const SearchResult *search,
            const ReplacementData &replacementData,
            const QString &newSymbolName,
            const SearchResultItems &checkedItems,
            bool preserveCase,
            bool preferLowerCaseFileNames);
    void handleFindUsagesResult(const QList<Location> &locations);
    void finishSearch();
    void reportAllSearchResultsAndFinish();
    void addSearchResultsForFile(const FilePath &file, const ReferencesFileData &fileData);
    ClangdAstNode getContainingFunction(const ClangdAstPath &astPath, const Range& range);

    ClangdFindReferences * const q;
    QMap<DocumentUri, ReferencesFileData> fileData;
    QList<MessageId> pendingAstRequests;
    QPointer<SearchResult> search;
    std::optional<ReplacementData> replacementData;
    QString searchTerm;
    std::optional<CheckUnusedData> checkUnusedData;
    bool canceled = false;
    bool categorize = false;
};

ClangdFindReferences::ClangdFindReferences(ClangdClient *client, TextDocument *document,
        const QTextCursor &cursor, const QString &searchTerm,
        const std::optional<QString> &replacement, const std::function<void()> &callback,
        bool categorize)
    : QObject(client), d(new ClangdFindReferences::Private(this))
{
    d->categorize = categorize;
    d->searchTerm = searchTerm;
    if (replacement) {
        ReplacementData replacementData;
        replacementData.oldSymbolName = searchTerm;
        replacementData.newSymbolName = *replacement;
        if (replacementData.newSymbolName.isEmpty())
            replacementData.newSymbolName = replacementData.oldSymbolName;
        d->replacementData = replacementData;
    }

    d->search = SearchResultWindow::instance()->startNewSearch(
                Tr::tr("C++ Usages:"),
                {},
                searchTerm,
                replacement ? SearchResultWindow::SearchAndReplace : SearchResultWindow::SearchOnly,
                SearchResultWindow::PreserveCaseDisabled,
                "CppEditor");
    if (callback)
        d->search->makeNonInteractive(callback);
    if (categorize)
        d->search->setFilter(new CppSearchResultFilter);
    if (d->replacementData) {
        d->search->setTextToReplace(d->replacementData->newSymbolName);
        const auto renameFilesCheckBox = new QCheckBox;
        renameFilesCheckBox->setVisible(false);
        d->search->setAdditionalReplaceWidget(renameFilesCheckBox);
        const bool preferLowerCase = CppEditor::preferLowerCaseFileNames(client->project());
        const auto renameHandler = [search = d->search, preferLowerCase](
                                       const QString &newSymbolName,
                                       const SearchResultItems &checkedItems,
                                       bool preserveCase) {
            const auto replacementData = search->userData().value<ReplacementData>();
            Private::handleRenameRequest(search, replacementData, newSymbolName, checkedItems,
                                         preserveCase, preferLowerCase);
        };
        connect(d->search, &SearchResult::replaceButtonClicked, renameHandler);
    }
    connect(d->search, &SearchResult::activated, [](const SearchResultItem &item) {
        EditorManager::openEditorAtSearchResult(item);
    });
    if (d->search->isInteractive())
        SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);

    const std::optional<MessageId> requestId = client->symbolSupport().findUsages(
                document, cursor, [self = QPointer(this)](const QList<Location> &locations) {
        if (self)
            self->d->handleFindUsagesResult(locations);
    });

    if (!requestId) {
        d->finishSearch();
        return;
    }
    QObject::connect(d->search, &SearchResult::canceled, this, [this, requestId] {
        d->client()->cancelRequest(*requestId);
        d->canceled = true;
        d->search->disconnect(this);
        d->finishSearch();
    });

    connect(client, &ClangdClient::initialized, this, [this] {
        // On a client crash, report all search results found so far.
        d->reportAllSearchResultsAndFinish();
    });
}

ClangdFindReferences::ClangdFindReferences(ClangdClient *client, const Link &link,
                                           SearchResult *search, const LinkHandler &callback)
    : QObject(client), d(new Private(this))
{
    d->checkUnusedData.emplace(this, link, search, callback);
    d->categorize = true;
    d->search = search;

    if (!client->documentForFilePath(link.targetFilePath)) {
        QFile f(link.targetFilePath.toString());
        if (!f.open(QIODevice::ReadOnly)) {
            d->finishSearch();
            return;
        }
        const QString contents = QString::fromUtf8(f.readAll());
        QTextDocument doc(contents);
        QTextCursor cursor(&doc);
        cursor.setPosition(Text::positionInText(&doc, link.targetLine, link.targetColumn + 1));
        cursor.select(QTextCursor::WordUnderCursor);
        d->searchTerm = cursor.selectedText();
        client->openExtraFile(link.targetFilePath, contents);
        d->checkUnusedData->openedExtraFileForLink = true;
    }
    const TextDocumentIdentifier documentId(client->hostPathToServerUri(link.targetFilePath));
    const Position pos(link.targetLine - 1, link.targetColumn);
    ReferenceParams params(TextDocumentPositionParams(documentId, pos));
    params.setContext(ReferenceParams::ReferenceContext(true));
    FindReferencesRequest request(params);
    request.setResponseCallback([self = QPointer(this)]
                                (const FindReferencesRequest::Response &response) {
        if (self) {
            const LanguageClientArray<Location> locations = response.result().value_or(nullptr);
            self->d->handleFindUsagesResult(locations.isNull() ? QList<Location>()
                                                               : locations.toList());
        }
    });

    client->sendMessage(request, ClangdClient::SendDocUpdates::Ignore);
    QObject::connect(d->search, &SearchResult::canceled, this, [this, client, id = request.id()] {
        client->cancelRequest(id);
        d->canceled = true;
        d->finishSearch();
    });
    QObject::connect(d->search, &SearchResult::destroyed, this, [this, client, id = request.id()] {
        client->cancelRequest(id);
        d->canceled = true;
        d->finishSearch();
    });
    connect(client, &ClangdClient::initialized, this, [this] {
        d->checkUnusedData->serverRestarted = true;
        d->finishSearch();
    });
}

ClangdFindReferences::~ClangdFindReferences()
{
    delete d;
}

void ClangdFindReferences::Private::handleRenameRequest(
        const SearchResult *search,
        const ReplacementData &replacementData,
        const QString &newSymbolName,
        const SearchResultItems &checkedItems,
        bool preserveCase,
        bool preferLowerCaseFileNames)
{
    const Utils::FilePaths filePaths = BaseFileFind::replaceAll(newSymbolName, checkedItems,
                                                                preserveCase);
    if (!filePaths.isEmpty())
        SearchResultWindow::instance()->hide();

    const auto renameFilesCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
    QTC_ASSERT(renameFilesCheckBox, return);
    if (!renameFilesCheckBox->isChecked())
        return;

    ProjectExplorerPlugin::renameFilesForSymbol(
                replacementData.oldSymbolName, newSymbolName,
                Utils::toList(replacementData.fileRenameCandidates),
                preferLowerCaseFileNames);
}

void ClangdFindReferences::Private::handleFindUsagesResult(const QList<Location> &locations)
{
    if (!search || canceled) {
        finishSearch();
        return;
    }
    search->disconnect(q);

    qCDebug(clangdLog) << "found" << locations.size() << "locations";
    if (locations.isEmpty()) {
        finishSearch();
        return;
    }

    QObject::connect(search, &SearchResult::canceled, q, [this] {
        canceled = true;
        search->disconnect(q);
        for (const MessageId &id : std::as_const(pendingAstRequests))
            client()->cancelRequest(id);
        pendingAstRequests.clear();
        finishSearch();
    });

    for (const Location &loc : locations)
        fileData[loc.uri()].rangesAndLineText.push_back({loc.range(), {}});
    for (auto it = fileData.begin(); it != fileData.end();) {
        const Utils::FilePath filePath = client()->serverUriToHostPath(it.key());
        if (!filePath.exists()) { // https://github.com/clangd/clangd/issues/935
            it = fileData.erase(it);
            continue;
        }
        const QStringList lines = SymbolSupport::getFileContents(filePath);
        it->fileContent = lines.join('\n');
        for (auto &rangeWithText : it.value().rangesAndLineText) {
            const int lineNo = rangeWithText.first.start().line();
            if (lineNo >= 0 && lineNo < lines.size())
                rangeWithText.second = lines.at(lineNo);
        }
        ++it;
    }

    qCDebug(clangdLog) << "document count is" << fileData.size();
    if (replacementData || !categorize) {
        qCDebug(clangdLog) << "skipping AST retrieval";
        reportAllSearchResultsAndFinish();
        return;
    }

    for (auto it = fileData.begin(); it != fileData.end(); ++it) {
        const FilePath filePath = client()->serverUriToHostPath(it.key());
        const TextDocument * const doc = client()->documentForFilePath(filePath);
        const bool openExtraFile = !doc && (!checkUnusedData
                || !checkUnusedData->openedExtraFileForLink
                || checkUnusedData->link.targetFilePath != filePath);
        if (openExtraFile)
            client()->openExtraFile(filePath, it->fileContent);
        it->fileContent.clear();
        const auto docVariant = doc ? ClangdClient::TextDocOrFile(doc)
                                    : ClangdClient::TextDocOrFile(filePath);
        const auto astHandler = [sentinel = QPointer(q), this, loc = it.key(), filePath](
                const ClangdAstNode &ast, const MessageId &reqId) {
            qCDebug(clangdLog) << "AST for" << filePath;
            if (!sentinel)
                return;
            if (!search || canceled)
                return;
            ReferencesFileData &data = fileData[loc];
            data.ast = ast;
            pendingAstRequests.removeOne(reqId);
            qCDebug(clangdLog) << pendingAstRequests.size() << "AST requests still pending";
            addSearchResultsForFile(filePath, data);
            fileData.remove(loc);
            if (pendingAstRequests.isEmpty() && !canceled) {
                qCDebug(clangdLog) << "retrieved all ASTs";
                finishSearch();
            }
        };
        const MessageId reqId = client()->getAndHandleAst(
                    docVariant, astHandler, ClangdClient::AstCallbackMode::AlwaysAsync, {});
        pendingAstRequests << reqId;
        if (openExtraFile)
            client()->closeExtraFile(filePath);
    }
}

void ClangdFindReferences::Private::finishSearch()
{
    if (checkUnusedData) {
        q->deleteLater();
        return;
    }

    if (!client()->testingEnabled() && search) {
        search->finishSearch(canceled);
        search->disconnect(q);
        if (replacementData) {
            const auto renameCheckBox = qobject_cast<QCheckBox *>(
                        search->additionalReplaceWidget());
            QTC_CHECK(renameCheckBox);
            const QSet<Utils::FilePath> files = replacementData->fileRenameCandidates;
            renameCheckBox->setText(Tr::tr("Re&name %n files", nullptr, files.size()));
            const QStringList filesForUser = Utils::transform<QStringList>(files,
                        [](const Utils::FilePath &fp) { return fp.toUserOutput(); });
            renameCheckBox->setToolTip(Tr::tr("Files:\n%1").arg(filesForUser.join('\n')));
            renameCheckBox->setVisible(true);
            search->setUserData(QVariant::fromValue(*replacementData));
        }
    }
    emit q->done();
    q->deleteLater();
}

void ClangdFindReferences::Private::reportAllSearchResultsAndFinish()
{
    if (!checkUnusedData) {
        for (auto it = fileData.begin(); it != fileData.end(); ++it)
            addSearchResultsForFile(client()->serverUriToHostPath(it.key()), it.value());
    }
    finishSearch();
}

static Usage::Tags getUsageType(const ClangdAstPath &path, const QString &searchTerm,
                                const QStringList &expectedDeclTypes);

void ClangdFindReferences::Private::addSearchResultsForFile(const FilePath &file,
                                                            const ReferencesFileData &fileData)
{
    SearchResultItems items;
    qCDebug(clangdLog) << file << "has valid AST:" << fileData.ast.isValid();
    const auto expectedDeclTypes = [this]() -> QStringList {
        if (checkUnusedData)
            return {"Function", "CXXMethod"};
        return {};
    }();
    for (const auto &rangeWithText : fileData.rangesAndLineText) {
        const Range &range = rangeWithText.first;
        const ClangdAstPath astPath = getAstPath(fileData.ast, range);
        const Usage::Tags usageType = fileData.ast.isValid()
                ? getUsageType(astPath, searchTerm, expectedDeclTypes)
                : Usage::Tags();
        if (checkUnusedData) {
            bool isProperUsage = false;
            if (usageType.testFlag(Usage::Tag::Declaration)) {
                checkUnusedData->declHasUsedTag = checkUnusedData->declHasUsedTag
                        || usageType.testFlag(Usage::Tag::Used);
                isProperUsage = usageType.testAnyFlags({
                        Usage::Tag::Override, Usage::Tag::MocInvokable,
                        Usage::Tag::ConstructorDestructor, Usage::Tag::Template,
                        Usage::Tag::Operator});
            } else {
                bool isRecursiveCall = false;
                if (checkUnusedData->link.targetFilePath == file) {
                    const ClangdAstNode containingFunction = getContainingFunction(astPath, range);
                    isRecursiveCall = containingFunction.hasRange()
                            && containingFunction.range().contains(checkUnusedData->linkAsPosition);
                }
                checkUnusedData->recursiveCallDetected = checkUnusedData->recursiveCallDetected
                        || isRecursiveCall;
                isProperUsage = !isRecursiveCall;
            }
            if (isProperUsage) {
                qCDebug(clangdLog) << "proper usage at" << rangeWithText.second;
                canceled = true;
                finishSearch();
                return;
            }
        }
        SearchResultItem item;
        item.setUserData(usageType.toInt());
        item.setStyle(CppEditor::colorStyleForUsageType(usageType));
        item.setFilePath(file);
        item.setMainRange(SymbolSupport::convertRange(range));
        item.setUseTextEditorFont(true);
        item.setLineText(rangeWithText.second);
        if (checkUnusedData) {
            if (rangeWithText.second.contains("template<>")) {
                // Hack: Function specializations are not detectable in the AST.
                canceled = true;
                finishSearch();
                return;
            }
            qCDebug(clangdLog) << "collecting decl/def" << rangeWithText.second;
            checkUnusedData->declDefItems << item;
            continue;
        }
        item.setContainingFunctionName(getContainingFunction(astPath, range).detail());

        if (search->supportsReplace()) {
            const Node * const node = ProjectTree::nodeForFile(file);
            item.setSelectForReplacement(!ProjectManager::hasProjects()
                                         || (node && !node->isGenerated()));
            if (node && file.baseName().compare(replacementData->oldSymbolName,
                                                         Qt::CaseInsensitive) == 0) {
                replacementData->fileRenameCandidates << file;
            }
        }
        items << item;
    }
    if (client()->testingEnabled())
        emit q->foundReferences(items);
    else if (!checkUnusedData)
        search->addResults(items, SearchResult::AddOrdered);
}

ClangdAstNode ClangdFindReferences::Private::getContainingFunction(
        const ClangdAstPath &astPath, const Range& range)
{
    const ClangdAstNode* containingFuncNode{nullptr};
    const ClangdAstNode* lastCompoundStmtNode{nullptr};

    for (auto it = astPath.crbegin(); it != astPath.crend(); ++it) {
        if (it->arcanaContains("CompoundStmt"))
            lastCompoundStmtNode = &*it;

        if (it->isFunction()) {
            if (lastCompoundStmtNode && lastCompoundStmtNode->hasRange()
                && lastCompoundStmtNode->range().contains(range)) {
                containingFuncNode = &*it;
                break;
            }
        }
    }

    if (!containingFuncNode || !containingFuncNode->isValid())
        return {};

    return *containingFuncNode;
}

static Usage::Tags getUsageType(const ClangdAstPath &path, const QString &searchTerm,
                                const QStringList &expectedDeclTypes)
{
    bool potentialWrite = false;
    bool isFunction = false;
    const bool symbolIsDataType = path.last().role() == "type" && path.last().kind() == "Record";
    QString invokedConstructor;
    if (path.last().role() == "expression" && path.last().kind() == "CXXConstruct")
        invokedConstructor = path.last().detail().value_or(QString());

    // Sometimes (TM), it can happen that none of the AST nodes have a range,
    // so path construction fails.
    if (path.last().kind() == "TranslationUnit")
        return Usage::Tag::Used;

    const auto isPotentialWrite = [&] { return potentialWrite && !isFunction; };
    const auto isSomeSortOfTemplate = [&](auto declPathIt) {
        if (declPathIt->kind() == "Function") {
            const auto children = declPathIt->children().value_or(QList<ClangdAstNode>());
            for (const ClangdAstNode &child : children) {
                if (child.role() == "template argument")
                    return true;
            }
        }
        for (; declPathIt != path.rend(); ++declPathIt) {
            if (declPathIt->kind() == "FunctionTemplate" || declPathIt->kind() == "ClassTemplate"
                    || declPathIt->kind() == "ClassTemplatePartialSpecialization") {
                return true;
            }
        }
        return false;
    };
    for (auto pathIt = path.rbegin(); pathIt != path.rend(); ++pathIt) {
        if (pathIt->arcanaContains("non_odr_use_unevaluated"))
            return {};
        if (pathIt->kind() == "CXXDelete")
            return Usage::Tag::Write;
        if (pathIt->kind() == "CXXNew")
            return {};
        if (pathIt->kind() == "Switch" || pathIt->kind() == "If")
            return Usage::Tag::Read;
        if (pathIt->kind() == "Call")
            return isFunction ? Usage::Tags()
                              : potentialWrite ? Usage::Tag::WritableRef : Usage::Tag::Read;
        if (pathIt->kind() == "CXXMemberCall") {
            const auto children = pathIt->children();
            if (children && children->size() == 1
                    && children->first() == path.last()
                    && children->first().arcanaContains("bound member function")) {
                return {};
            }
            return isPotentialWrite() ? Usage::Tag::WritableRef : Usage::Tag::Read;
        }
        if ((pathIt->kind() == "DeclRef" || pathIt->kind() == "Member")
                && pathIt->arcanaContains("lvalue")) {
            if (pathIt->arcanaContains(" Function "))
                isFunction = true;
            else
                potentialWrite = true;
        }
        if (pathIt->role() == "declaration") {
            if (symbolIsDataType)
                return {};
            if (!expectedDeclTypes.isEmpty() && !expectedDeclTypes.contains(pathIt->kind()))
                return {};
            if (!invokedConstructor.isEmpty() && invokedConstructor == searchTerm)
                return Usage::Tag::ConstructorDestructor;
            if (pathIt->arcanaContains("cinit")) {
                if (pathIt == path.rbegin())
                    return {Usage::Tag::Declaration, Usage::Tag::Write};
                if (pathIt->childContainsRange(0, path.last().range()))
                    return {Usage::Tag::Declaration, Usage::Tag::Write};
                if (isFunction)
                    return Usage::Tag::Read;
                if (!pathIt->hasConstType())
                    return Usage::Tag::WritableRef;
                return Usage::Tag::Read;
            }
            Usage::Tags tags = Usage::Tag::Declaration;
            if (pathIt->arcanaContains(" used ") || pathIt->arcanaContains(" referenced "))
                tags |= Usage::Tag::Used;
            if (pathIt->kind() == "CXXConstructor" || pathIt->kind() == "CXXDestructor")
                tags |= Usage::Tag::ConstructorDestructor;
            const auto children = pathIt->children().value_or(QList<ClangdAstNode>());
            for (const ClangdAstNode &child : children) {
                if (child.role() == "attribute") {
                    if (child.kind() == "Override" || child.kind() == "Final")
                        tags |= Usage::Tag::Override;
                    else if (child.kind() == "Annotate" && child.arcanaContains("qt_"))
                        tags |= Usage::Tag::MocInvokable;
                }
            }
            if (isSomeSortOfTemplate(pathIt))
                tags |= Usage::Tag::Template;
            if (pathIt->kind() == "Function" || pathIt->kind() == "CXXMethod") {
                const QString detail = pathIt->detail().value_or(QString());
                static const QString opString(QLatin1String("operator"));
                if (detail.size() > opString.size() && detail.startsWith(opString)
                        && !detail.at(opString.size()).isLetterOrNumber()
                        && detail.at(opString.size()) != '_') {
                    tags |= Usage::Tag::Operator;
                }
            }
            if (pathIt->kind() == "CXXMethod") {
                const ClangdAstNode &classNode = *std::next(pathIt);
                if (classNode.hasChild([&](const ClangdAstNode &n) {
                    if (n.kind() != "StaticAssert")
                        return false;
                    return n.hasChild([&](const ClangdAstNode &n) {
                        return n.arcanaContains("Q_PROPERTY"); }, true)
                           && n.hasChild([&](const ClangdAstNode &n) {
                                  return n.arcanaContains(" " + searchTerm); }, true);
                    }, false)) {
                    tags |= Usage::Tag::MocInvokable;
                }
            }
            return tags;
        }
        if (pathIt->kind() == "MemberInitializer")
            return pathIt == path.rbegin() ? Usage::Tag::Write : Usage::Tag::Read;
        if (pathIt->kind() == "UnaryOperator"
                && (pathIt->detailIs("++") || pathIt->detailIs("--"))) {
            return Usage::Tag::Write;
        }

        // LLVM uses BinaryOperator only for built-in types; for classes, CXXOperatorCall
        // is used. The latter has an additional node at index 0, so the left-hand side
        // of an assignment is at index 1.
        const bool isBinaryOp = pathIt->kind() == "BinaryOperator";
        const bool isOpCall = pathIt->kind() == "CXXOperatorCall";
        if (isBinaryOp || isOpCall) {
            if (isOpCall && symbolIsDataType) { // Constructor invocation.
                if (searchTerm == invokedConstructor)
                    return Usage::Tag::ConstructorDestructor;
                return {};}

            const QString op = pathIt->operatorString();
            if (op.endsWith("=") && op != "==") { // Assignment.
                const int lhsIndex = isBinaryOp ? 0 : 1;
                if (pathIt->childContainsRange(lhsIndex, path.last().range()))
                    return Usage::Tag::Write;
                return isPotentialWrite() ? Usage::Tag::WritableRef : Usage::Tag::Read;
            }
            return Usage::Tag::Read;
        }

        if (pathIt->kind() == "ImplicitCast") {
            if (pathIt->detailIs("FunctionToPointerDecay"))
                return {};
            if (pathIt->hasConstType())
                return Usage::Tag::Read;
            potentialWrite = true;
            continue;
        }
    }

    return {};
}

ClangdFindReferences::CheckUnusedData::~CheckUnusedData()
{
    if (!serverRestarted) {
        if (openedExtraFileForLink && q->d->client() && q->d->client()->reachable()
                && !q->d->client()->documentForFilePath(link.targetFilePath)) {
            q->d->client()->closeExtraFile(link.targetFilePath);
        }
        if (!q->d->canceled && (!declHasUsedTag || recursiveCallDetected) && QTC_GUARD(search))
            search->addResults(declDefItems, SearchResult::AddOrdered);
    }
    callback(link);
}

class ClangdFindLocalReferences::Private
{
public:
    Private(ClangdFindLocalReferences *q, CppEditorWidget *editorWidget, const QTextCursor &cursor,
            const RenameCallback &callback)
        : q(q), editorWidget(editorWidget), document(editorWidget->textDocument()), cursor(cursor),
          callback(callback), uri(client()->hostPathToServerUri(document->filePath())),
          revision(document->document()->revision())
    {}

    ClangdClient *client() const { return qobject_cast<ClangdClient *>(q->parent()); }
    void findDefinition();
    void getDefinitionAst(const Link &link);
    void checkDefinitionAst(const ClangdAstNode &ast);
    void handleReferences(const QList<Location> &references);
    void finish();

    ClangdFindLocalReferences * const q;
    const QPointer<CppEditorWidget> editorWidget;
    const QPointer<TextDocument> document;
    const QTextCursor cursor;
    RenameCallback callback;
    const DocumentUri uri;
    const int revision;
    Link defLink;
};

ClangdFindLocalReferences::ClangdFindLocalReferences(
    ClangdClient *client, CppEditorWidget *editorWidget, const QTextCursor &cursor,
    const RenameCallback &callback)
    : QObject(client), d(new Private(this, editorWidget, cursor, callback))
{
    d->findDefinition();
}

ClangdFindLocalReferences::~ClangdFindLocalReferences()
{
    delete d;
}

void ClangdFindLocalReferences::Private::findDefinition()
{
    const auto linkHandler = [sentinel = QPointer(q), this](const Link &l) {
        if (sentinel)
            getDefinitionAst(l);
    };
    client()->symbolSupport().findLinkAt(document,
                                         cursor,
                                         linkHandler,
                                         true,
                                         LanguageClient::LinkTarget::SymbolDef);
}

void ClangdFindLocalReferences::Private::getDefinitionAst(const Link &link)
{
    qCDebug(clangdLog) << "received go to definition response" << link.targetFilePath
                       << link.targetLine << (link.targetColumn + 1);

    if (!link.hasValidTarget() || !document
            || link.targetFilePath.canonicalPath() != document->filePath().canonicalPath()) {
        finish();
        return;
    }

    defLink = link;
    qCDebug(clangdLog) << "sending ast request for link";
    const auto astHandler = [sentinel = QPointer(q), this]
            (const ClangdAstNode &ast, const MessageId &) {
        if (sentinel)
            checkDefinitionAst(ast);
    };
    client()->getAndHandleAst(document, astHandler, ClangdClient::AstCallbackMode::SyncIfPossible,
                              {});
}

void ClangdFindLocalReferences::Private::checkDefinitionAst(const ClangdAstNode &ast)
{
    qCDebug(clangdLog) << "received ast response";
    if (!ast.isValid() || !document) {
        finish();
        return;
    }

    const Position linkPos(defLink.targetLine - 1, defLink.targetColumn);
    const ClangdAstPath astPath = getAstPath(ast, linkPos);
    bool isVar = false;
    for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
        if (it->role() == "declaration"
                && (it->kind() == "Function" || it->kind() == "CXXMethod"
                    || it->kind() == "CXXConstructor" || it->kind() == "CXXDestructor"
                    || it->kind() == "Lambda")) {
            if (!isVar)
                break;

            qCDebug(clangdLog) << "finding references for local var";
            const auto refsHandler = [sentinel = QPointer(q), this](const QList<Location> &refs) {
                if (sentinel)
                    handleReferences(refs);
            };
            client()->symbolSupport().findUsages(document, cursor, refsHandler);
            return;
        }
        if (!isVar && it->role() == "declaration"
                && (it->kind() == "Var" || it->kind() == "ParmVar")) {
            isVar = true;
        }
    }
    finish();
}

void ClangdFindLocalReferences::Private::handleReferences(const QList<Location> &references)
{
    qCDebug(clangdLog) << "found" << references.size() << "local references";

    const auto transformLocation = [mapper = client()->hostPathMapper()](const Location &loc) {
        return loc.toLink(mapper);
    };

    Utils::Links links = Utils::transform(references, transformLocation);

    // The callback only uses the symbol length, so we just create a dummy.
    // Note that the calculation will be wrong for identifiers with
    // embedded newlines, but we've never supported that.
    QString symbol;
    if (!references.isEmpty()) {
        const Range r = references.first().range();
        const Position pos = r.start();
        symbol = QString(r.end().character() - pos.character(), 'x');
        if (editorWidget && document) {
            QTextCursor cursor(document->document());
            cursor.setPosition(Text::positionInText(document->document(), pos.line() + 1,
                                                    pos.character() + 1));
            const QList<Text::Range> occurrencesInComments
                = symbolOccurrencesInDeclarationComments(editorWidget, cursor);
            for (const Text::Range &range : occurrencesInComments) {
                static const auto cmp = [](const Link &l, const Text::Range &r) {
                    if (l.targetLine < r.begin.line)
                        return true;
                    if (l.targetLine > r.begin.line)
                        return false;
                    return l.targetColumn < r.begin.column;
                };
                const auto it = std::lower_bound(links.begin(), links.end(), range, cmp);
                links.emplace(it, links.first().targetFilePath, range.begin.line,
                              range.begin.column);
            }
        }
    }
    callback(symbol, links, revision);
    callback = {};
    finish();
}

void ClangdFindLocalReferences::Private::finish()
{
    if (callback)
        callback({}, {}, revision);
    emit q->done();
}

} // namespace ClangCodeModel::Internal

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ReplacementData)
