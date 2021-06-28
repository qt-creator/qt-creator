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

#include "clangdiagnosticmanager.h"
#include "clangtextmark.h"

#include <clangsupport/sourcelocationscontainer.h>
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
#include <languageclient/languageclientutils.h>
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
    AstParams(const TextDocumentIdentifier &document, const Range &range = {})
    {
        setTextDocument(document);
        if (range.isValid())
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
        return role() == "expression" && (kind() == "CXXMemberCall"
                || (kind() == "Member" && arcanaContains("member function")));
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
        bool hasBaseCast = false;
        bool hasRecordType = false;
        const QList<AstNode> childList = children().value_or(QList<AstNode>());
        for (const AstNode &c : childList) {
            if (!hasBaseCast && c.detailIs("UncheckedDerivedToBase"))
                hasBaseCast = true;
            if (!hasRecordType && c.role() == "specifier" && c.kind() == "TypeSpec")
                hasRecordType = true;
            if (hasBaseCast && hasRecordType)
                return false;
        }
        return true;
    }

    bool isNamespace() const { return role() == "declaration" && kind() == "Namespace"; }

    QString type() const
    {
        const Utils::optional<QString> arcanaString = arcana();
        if (!arcanaString)
            return {};
        return typeFromPos(*arcanaString, 0);
    }

    QString typeFromPos(const QString &s, int pos) const
    {
        const int quote1Offset = s.indexOf('\'', pos);
        if (quote1Offset == -1)
            return {};
        const int quote2Offset = s.indexOf('\'', quote1Offset + 1);
        if (quote2Offset == -1)
            return {};
        if (s.mid(quote2Offset + 1, 2) == ":'")
            return typeFromPos(s, quote2Offset + 2);
        return s.mid(quote1Offset + 1, quote2Offset - quote1Offset - 1);
    }

    HelpItem::Category qdocCategoryForDeclaration(HelpItem::Category fallback)
    {
        const auto childList = children();
        if (!childList || childList->size() < 2)
            return fallback;
        const AstNode c1 = childList->first();
        if (c1.role() != "type" || c1.kind() != "Auto")
            return fallback;
        QList<AstNode> typeCandidates = {childList->at(1)};
        while (!typeCandidates.isEmpty()) {
            const AstNode n = typeCandidates.takeFirst();
            if (n.role() == "type") {
                if (n.kind() == "Enum")
                    return HelpItem::Enum;
                if (n.kind() == "Record")
                    return HelpItem::ClassOrNamespace;
                return fallback;
            }
            typeCandidates << n.children().value_or(QList<AstNode>());
        }
        return fallback;
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
                 << detail().value_or(QString()) << arcana().value_or(QString())
                 << range();
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

static BaseClientInterface *clientInterface(Project *project, const Utils::FilePath &jsonDbDir)
{
    QString indexingOption = "--background-index";
    const CppTools::ClangdSettings settings = CppTools::ClangdProjectSettings(project).settings();
    if (!settings.indexingEnabled())
        indexingOption += "=0";
    Utils::CommandLine cmd{settings.clangdFilePath(), {indexingOption, "--limit-results=0"}};
    if (settings.workerThreadLimit() != 0)
        cmd.addArg("-j=" + QString::number(settings.workerThreadLimit()));
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

    void update();
    void finalize();

private:
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *) override
    {
        return nullptr;
    }

    TextEditor::IAssistProposal *immediateProposal(const TextEditor::AssistInterface *) override
    {
        return createProposal(false);
    }

    void resetData();

    TextEditor::IAssistProposal *immediateProposalImpl() const;
    TextEditor::IAssistProposal *createProposal(bool final) const;
    CppTools::VirtualFunctionProposalItem *createEntry(const QString &name,
                                                       const Utils::Link &link) const;

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
        for (const MessageId &id : qAsConst(pendingGotoDefRequests))
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
    QList<MessageId> pendingGotoDefRequests;
    const bool openInSplit;

    Utils::Link defLink;
    QList<Utils::Link> allLinks;
    QHash<Utils::Link, Utils::Link> declDefMap;
    Utils::optional<AstNode> cursorNode;
    AstNode defLinkNode;
    SymbolDataList symbolsToDisplay;
    std::set<Utils::FilePath> openedFiles;
    VirtualFunctionAssistProcessor *virtualFuncAssistProcessor = nullptr;
    bool finished = false;
};

class SwitchDeclDefData {
public:
    SwitchDeclDefData(quint64 id, TextEditor::TextDocument *doc, const QTextCursor &cursor,
                      CppTools::CppEditorWidgetInterface *editorWidget,
                      Utils::ProcessLinkCallback &&callback)
        : id(id), document(doc), uri(DocumentUri::fromFilePath(doc->filePath())),
          cursor(cursor), editorWidget(editorWidget), callback(std::move(callback)) {}

    Utils::optional<AstNode> getFunctionNode() const
    {
        QTC_ASSERT(ast, return {});

        const QList<AstNode> path = getAstPath(*ast, Range(cursor));
        for (auto it = path.rbegin(); it != path.rend(); ++it) {
            if (it->role() == "declaration"
                    && (it->kind() == "CXXMethod" || it->kind() == "CXXConversion"
                        || it->kind() == "CXXConstructor" || it->kind() == "CXXDestructor")) {
                return *it;
            }
        }
        return {};
    }

    QTextCursor cursorForFunctionName(const AstNode &functionNode) const
    {
        QTC_ASSERT(docSymbols, return {});

        const auto symbolList = Utils::get_if<QList<DocumentSymbol>>(&*docSymbols);
        if (!symbolList)
            return {};
        const Range &astRange = functionNode.range();
        QList symbolsToCheck = *symbolList;
        while (!symbolsToCheck.isEmpty()) {
            const DocumentSymbol symbol = symbolsToCheck.takeFirst();
            if (symbol.range() == astRange)
                return symbol.selectionRange().start().toTextCursor(document->document());
            if (symbol.range().contains(astRange))
                symbolsToCheck << symbol.children().value_or(QList<DocumentSymbol>());
        }
        return {};
    }

    const quint64 id;
    const QPointer<TextEditor::TextDocument> document;
    const DocumentUri uri;
    const QTextCursor cursor;
    CppTools::CppEditorWidgetInterface * const editorWidget;
    Utils::ProcessLinkCallback callback;
    Utils::optional<DocumentSymbolsResult> docSymbols;
    Utils::optional<AstNode> ast;
};

class LocalRefsData {
public:
    LocalRefsData(quint64 id, TextEditor::TextDocument *doc, const QTextCursor &cursor,
                      CppTools::RefactoringEngineInterface::RenameCallback &&callback)
        : id(id), document(doc), cursor(cursor), callback(std::move(callback)),
          uri(DocumentUri::fromFilePath(doc->filePath())), revision(doc->document()->revision())
    {}

    ~LocalRefsData()
    {
        if (callback)
            callback({}, {}, revision);
    }

    const quint64 id;
    const QPointer<TextEditor::TextDocument> document;
    const QTextCursor cursor;
    CppTools::RefactoringEngineInterface::RenameCallback callback;
    const DocumentUri uri;
    const int revision;
};

class DiagnosticsCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;
    void enableCategorySupport() { insert("categorySupport", true); }
    void enableCodeActionsInline() {insert("codeActionsInline", true);}
};

class ClangdTextDocumentClientCapabilities : public TextDocumentClientCapabilities
{
public:
    using TextDocumentClientCapabilities::TextDocumentClientCapabilities;


    void setPublishDiagnostics(const DiagnosticsCapabilities &caps)
    { insert("publishDiagnostics", caps); }
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

    void handleDeclDefSwitchReplies();

    QString searchTermFromCursor(const QTextCursor &cursor) const;

    void setHelpItemForTooltip(const MessageId &token, const QString &fqn = {},
                               HelpItem::Category category = HelpItem::Unknown,
                               const QString &type = {});

    ClangdClient * const q;
    QHash<quint64, ReferencesData> runningFindUsages;
    Utils::optional<FollowSymbolData> followSymbolData;
    Utils::optional<SwitchDeclDefData> switchDeclDefData;
    Utils::optional<LocalRefsData> localRefsData;
    Utils::optional<QVersionNumber> versionNumber;
    quint64 nextJobId = 0;
    bool isFullyIndexed = false;
    bool isTesting = false;
};

ClangdClient::ClangdClient(Project *project, const Utils::FilePath &jsonDbDir)
    : Client(clientInterface(project, jsonDbDir)), d(new Private(this))
{
    setName(tr("clangd"));
    LanguageFilter langFilter;
    langFilter.mimeTypes = QStringList{"text/x-chdr", "text/x-csrc",
            "text/x-c++hdr", "text/x-c++src", "text/x-objc++src", "text/x-objcsrc"};
    setSupportedLanguage(langFilter);
    setActivateDocumentAutomatically(true);
    ClientCapabilities caps = Client::defaultClientCapabilities();
    Utils::optional<TextDocumentClientCapabilities> textCaps = caps.textDocument();
    if (textCaps) {
        ClangdTextDocumentClientCapabilities clangdTextCaps(*textCaps);
        clangdTextCaps.clearCompletion();
        clangdTextCaps.clearDocumentHighlight();
        DiagnosticsCapabilities diagnostics;
        diagnostics.enableCategorySupport();
        diagnostics.enableCodeActionsInline();
        clangdTextCaps.setPublishDiagnostics(diagnostics);
        caps.setTextDocument(clangdTextCaps);
    }
    caps.clearExperimental();
    setClientCapabilities(caps);
    setLocatorsEnabled(false);
    setProgressTitleForToken(indexingToken(), tr("Parsing C/C++ Files (clangd)"));
    setCurrentProject(project);

    const auto textMarkCreator = [this](const Utils::FilePath &filePath,
            const Diagnostic &diag) { return new ClangdTextMark(filePath, diag, this); };
    const auto hideDiagsHandler = []{ ClangDiagnosticManager::clearTaskHubIssues(); };
    setDiagnosticsHandlers(textMarkCreator, hideDiagsHandler);

    hoverHandler()->setHelpItemProvider([this](const HoverRequest::Response &response,
                                               const DocumentUri &uri) {
        gatherHelpItemForTooltip(response, uri);
    });

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

    connect(documentSymbolCache(), &DocumentSymbolCache::gotSymbols, this,
            [this](const DocumentUri &uri, const DocumentSymbolsResult &symbols) {
        if (!d->switchDeclDefData || d->switchDeclDefData->uri != uri)
            return;
        d->switchDeclDefData->docSymbols = symbols;
        if (d->switchDeclDefData->ast)
            d->handleDeclDefSwitchReplies();
    });

    start();
}

ClangdClient::~ClangdClient()
{
    if (d->followSymbolData) {
        d->followSymbolData->openedFiles.clear();
        d->followSymbolData->pendingSymbolInfoRequests.clear();
        d->followSymbolData->pendingGotoImplRequests.clear();
        d->followSymbolData->pendingGotoDefRequests.clear();
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
    // TODO: This will be wrong for e.g. operators. Use a Symbol info request to get the real symbol string.
    const QString searchTerm = d->searchTermFromCursor(cursor);
    if (searchTerm.isEmpty())
        return;

    ReferencesData refData;
    refData.key = d->nextJobId++;
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

void ClangdClient::handleDiagnostics(const PublishDiagnosticsParams &params)
{
    const DocumentUri &uri = params.uri();
    Client::handleDiagnostics(params);
    for (const Diagnostic &diagnostic : params.diagnostics()) {
        const ClangdDiagnostic clangdDiagnostic(diagnostic);
        for (const CodeAction &action : clangdDiagnostic.codeActions().value_or(QList<CodeAction>{}))
            LanguageClient::updateCodeActionRefactoringMarker(this, action, uri);
    }
}

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
    const Utils::FilePaths filePaths = TextEditor::BaseFileFind::replaceAll(newSymbolName,
                                                                            checkedItems,
                                                                            preserveCase);
    if (!filePaths.isEmpty())
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
    d->followSymbolData.emplace(this, ++d->nextJobId, cursor, editorWidget,
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
        if (d->followSymbolData->cursorNode)
            d->handleGotoDefinitionResult();
    };
    symbolSupport().findLinkAt(document, cursor, std::move(gotoDefCallback), true);

    if (versionNumber() < QVersionNumber(12)) {
        d->followSymbolData->cursorNode.emplace(AstNode());
        return;
    }

    AstRequest astRequest(AstParams(TextDocumentIdentifier(d->followSymbolData->uri),
                                    Range(cursor)));
    astRequest.setResponseCallback([this, id = d->followSymbolData->id](
                                   const AstRequest::Response &response) {
        qCDebug(clangdLog) << "received ast response for cursor";
        if (!d->followSymbolData || d->followSymbolData->id != id)
            return;
        const auto result = response.result();
        if (result)
            d->followSymbolData->cursorNode = *result;
        else
            d->followSymbolData->cursorNode.emplace(AstNode());
        if (d->followSymbolData->defLink.hasValidTarget())
            d->handleGotoDefinitionResult();
    });
    sendContent(astRequest);
}

void ClangdClient::switchDeclDef(TextEditor::TextDocument *document, const QTextCursor &cursor,
                                 CppTools::CppEditorWidgetInterface *editorWidget,
                                 Utils::ProcessLinkCallback &&callback)
{
    QTC_ASSERT(documentOpen(document), openDocument(document));

    qCDebug(clangdLog) << "switch decl/dev requested" << document->filePath()
                       << cursor.blockNumber() << cursor.positionInBlock();
    d->switchDeclDefData.emplace(++d->nextJobId, document, cursor, editorWidget,
                                 std::move(callback));

    // Retrieve AST and document symbols.
    AstParams astParams;
    astParams.setTextDocument(TextDocumentIdentifier(d->switchDeclDefData->uri));
    AstRequest astRequest(astParams);
    astRequest.setResponseCallback([this, id = d->switchDeclDefData->id]
                                   (const AstRequest::Response &response) {
        qCDebug(clangdLog) << "received ast for decl/def switch";
        if (!d->switchDeclDefData || d->switchDeclDefData->id != id
                || !d->switchDeclDefData->document)
            return;
        const auto result = response.result();
        if (!result) {
            d->switchDeclDefData.reset();
            return;
        }
        d->switchDeclDefData->ast = *result;
        if (d->switchDeclDefData->docSymbols)
            d->handleDeclDefSwitchReplies();

    });
    sendContent(astRequest);
    documentSymbolCache()->requestSymbols(d->switchDeclDefData->uri);

}

void ClangdClient::findLocalUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
        CppTools::RefactoringEngineInterface::RenameCallback &&callback)
{
    QTC_ASSERT(documentOpen(document), openDocument(document));

    qCDebug(clangdLog) << "local references requested" << document->filePath()
                       << (cursor.blockNumber() + 1) << (cursor.positionInBlock() + 1);

    d->localRefsData.emplace(++d->nextJobId, document, cursor, std::move(callback));
    const QString searchTerm = d->searchTermFromCursor(cursor);
    if (searchTerm.isEmpty()) {
        d->localRefsData.reset();
        return;
    }

    // Step 1: Go to definition
    const auto gotoDefCallback = [this, id = d->localRefsData->id](const Utils::Link &link) {
        qCDebug(clangdLog) << "received go to definition response" << link.targetFilePath
                           << link.targetLine << (link.targetColumn + 1);
        if (!d->localRefsData || id != d->localRefsData->id)
            return;
        if (!link.hasValidTarget()) {
            d->localRefsData.reset();
            return;
        }

        // Step 2: Get AST and check whether it's a local variable.
        AstRequest astRequest(AstParams(TextDocumentIdentifier(d->localRefsData->uri)));
        astRequest.setResponseCallback([this, link, id](const AstRequest::Response &response) {
            qCDebug(clangdLog) << "received ast response";
            if (!d->localRefsData || id != d->localRefsData->id)
                return;
            const auto result = response.result();
            if (!result || !d->localRefsData->document) {
                d->localRefsData.reset();
                return;
            }

            const Position linkPos(link.targetLine - 1, link.targetColumn);
            const QList<AstNode> astPath = getAstPath(*result, Range(linkPos, linkPos));
            bool isVar = false;
            for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
                if (it->role() == "declaration" && it->kind() == "Function") {
                    if (!isVar)
                        break;

                    // Step 3: Find references.
                    qCDebug(clangdLog) << "finding references for local var";
                    symbolSupport().findUsages(d->localRefsData->document,
                                               d->localRefsData->cursor,
                                               [this, id](const QList<Location> &locations) {
                        qCDebug(clangdLog) << "found" << locations.size() << "local references";
                        if (!d->localRefsData || id != d->localRefsData->id)
                            return;
                        ClangBackEnd::SourceLocationsContainer container;
                        for (const Location &loc : locations) {
                            container.insertSourceLocation({}, loc.range().start().line() + 1,
                                                           loc.range().start().character() + 1);
                        }

                        // The callback only uses the symbol length, so we just create a dummy.
                        // Note that the calculation will be wrong for identifiers with
                        // embedded newlines, but we've never supported that.
                        QString symbol;
                        if (!locations.isEmpty()) {
                            const Range r = locations.first().range();
                            symbol = QString(r.end().character() - r.start().character(), 'x');
                        }
                        d->localRefsData->callback(symbol, container, d->localRefsData->revision);
                        d->localRefsData->callback = {};
                        d->localRefsData.reset();
                    });
                    return;
                }
                if (!isVar && it->role() == "declaration"
                        && (it->kind() == "Var" || it->kind() == "ParmVar")) {
                    isVar = true;
                }
            }
            d->localRefsData.reset();
        });
        qCDebug(clangdLog) << "sending ast request for link";
        sendContent(astRequest);

    };
    symbolSupport().findLinkAt(document, cursor, std::move(gotoDefCallback), true);
}

void ClangdClient::gatherHelpItemForTooltip(const HoverRequest::Response &hoverResponse,
                                            const DocumentUri &uri)
{
    // Macros aren't locatable via the AST, so parse the formatted string.
    if (const Utils::optional<Hover> result = hoverResponse.result()) {
        const HoverContent content = result->content();
        const MarkupContent * const markup = Utils::get_if<MarkupContent>(&content);
        if (markup) {
            const QString markupString = markup->content();
            static const QString magicMacroPrefix = "### macro `";
            if (markupString.startsWith(magicMacroPrefix)) {
                const int nameStart = magicMacroPrefix.length();
                const int closingQuoteIndex = markupString.indexOf('`', nameStart);
                if (closingQuoteIndex != -1) {
                    const QString macroName = markupString.mid(nameStart,
                                                               closingQuoteIndex - nameStart);
                    d->setHelpItemForTooltip(hoverResponse.id(), macroName, HelpItem::Macro);
                    return;
                }
            }
        }
    }

    AstRequest req((AstParams(TextDocumentIdentifier(uri))));
    req.setResponseCallback([this, uri, hoverResponse](const AstRequest::Response &response) {
        const MessageId id = hoverResponse.id();
        const AstNode ast = response.result().value_or(AstNode());
        const Range range = hoverResponse.result()->range().value_or(Range());
        const QList<AstNode> path = getAstPath(ast, range);
        if (path.isEmpty()) {
            d->setHelpItemForTooltip(id);
            return;
        }
        AstNode node = path.last();
        if (node.role() == "expression" && node.kind() == "ImplicitCast") {
            const Utils::optional<QList<AstNode>> children = node.children();
            if (children && !children->isEmpty())
                node = children->first();
        }
        while (node.kind() == "Qualified") {
            const Utils::optional<QList<AstNode>> children = node.children();
            if (children && !children->isEmpty())
                node = children->first();
        }
        if (clangdLog().isDebugEnabled())
            node.print(0);

        QString type = node.type();
        const auto stripTemplatePartOffType = [&type] {
            const int angleBracketIndex = type.indexOf('<');
            if (angleBracketIndex != -1)
                type = type.left(angleBracketIndex);
        };

        const bool isMemberFunction = node.role() == "expression" && node.kind() == "Member"
                && (node.arcanaContains("member function") || type.contains('('));
        const bool isFunction = node.role() == "expression" && node.kind() == "DeclRef"
                && type.contains('(');
        if (isMemberFunction || isFunction) {
            const TextDocumentPositionParams params(TextDocumentIdentifier(uri), range.start());
            SymbolInfoRequest symReq(params);
            symReq.setResponseCallback([this, id, type, isFunction]
                                       (const SymbolInfoRequest::Response &response) {
                qCDebug(clangdLog) << "handling symbol info reply";
                QString fqn;
                if (const auto result = response.result()) {
                    if (const auto list = Utils::get_if<QList<SymbolDetails>>(&result.value())) {
                        if (!list->isEmpty()) {
                           const SymbolDetails &sd = list->first();
                           fqn = sd.containerName() + sd.name();
                        }
                    }
                }

                // Unfortunately, the arcana string contains the signature only for
                // free functions, so we can't distinguish member function overloads.
                // But since HtmlDocExtractor::getFunctionDescription() is always called
                // with mainOverload = true, such information would get ignored anyway.
                d->setHelpItemForTooltip(id, fqn, HelpItem::Function, isFunction ? type : "()");
            });
            sendContent(symReq);
            return;
        }
        if ((node.role() == "expression" && node.kind() == "DeclRef")
                || (node.role() == "declaration"
                    && (node.kind() == "Var" || node.kind() == "ParmVar"
                        || node.kind() == "Field"))) {
            if (node.arcanaContains("EnumConstant")) {
                d->setHelpItemForTooltip(id, node.detail().value_or(QString()),
                                         HelpItem::Enum, type);
                return;
            }
            stripTemplatePartOffType();
            type.remove("&").remove("*").remove("const ").remove(" const")
                    .remove("volatile ").remove(" volatile");
            type = type.simplified();
            if (type != "int" && !type.contains(" int")
                    && type != "char" && !type.contains(" char")
                    && type != "double" && !type.contains(" double")
                    && type != "float" && type != "bool") {
                d->setHelpItemForTooltip(id, type, node.qdocCategoryForDeclaration(
                                             HelpItem::ClassOrNamespace));
            } else {
                d->setHelpItemForTooltip(id);
            }
            return;
        }
        if (node.isNamespace()) {
            QString ns = node.detail().value_or(QString());
            for (auto it = path.rbegin() + 1; it != path.rend(); ++it) {
                if (it->isNamespace()) {
                    const QString name = it->detail().value_or(QString());
                    if (!name.isEmpty())
                        ns.prepend("::").prepend(name);
                }
            }
            d->setHelpItemForTooltip(hoverResponse.id(), ns, HelpItem::ClassOrNamespace);
            return;
        }
        if (node.role() == "type") {
            if (node.kind() == "Enum") {
                d->setHelpItemForTooltip(id, node.detail().value_or(QString()), HelpItem::Enum);
            } else if (node.kind() == "Record" || node.kind() == "TemplateSpecialization") {
                stripTemplatePartOffType();
                d->setHelpItemForTooltip(id, type, HelpItem::ClassOrNamespace);
            } else if (node.kind() == "Typedef") {
                d->setHelpItemForTooltip(id, type, HelpItem::Typedef);
            } else {
                d->setHelpItemForTooltip(id);
            }
            return;
        }
        if (node.role() == "expression" && node.kind() == "CXXConstruct") {
            const QString name = node.detail().value_or(QString());
            if (!name.isEmpty())
                type = name;
            d->setHelpItemForTooltip(id, type, HelpItem::ClassOrNamespace);
        }
        if (node.role() == "specifier" && node.kind() == "NamespaceAlias") {
            d->setHelpItemForTooltip(id, node.detail().value_or(QString()).chopped(2),
                                     HelpItem::ClassOrNamespace);
            return;
        }
        d->setHelpItemForTooltip(id);
    });
    sendContent(req);
}

void ClangdClient::Private::handleGotoDefinitionResult()
{
    QTC_ASSERT(followSymbolData->defLink.hasValidTarget(), return);

    qCDebug(clangdLog) << "handling go to definition result";

    // No dis-ambiguation necessary. Call back with the link and finish.
    if (!followSymbolData->cursorNode->mightBeAmbiguousVirtualCall()
            && !followSymbolData->cursorNode->isPureVirtualDeclaration()) {
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
    if (!q->documentForFilePath(link.targetFilePath)
        && followSymbolData->openedFiles.insert(link.targetFilePath).second) {
        q->openExtraFile(link.targetFilePath);
    }
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
    //         In addition, we need to follow all override links, because for these, clangd
    //         gives us the declaration instead of the definition.
    for (const Utils::Link &link : qAsConst(followSymbolData->allLinks)) {
        if (!q->documentForFilePath(link.targetFilePath)
                && followSymbolData->openedFiles.insert(link.targetFilePath).second) {
            q->openExtraFile(link.targetFilePath);
        }
        const TextDocumentIdentifier doc(DocumentUri::fromFilePath(link.targetFilePath));
        const Position pos(link.targetLine - 1, link.targetColumn);
        const TextDocumentPositionParams params(doc, pos);
        SymbolInfoRequest symReq(params);
        symReq.setResponseCallback([this, link, id = followSymbolData->id, reqId = symReq.id()](
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
            followSymbolData->virtualFuncAssistProcessor->update();
            if (followSymbolData->pendingSymbolInfoRequests.isEmpty()
                    && followSymbolData->pendingGotoDefRequests.isEmpty()
                    && followSymbolData->defLinkNode.isValid()) {
                handleDocumentInfoResults();
            }
        });
        followSymbolData->pendingSymbolInfoRequests << symReq.id();
        qCDebug(clangdLog) << "sending symbol info request";
        q->sendContent(symReq);

        if (link == followSymbolData->defLink)
            continue;

        GotoDefinitionRequest defReq(params);
        defReq.setResponseCallback([this, link, id = followSymbolData->id, reqId = defReq.id()]
                (const GotoDefinitionRequest::Response &response) {
            qCDebug(clangdLog) << "handling additional go to definition reply for"
                               << link.targetFilePath << link.targetLine;
            if (!followSymbolData || id != followSymbolData->id)
                return;
            Utils::Link newLink;
            if (Utils::optional<GotoResult> _result = response.result()) {
                const GotoResult result = _result.value();
                if (const auto ploc = Utils::get_if<Location>(&result)) {
                    newLink = ploc->toLink();
                } else if (const auto plloc = Utils::get_if<QList<Location>>(&result)) {
                    if (!plloc->isEmpty())
                        newLink = plloc->value(0).toLink();
                }
            }
            qCDebug(clangdLog) << "def link is" << newLink.targetFilePath << newLink.targetLine;
            followSymbolData->declDefMap.insert(link, newLink);
            followSymbolData->pendingGotoDefRequests.removeOne(reqId);
            if (followSymbolData->pendingSymbolInfoRequests.isEmpty()
                    && followSymbolData->pendingGotoDefRequests.isEmpty()
                    && followSymbolData->defLinkNode.isValid()) {
                handleDocumentInfoResults();
            }
        });
        followSymbolData->pendingGotoDefRequests << defReq.id();
        qCDebug(clangdLog) << "sending additional go to definition request"
                           << link.targetFilePath << link.targetLine;
        q->sendContent(defReq);
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
        if (followSymbolData->pendingSymbolInfoRequests.isEmpty()
                && followSymbolData->pendingGotoDefRequests.isEmpty()) {
            handleDocumentInfoResults();
        }
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

void ClangdClient::Private::handleDeclDefSwitchReplies()
{
    if (!switchDeclDefData->document) {
        switchDeclDefData.reset();
        return;
    }

    // Find the function declaration or definition associated with the cursor.
    // For instance, the cursor could be somwehere inside a function body or
    // on a function return type, or ...
    if (clangdLog().isDebugEnabled())
        switchDeclDefData->ast->print(0);
    const Utils::optional<AstNode> functionNode = switchDeclDefData->getFunctionNode();
    if (!functionNode) {
        switchDeclDefData.reset();
        return;
    }

    // Unfortunately, the AST does not contain the location of the actual function name symbol,
    // so we have to look for it in the document symbols.
    const QTextCursor funcNameCursor = switchDeclDefData->cursorForFunctionName(*functionNode);
    if (!funcNameCursor.isNull()) {
        q->followSymbol(switchDeclDefData->document.data(), funcNameCursor,
                        switchDeclDefData->editorWidget, std::move(switchDeclDefData->callback),
                        true, false);
    }
    switchDeclDefData.reset();
}

QString ClangdClient::Private::searchTermFromCursor(const QTextCursor &cursor) const
{
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    return termCursor.selectedText();
}

void ClangdClient::Private::setHelpItemForTooltip(const MessageId &token, const QString &fqn,
                                                  HelpItem::Category category,
                                                  const QString &type)
{
    QStringList helpIds;
    QString mark;
    if (!fqn.isEmpty()) {
        helpIds << fqn;
        int sepSearchStart = 0;
        while (true) {
            sepSearchStart = fqn.indexOf("::", sepSearchStart);
            if (sepSearchStart == -1)
                break;
            sepSearchStart += 2;
            helpIds << fqn.mid(sepSearchStart);
        }
        mark = helpIds.last();
        if (category == HelpItem::Function)
            mark += type.mid(type.indexOf('('));
    }
    if (category == HelpItem::Enum && !type.isEmpty())
        mark = type;

    HelpItem helpItem(helpIds, mark, category);
    if (isTesting)
        emit q->helpItemGathered(helpItem);
    else
        q->hoverHandler()->setHelpItem(token, helpItem);
}

void ClangdClient::VirtualFunctionAssistProcessor::cancel()
{
    resetData();
}

void ClangdClient::VirtualFunctionAssistProcessor::update()
{
    if (!m_data->followSymbolData->isEditorWidgetStillAlive())
        return;
    setAsyncProposalAvailable(createProposal(false));
}

void ClangdClient::VirtualFunctionAssistProcessor::finalize()
{
    if (!m_data->followSymbolData->isEditorWidgetStillAlive())
        return;
    const auto proposal = createProposal(true);
    if (m_data->followSymbolData->editorWidget->inTestMode) {
        m_data->followSymbolData->symbolsToDisplay.clear();
        const auto immediateProposal = createProposal(false);
        m_data->followSymbolData->editorWidget->setProposals(immediateProposal, proposal);
    } else {
        setAsyncProposalAvailable(proposal);
    }
    resetData();
}

void ClangdClient::VirtualFunctionAssistProcessor::resetData()
{
    if (!m_data)
        return;
    m_data->followSymbolData->virtualFuncAssistProcessor = nullptr;
    m_data->followSymbolData.reset();
    m_data = nullptr;
}

TextEditor::IAssistProposal *ClangdClient::VirtualFunctionAssistProcessor::createProposal(bool final) const
{
    QTC_ASSERT(m_data && m_data->followSymbolData, return nullptr);

    QList<TextEditor::AssistProposalItemInterface *> items;
    bool needsBaseDeclEntry = !m_data->followSymbolData->defLinkNode.range()
            .contains(Position(m_data->followSymbolData->cursor));
    for (const SymbolData &symbol : qAsConst(m_data->followSymbolData->symbolsToDisplay)) {
        Utils::Link link = symbol.second;
        if (m_data->followSymbolData->defLink == link) {
            if (!needsBaseDeclEntry)
                continue;
            needsBaseDeclEntry = false;
        } else {
            const Utils::Link defLink = m_data->followSymbolData->declDefMap.value(symbol.second);
            if (defLink.hasValidTarget())
                link = defLink;
        }
        items << createEntry(symbol.first, link);
    }
    if (needsBaseDeclEntry)
        items << createEntry({}, m_data->followSymbolData->defLink);
    if (!final) {
        const auto infoItem = new CppTools::VirtualFunctionProposalItem({}, false);
        infoItem->setText(ClangdClient::tr("collecting overrides ..."));
        infoItem->setOrder(-1);
        items << infoItem;
    }

    return new CppTools::VirtualFunctionProposal(
                m_data->followSymbolData->cursor.position(),
                items, m_data->followSymbolData->openInSplit);
}

CppTools::VirtualFunctionProposalItem *
ClangdClient::VirtualFunctionAssistProcessor::createEntry(const QString &name,
                                                          const Utils::Link &link) const
{
    const auto item = new CppTools::VirtualFunctionProposalItem(
                link, m_data->followSymbolData->openInSplit);
    QString text = name;
    if (link == m_data->followSymbolData->defLink) {
        item->setOrder(1000); // Ensure base declaration is on top.
        if (text.isEmpty()) {
            text = ClangdClient::tr("<base declaration>");
        } else if (m_data->followSymbolData->defLinkNode.isPureVirtualDeclaration()
                   || m_data->followSymbolData->defLinkNode.isPureVirtualDefinition()) {
            text += " = 0";
        }
    }
    item->setText(text);
    return item;
}

TextEditor::IAssistProcessor *ClangdClient::VirtualFunctionAssistProvider::createProcessor() const
{
    return m_data->followSymbolData->virtualFuncAssistProcessor
            = new VirtualFunctionAssistProcessor(m_data);
}

Utils::optional<QList<CodeAction> > ClangdDiagnostic::codeActions() const
{
    return optionalArray<LanguageServerProtocol::CodeAction>("codeActions");
}

QString ClangdDiagnostic::category() const
{
    return typedValue<QString>("category");
}

} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ReplacementData)
