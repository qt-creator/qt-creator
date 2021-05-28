/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "clangdclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultitem.h>
#include <coreplugin/find/searchresultwindow.h>
#include <cplusplus/FindUsages.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppeditorwidgetinterface.h>
#include <cpptools/cppfindreferences.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppvirtualfunctionassistprovider.h>
#include <cpptools/cppvirtualfunctionproposalitem.h>
#include <languageclient/languageclientinterface.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <texteditor/basefilefind.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>

#include <QCheckBox>
#include <QFile>
#include <QHash>
#include <QPair>
#include <QPointer>
#include <QRegularExpression>

#include <set>

using namespace CPlusPlus;
using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;

namespace ClangCodeModel {
namespace Internal {

static Q_LOGGING_CATEGORY(clangdLog, "qtc.clangcodemodel.clangd", QtWarningMsg);
static QString indexingToken() { return "backgroundIndexProgress"; }

class AstParams : public JsonObject
{
public:
    AstParams() {}
    AstParams(const TextDocumentIdentifier &document, const Range &range)
    {
        setTextDocument(document);
        setRange(range);
    }

    using JsonObject::JsonObject;

    // The open file to inspect.
    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &id) { insert(textDocumentKey, id); }

    // The region of the source code whose AST is fetched. The highest-level node that entirely
    // contains the range is returned.
    Utils::optional<Range> range() const { return optionalValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    bool isValid() const override { return contains(textDocumentKey); }
};

class AstNode : public JsonObject
{
public:
    using JsonObject::JsonObject;

    static constexpr char roleKey[] = "role";
    static constexpr char arcanaKey[] = "arcana";

    // The general kind of node, such as “expression”. Corresponds to clang’s base AST node type,
    // such as Expr. The most common are “expression”, “statement”, “type” and “declaration”.
    QString role() const { return typedValue<QString>(roleKey); }

    // The specific kind of node, such as “BinaryOperator”. Corresponds to clang’s concrete
    // node class, with Expr etc suffix dropped.
    QString kind() const { return typedValue<QString>(kindKey); }

    // Brief additional details, such as ‘||’. Information present here depends on the node kind.
    Utils::optional<QString> detail() const { return optionalValue<QString>(detailKey); }

    // One line dump of information, similar to that printed by clang -Xclang -ast-dump.
    // Only available for certain types of nodes.
    Utils::optional<QString> arcana() const { return optionalValue<QString>(arcanaKey); }

    // The part of the code that produced this node. Missing for implicit nodes, nodes produced
    // by macro expansion, etc.
    Range range() const { return typedValue<Range>(rangeKey); }

    // Descendants describing the internal structure. The tree of nodes is similar to that printed
    // by clang -Xclang -ast-dump, or that traversed by clang::RecursiveASTVisitor.
    Utils::optional<QList<AstNode>> children() const { return optionalArray<AstNode>(childrenKey); }

    bool hasRange() const { return contains(rangeKey); }

    bool arcanaContains(const QString &s) const
    {
        const Utils::optional<QString> arcanaString = arcana();
        return arcanaString && arcanaString->contains(s);
    }

    bool detailIs(const QString &s) const
    {
        return detail() && detail().value() == s;
    }

    bool isMemberFunctionCall() const
    {
        return role() == "expression" && kind() == "Member" && arcanaContains("member function");
    }

    bool isPureVirtualDeclaration() const
    {
        return role() == "declaration" && kind() == "CXXMethod" && arcanaContains("virtual pure");
    }

    bool isPureVirtualDefinition() const
    {
        return role() == "declaration" && kind() == "CXXMethod" && arcanaContains("' pure");
    }

    bool mightBeAmbiguousVirtualCall() const
    {
        if (!isMemberFunctionCall())
            return false;
        const Utils::optional<QList<AstNode>> childList = children();
        if (!childList)
            return true;
        for (const AstNode &c : qAsConst(*childList)) {
            if (c.detailIs("UncheckedDerivedToBase"))
                return false;
        }
        return true;
    }

    QString type() const
    {
        const Utils::optional<QString> arcanaString = arcana();
        if (!arcanaString)
            return {};
        const int quote1Offset = arcanaString->indexOf('\'');
        if (quote1Offset == -1)
            return {};
        const int quote2Offset = arcanaString->indexOf('\'', quote1Offset + 1);
        if (quote2Offset == -1)
            return {};
        return arcanaString->mid(quote1Offset + 1, quote2Offset - quote1Offset - 1);
    }

    // Returns true <=> the type is "recursively const".
    // E.g. returns true for "const int &", "const int *" and "const int * const *",
    // and false for "int &" and "const int **".
    // For non-pointer types such as "int", we check whether they are uses as lvalues
    // or rvalues.
    bool hasConstType() const
    {
        QString theType = type();
        if (theType.endsWith("const"))
            theType.chop(5);
        const int ptrRefCount = theType.count('*') + theType.count('&');
        const int constCount = theType.count("const");
        if (ptrRefCount == 0)
            return constCount > 0 || detailIs("LValueToRValue");
        return ptrRefCount <= constCount;
    }

    bool childContainsRange(int index, const Range &range) const
    {
        const Utils::optional<QList<AstNode>> childList = children();
        return childList && childList->size() > index
                && childList->at(index).range().contains(range);
    }

    QString operatorString() const
    {
        if (kind() == "BinaryOperator")
            return detail().value_or(QString());
        QTC_ASSERT(kind() == "CXXOperatorCall", return {});
        const Utils::optional<QString> arcanaString = arcana();
        if (!arcanaString)
            return {};
        const int closingQuoteOffset = arcanaString->lastIndexOf('\'');
        if (closingQuoteOffset <= 0)
            return {};
        const int openingQuoteOffset = arcanaString->lastIndexOf('\'', closingQuoteOffset - 1);
        if (openingQuoteOffset == -1)
            return {};
        return arcanaString->mid(openingQuoteOffset + 1, closingQuoteOffset
                                 - openingQuoteOffset - 1);
    }

    // For debugging.
    void print(int indent = 0) const
    {
        (qDebug().noquote() << QByteArray(indent, ' ')).quote() << role() << kind()
                 << detail().value_or(QString()) << arcana().value_or(QString());
        for (const AstNode &c : children().value_or(QList<AstNode>()))
            c.print(indent + 2);
    }

    bool isValid() const override
    {
        return contains(roleKey) && contains(kindKey);
    }
};

static QList<AstNode> getAstPath(const AstNode &root, const Range &range)
{
    QList<AstNode> path;
    QList<AstNode> queue{root};
    bool isRoot = true;
    while (!queue.isEmpty()) {
        AstNode curNode = queue.takeFirst();
        if (!isRoot && !curNode.hasRange())
            continue;
        if (curNode.range() == range)
            return path << curNode;
        if (isRoot || curNode.range().contains(range)) {
            path << curNode;
            const auto children = curNode.children();
            if (!children)
                break;
            queue = children.value();
        }
        isRoot = false;
    }
    return path;
}

static Usage::Type getUsageType(const QList<AstNode> &path)
{
    bool potentialWrite = false;
    const bool symbolIsDataType = path.last().role() == "type" && path.last().kind() == "Record";
    for (auto pathIt = path.rbegin(); pathIt != path.rend(); ++pathIt) {
        if (pathIt->arcanaContains("non_odr_use_unevaluated"))
            return Usage::Type::Other;
        if (pathIt->kind() == "CXXDelete")
            return Usage::Type::Write;
        if (pathIt->kind() == "CXXNew")
            return Usage::Type::Other;
        if (pathIt->kind() == "Switch" || pathIt->kind() == "If")
            return Usage::Type::Read;
        if (pathIt->kind() == "Call" || pathIt->kind() == "CXXMemberCall")
            return potentialWrite ? Usage::Type::WritableRef : Usage::Type::Read;
        if ((pathIt->kind() == "DeclRef" || pathIt->kind() == "Member")
                && pathIt->arcanaContains("lvalue")) {
            potentialWrite = true;
        }
        if (pathIt->role() == "declaration") {
            if (symbolIsDataType)
                return Usage::Type::Other;
            if (pathIt->arcanaContains("cinit")) {
                if (pathIt == path.rbegin())
                    return Usage::Type::Initialization;
                if (pathIt->childContainsRange(0, path.last().range()))
                    return Usage::Type::Initialization;
                if (!pathIt->hasConstType())
                    return Usage::Type::WritableRef;
                return Usage::Type::Read;
            }
            return Usage::Type::Declaration;
        }
        if (pathIt->kind() == "MemberInitializer")
            return pathIt == path.rbegin() ? Usage::Type::Write : Usage::Type::Read;
        if (pathIt->kind() == "UnaryOperator"
                && (pathIt->detailIs("++") || pathIt->detailIs("--"))) {
            return Usage::Type::Write;
        }

        // LLVM uses BinaryOperator only for built-in types; for classes, CXXOperatorCall
        // is used. The latter has an additional node at index 0, so the left-hand side
        // of an assignment is at index 1.
        const bool isBinaryOp = pathIt->kind() == "BinaryOperator";
        const bool isOpCall = pathIt->kind() == "CXXOperatorCall";
        if (isBinaryOp || isOpCall) {
            if (isOpCall && symbolIsDataType) // Constructor invocation.
                return Usage::Type::Other;

            const QString op = pathIt->operatorString();
            if (op.endsWith("=") && op != "==") { // Assignment.
                const int lhsIndex = isBinaryOp ? 0 : 1;
                if (pathIt->childContainsRange(lhsIndex, path.last().range()))
                    return Usage::Type::Write;
                return potentialWrite ? Usage::Type::WritableRef : Usage::Type::Read;
            }
            return Usage::Type::Read;
        }

        if (pathIt->kind() == "ImplicitCast") {
            if (pathIt->detailIs("FunctionToPointerDecay"))
                return Usage::Type::Other;
            if (pathIt->hasConstType())
                return Usage::Type::Read;
            potentialWrite = true;
            continue;
        }
    }

    return Usage::Type::Other;
}

class AstRequest : public Request<AstNode, std::nullptr_t, AstParams>
{
public:
    using Request::Request;
    explicit AstRequest(const AstParams &params) : Request("textDocument/ast", params) {}
};

class SymbolDetails : public JsonObject
{
public:
    using JsonObject::JsonObject;

    static constexpr char usrKey[] = "usr";

    // the unqualified name of the symbol
    QString name() const { return typedValue<QString>(nameKey); }

    // the enclosing namespace, class etc (without trailing ::)
    // [NOTE: This is not true, the trailing colons are included]
    QString containerName() const { return typedValue<QString>(containerNameKey); }

    // the clang-specific “unified symbol resolution” identifier
    QString usr() const { return typedValue<QString>(usrKey); }

    // the clangd-specific opaque symbol ID
    Utils::optional<QString> id() const { return optionalValue<QString>(idKey); }

    bool isValid() const override
    {
        return contains(nameKey) && contains(containerNameKey) && contains(usrKey);
    }
};

class SymbolInfoRequest : public Request<LanguageClientArray<SymbolDetails>, std::nullptr_t, TextDocumentPositionParams>
{
public:
    using Request::Request;
    explicit SymbolInfoRequest(const TextDocumentPositionParams &params)
        : Request("textDocument/symbolInfo", params) {}
};

static BaseClientInterface *clientInterface(const Utils::FilePath &jsonDbDir)
{
    Utils::CommandLine cmd{CppTools::codeModelSettings()->clangdFilePath(),
                           {"--background-index", "--limit-results=0"}};
    if (!jsonDbDir.isEmpty())
        cmd.addArg("--compile-commands-dir=" + jsonDbDir.toString());
    if (clangdLog().isDebugEnabled())
        cmd.addArgs({"--log=verbose", "--pretty"});
    const auto interface = new StdIOClientInterface;
    interface->setCommandLine(cmd);
    return interface;
}

class ReferencesFileData {
public:
    QList<QPair<Range, QString>> rangesAndLineText;
    QString fileContent;
    AstNode ast;
};
class ReplacementData {
public:
    QString oldSymbolName;
    QString newSymbolName;
    QSet<Utils::FilePath> fileRenameCandidates;
};
class ReferencesData {
public:
    QMap<DocumentUri, ReferencesFileData> fileData;
    QList<MessageId> pendingAstRequests;
    QPointer<SearchResult> search;
    Utils::optional<ReplacementData> replacementData;
    quint64 key;
    bool canceled = false;
    bool categorize = CppTools::codeModelSettings()->categorizeFindReferences();
};

using SymbolData = QPair<QString, Utils::Link>;
using SymbolDataList = QList<SymbolData>;

class ClangdClient::VirtualFunctionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    VirtualFunctionAssistProcessor(ClangdClient::Private *data) : m_data(data) {}

    void cancel() override;
    bool running() override { return m_data; }

    void finalize();

private:
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *) override
    {
        return nullptr;
    }

    TextEditor::IAssistProposal *immediateProposal(const TextEditor::AssistInterface *) override;

    TextEditor::IAssistProposal *immediateProposalImpl() const;

    ClangdClient::Private *m_data = nullptr;
};

class ClangdClient::VirtualFunctionAssistProvider : public TextEditor::IAssistProvider
{
public:
    VirtualFunctionAssistProvider(ClangdClient::Private *data) : m_data(data) {}

private:
    RunType runType() const override { return Asynchronous; }
    TextEditor::IAssistProcessor *createProcessor() const override;

    ClangdClient::Private * const m_data;
};

class ClangdClient::FollowSymbolData {
public:
    FollowSymbolData(ClangdClient *q, quint64 id, const QTextCursor &cursor,
                     CppTools::CppEditorWidgetInterface *editorWidget,
                     const DocumentUri &uri, Utils::ProcessLinkCallback &&callback,
                     bool openInSplit)
        : q(q), id(id), cursor(cursor), editorWidget(editorWidget), uri(uri),
          callback(std::move(callback)), virtualFuncAssistProvider(q->d),
          openInSplit(openInSplit) {}

    ~FollowSymbolData()
    {
        closeTempDocuments();
        if (virtualFuncAssistProcessor)
            virtualFuncAssistProcessor->cancel();
        for (const MessageId &id : qAsConst(pendingSymbolInfoRequests))
            q->cancelRequest(id);
        for (const MessageId &id : qAsConst(pendingGotoImplRequests))
            q->cancelRequest(id);
    }

    void closeTempDocuments()
    {
        for (const Utils::FilePath &fp : qAsConst(openedFiles))
            q->closeExtraFile(fp);
        openedFiles.clear();
    }

    bool isEditorWidgetStillAlive() const
    {
        return Utils::anyOf(EditorManager::visibleEditors(), [this](IEditor *editor) {
            const auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
            return textEditor && dynamic_cast<CppTools::CppEditorWidgetInterface *>(
                    textEditor->editorWidget()) == editorWidget;
        });
    }

    ClangdClient * const q;
    const quint64 id;
    const QTextCursor cursor;
    CppTools::CppEditorWidgetInterface * const editorWidget;
    const DocumentUri uri;
    const Utils::ProcessLinkCallback callback;
    VirtualFunctionAssistProvider virtualFuncAssistProvider;
    QList<MessageId> pendingSymbolInfoRequests;
    QList<MessageId> pendingGotoImplRequests;
    const bool openInSplit;

    Utils::Link defLink;
    QList<Utils::Link> allLinks;
    AstNode cursorNode;
    AstNode defLinkNode;
    SymbolDataList symbolsToDisplay;
    std::set<Utils::FilePath> openedFiles;
    VirtualFunctionAssistProcessor *virtualFuncAssistProcessor = nullptr;
};


class ClangdClient::Private
{
public:
    Private(ClangdClient *q) : q(q) {}

    void handleFindUsagesResult(quint64 key, const QList<Location> &locations);
    static void handleRenameRequest(const SearchResult *search,
                                    const ReplacementData &replacementData,
                                    const QString &newSymbolName,
                                    const QList<Core::SearchResultItem> &checkedItems,
                                    bool preserveCase);
    void addSearchResultsForFile(ReferencesData &refData, const Utils::FilePath &file,
                                 const ReferencesFileData &fileData);
    void reportAllSearchResultsAndFinish(ReferencesData &data);
    void finishSearch(const ReferencesData &refData, bool canceled);

    void handleGotoDefinitionResult();
    void sendGotoImplementationRequest(const Utils::Link &link);
    void handleGotoImplementationResult(const GotoImplementationRequest::Response &response);
    void handleDocumentInfoResults();
    void closeTempDocuments();

    ClangdClient * const q;
    QHash<quint64, ReferencesData> runningFindUsages;
    Utils::optional<FollowSymbolData> followSymbolData;
    Utils::optional<QVersionNumber> versionNumber;
    quint64 nextFindUsagesKey = 0;
    quint64 nextFollowSymbolId = 0;
    bool isFullyIndexed = false;
    bool isTesting = false;
};

ClangdClient::ClangdClient(Project *project, const Utils::FilePath &jsonDbDir)
    : Client(clientInterface(jsonDbDir)), d(new Private(this))
{
    setName(tr("clangd"));
    LanguageFilter langFilter;
    langFilter.mimeTypes = QStringList{"text/x-chdr", "text/x-c++hdr", "text/x-c++src",
            "text/x-objc++src", "text/x-objcsrc"};
    setSupportedLanguage(langFilter);
    LanguageServerProtocol::ClientCapabilities caps = Client::defaultClientCapabilities();
    caps.clearExperimental();
    caps.clearTextDocument();
    setClientCapabilities(caps);
    setLocatorsEnabled(false);
    setProgressTitleForToken(indexingToken(), tr("Parsing C/C++ Files (clangd)"));
    setCurrentProject(project);
    connect(this, &Client::workDone, this, [this, project](const ProgressToken &token) {
        const QString * const val = Utils::get_if<QString>(&token);
        if (val && *val == indexingToken()) {
            d->isFullyIndexed = true;
            emit indexingFinished();
#ifdef WITH_TESTS
            emit project->indexingFinished("Indexer.Clangd");
#endif
        }
    });

    connect(this, &Client::initialized, this, [this] {
        // If we get this signal while there are pending searches, it means that
        // the client was re-initialized, i.e. clangd crashed.

        // Report all search results found so far.
        for (quint64 key : d->runningFindUsages.keys())
            d->reportAllSearchResultsAndFinish(d->runningFindUsages[key]);
        QTC_CHECK(d->runningFindUsages.isEmpty());
    });

    start();
}

ClangdClient::~ClangdClient()
{
    if (d->followSymbolData) {
        d->followSymbolData->openedFiles.clear();
        d->followSymbolData->pendingSymbolInfoRequests.clear();
        d->followSymbolData->pendingGotoImplRequests.clear();
    }
    delete d;
}

bool ClangdClient::isFullyIndexed() const { return d->isFullyIndexed; }

void ClangdClient::openExtraFile(const Utils::FilePath &filePath, const QString &content)
{
    QFile cxxFile(filePath.toString());
    if (content.isEmpty() && !cxxFile.open(QIODevice::ReadOnly))
        return;
    TextDocumentItem item;
    item.setLanguageId("cpp");
    item.setUri(DocumentUri::fromFilePath(filePath));
    item.setText(!content.isEmpty() ? content : QString::fromUtf8(cxxFile.readAll()));
    item.setVersion(0);
    sendContent(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)));
}

void ClangdClient::closeExtraFile(const Utils::FilePath &filePath)
{
    sendContent(DidCloseTextDocumentNotification(DidCloseTextDocumentParams(
            TextDocumentIdentifier{DocumentUri::fromFilePath(filePath)})));
}

void ClangdClient::findUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
                              const Utils::optional<QString> &replacement)
{
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    const QString searchTerm = termCursor.selectedText(); // TODO: This will be wrong for e.g. operators. Use a Symbol info request to get the real symbol string.
    if (searchTerm.isEmpty())
        return;

    ReferencesData refData;
    refData.key = d->nextFindUsagesKey++;
    if (replacement) {
        ReplacementData replacementData;
        replacementData.oldSymbolName = searchTerm;
        replacementData.newSymbolName = *replacement;
        if (replacementData.newSymbolName.isEmpty())
            replacementData.newSymbolName = replacementData.oldSymbolName;
        refData.replacementData = replacementData;
    }
    refData.search = SearchResultWindow::instance()->startNewSearch(
                tr("C++ Usages:"),
                {},
                searchTerm,
                replacement ? SearchResultWindow::SearchAndReplace : SearchResultWindow::SearchOnly,
                SearchResultWindow::PreserveCaseDisabled,
                "CppEditor");
    if (refData.categorize)
        refData.search->setFilter(new CppTools::CppSearchResultFilter);
    if (refData.replacementData) {
        refData.search->setTextToReplace(refData.replacementData->newSymbolName);
        const auto renameFilesCheckBox = new QCheckBox;
        renameFilesCheckBox->setVisible(false);
        refData.search->setAdditionalReplaceWidget(renameFilesCheckBox);
        const auto renameHandler =
                [search = refData.search](const QString &newSymbolName,
                                          const QList<SearchResultItem> &checkedItems,
                                          bool preserveCase) {
            const auto replacementData = search->userData().value<ReplacementData>();
            Private::handleRenameRequest(search, replacementData, newSymbolName, checkedItems,
                                         preserveCase);
        };
        connect(refData.search, &SearchResult::replaceButtonClicked, renameHandler);
    }
    connect(refData.search, &SearchResult::activated, [](const SearchResultItem& item) {
        Core::EditorManager::openEditorAtSearchResult(item);
    });
    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    d->runningFindUsages.insert(refData.key, refData);

    const Utils::optional<MessageId> requestId = symbolSupport().findUsages(
                document, cursor, [this, key = refData.key](const QList<Location> &locations) {
        d->handleFindUsagesResult(key, locations);
    });

    if (!requestId) {
        d->finishSearch(refData, false);
        return;
    }
    connect(refData.search, &SearchResult::cancelled, this, [this, requestId, key = refData.key] {
        const auto refData = d->runningFindUsages.find(key);
        if (refData == d->runningFindUsages.end())
            return;
        cancelRequest(*requestId);
        refData->canceled = true;
        refData->search->disconnect(this);
        d->finishSearch(*refData, true);
    });
}

void ClangdClient::enableTesting() { d->isTesting = true; }

QVersionNumber ClangdClient::versionNumber() const
{
    if (d->versionNumber)
        return d->versionNumber.value();

    const QRegularExpression versionPattern("^clangd version (\\d+)\\.(\\d+)\\.(\\d+).*$");
    QTC_CHECK(versionPattern.isValid());
    const QRegularExpressionMatch match = versionPattern.match(serverVersion());
    if (match.isValid()) {
        d->versionNumber.emplace({match.captured(1).toInt(), match.captured(2).toInt(),
                                 match.captured(3).toInt()});
    } else {
        qCWarning(clangdLog) << "Failed to parse clangd server string" << serverVersion();
        d->versionNumber.emplace({0});
    }
    return d->versionNumber.value();
}

void ClangdClient::Private::handleFindUsagesResult(quint64 key, const QList<Location> &locations)
{
    const auto refData = runningFindUsages.find(key);
    if (refData == runningFindUsages.end())
        return;
    if (!refData->search || refData->canceled) {
        finishSearch(*refData, true);
        return;
    }
    refData->search->disconnect(q);

    qCDebug(clangdLog) << "found" << locations.size() << "locations";
    if (locations.isEmpty()) {
        finishSearch(*refData, false);
        return;
    }

    QObject::connect(refData->search, &SearchResult::cancelled, q, [this, key] {
        const auto refData = runningFindUsages.find(key);
        if (refData == runningFindUsages.end())
            return;
        refData->canceled = true;
        refData->search->disconnect(q);
        for (const MessageId &id : qAsConst(refData->pendingAstRequests))
            q->cancelRequest(id);
        refData->pendingAstRequests.clear();
        finishSearch(*refData, true);
    });

    for (const Location &loc : locations) // TODO: Can contain duplicates. Rather fix in clang than work around it here.
        refData->fileData[loc.uri()].rangesAndLineText << qMakePair(loc.range(), QString()); // TODO: Can we assume that locations for the same file are grouped?
    for (auto it = refData->fileData.begin(); it != refData->fileData.end(); ++it) {
        const QStringList lines = SymbolSupport::getFileContents(it.key().toFilePath());
        it->fileContent = lines.join('\n');
        for (auto &rangeWithText : it.value().rangesAndLineText) {
            const int lineNo = rangeWithText.first.start().line();
            if (lineNo >= 0 && lineNo < lines.size())
                rangeWithText.second = lines.at(lineNo);
        }
    }

    qCDebug(clangdLog) << "document count is" << refData->fileData.size();
    if (refData->replacementData || q->versionNumber() < QVersionNumber(13)
            || !refData->categorize) {
        qCDebug(clangdLog) << "skipping AST retrieval";
        reportAllSearchResultsAndFinish(*refData);
        return;
    }

    for (auto it = refData->fileData.begin(); it != refData->fileData.end(); ++it) {
        const bool extraOpen = !q->documentForFilePath(it.key().toFilePath());
        if (extraOpen)
            q->openExtraFile(it.key().toFilePath(), it->fileContent);
        it->fileContent.clear();

        AstParams params;
        params.setTextDocument(TextDocumentIdentifier(it.key()));
        AstRequest request(params);
        request.setResponseCallback([this, key, loc = it.key(), request]
                                    (AstRequest::Response response) {
            qCDebug(clangdLog) << "AST response for" << loc.toFilePath();
            const auto refData = runningFindUsages.find(key);
            if (refData == runningFindUsages.end())
                return;
            if (!refData->search || refData->canceled)
                return;
            ReferencesFileData &data = refData->fileData[loc];
            const auto result = response.result();
            if (result)
                data.ast = *result;
            refData->pendingAstRequests.removeOne(request.id());
            qCDebug(clangdLog) << refData->pendingAstRequests.size()
                               << "AST requests still pending";
            addSearchResultsForFile(*refData, loc.toFilePath(), data);
            refData->fileData.remove(loc);
            if (refData->pendingAstRequests.isEmpty()) {
                qDebug(clangdLog) << "retrieved all ASTs";
                finishSearch(*refData, false);
            }
        });
        qCDebug(clangdLog) << "requesting AST for" << it.key().toFilePath();
        refData->pendingAstRequests << request.id();
        q->sendContent(request);

        if (extraOpen)
            q->closeExtraFile(it.key().toFilePath());
    }
}

void ClangdClient::Private::handleRenameRequest(const SearchResult *search,
                                                const ReplacementData &replacementData,
                                                const QString &newSymbolName,
                                                const QList<SearchResultItem> &checkedItems,
                                                bool preserveCase)
{
    const QStringList fileNames = TextEditor::BaseFileFind::replaceAll(newSymbolName, checkedItems,
                                                                       preserveCase);
    if (!fileNames.isEmpty())
        SearchResultWindow::instance()->hide();

    const auto renameFilesCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
    QTC_ASSERT(renameFilesCheckBox, return);
    if (!renameFilesCheckBox->isChecked())
        return;

    QVector<Node *> fileNodes;
    for (const Utils::FilePath &file : replacementData.fileRenameCandidates) {
        Node * const node = ProjectTree::nodeForFile(file);
        if (node)
            fileNodes << node;
    }
    if (!fileNodes.isEmpty())
        CppTools::renameFilesForSymbol(replacementData.oldSymbolName, newSymbolName, fileNodes);
}

void ClangdClient::Private::addSearchResultsForFile(ReferencesData &refData,
        const Utils::FilePath &file,
        const ReferencesFileData &fileData)
{
    QList<SearchResultItem> items;
    qCDebug(clangdLog) << file << "has valid AST:" << fileData.ast.isValid();
    for (const auto &rangeWithText : fileData.rangesAndLineText) {
        const Range &range = rangeWithText.first;
        const Usage::Type usageType = fileData.ast.isValid()
                ? getUsageType(getAstPath(fileData.ast, qAsConst(range)))
                : Usage::Type::Other;
        SearchResultItem item;
        item.setUserData(int(usageType));
        item.setStyle(CppTools::colorStyleForUsageType(usageType));
        item.setFilePath(file);
        item.setMainRange(SymbolSupport::convertRange(range));
        item.setUseTextEditorFont(true);
        item.setLineText(rangeWithText.second);
        if (refData.search->supportsReplace()) {
            const bool fileInSession = SessionManager::projectForFile(file);
            item.setSelectForReplacement(fileInSession);
            if (fileInSession && file.baseName().compare(
                        refData.replacementData->oldSymbolName,
                        Qt::CaseInsensitive) == 0) {
                refData.replacementData->fileRenameCandidates << file; // TODO: We want to do this only for types. Use SymbolInformation once we have it.
            }
        }
        items << item;
    }
    if (isTesting)
        emit q->foundReferences(items);
    else
        refData.search->addResults(items, SearchResult::AddOrdered);
}

void ClangdClient::Private::reportAllSearchResultsAndFinish(ReferencesData &refData)
{
    for (auto it = refData.fileData.begin(); it != refData.fileData.end(); ++it)
        addSearchResultsForFile(refData, it.key().toFilePath(), it.value());
    finishSearch(refData, refData.canceled);
}

void ClangdClient::Private::finishSearch(const ReferencesData &refData, bool canceled)
{
    if (isTesting) {
        emit q->findUsagesDone();
    } else if (refData.search) {
        refData.search->finishSearch(canceled);
        refData.search->disconnect(q);
        if (refData.replacementData) {
            const auto renameCheckBox = qobject_cast<QCheckBox *>(
                        refData.search->additionalReplaceWidget());
            QTC_CHECK(renameCheckBox);
            const QSet<Utils::FilePath> files = refData.replacementData->fileRenameCandidates;
            renameCheckBox->setText(tr("Re&name %n files", nullptr, files.size()));
            const QStringList filesForUser = Utils::transform<QStringList>(files,
                        [](const Utils::FilePath &fp) { return fp.toUserOutput(); });
            renameCheckBox->setToolTip(tr("Files:\n%1").arg(filesForUser.join('\n')));
            renameCheckBox->setVisible(true);
            refData.search->setUserData(QVariant::fromValue(*refData.replacementData));
        }
    }
    runningFindUsages.remove(refData.key);
}

void ClangdClient::followSymbol(
        TextEditor::TextDocument *document,
        const QTextCursor &cursor,
        CppTools::CppEditorWidgetInterface *editorWidget,
        Utils::ProcessLinkCallback &&callback,
        bool resolveTarget,
        bool openInSplit
        )
{
    QTC_ASSERT(documentOpen(document), openDocument(document));
    if (!resolveTarget) {
        d->followSymbolData.reset();
        symbolSupport().findLinkAt(document, cursor, std::move(callback), false);
        return;
    }

    qCDebug(clangdLog) << "follow symbol requested" << document->filePath()
                       << cursor.blockNumber() << cursor.positionInBlock();
    d->followSymbolData.emplace(this, ++d->nextFollowSymbolId, cursor, editorWidget,
                                DocumentUri::fromFilePath(document->filePath()),
                                std::move(callback), openInSplit);

    // Step 1: Follow the symbol via "Go to Definition". At the same time, request the
    //         AST node corresponding to the cursor position, so we can find out whether
    //         we have to look for overrides.
    const auto gotoDefCallback = [this, id = d->followSymbolData->id](const Utils::Link &link) {
        qCDebug(clangdLog) << "received go to definition response";
        if (!link.hasValidTarget()) {
            d->followSymbolData.reset();
            return;
        }
        if (!d->followSymbolData || id != d->followSymbolData->id)
            return;
        d->followSymbolData->defLink = link;
        if (d->followSymbolData->cursorNode.isValid())
            d->handleGotoDefinitionResult();
    };
    symbolSupport().findLinkAt(document, cursor, std::move(gotoDefCallback), true);

    AstRequest astRequest(AstParams(TextDocumentIdentifier(d->followSymbolData->uri),
                                    Range(cursor)));
    astRequest.setResponseCallback([this, id = d->followSymbolData->id](
                                   const AstRequest::Response &response) {
        qCDebug(clangdLog) << "received ast response for cursor";
        if (!d->followSymbolData || d->followSymbolData->id != id)
            return;
        const auto result = response.result();
        if (!result) {
            d->followSymbolData.reset();
            return;
        }
        d->followSymbolData->cursorNode = *result;
        if (d->followSymbolData->defLink.hasValidTarget())
            d->handleGotoDefinitionResult();
    });
    sendContent(astRequest);
}

void ClangdClient::Private::handleGotoDefinitionResult()
{
    QTC_ASSERT(followSymbolData->defLink.hasValidTarget(), return);

    qCDebug(clangdLog) << "handling go to definition result";

    // No dis-ambiguation necessary. Call back with the link and finish.
    if (!followSymbolData->cursorNode.mightBeAmbiguousVirtualCall()
            && !followSymbolData->cursorNode.isPureVirtualDeclaration()) {
        followSymbolData->callback(followSymbolData->defLink);
        followSymbolData.reset();
        return;
    }

    // Step 2: Get all possible overrides via "Go to Implementation".
    // Note that we have to do this for all member function calls, because
    // we cannot tell here whether the member function is virtual.
    followSymbolData->allLinks << followSymbolData->defLink;
    sendGotoImplementationRequest(followSymbolData->defLink);
}

void ClangdClient::Private::sendGotoImplementationRequest(const Utils::Link &link)
{
    const Position position(link.targetLine - 1, link.targetColumn);
    const TextDocumentIdentifier documentId(DocumentUri::fromFilePath(link.targetFilePath));
    GotoImplementationRequest req(TextDocumentPositionParams(documentId, position));
    req.setResponseCallback([this, id = followSymbolData->id, reqId = req.id()](
                            const GotoImplementationRequest::Response &response) {
        qCDebug(clangdLog) << "received go to implementation reply";
        if (!followSymbolData || id != followSymbolData->id)
            return;
        followSymbolData->pendingGotoImplRequests.removeOne(reqId);
        handleGotoImplementationResult(response);
    });
    q->sendContent(req);
    followSymbolData->pendingGotoImplRequests << req.id();
    qCDebug(clangdLog) << "sending go to implementation request" << link.targetLine;
}

void ClangdClient::Private::handleGotoImplementationResult(
        const GotoImplementationRequest::Response &response)
{
    if (const Utils::optional<GotoResult> &result = response.result()) {
        QList<Utils::Link> newLinks;
        if (const auto ploc = Utils::get_if<Location>(&*result))
            newLinks = {ploc->toLink()};
        if (const auto plloc = Utils::get_if<QList<Location>>(&*result))
            newLinks = Utils::transform(*plloc, &Location::toLink);
        for (const Utils::Link &link : qAsConst(newLinks)) {
            if (!followSymbolData->allLinks.contains(link)) {
                followSymbolData->allLinks << link;

                // We must do this recursively, because clangd reports only the first
                // level of overrides.
                sendGotoImplementationRequest(link);
            }
        }
    }

    // We didn't find any further candidates, so jump to the original definition link.
    if (followSymbolData->allLinks.size() == 1
            && followSymbolData->pendingGotoImplRequests.isEmpty()) {
        followSymbolData->callback(followSymbolData->allLinks.first());
        followSymbolData.reset();
        return;
    }

    // As soon as we know that there is more than one candidate, we start the code assist
    // procedure, to let the user know that things are happening.
    if (followSymbolData->allLinks.size() > 1 && !followSymbolData->virtualFuncAssistProcessor
            && followSymbolData->isEditorWidgetStillAlive()) {
        followSymbolData->editorWidget->invokeTextEditorWidgetAssist(
                    TextEditor::FollowSymbol, &followSymbolData->virtualFuncAssistProvider);
    }

    if (!followSymbolData->pendingGotoImplRequests.isEmpty())
        return;

    // Step 3: We are done looking for overrides, and we found at least one.
    //         Make a symbol info request for each link to get the class names.
    //         Also get the AST for the base declaration, so we can find out whether it's
    //         pure virtual and mark it accordingly.
    for (const Utils::Link &link : qAsConst(followSymbolData->allLinks)) {
        if (!q->documentForFilePath(link.targetFilePath)
                && followSymbolData->openedFiles.insert(link.targetFilePath).second) {
            q->openExtraFile(link.targetFilePath);
        }
        const TextDocumentIdentifier doc(DocumentUri::fromFilePath(link.targetFilePath));
        const Position pos(link.targetLine - 1, link.targetColumn);
        SymbolInfoRequest req(TextDocumentPositionParams(doc, pos));
        req.setResponseCallback([this, link, id = followSymbolData->id, reqId = req.id()](
                                const SymbolInfoRequest::Response &response) {
            qCDebug(clangdLog) << "handling symbol info reply"
                               << link.targetFilePath.toUserOutput() << link.targetLine;
            if (!followSymbolData || id != followSymbolData->id)
                return;
            if (const auto result = response.result()) {
                if (const auto list = Utils::get_if<QList<SymbolDetails>>(&result.value())) {
                    if (!list->isEmpty()) {
                        // According to the documentation, we should receive a single
                        // object here, but it's a list. No idea what it means if there's
                        // more than one entry. We choose the first one.
                        const SymbolDetails &sd = list->first();
                        followSymbolData->symbolsToDisplay << qMakePair(sd.containerName()
                                                                        + sd.name(), link);
                    }
                }
            }
            followSymbolData->pendingSymbolInfoRequests.removeOne(reqId);
            if (followSymbolData->pendingSymbolInfoRequests.isEmpty()
                    && followSymbolData->defLinkNode.isValid()) {
                handleDocumentInfoResults();
            }
        });
        followSymbolData->pendingSymbolInfoRequests << req.id();
        qCDebug(clangdLog) << "sending symbol info request";
        q->sendContent(req);
    }

    const DocumentUri defLinkUri
            = DocumentUri::fromFilePath(followSymbolData->defLink.targetFilePath);
    const Position defLinkPos(followSymbolData->defLink.targetLine - 1,
                              followSymbolData->defLink.targetColumn);
    AstRequest astRequest(AstParams(TextDocumentIdentifier(defLinkUri),
                                    Range(defLinkPos, defLinkPos)));
    astRequest.setResponseCallback([this, id = followSymbolData->id](
                                   const AstRequest::Response &response) {
        qCDebug(clangdLog) << "received ast response for def link";
        if (!followSymbolData || followSymbolData->id != id)
            return;
        const auto result = response.result();
        if (result)
            followSymbolData->defLinkNode = *result;
        if (followSymbolData->pendingSymbolInfoRequests.isEmpty())
            handleDocumentInfoResults();
    });
    qCDebug(clangdLog) << "sending ast request for def link";
    q->sendContent(astRequest);
}

void ClangdClient::Private::handleDocumentInfoResults()
{
    followSymbolData->closeTempDocuments();

    // If something went wrong, we just follow the original link.
    if (followSymbolData->symbolsToDisplay.isEmpty()) {
        followSymbolData->callback(followSymbolData->defLink);
        followSymbolData.reset();
        return;
    }
    if (followSymbolData->symbolsToDisplay.size() == 1) {
        followSymbolData->callback(followSymbolData->symbolsToDisplay.first().second);
        followSymbolData.reset();
        return;
    }
    QTC_ASSERT(followSymbolData->virtualFuncAssistProcessor
               && followSymbolData->virtualFuncAssistProcessor->running(),
               followSymbolData.reset(); return);
    followSymbolData->virtualFuncAssistProcessor->finalize();
}

void ClangdClient::VirtualFunctionAssistProcessor::cancel()
{
    if (!m_data)
        return;
    m_data->followSymbolData->virtualFuncAssistProcessor = nullptr;
    m_data->followSymbolData.reset();
    m_data = nullptr;
}

void ClangdClient::VirtualFunctionAssistProcessor::finalize()
{
    QList<TextEditor::AssistProposalItemInterface *> items;
    for (const SymbolData &symbol : qAsConst(m_data->followSymbolData->symbolsToDisplay)) {
        const auto item = new CppTools::VirtualFunctionProposalItem(
                    symbol.second, m_data->followSymbolData->openInSplit);
        QString text = symbol.first;
        if (m_data->followSymbolData->defLink == symbol.second
                && (m_data->followSymbolData->defLinkNode.isPureVirtualDeclaration()
                    || m_data->followSymbolData->defLinkNode.isPureVirtualDefinition())) {
            text += " = 0";
        }
        item->setText(text);
        items << item;
    }
    const auto finalProposal = new CppTools::VirtualFunctionProposal(
                m_data->followSymbolData->cursor.position(),
                items, m_data->followSymbolData->openInSplit);
    if (m_data->followSymbolData->isEditorWidgetStillAlive()
            && m_data->followSymbolData->editorWidget->inTestMode) {
        m_data->followSymbolData->editorWidget->setProposals(immediateProposalImpl(),
                                                             finalProposal);
    } else {
        setAsyncProposalAvailable(finalProposal);
    }

    m_data->followSymbolData->virtualFuncAssistProcessor = nullptr;
    m_data->followSymbolData.reset();
    m_data = nullptr;
}

TextEditor::IAssistProposal *ClangdClient::VirtualFunctionAssistProcessor::immediateProposal(
        const TextEditor::AssistInterface *)
{
    if (m_data->followSymbolData->isEditorWidgetStillAlive()
            && m_data->followSymbolData->editorWidget->inTestMode) {
        return nullptr;
    }
    return immediateProposalImpl();
}

TextEditor::IAssistProposal *
ClangdClient::VirtualFunctionAssistProcessor::immediateProposalImpl() const
{
    QTC_ASSERT(m_data && m_data->followSymbolData, return nullptr);

    QList<TextEditor::AssistProposalItemInterface *> items;
    if (!m_data->followSymbolData->cursorNode.isPureVirtualDeclaration()) {
        const auto defLinkItem = new CppTools::VirtualFunctionProposalItem(
                    m_data->followSymbolData->defLink, m_data->followSymbolData->openInSplit);
        defLinkItem->setText(ClangdClient::tr("<base declaration>"));
        items << defLinkItem;
    }
    const auto infoItem = new CppTools::VirtualFunctionProposalItem({}, false);
    infoItem->setText(ClangdClient::tr("collecting overrides ..."));
    items << infoItem;
    return new CppTools::VirtualFunctionProposal(m_data->followSymbolData->cursor.position(),
                                                 items, m_data->followSymbolData->openInSplit);
}

TextEditor::IAssistProcessor *ClangdClient::VirtualFunctionAssistProvider::createProcessor() const
{
    return m_data->followSymbolData->virtualFuncAssistProcessor
            = new VirtualFunctionAssistProcessor(m_data);
}

} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ReplacementData)
