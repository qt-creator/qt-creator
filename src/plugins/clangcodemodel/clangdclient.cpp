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

#include "clangcompletionassistprocessor.h"
#include "clangcompletioncontextanalyzer.h"
#include "clangdiagnosticmanager.h"
#include "clangdqpropertyhighlighter.h"
#include "clangmodelmanagersupport.h"
#include "clangpreprocessorassistproposalitem.h"
#include "clangtextmark.h"
#include "clangutils.h"

#include <clangsupport/sourcelocationscontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultitem.h>
#include <coreplugin/find/searchresultwindow.h>
#include <cplusplus/AST.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Icons.h>
#include <cplusplus/MatchingText.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppcompletionassistprocessor.h>
#include <cppeditor/cppcompletionassistprovider.h>
#include <cppeditor/cppdoxygen.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppfindreferences.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpprefactoringchanges.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppvirtualfunctionassistprovider.h>
#include <cppeditor/cppvirtualfunctionproposalitem.h>
#include <cppeditor/semantichighlighter.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientutils.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <texteditor/basefilefind.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/codeassist/textdocumentmanipulatorinterface.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/itemviews.h>
#include <utils/runextensions.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QCheckBox>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QHeaderView>
#include <QMenu>
#include <QPair>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

#include <cmath>
#include <set>
#include <unordered_map>
#include <utility>

using namespace CPlusPlus;
using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace ClangCodeModel {
namespace Internal {

static Q_LOGGING_CATEGORY(clangdLog, "qtc.clangcodemodel.clangd", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogServer, "qtc.clangcodemodel.clangd.server", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogAst, "qtc.clangcodemodel.clangd.ast", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogHighlight, "qtc.clangcodemodel.clangd.highlight", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogTiming, "qtc.clangcodemodel.clangd.timing", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogCompletion, "qtc.clangcodemodel.clangd.completion",
                          QtWarningMsg);
static QString indexingToken() { return "backgroundIndexProgress"; }

static QStringView subViewLen(const QString &s, qsizetype start, qsizetype length)
{
    if (start < 0 || length < 0 || start + length > s.length())
        return {};
    return QStringView(s).mid(start, length);
}

static QStringView subViewEnd(const QString &s, qsizetype start, qsizetype end)
{
    return subViewLen(s, start, end - start);
}

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

    bool isTemplateParameterDeclaration() const
    {
        return role() == "declaration" && (kind() == "TemplateTypeParm"
                                           || kind() == "NonTypeTemplateParm");
    };

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

        // We don't care about the "inner" type of templates.
        const int openAngleBracketPos = theType.indexOf('<');
        if (openAngleBracketPos != -1) {
            const int closingAngleBracketPos = theType.lastIndexOf('>');
            if (closingAngleBracketPos > openAngleBracketPos) {
                theType = theType.left(openAngleBracketPos)
                        + theType.mid(closingAngleBracketPos + 1);
            }
        }
        const int xrefCount = theType.count("&&");
        const int refCount = theType.count('&') - 2 * xrefCount;
        const int ptrRefCount = theType.count('*') + refCount;
        const int constCount = theType.count("const");
        if (ptrRefCount == 0)
            return constCount > 0 || detailIs("LValueToRValue") || arcanaContains("xvalue");
        return ptrRefCount <= constCount;
    }

    bool childContainsRange(int index, const Range &range) const
    {
        const Utils::optional<QList<AstNode>> childList = children();
        return childList && childList->size() > index
                && childList->at(index).range().contains(range);
    }

    bool hasChildWithRole(const QString &role) const
    {
        return Utils::contains(children().value_or(QList<AstNode>()), [&role](const AstNode &c) {
            return c.role() == role;
        });
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

    enum class FileStatus { Ours, Foreign, Mixed, Unknown };
    FileStatus fileStatus(const Utils::FilePath &thisFile) const
    {
        const Utils::optional<QString> arcanaString = arcana();
        if (!arcanaString)
            return FileStatus::Unknown;

        // Example arcanas:
        // "FunctionDecl 0x7fffb5d0dbd0 </tmp/test.cpp:1:1, line:5:1> line:1:6 func 'void ()'"
        // "VarDecl 0x7fffb5d0dcf0 </tmp/test.cpp:2:5, /tmp/test.h:1:1> /tmp/test.cpp:2:10 b 'bool' cinit"
        // The second one is for a particularly silly construction where the RHS of an
        // initialization comes from an included header.
        const int openPos = arcanaString->indexOf('<');
        if (openPos == -1)
            return FileStatus::Unknown;
        const int closePos = arcanaString->indexOf('>', openPos + 1);
        if (closePos == -1)
            return FileStatus::Unknown;
        bool hasOurs = false;
        bool hasOther = false;
        for (int startPos = openPos + 1; startPos < closePos;) {
            int colon1Pos = arcanaString->indexOf(':', startPos);
            if (colon1Pos == -1 || colon1Pos > closePos)
                break;
            if (Utils::HostOsInfo::isWindowsHost())
                colon1Pos = arcanaString->indexOf(':', colon1Pos + 1);
            if (colon1Pos == -1 || colon1Pos > closePos)
                break;
            const int colon2Pos = arcanaString->indexOf(':', colon1Pos + 2);
            if (colon2Pos == -1 || colon2Pos > closePos)
                break;
            const int line = subViewEnd(*arcanaString, colon1Pos + 1, colon2Pos).toString().toInt(); // TODO: Drop toString() once we require >= Qt 5.15
            if (line == 0)
                break;
            const QStringView fileOrLineString = subViewEnd(*arcanaString, startPos, colon1Pos);
            if (fileOrLineString != QLatin1String("line")) {
                if (Utils::FilePath::fromUserInput(fileOrLineString.toString()) == thisFile)
                    hasOurs = true;
                else
                    hasOther = true;
            }
            const int commaPos = arcanaString->indexOf(',', colon2Pos + 2);
            if (commaPos != -1)
                startPos = commaPos + 2;
            else
                break;
        }
        if (hasOurs)
            return hasOther ? FileStatus::Mixed : FileStatus::Ours;
        return hasOther ? FileStatus::Foreign : FileStatus::Unknown;
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

class AstPathCollector
{
public:
    AstPathCollector(const AstNode &root, const Range &range) : m_root(root), m_range(range) {}

    QList<AstNode> collectPath()
    {
        if (!m_root.isValid())
            return {};
        visitNode(m_root, true);
        return m_done ? m_path : m_longestSubPath;
    }

private:
    void visitNode(const AstNode &node, bool isRoot = false)
    {
        if (!isRoot && (!node.hasRange() || !node.range().contains(m_range)))
            return;
        m_path << node;

        class PathDropper {
        public:
            PathDropper(AstPathCollector &collector) : m_collector(collector) {};
            ~PathDropper() {
                if (m_collector.m_done)
                    return;
                if (m_collector.m_path.size() > m_collector.m_longestSubPath.size())
                    m_collector.m_longestSubPath = m_collector.m_path;
                m_collector.m_path.removeLast();
            }
        private:
            AstPathCollector &m_collector;
        } pathDropper(*this);

        // Still traverse the children, because they could have the same range.
        if (node.range() == m_range)
            m_done = true;

        const auto children = node.children();
        if (!children)
            return;

        QList<AstNode> childrenToCheck;
        if (node.kind() == "Function" || node.role() == "expression") {
            // Functions and expressions can contain implicit nodes that make the list unsorted.
            // They cannot be ignored, as we need to consider them in certain contexts.
            // Therefore, the binary search cannot be used here.
            childrenToCheck = *children;
        } else {
            for (auto it = std::lower_bound(children->cbegin(), children->cend(), m_range,
                                            leftOfRange);
                 it != children->cend() && !m_range.isLeftOf(it->range()); ++it) {
                childrenToCheck << *it;
            }
        }

        const bool wasDone = m_done;
        for (const AstNode &child : qAsConst(childrenToCheck)) {
            visitNode(child);
            if (m_done && !wasDone)
                break;
        }
    }

    static bool leftOfRange(const AstNode &node, const Range &range)
    {
        // Class and struct nodes can contain implicit constructors, destructors and
        // operators, which appear at the end of the list, but whose range is the same
        // as the class name. Therefore, we must force them not to compare less to
        // anything else.
        return node.range().isLeftOf(range) && !node.arcanaContains(" implicit ");
    };

    const AstNode &m_root;
    const Range &m_range;
    QList<AstNode> m_path;
    QList<AstNode> m_longestSubPath;
    bool m_done = false;
};

static QList<AstNode> getAstPath(const AstNode &root, const Range &range)
{
    return AstPathCollector(root, range).collectPath();
}

static QList<AstNode> getAstPath(const AstNode &root, const Position &pos)
{
    return getAstPath(root, Range(pos, pos));
}

static Usage::Type getUsageType(const QList<AstNode> &path)
{
    bool potentialWrite = false;
    bool isFunction = false;
    const bool symbolIsDataType = path.last().role() == "type" && path.last().kind() == "Record";
    const auto isPotentialWrite = [&] { return potentialWrite && !isFunction; };
    for (auto pathIt = path.rbegin(); pathIt != path.rend(); ++pathIt) {
        if (pathIt->arcanaContains("non_odr_use_unevaluated"))
            return Usage::Type::Other;
        if (pathIt->kind() == "CXXDelete")
            return Usage::Type::Write;
        if (pathIt->kind() == "CXXNew")
            return Usage::Type::Other;
        if (pathIt->kind() == "Switch" || pathIt->kind() == "If")
            return Usage::Type::Read;
        if (pathIt->kind() == "Call")
            return isFunction ? Usage::Type::Other
                              : potentialWrite ? Usage::Type::WritableRef : Usage::Type::Read;
        if (pathIt->kind() == "CXXMemberCall") {
            const auto children = pathIt->children();
            if (children && children->size() == 1
                    && children->first() == path.last()
                    && children->first().arcanaContains("bound member function")) {
                return Usage::Type::Other;
            }
            return isPotentialWrite() ? Usage::Type::WritableRef : Usage::Type::Read;
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
                return Usage::Type::Other;
            if (pathIt->arcanaContains("cinit")) {
                if (pathIt == path.rbegin())
                    return Usage::Type::Initialization;
                if (pathIt->childContainsRange(0, path.last().range()))
                    return Usage::Type::Initialization;
                if (isFunction)
                    return Usage::Type::Read;
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
                return isPotentialWrite() ? Usage::Type::WritableRef : Usage::Type::Read;
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

void setupClangdConfigFile()
{
    const Utils::FilePath baseDir = Utils::FilePath::fromString(
                QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)) / "clangd";
    baseDir.ensureWritableDir();
    const Utils::FilePath targetConfigFile = baseDir / "config.yaml";
    Utils::FileReader configReader;
    const QByteArray firstLine = "# This file was generated by Qt Creator and will be overwritten "
                                 "unless you remove this line.";
    if (!configReader.fetch(targetConfigFile) || configReader.data().startsWith(firstLine)) {
        Utils::FileSaver saver(targetConfigFile);
        saver.write(firstLine + '\n');
        saver.write("Hover:\n");
        saver.write("  ShowAKA: Yes\n");
        QTC_CHECK(saver.finalize());
    }
}

static BaseClientInterface *clientInterface(Project *project, const Utils::FilePath &jsonDbDir)
{
    QString indexingOption = "--background-index";
    const CppEditor::ClangdSettings settings(CppEditor::ClangdProjectSettings(project).settings());
    if (!settings.indexingEnabled() || jsonDbDir.isEmpty())
        indexingOption += "=0";
    const QString headerInsertionOption = QString("--header-insertion=")
            + (settings.autoIncludeHeaders() ? "iwyu" : "never");
    Utils::CommandLine cmd{settings.clangdFilePath(), {indexingOption, headerInsertionOption,
            "--limit-results=0", "--limit-references=0", "--clang-tidy=0"}};
    if (settings.workerThreadLimit() != 0)
        cmd.addArg("-j=" + QString::number(settings.workerThreadLimit()));
    if (!jsonDbDir.isEmpty())
        cmd.addArg("--compile-commands-dir=" + jsonDbDir.toString());
    if (clangdLogServer().isDebugEnabled())
        cmd.addArgs({"--log=verbose", "--pretty"});
    if (settings.clangdVersion() >= QVersionNumber(14))
        cmd.addArg("--use-dirty-headers");
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
    bool categorize = CppEditor::codeModelSettings()->categorizeFindReferences();
};

using SymbolData = QPair<QString, Utils::Link>;
using SymbolDataList = QList<SymbolData>;

class ClangdClient::VirtualFunctionAssistProcessor : public IAssistProcessor
{
public:
    VirtualFunctionAssistProcessor(ClangdClient::Private *data) : m_data(data) {}

    void cancel() override;
    bool running() override { return m_data; }

    void update();
    void finalize();

private:
    IAssistProposal *perform(const AssistInterface *) override
    {
        return nullptr;
    }

    IAssistProposal *immediateProposal(const AssistInterface *) override
    {
        return createProposal(false);
    }

    void resetData();

    IAssistProposal *immediateProposalImpl() const;
    IAssistProposal *createProposal(bool final) const;
    CppEditor::VirtualFunctionProposalItem *createEntry(const QString &name,
                                                       const Utils::Link &link) const;

    ClangdClient::Private *m_data = nullptr;
};

class ClangdClient::VirtualFunctionAssistProvider : public IAssistProvider
{
public:
    VirtualFunctionAssistProvider(ClangdClient::Private *data) : m_data(data) {}

private:
    RunType runType() const override { return Asynchronous; }
    IAssistProcessor *createProcessor(const AssistInterface *) const override;

    ClangdClient::Private * const m_data;
};

class ClangdClient::FollowSymbolData {
public:
    FollowSymbolData(ClangdClient *q, quint64 id, const QTextCursor &cursor,
                     CppEditor::CppEditorWidget *editorWidget,
                     const DocumentUri &uri, Utils::ProcessLinkCallback &&callback,
                     bool openInSplit)
        : q(q), id(id), cursor(cursor), editorWidget(editorWidget), uri(uri),
          callback(std::move(callback)), virtualFuncAssistProvider(q->d),
          docRevision(editorWidget ? editorWidget->textDocument()->document()->revision() : -1),
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
        for (const Utils::FilePath &fp : qAsConst(openedFiles)) {
            if (!q->documentForFilePath(fp))
                q->closeExtraFile(fp);
        }
        openedFiles.clear();
    }

    bool defLinkIsAmbiguous() const;

    ClangdClient * const q;
    const quint64 id;
    const QTextCursor cursor;
    const QPointer<CppEditor::CppEditorWidget> editorWidget;
    const DocumentUri uri;
    const Utils::ProcessLinkCallback callback;
    VirtualFunctionAssistProvider virtualFuncAssistProvider;
    QList<MessageId> pendingSymbolInfoRequests;
    QList<MessageId> pendingGotoImplRequests;
    QList<MessageId> pendingGotoDefRequests;
    const int docRevision;
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
    SwitchDeclDefData(quint64 id, TextDocument *doc, const QTextCursor &cursor,
                      CppEditor::CppEditorWidget *editorWidget,
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
    const QPointer<TextDocument> document;
    const DocumentUri uri;
    const QTextCursor cursor;
    const QPointer<CppEditor::CppEditorWidget> editorWidget;
    Utils::ProcessLinkCallback callback;
    Utils::optional<DocumentSymbolsResult> docSymbols;
    Utils::optional<AstNode> ast;
};

class LocalRefsData {
public:
    LocalRefsData(quint64 id, TextDocument *doc, const QTextCursor &cursor,
                  CppEditor::RefactoringEngineInterface::RenameCallback &&callback)
        : id(id), document(doc), cursor(cursor), callback(std::move(callback)),
          uri(DocumentUri::fromFilePath(doc->filePath())), revision(doc->document()->revision())
    {}

    ~LocalRefsData()
    {
        if (callback)
            callback({}, {}, revision);
    }

    const quint64 id;
    const QPointer<TextDocument> document;
    const QTextCursor cursor;
    CppEditor::RefactoringEngineInterface::RenameCallback callback;
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


enum class CustomAssistMode { Doxygen, Preprocessor, IncludePath };
class CustomAssistProcessor : public IAssistProcessor
{
public:
    CustomAssistProcessor(ClangdClient *client, int position, int endPos,
                          unsigned completionOperator, CustomAssistMode mode)
        : m_client(client)
        , m_position(position)
        , m_endPos(endPos)
        , m_completionOperator(completionOperator)
        , m_mode(mode)
    {}

private:
    IAssistProposal *perform(const AssistInterface *interface) override
    {
        QList<AssistProposalItemInterface *> completions;
        switch (m_mode) {
        case CustomAssistMode::Doxygen:
            for (int i = 1; i < CppEditor::T_DOXY_LAST_TAG; ++i) {
                completions << createItem(QLatin1String(CppEditor::doxygenTagSpell(i)),
                                          CPlusPlus::Icons::keywordIcon());
            }
            break;
        case CustomAssistMode::Preprocessor: {
            static QIcon macroIcon = Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Macro);
            for (const QString &completion
                 : CppEditor::CppCompletionAssistProcessor::preprocessorCompletions()) {
                completions << createItem(completion, macroIcon);
            }
            if (CppEditor::ProjectFile::isObjC(interface->filePath().toString()))
                completions << createItem("import", macroIcon);
            break;
        }
        case ClangCodeModel::Internal::CustomAssistMode::IncludePath: {
            HeaderPaths headerPaths;
            const CppEditor::ProjectPart::ConstPtr projectPart
                    = projectPartForFile(interface->filePath().toString());
            if (projectPart)
                headerPaths = projectPart->headerPaths;
            completions = ClangCompletionAssistProcessor::completeInclude(
                        m_endPos, m_completionOperator, interface, headerPaths);
            break;
        }
        }
        GenericProposalModelPtr model(new GenericProposalModel);
        model->loadContent(completions);
        const auto proposal = new GenericProposal(m_position, model);
        if (m_client->testingEnabled()) {
            emit m_client->proposalReady(proposal);
            return nullptr;
        }
        return proposal;
    }

    AssistProposalItemInterface *createItem(const QString &text, const QIcon &icon) const
    {
        const auto item = new ClangPreprocessorAssistProposalItem;
        item->setText(text);
        item->setIcon(icon);
        item->setCompletionOperator(m_completionOperator);
        return item;
    }

    ClangdClient * const m_client;
    const int m_position;
    const int m_endPos;
    const unsigned m_completionOperator;
    const CustomAssistMode m_mode;
};

static qint64 getRevision(const TextDocument *doc)
{
    return doc->document()->revision();
}
static qint64 getRevision(const Utils::FilePath &fp)
{
    return fp.lastModified().toMSecsSinceEpoch();
}

template<typename DocType, typename DataType> class VersionedDocData
{
public:
    VersionedDocData(const DocType &doc, const DataType &data) :
        revision(getRevision(doc)), data(data) {}

    const qint64 revision;
    const DataType data;
};

template<typename DocType, typename DataType> class VersionedDataCache
{
public:
    void insert(const DocType &doc, const DataType &data)
    {
        m_data.emplace(std::make_pair(doc, VersionedDocData(doc, data)));
    }
    void remove(const DocType &doc) { m_data.erase(doc); }
    Utils::optional<VersionedDocData<DocType, DataType>> take(const DocType &doc)
    {
        const auto it = m_data.find(doc);
        if (it == m_data.end())
            return {};
        const auto data = it->second;
        m_data.erase(it);
        return data;
    }
    Utils::optional<DataType> get(const DocType &doc)
    {
        const auto it = m_data.find(doc);
        if (it == m_data.end())
            return {};
        if (it->second.revision != getRevision(doc)) {
            m_data.erase(it);
            return {};
        }
        return it->second.data;
    }
private:
    std::unordered_map<DocType, VersionedDocData<DocType, DataType>> m_data;
};

class TaskTimer
{
public:
    TaskTimer(const QString &task) : m_task(task) {}

    void stopTask()
    {
        // This can happen due to the RAII mechanism employed with SubtaskTimer.
        // The subtask timers will expire immediately after, so this does not distort
        // the timing data.
        if (m_subtasks > 0) {
            QTC_CHECK(m_timer.isValid());
            m_elapsedMs += m_timer.elapsed();
            m_timer.invalidate();
            m_subtasks = 0;
        }
        m_started = false;
        qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": took " << m_elapsedMs
                                                     << " ms in UI thread";
        m_elapsedMs = 0;
    }
    void startSubtask()
    {
        // We have some callbacks that are either synchronous or asynchronous, depending on
        // dynamic conditions. In the sync case, we will have nested subtasks, in which case
        // the inner ones must not collect timing data, as their code blocks are already covered.
        if (++m_subtasks > 1)
            return;
        if (!m_started) {
            QTC_ASSERT(m_elapsedMs == 0, m_elapsedMs = 0);
            m_started = true;
            m_finalized = false;
            qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": starting";
        }
        qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": subtask started at "
                                                     << QDateTime::currentDateTime().toString();
        QTC_CHECK(!m_timer.isValid());
        m_timer.start();
    }
    void stopSubtask(bool isFinalizing)
    {
        if (m_subtasks == 0) // See stopTask().
            return;
        if (isFinalizing)
            m_finalized = true;
        if (--m_subtasks > 0) // See startSubtask().
            return;
        qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": subtask stopped at "
                                                     << QDateTime::currentDateTime().toString();
        QTC_CHECK(m_timer.isValid());
        m_elapsedMs += m_timer.elapsed();
        m_timer.invalidate();
        if (m_finalized)
            stopTask();
    }

private:
    const QString m_task;
    QElapsedTimer m_timer;
    qint64 m_elapsedMs = 0;
    int m_subtasks = 0;
    bool m_started = false;
    bool m_finalized = false;
};

class SubtaskTimer
{
public:
    SubtaskTimer(TaskTimer &timer) : m_timer(timer) { m_timer.startSubtask(); }
    ~SubtaskTimer() { m_timer.stopSubtask(m_isFinalizing); }

protected:
    void makeFinalizing() { m_isFinalizing = true; }

private:
    TaskTimer &m_timer;
    bool m_isFinalizing = false;
};

class FinalizingSubtaskTimer : public SubtaskTimer
{
public:
    FinalizingSubtaskTimer(TaskTimer &timer) : SubtaskTimer(timer) { makeFinalizing(); }
};

class ThreadedSubtaskTimer
{
public:
    ThreadedSubtaskTimer(const QString &task) : m_task(task)
    {
        qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": starting thread";
        m_timer.start();
    }

    ~ThreadedSubtaskTimer()
    {
        qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": took " << m_timer.elapsed()
                                                     << " ms in dedicated thread";
    }
private:
    const QString m_task;
    QElapsedTimer m_timer;
};

class MemoryTreeModel;
class MemoryUsageWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(MemoryUsageWidget)
public:
    MemoryUsageWidget(ClangdClient *client);
    ~MemoryUsageWidget();

private:
    void setupUi();
    void getMemoryTree();

    ClangdClient * const m_client;
    MemoryTreeModel * const m_model;
    Utils::TreeView m_view;
    Utils::optional<MessageId> m_currentRequest;
};

class ClangdClient::Private
{
public:
    Private(ClangdClient *q, Project *project)
        : q(q), settings(CppEditor::ClangdProjectSettings(project).settings()) {}

    void findUsages(TextDocument *document, const QTextCursor &cursor,
                    const QString &searchTerm, const Utils::optional<QString> &replacement,
                    bool categorize);
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

    void handleDeclDefSwitchReplies();

    static CppEditor::CppEditorWidget *widgetFromDocument(const TextDocument *doc);
    QString searchTermFromCursor(const QTextCursor &cursor) const;
    QTextCursor adjustedCursor(const QTextCursor &cursor, const TextDocument *doc);

    void setHelpItemForTooltip(const MessageId &token, const QString &fqn = {},
                               HelpItem::Category category = HelpItem::Unknown,
                               const QString &type = {});

    void handleSemanticTokens(TextDocument *doc, const QList<ExpandedSemanticToken> &tokens,
                              int version, bool force);

    enum class AstCallbackMode { SyncIfPossible, AlwaysAsync };
    using TextDocOrFile = const Utils::variant<const TextDocument *, Utils::FilePath>;
    using AstHandler = const std::function<void(const AstNode &ast, const MessageId &)>;
    MessageId getAndHandleAst(TextDocOrFile &doc, AstHandler &astHandler,
                              AstCallbackMode callbackMode, const Range &range = {});

    ClangdClient * const q;
    const CppEditor::ClangdSettings::Data settings;
    QHash<quint64, ReferencesData> runningFindUsages;
    Utils::optional<FollowSymbolData> followSymbolData;
    Utils::optional<SwitchDeclDefData> switchDeclDefData;
    Utils::optional<LocalRefsData> localRefsData;
    Utils::optional<QVersionNumber> versionNumber;

    //  The highlighters are owned by their respective documents.
    std::unordered_map<TextDocument *, CppEditor::SemanticHighlighter *> highlighters;

    QHash<TextDocument *, QPair<QList<ExpandedSemanticToken>, int>> previousTokens;
    QHash<Utils::FilePath, CppEditor::BaseEditorDocumentParser::Configuration> parserConfigs;

    // The ranges of symbols referring to virtual functions, with document version,
    // as extracted by the highlighting procedure.
    QHash<TextDocument *, QPair<QList<Range>, int>> virtualRanges;

    VersionedDataCache<const TextDocument *, AstNode> astCache;
    VersionedDataCache<Utils::FilePath, AstNode> externalAstCache;
    TaskTimer highlightingTimer{"highlighting"};
    quint64 nextJobId = 0;
    bool isFullyIndexed = false;
    bool isTesting = false;
};

class ClangdCompletionCapabilities : public TextDocumentClientCapabilities::CompletionCapabilities
{
public:
    explicit ClangdCompletionCapabilities(const JsonObject &object)
        : TextDocumentClientCapabilities::CompletionCapabilities(object)
    {
        insert("editsNearCursor", true); // For dot-to-arrow correction.
        if (Utils::optional<CompletionItemCapbilities> completionItemCaps = completionItem()) {
            completionItemCaps->setSnippetSupport(false);
            setCompletionItem(*completionItemCaps);
        }
    }
};

class ClangdCompletionItem : public LanguageClientCompletionItem
{
public:
    using LanguageClientCompletionItem::LanguageClientCompletionItem;
    void apply(TextDocumentManipulatorInterface &manipulator,
               int basePosition) const override;

    enum class SpecialQtType { Signal, Slot, None };
    static SpecialQtType getQtType(const CompletionItem &item);

private:
    QIcon icon() const override;
};

class ClangdClient::ClangdCompletionAssistProcessor : public LanguageClientCompletionAssistProcessor
{
public:
    ClangdCompletionAssistProcessor(ClangdClient *client, const QString &snippetsGroup)
        : LanguageClientCompletionAssistProcessor(client, snippetsGroup)
        , m_client(client)
    {
    }

private:
    IAssistProposal *perform(const AssistInterface *interface) override
    {
        if (m_client->d->isTesting) {
            setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal) {
                emit m_client->proposalReady(proposal);
            });
        }
        return LanguageClientCompletionAssistProcessor::perform(interface);
    }

    QList<AssistProposalItemInterface *> generateCompletionItems(
        const QList<LanguageServerProtocol::CompletionItem> &items) const override;

    ClangdClient * const m_client;
};

QList<AssistProposalItemInterface *>
ClangdClient::ClangdCompletionAssistProcessor::generateCompletionItems(
    const QList<LanguageServerProtocol::CompletionItem> &items) const
{
    qCDebug(clangdLog) << "received" << items.count() << "completions";

    auto itemGenerator = [](const QList<LanguageServerProtocol::CompletionItem> &items) {
        return Utils::transform<QList<AssistProposalItemInterface *>>(
            items, [](const LanguageServerProtocol::CompletionItem &item) {
                return new ClangdCompletionItem(item);
            });
    };

    // If there are signals among the candidates, we employ the built-in code model to find out
    // whether the cursor was on the second argument of a (dis)connect() call.
    // If so, we offer only signals, as nothing else makes sense in that context.
    static const auto criterion = [](const CompletionItem &ci) {
        return ClangdCompletionItem::getQtType(ci) == ClangdCompletionItem::SpecialQtType::Signal;
    };
    const QTextDocument *doc = document();
    const int pos = basePos();
    if (!doc || pos < 0 || !Utils::anyOf(items, criterion))
        return itemGenerator(items);
    const QString content = doc->toPlainText();
    const bool requiresSignal = CppEditor::CppModelManager::instance()
                                    ->positionRequiresSignal(filePath().toString(),
                                                             content.toUtf8(),
                                                             pos);
    if (requiresSignal)
        return itemGenerator(Utils::filtered(items, criterion));
    return itemGenerator(items);
}

class ClangdClient::ClangdCompletionAssistProvider : public LanguageClientCompletionAssistProvider
{
public:
    ClangdCompletionAssistProvider(ClangdClient *client);

private:
    IAssistProcessor *createProcessor(const AssistInterface *interface) const override;

    int activationCharSequenceLength() const override { return 3; }
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    bool isInCommentOrString(const AssistInterface *interface) const;

    ClangdClient * const m_client;
};

class ClangdQuickFixProcessor : public LanguageClientQuickFixAssistProcessor
{
public:
    ClangdQuickFixProcessor(LanguageClient::Client *client)
        : LanguageClientQuickFixAssistProcessor(client)
    {
    }

private:
    IAssistProposal *perform(const AssistInterface *interface) override
    {
        m_interface = interface;

        // Step 1: Collect clangd code actions asynchronously
        LanguageClientQuickFixAssistProcessor::perform(interface);

        // Step 2: Collect built-in quickfixes synchronously
        m_builtinOps = CppEditor::quickFixOperations(interface);

        return nullptr;
    }

    void handleProposalReady(const QuickFixOperations &ops) override
    {
        // Step 3: Merge the results upon callback from clangd.
        for (const auto &op : ops)
            op->setDescription("clangd: " + op->description());
        setAsyncProposalAvailable(GenericProposal::createProposal(m_interface, ops + m_builtinOps));
    }

    QuickFixOperations m_builtinOps;
    const AssistInterface *m_interface = nullptr;
};

class ClangdQuickFixProvider : public LanguageClientQuickFixProvider
{
public:
    ClangdQuickFixProvider(ClangdClient *client) : LanguageClientQuickFixProvider(client) {};

private:
    IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override
    {
        return new ClangdQuickFixProcessor(client());
    }
};

ClangdClient::ClangdClient(Project *project, const Utils::FilePath &jsonDbDir)
    : Client(clientInterface(project, jsonDbDir)), d(new Private(this, project))
{
    setName(tr("clangd"));
    LanguageFilter langFilter;
    langFilter.mimeTypes = QStringList{"text/x-chdr", "text/x-csrc",
            "text/x-c++hdr", "text/x-c++src", "text/x-objc++src", "text/x-objcsrc"};
    setSupportedLanguage(langFilter);
    setActivateDocumentAutomatically(true);
    setLogTarget(LogTarget::Console);
    setCompletionAssistProvider(new ClangdCompletionAssistProvider(this));
    setQuickFixAssistProvider(new ClangdQuickFixProvider(this));
    if (!project) {
        QJsonObject initOptions;
        const QStringList clangOptions = createClangOptions(
                    *CppEditor::CppModelManager::instance()->fallbackProjectPart(), {},
                    warningsConfigForProject(nullptr), optionsForProject(nullptr));
        initOptions.insert("fallbackFlags", QJsonArray::fromStringList(clangOptions));
        setInitializationOptions(initOptions);
    }
    auto isRunningClangdClient = [](const LanguageClient::Client *c) {
        return qobject_cast<const ClangdClient *>(c) && c->state() != Client::ShutdownRequested
               && c->state() != Client::Shutdown;
    };
    const QList<Client *> clients =
        Utils::filtered(LanguageClientManager::clientsForProject(project), isRunningClangdClient);
    QTC_CHECK(clients.isEmpty());
    for (const Client *client : clients)
        qCWarning(clangdLog) << client->name() << client->stateString();
    ClientCapabilities caps = Client::defaultClientCapabilities();
    Utils::optional<TextDocumentClientCapabilities> textCaps = caps.textDocument();
    if (textCaps) {
        ClangdTextDocumentClientCapabilities clangdTextCaps(*textCaps);
        clangdTextCaps.clearDocumentHighlight();
        DiagnosticsCapabilities diagnostics;
        diagnostics.enableCategorySupport();
        diagnostics.enableCodeActionsInline();
        clangdTextCaps.setPublishDiagnostics(diagnostics);
        Utils::optional<TextDocumentClientCapabilities::CompletionCapabilities> completionCaps
                = textCaps->completion();
        if (completionCaps)
            clangdTextCaps.setCompletion(ClangdCompletionCapabilities(*completionCaps));
        caps.setTextDocument(clangdTextCaps);
    }
    caps.clearExperimental();
    setClientCapabilities(caps);
    setLocatorsEnabled(false);
    setAutoRequestCodeActions(false); // clangd sends code actions inside diagnostics
    setProgressTitleForToken(indexingToken(), tr("Parsing C/C++ Files (clangd)"));
    setCurrentProject(project);
    setDocumentChangeUpdateThreshold(d->settings.documentUpdateThreshold);

    const auto textMarkCreator = [this](const Utils::FilePath &filePath,
            const Diagnostic &diag) {
        if (d->isTesting)
            emit textMarkCreated(filePath);
        return new ClangdTextMark(filePath, diag, this);
    };
    const auto hideDiagsHandler = []{ ClangDiagnosticManager::clearTaskHubIssues(); };
    setDiagnosticsHandlers(textMarkCreator, hideDiagsHandler);
    setSymbolStringifier(displayNameFromDocumentSymbol);
    setSemanticTokensHandler([this](TextDocument *doc, const QList<ExpandedSemanticToken> &tokens,
                                    int version, bool force) {
        d->handleSemanticTokens(doc, tokens, version, force);
    });
    hoverHandler()->setHelpItemProvider([this](const HoverRequest::Response &response,
                                               const DocumentUri &uri) {
        gatherHelpItemForTooltip(response, uri);
    });

    connect(this, &Client::workDone, this,
            [this, p = QPointer(project)](const ProgressToken &token) {
        const QString * const val = Utils::get_if<QString>(&token);
        if (val && *val == indexingToken()) {
            d->isFullyIndexed = true;
            emit indexingFinished();
#ifdef WITH_TESTS
            if (p)
                emit p->indexingFinished("Indexer.Clangd");
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
    sendContent(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)),
                SendDocUpdates::Ignore);
}

void ClangdClient::closeExtraFile(const Utils::FilePath &filePath)
{
    sendContent(DidCloseTextDocumentNotification(DidCloseTextDocumentParams(
            TextDocumentIdentifier{DocumentUri::fromFilePath(filePath)})),
                SendDocUpdates::Ignore);
}

void ClangdClient::findUsages(TextDocument *document, const QTextCursor &cursor,
                              const Utils::optional<QString> &replacement)
{
    // Quick check: Are we even on anything searchable?
    const QString searchTerm = d->searchTermFromCursor(cursor);
    if (searchTerm.isEmpty())
        return;

    const QTextCursor adjustedCursor = d->adjustedCursor(cursor, document);
    const bool categorize = CppEditor::codeModelSettings()->categorizeFindReferences();

    // If it's a "normal" symbol, go right ahead.
    if (searchTerm != "operator" && Utils::allOf(searchTerm, [](const QChar &c) {
            return c.isLetterOrNumber() || c == '_';
    })) {
        d->findUsages(document, adjustedCursor, searchTerm, replacement, categorize);
        return;
    }

    // Otherwise get the proper spelling of the search term from clang, so we can put it into the
    // search widget.
    const TextDocumentIdentifier docId(DocumentUri::fromFilePath(document->filePath()));
    const TextDocumentPositionParams params(docId, Range(adjustedCursor).start());
    SymbolInfoRequest symReq(params);
    symReq.setResponseCallback([this, doc = QPointer(document), adjustedCursor, replacement, categorize]
                               (const SymbolInfoRequest::Response &response) {
        if (!doc)
            return;
        const auto result = response.result();
        if (!result)
            return;
        const auto list = Utils::get_if<QList<SymbolDetails>>(&result.value());
        if (!list || list->isEmpty())
            return;
        const SymbolDetails &sd = list->first();
        if (sd.name().isEmpty())
            return;
        d->findUsages(doc.data(), adjustedCursor, sd.name(), replacement, categorize);
    });
    sendContent(symReq);
}

void ClangdClient::handleDiagnostics(const PublishDiagnosticsParams &params)
{
    const DocumentUri &uri = params.uri();
    Client::handleDiagnostics(params);
    const int docVersion = documentVersion(uri.toFilePath());
    if (params.version().value_or(docVersion) != docVersion)
        return;
    for (const Diagnostic &diagnostic : params.diagnostics()) {
        const ClangdDiagnostic clangdDiagnostic(diagnostic);
        for (const CodeAction &action : clangdDiagnostic.codeActions().value_or(QList<CodeAction>{}))
            LanguageClient::updateCodeActionRefactoringMarker(this, action, uri);
    }
}

void ClangdClient::handleDocumentOpened(TextDocument *doc)
{
    const auto data = d->externalAstCache.take(doc->filePath());
    if (!data)
        return;
    if (data->revision == getRevision(doc->filePath()))
       d->astCache.insert(doc, data->data);
}

void ClangdClient::handleDocumentClosed(TextDocument *doc)
{
    d->highlighters.erase(doc);
    d->astCache.remove(doc);
    d->previousTokens.remove(doc);
    d->virtualRanges.remove(doc);
    d->parserConfigs.remove(doc->filePath());
}

QTextCursor ClangdClient::adjustedCursorForHighlighting(const QTextCursor &cursor,
                                                        TextEditor::TextDocument *doc)
{
    return d->adjustedCursor(cursor, doc);
}

const LanguageClient::Client::CustomInspectorTabs ClangdClient::createCustomInspectorTabs()
{
    return {std::make_pair(new MemoryUsageWidget(this), tr("Memory Usage"))};
}

RefactoringChangesData *ClangdClient::createRefactoringChangesBackend() const
{
    return new CppEditor::CppRefactoringChangesData(
                CppEditor::CppModelManager::instance()->snapshot());
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

CppEditor::ClangdSettings::Data ClangdClient::settingsData() const { return d->settings; }

void ClangdClient::Private::findUsages(TextDocument *document,
        const QTextCursor &cursor, const QString &searchTerm,
        const Utils::optional<QString> &replacement, bool categorize)
{
    ReferencesData refData;
    refData.key = nextJobId++;
    refData.categorize = categorize;
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
        refData.search->setFilter(new CppEditor::CppSearchResultFilter);
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
    runningFindUsages.insert(refData.key, refData);

    const Utils::optional<MessageId> requestId = q->symbolSupport().findUsages(
                document, cursor, [this, key = refData.key](const QList<Location> &locations) {
        handleFindUsagesResult(key, locations);
    });

    if (!requestId) {
        finishSearch(refData, false);
        return;
    }
    QObject::connect(refData.search, &SearchResult::cancelled, q, [this, requestId, key = refData.key] {
        const auto refData = runningFindUsages.find(key);
        if (refData == runningFindUsages.end())
            return;
        q->cancelRequest(*requestId);
        refData->canceled = true;
        refData->search->disconnect(q);
        finishSearch(*refData, true);
    });
}

void ClangdClient::enableTesting()
{
    d->isTesting = true;
}

bool ClangdClient::testingEnabled() const
{
    return d->isTesting;
}

QString ClangdClient::displayNameFromDocumentSymbol(SymbolKind kind, const QString &name,
                                                    const QString &detail)
{
    switch (kind) {
    case SymbolKind::Constructor:
        return name + detail;
    case SymbolKind::Method:
    case LanguageServerProtocol::SymbolKind::Function: {
        const int parenOffset = detail.indexOf(" (");
        if (parenOffset == -1)
            return name;
        return name + detail.mid(parenOffset + 1) + " -> " + detail.mid(0, parenOffset);
    }
    case SymbolKind::Variable:
    case SymbolKind::Field:
    case SymbolKind::Constant:
        if (detail.isEmpty())
            return name;
        return name + " -> " + detail;
    default:
        return name;
    }
}

// Force re-parse of all open files that include the changed ui header.
// Otherwise, we potentially have stale diagnostics.
void ClangdClient::handleUiHeaderChange(const QString &fileName)
{
    const QRegularExpression includeRex("#include.*" + fileName + R"([>"])");
    const QVector<Client *> &allClients = LanguageClientManager::clients();
    for (Client * const client : allClients) {
        if (!client->reachable() || !qobject_cast<ClangdClient *>(client))
            continue;
        for (IDocument * const doc : DocumentModel::openedDocuments()) {
            const auto textDoc = qobject_cast<TextDocument *>(doc);
            if (!textDoc || !client->documentOpen(textDoc))
                continue;
            const QTextCursor includePos = textDoc->document()->find(includeRex);
            if (includePos.isNull())
                continue;
            qCDebug(clangdLog) << "updating" << textDoc->filePath() << "due to change in UI header"
                               << fileName;
            client->documentContentsChanged(textDoc, 0, 0, 0);
            break; // No sane project includes the same UI header twice.
        }
    }
}

void ClangdClient::updateParserConfig(const Utils::FilePath &filePath,
        const CppEditor::BaseEditorDocumentParser::Configuration &config)
{
    if (config.preferredProjectPartId.isEmpty())
        return;

    CppEditor::BaseEditorDocumentParser::Configuration &cachedConfig = d->parserConfigs[filePath];
    if (cachedConfig == config)
        return;
    cachedConfig = config;

    // TODO: Also handle editorDefines (and usePrecompiledHeaders?)
    const auto projectPart = CppEditor::CppModelManager::instance()
            ->projectPartForId(config.preferredProjectPartId);
    if (!projectPart)
        return;
    const CppEditor::ClangDiagnosticConfig projectWarnings = warningsConfigForProject(project());
    const QStringList projectOptions = optionsForProject(project());
    QJsonObject cdbChanges;
    QStringList args = createClangOptions(*projectPart, filePath.toString(), projectWarnings,
                                          projectOptions);
    args.prepend("clang");
    args.append(filePath.toString());
    QJsonObject value;
    value.insert("workingDirectory", filePath.parentDir().toString());
    value.insert("compilationCommand", QJsonArray::fromStringList(args));
    cdbChanges.insert(filePath.toUserOutput(), value);
    const QJsonObject settings({qMakePair(QString("compilationDatabaseChanges"), cdbChanges)});
    DidChangeConfigurationParams configChangeParams;
    configChangeParams.setSettings(settings);
    sendContent(DidChangeConfigurationNotification(configChangeParams));
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

    for (const Location &loc : locations)
        refData->fileData[loc.uri()].rangesAndLineText << qMakePair(loc.range(), QString());
    for (auto it = refData->fileData.begin(); it != refData->fileData.end();) {
        const Utils::FilePath filePath = it.key().toFilePath();
        if (!filePath.exists()) { // https://github.com/clangd/clangd/issues/935
            it = refData->fileData.erase(it);
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

    qCDebug(clangdLog) << "document count is" << refData->fileData.size();
    if (refData->replacementData || q->versionNumber() < QVersionNumber(13)
            || !refData->categorize) {
        qCDebug(clangdLog) << "skipping AST retrieval";
        reportAllSearchResultsAndFinish(*refData);
        return;
    }

    for (auto it = refData->fileData.begin(); it != refData->fileData.end(); ++it) {
        const TextDocument * const doc = q->documentForFilePath(it.key().toFilePath());
        if (!doc)
            q->openExtraFile(it.key().toFilePath(), it->fileContent);
        it->fileContent.clear();
        const auto docVariant = doc ? TextDocOrFile(doc) : TextDocOrFile(it.key().toFilePath());
        const auto astHandler = [this, key, loc = it.key()](const AstNode &ast,
                                                            const MessageId &reqId) {
            qCDebug(clangdLog) << "AST for" << loc.toFilePath();
            const auto refData = runningFindUsages.find(key);
            if (refData == runningFindUsages.end())
                return;
            if (!refData->search || refData->canceled)
                return;
            ReferencesFileData &data = refData->fileData[loc];
            data.ast = ast;
            refData->pendingAstRequests.removeOne(reqId);
            qCDebug(clangdLog) << refData->pendingAstRequests.size()
                               << "AST requests still pending";
            addSearchResultsForFile(*refData, loc.toFilePath(), data);
            refData->fileData.remove(loc);
            if (refData->pendingAstRequests.isEmpty()) {
                qDebug(clangdLog) << "retrieved all ASTs";
                finishSearch(*refData, false);
            }
        };
        const MessageId reqId = getAndHandleAst(docVariant, astHandler,
                                                AstCallbackMode::AlwaysAsync);
        refData->pendingAstRequests << reqId;
        if (!doc)
            q->closeExtraFile(it.key().toFilePath());
    }
}

void ClangdClient::Private::handleRenameRequest(const SearchResult *search,
                                                const ReplacementData &replacementData,
                                                const QString &newSymbolName,
                                                const QList<SearchResultItem> &checkedItems,
                                                bool preserveCase)
{
    const Utils::FilePaths filePaths = BaseFileFind::replaceAll(newSymbolName, checkedItems,
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
        CppEditor::renameFilesForSymbol(replacementData.oldSymbolName, newSymbolName, fileNodes);
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
        item.setStyle(CppEditor::colorStyleForUsageType(usageType));
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
                refData.replacementData->fileRenameCandidates << file;
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

void ClangdClient::followSymbol(TextDocument *document,
        const QTextCursor &cursor,
        CppEditor::CppEditorWidget *editorWidget,
        Utils::ProcessLinkCallback &&callback,
        bool resolveTarget,
        bool openInSplit
        )
{
    QTC_ASSERT(documentOpen(document), openDocument(document));
    const QTextCursor adjustedCursor = d->adjustedCursor(cursor, document);
    if (!resolveTarget) {
        d->followSymbolData.reset();
        symbolSupport().findLinkAt(document, adjustedCursor, std::move(callback), false);
        return;
    }

    qCDebug(clangdLog) << "follow symbol requested" << document->filePath()
                       << adjustedCursor.blockNumber() << adjustedCursor.positionInBlock();
    d->followSymbolData.emplace(this, ++d->nextJobId, adjustedCursor, editorWidget,
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
    symbolSupport().findLinkAt(document, adjustedCursor, std::move(gotoDefCallback), true);

    if (versionNumber() < QVersionNumber(12)) {
        d->followSymbolData->cursorNode.emplace(AstNode());
        return;
    }

    const auto astHandler = [this, id = d->followSymbolData->id]
            (const AstNode &ast, const MessageId &) {
        qCDebug(clangdLog) << "received ast response for cursor";
        if (!d->followSymbolData || d->followSymbolData->id != id)
            return;
        d->followSymbolData->cursorNode = ast;
        if (d->followSymbolData->defLink.hasValidTarget())
            d->handleGotoDefinitionResult();
    };
    d->getAndHandleAst(document, astHandler, Private::AstCallbackMode::AlwaysAsync,
                       Range(adjustedCursor));
}

void ClangdClient::switchDeclDef(TextDocument *document, const QTextCursor &cursor,
                                 CppEditor::CppEditorWidget *editorWidget,
                                 Utils::ProcessLinkCallback &&callback)
{
    QTC_ASSERT(documentOpen(document), openDocument(document));

    qCDebug(clangdLog) << "switch decl/dev requested" << document->filePath()
                       << cursor.blockNumber() << cursor.positionInBlock();
    d->switchDeclDefData.emplace(++d->nextJobId, document, cursor, editorWidget,
                                 std::move(callback));

    // Retrieve AST and document symbols.
    const auto astHandler = [this, id = d->switchDeclDefData->id](const AstNode &ast,
                                                                  const MessageId &) {
        qCDebug(clangdLog) << "received ast for decl/def switch";
        if (!d->switchDeclDefData || d->switchDeclDefData->id != id
                || !d->switchDeclDefData->document)
            return;
        if (!ast.isValid()) {
            d->switchDeclDefData.reset();
            return;
        }
        d->switchDeclDefData->ast = ast;
        if (d->switchDeclDefData->docSymbols)
            d->handleDeclDefSwitchReplies();

    };
    d->getAndHandleAst(document, astHandler, Private::AstCallbackMode::SyncIfPossible);
    documentSymbolCache()->requestSymbols(d->switchDeclDefData->uri, Schedule::Now);
}

void ClangdClient::findLocalUsages(TextDocument *document, const QTextCursor &cursor,
        CppEditor::RefactoringEngineInterface::RenameCallback &&callback)
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
        const auto astHandler = [this, link, id](const AstNode &ast, const MessageId &) {
            qCDebug(clangdLog) << "received ast response";
            if (!d->localRefsData || id != d->localRefsData->id)
                return;
            if (!ast.isValid() || !d->localRefsData->document) {
                d->localRefsData.reset();
                return;
            }

            const Position linkPos(link.targetLine - 1, link.targetColumn);
            const QList<AstNode> astPath = getAstPath(ast, linkPos);
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
        };
        qCDebug(clangdLog) << "sending ast request for link";
        d->getAndHandleAst(d->localRefsData->document, astHandler,
                           Private::AstCallbackMode::SyncIfPossible);
    };
    symbolSupport().findLinkAt(document, cursor, std::move(gotoDefCallback), true);
}

void ClangdClient::gatherHelpItemForTooltip(const HoverRequest::Response &hoverResponse,
                                            const DocumentUri &uri)
{
    if (const Utils::optional<Hover> result = hoverResponse.result()) {
        const HoverContent content = result->content();
        const MarkupContent * const markup = Utils::get_if<MarkupContent>(&content);
        if (markup) {
            const QString markupString = markup->content();

            // Macros aren't locatable via the AST, so parse the formatted string.
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

            // Is it the file path for an include directive?
            QString cleanString = markupString;
            cleanString.remove('`');
            const QStringList lines = cleanString.trimmed().split('\n');
            if (!lines.isEmpty()) {
                const auto filePath = Utils::FilePath::fromUserInput(lines.last().simplified());
                if (filePath.exists()) {
                    d->setHelpItemForTooltip(hoverResponse.id(), filePath.fileName(),
                                             HelpItem::Brief);
                    return;
                }
            }
        }
    }

    const TextDocument * const doc = documentForFilePath(uri.toFilePath());
    QTC_ASSERT(doc, return);
    const auto astHandler = [this, uri, hoverResponse](const AstNode &ast, const MessageId &) {
        const MessageId id = hoverResponse.id();
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
        if (clangdLogAst().isDebugEnabled())
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
            sendContent(symReq, SendDocUpdates::Ignore);
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
    };
    d->getAndHandleAst(doc, astHandler, Private::AstCallbackMode::SyncIfPossible);
}

void ClangdClient::setVirtualRanges(const Utils::FilePath &filePath, const QList<Range> &ranges,
                                    int revision)
{
    TextDocument * const doc = documentForFilePath(filePath);
    if (doc && doc->document()->revision() == revision)
        d->virtualRanges.insert(doc, {ranges, revision});
}

void ClangdClient::Private::handleGotoDefinitionResult()
{
    QTC_ASSERT(followSymbolData->defLink.hasValidTarget(), return);

    qCDebug(clangdLog) << "handling go to definition result";

    // No dis-ambiguation necessary. Call back with the link and finish.
    if (!followSymbolData->defLinkIsAmbiguous()) {
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
    q->sendContent(req, SendDocUpdates::Ignore);
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
            && followSymbolData->editorWidget) {
        followSymbolData->editorWidget->invokeTextEditorWidgetAssist(FollowSymbol,
                &followSymbolData->virtualFuncAssistProvider);
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
        q->sendContent(symReq, SendDocUpdates::Ignore);

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
        q->sendContent(defReq, SendDocUpdates::Ignore);
    }

    const Utils::FilePath defLinkFilePath = followSymbolData->defLink.targetFilePath;
    const TextDocument * const defLinkDoc = q->documentForFilePath(defLinkFilePath);
    const auto defLinkDocVariant = defLinkDoc ? TextDocOrFile(defLinkDoc)
                                              : TextDocOrFile(defLinkFilePath);
    const Position defLinkPos(followSymbolData->defLink.targetLine - 1,
                              followSymbolData->defLink.targetColumn);
    const auto astHandler = [this, id = followSymbolData->id]
            (const AstNode &ast, const MessageId &) {
        qCDebug(clangdLog) << "received ast response for def link";
        if (!followSymbolData || followSymbolData->id != id)
            return;
        followSymbolData->defLinkNode = ast;
        if (followSymbolData->pendingSymbolInfoRequests.isEmpty()
                && followSymbolData->pendingGotoDefRequests.isEmpty()) {
            handleDocumentInfoResults();
        }
    };
    getAndHandleAst(defLinkDocVariant, astHandler, AstCallbackMode::AlwaysAsync,
                    Range(defLinkPos, defLinkPos));
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
    if (clangdLogAst().isDebugEnabled())
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

CppEditor::CppEditorWidget *ClangdClient::Private::widgetFromDocument(const TextDocument *doc)
{
    IEditor * const editor = Utils::findOrDefault(EditorManager::visibleEditors(),
            [doc](const IEditor *editor) { return editor->document() == doc; });
    return qobject_cast<CppEditor::CppEditorWidget *>(TextEditorWidget::fromEditor(editor));
}

QString ClangdClient::Private::searchTermFromCursor(const QTextCursor &cursor) const
{
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    return termCursor.selectedText();
}

// https://github.com/clangd/clangd/issues/936
QTextCursor ClangdClient::Private::adjustedCursor(const QTextCursor &cursor,
                                                  const TextDocument *doc)
{
    CppEditor::CppEditorWidget * const widget = widgetFromDocument(doc);
    if (!widget)
        return cursor;
    const Document::Ptr cppDoc = widget->semanticInfo().doc;
    if (!cppDoc)
        return cursor;
    const QList<AST *> builtinAstPath = ASTPath(cppDoc)(cursor);
    const TranslationUnit * const tu = cppDoc->translationUnit();
    const auto posForToken = [doc, tu](int tok) {
        int line, column;
        tu->getTokenPosition(tok, &line, &column);
        return Utils::Text::positionInText(doc->document(), line, column);
    };
    const auto leftMovedCursor = [cursor] {
        QTextCursor c = cursor;
        c.setPosition(cursor.position() - 1);
        return c;
    };
    for (auto it = builtinAstPath.rbegin(); it != builtinAstPath.rend(); ++it) {

        // s|.x or s|->x
        if (const MemberAccessAST * const memberAccess = (*it)->asMemberAccess()) {
            switch (tu->tokenAt(memberAccess->access_token).kind()) {
            case T_DOT:
                break;
            case T_ARROW: {
                const Utils::optional<AstNode> clangdAst = astCache.get(doc);
                if (!clangdAst)
                    return cursor;
                const QList<AstNode> clangdAstPath = getAstPath(*clangdAst, Range(cursor));
                for (auto it = clangdAstPath.rbegin(); it != clangdAstPath.rend(); ++it) {
                    if (it->detailIs("operator->") && it->arcanaContains("CXXMethod"))
                        return cursor;
                }
                break;
            }
            default:
                return cursor;
            }
            if (posForToken(memberAccess->access_token) != cursor.position())
                return cursor;
            return leftMovedCursor();
        }

        // f(arg1|, arg2)
        if (const CallAST *const callAst = (*it)->asCall()) {
            const int tok = builtinAstPath.last()->lastToken();
            if (posForToken(tok) != cursor.position())
                return cursor;
            if (tok == callAst->rparen_token)
                return leftMovedCursor();
            if (tu->tokenKind(tok) != T_COMMA)
                return cursor;

            // Guard against edge case of overloaded comma operator.
            for (auto list = callAst->expression_list; list; list = list->next) {
                if (list->value->lastToken() == tok)
                    return leftMovedCursor();
            }
            return cursor;
        }

        // ~My|Class
        if (const DestructorNameAST * const destrAst = (*it)->asDestructorName()) {
            QTextCursor c = cursor;
            c.setPosition(posForToken(destrAst->tilde_token));
            return c;
        }
    }
    return cursor;
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

class ExtraHighlightingResultsCollector
{
public:
    ExtraHighlightingResultsCollector(QFutureInterface<HighlightingResult> &future,
                                      HighlightingResults &results,
                                      const Utils::FilePath &filePath, const AstNode &ast,
                                      const QTextDocument *doc, const QString &docContent);

    void collect();
private:
    static bool lessThan(const HighlightingResult &r1, const HighlightingResult &r2);
    static int onlyIndexOf(const QStringView &text, const QStringView &subString, int from = 0);
    int posForNodeStart(const AstNode &node) const;
    int posForNodeEnd(const AstNode &node) const;
    void insertResult(const HighlightingResult &result);
    void insertAngleBracketInfo(int searchStart1, int searchEnd1, int searchStart2, int searchEnd2);
    void setResultPosFromRange(HighlightingResult &result, const Range &range);
    void collectFromNode(const AstNode &node);
    void visitNode(const AstNode&node);

    QFutureInterface<HighlightingResult> &m_future;
    HighlightingResults &m_results;
    const Utils::FilePath m_filePath;
    const AstNode &m_ast;
    const QTextDocument * const m_doc;
    const QString &m_docContent;
};

// clangd reports also the #ifs, #elses and #endifs around the disabled code as disabled,
// and not even in a consistent manner. We don't want this, so we have to clean up here.
// But note that we require this behavior, as otherwise we would not be able to grey out
// e.g. empty lines after an #ifdef, due to the lack of symbols.
static QList<BlockRange> cleanupDisabledCode(HighlightingResults &results, const QTextDocument *doc,
                                             const QString &docContent)
{
    QList<BlockRange> ifdefedOutRanges;
    int rangeStartPos = -1;
    for (auto it = results.begin(); it != results.end();) {
        const bool wasIfdefedOut = rangeStartPos != -1;
        const bool isIfDefedOut = it->textStyles.mainStyle == C_DISABLED_CODE;
        if (!isIfDefedOut) {
            if (wasIfdefedOut) {
                const QTextBlock block = doc->findBlockByNumber(it->line - 1);
                ifdefedOutRanges << BlockRange(rangeStartPos, block.position());
                rangeStartPos = -1;
            }
            ++it;
            continue;
        }

        if (!wasIfdefedOut)
            rangeStartPos = doc->findBlockByNumber(it->line - 1).position();

        // Does the current line contain a potential "ifdefed-out switcher"?
        // If not, no state change is possible and we continue with the next line.
        const auto isPreprocessorControlStatement = [&] {
            const int pos = Utils::Text::positionInText(doc, it->line, it->column);
            const QStringView content = subViewLen(docContent, pos, it->length).trimmed();
            if (content.isEmpty() || content.first() != '#')
                return false;
            int offset = 1;
            while (offset < content.size() && content.at(offset).isSpace())
                ++offset;
            if (offset == content.size())
                return false;
            const QStringView ppDirective = content.mid(offset);
            return ppDirective.startsWith(QLatin1String("if"))
                    || ppDirective.startsWith(QLatin1String("elif"))
                    || ppDirective.startsWith(QLatin1String("else"))
                    || ppDirective.startsWith(QLatin1String("endif"));
        };
        if (!isPreprocessorControlStatement()) {
            ++it;
            continue;
        }

        if (!wasIfdefedOut) {
            // The #if or #else that starts disabled code should not be disabled.
            const QTextBlock nextBlock = doc->findBlockByNumber(it->line);
            rangeStartPos = nextBlock.isValid() ? nextBlock.position() : -1;
            it = results.erase(it);
            continue;
        }

        if (wasIfdefedOut && (it + 1 == results.end()
                || (it + 1)->textStyles.mainStyle != C_DISABLED_CODE
                || (it + 1)->line != it->line + 1)) {
            // The #else or #endif that ends disabled code should not be disabled.
            const QTextBlock block = doc->findBlockByNumber(it->line - 1);
            ifdefedOutRanges << BlockRange(rangeStartPos, block.position());
            rangeStartPos = -1;
            it = results.erase(it);
            continue;
        }
        ++it;
    }

    if (rangeStartPos != -1)
        ifdefedOutRanges << BlockRange(rangeStartPos, doc->characterCount());

    qCDebug(clangdLogHighlight) << "found" << ifdefedOutRanges.size() << "ifdefed-out ranges";
    if (clangdLogHighlight().isDebugEnabled()) {
        for (const BlockRange &r : qAsConst(ifdefedOutRanges))
            qCDebug(clangdLogHighlight) << r.first() << r.last();
    }

    return ifdefedOutRanges;
}

static void semanticHighlighter(QFutureInterface<HighlightingResult> &future,
                                const Utils::FilePath &filePath,
                                const QList<ExpandedSemanticToken> &tokens,
                                const QString &docContents, const AstNode &ast,
                                const QPointer<TextDocument> &textDocument,
                                int docRevision, const QVersionNumber &clangdVersion)
{
    ThreadedSubtaskTimer t("highlighting");
    if (future.isCanceled()) {
        future.reportFinished();
        return;
    }

    const QTextDocument doc(docContents);
    const auto tokenRange = [&doc](const ExpandedSemanticToken &token) {
        const Position startPos(token.line - 1, token.column - 1);
        const Position endPos = startPos.withOffset(token.length, &doc);
        return Range(startPos, endPos);
    };
    const auto isOutputParameter = [&ast, &tokenRange](const ExpandedSemanticToken &token) {
        if (token.modifiers.contains(QLatin1String("usedAsMutableReference")))
            return true;
        if (token.type != "variable" && token.type != "property" && token.type != "parameter")
            return false;
        const Range range = tokenRange(token);
        const QList<AstNode> path = getAstPath(ast, range);
        if (path.size() < 2)
            return false;
        for (auto it = path.rbegin() + 1; it != path.rend(); ++it) {
            if (it->kind() == "Call" || it->kind() == "CXXConstruct"
                    || it->kind() == "MemberInitializer") {
                return true;
            }

            // The token should get marked for e.g. lambdas, but not for assignment operators,
            // where the user sees that it's being written.
            if (it->kind() == "CXXOperatorCall") {
                const QList<AstNode> children = it->children().value_or(QList<AstNode>());

                // Child 1 is the call itself, Child 2 is the named entity on which the call happens
                // (a lambda or a class instance), after that follow the actual call arguments.
                if (children.size() < 2)
                    return false;

                // The call itself is never modifiable.
                if (children.first().range() == range)
                    return false;

                // The callable is never displayed as an output parameter.
                // TODO: A good argument can be made to display objects on which a non-const
                //       operator or function is called as output parameters.
                if (children.at(1).range() == range)
                    return false;

                QList<AstNode> firstChildTree{children.first()};
                while (!firstChildTree.isEmpty()) {
                    const AstNode n = firstChildTree.takeFirst();
                    const QString detail = n.detail().value_or(QString());
                    if (detail.startsWith("operator")) {
                        return !detail.contains('=') && !detail.contains("++")
                                && !detail.contains("--");
                    }
                    firstChildTree << n.children().value_or(QList<AstNode>());
                }
                return true;
            }

            if (it->kind() == "Lambda")
                return false;
            if (it->hasConstType())
                return false;
            if (it->kind() == "Member" && it->arcanaContains("(")
                    && !it->arcanaContains("bound member function type")) {
                return false;
            }
        }
        return false;
    };

    const std::function<HighlightingResult(const ExpandedSemanticToken &)> toResult
            = [&ast, &isOutputParameter, &clangdVersion, &tokenRange]
              (const ExpandedSemanticToken &token) {
        TextStyles styles;
        if (token.type == "variable") {
            if (token.modifiers.contains(QLatin1String("functionScope"))) {
                styles.mainStyle = C_LOCAL;
            } else if (token.modifiers.contains(QLatin1String("classScope"))) {
                styles.mainStyle = C_FIELD;
            } else if (token.modifiers.contains(QLatin1String("fileScope"))
                       || token.modifiers.contains(QLatin1String("globalScope"))) {
                styles.mainStyle = C_GLOBAL;
            }
        } else if (token.type == "function" || token.type == "method") {
            styles.mainStyle = token.modifiers.contains(QLatin1String("virtual"))
                    ? C_VIRTUAL_METHOD : C_FUNCTION;
            if (ast.isValid()) {
                const QList<AstNode> path = getAstPath(ast, tokenRange(token));
                if (path.length() > 1) {
                    const AstNode declNode = path.at(path.length() - 2);
                    if (declNode.kind() == "Function" || declNode.kind() == "CXXMethod") {
                        if (clangdVersion < QVersionNumber(14)
                                && declNode.arcanaContains("' virtual")) {
                            styles.mainStyle = C_VIRTUAL_METHOD;
                        }
                        if (declNode.hasChildWithRole("statement"))
                            styles.mixinStyles.push_back(C_FUNCTION_DEFINITION);
                    }
                }
            }
        } else if (token.type == "class") {
            styles.mainStyle = C_TYPE;

            // clang hardly ever differentiates between constructors and the associated class,
            // whereas we highlight constructors as functions.
            if (ast.isValid()) {
                const QList<AstNode> path = getAstPath(ast, tokenRange(token));
                if (!path.isEmpty()) {
                    if (path.last().kind() == "CXXConstructor") {
                        if (!path.last().arcanaContains("implicit"))
                            styles.mainStyle = C_FUNCTION;
                    } else if (path.last().kind() == "Record" && path.length() > 1) {
                        const AstNode node = path.at(path.length() - 2);
                        if (node.kind() == "CXXDestructor" && !node.arcanaContains("implicit")) {
                            styles.mainStyle = C_FUNCTION;

                            // https://github.com/clangd/clangd/issues/872
                            if (node.role() == "declaration")
                                styles.mixinStyles.push_back(C_DECLARATION);
                        }
                    }
                }
            }
        } else if (token.type == "comment") { // "comment" means code disabled via the preprocessor
            styles.mainStyle = C_DISABLED_CODE;
        } else if (token.type == "namespace") {
            styles.mainStyle = C_NAMESPACE;
        } else if (token.type == "property") {
            styles.mainStyle = C_FIELD;
        } else if (token.type == "enum") {
            styles.mainStyle = C_TYPE;
            styles.mixinStyles.push_back(C_ENUMERATION);
        } else if (token.type == "enumMember") {
            styles.mainStyle = C_ENUMERATION;
        } else if (token.type == "parameter") {
            styles.mainStyle = C_PARAMETER;
        } else if (token.type == "macro") {
            styles.mainStyle = C_PREPROCESSOR;
        } else if (token.type == "type") {
            styles.mainStyle = C_TYPE;
        } else if (token.type == "typeParameter") {
            // clangd reports both type and non-type template parameters as type parameters,
            // but the latter can be distinguished by the readonly modifier.
            styles.mainStyle = token.modifiers.contains(QLatin1String("readonly"))
                    ? C_PARAMETER : C_TYPE;
        }
        if (token.modifiers.contains(QLatin1String("declaration")))
            styles.mixinStyles.push_back(C_DECLARATION);
        if (token.modifiers.contains(QLatin1String("static")))
            styles.mixinStyles.push_back(C_STATIC_MEMBER);
        if (isOutputParameter(token))
            styles.mixinStyles.push_back(C_OUTPUT_ARGUMENT);
        qCDebug(clangdLogHighlight) << "adding highlighting result"
                           << token.line << token.column << token.length << int(styles.mainStyle);
        return HighlightingResult(token.line, token.column, token.length, styles);
    };

    auto results = QtConcurrent::blockingMapped<HighlightingResults>(tokens, toResult);
    const QList<BlockRange> ifdefedOutBlocks = cleanupDisabledCode(results, &doc, docContents);
    ExtraHighlightingResultsCollector(future, results, filePath, ast, &doc, docContents).collect();
    if (!future.isCanceled()) {
        qCDebug(clangdLog) << "reporting" << results.size() << "highlighting results";
        QMetaObject::invokeMethod(textDocument, [textDocument, ifdefedOutBlocks, docRevision] {
            if (textDocument && textDocument->document()->revision() == docRevision)
                textDocument->setIfdefedOutBlocks(ifdefedOutBlocks);
        }, Qt::QueuedConnection);
        QList<Range> virtualRanges;
        for (const HighlightingResult &r : results) {
            if (r.textStyles.mainStyle != C_VIRTUAL_METHOD)
                continue;
            const Position startPos(r.line - 1, r.column - 1);
            virtualRanges << Range(startPos, startPos.withOffset(r.length, &doc));
        }
        QMetaObject::invokeMethod(ClangModelManagerSupport::instance(),
                                  [filePath, virtualRanges, docRevision] {
            if (ClangdClient * const client
                    = ClangModelManagerSupport::instance()->clientForFile(filePath)) {
                client->setVirtualRanges(filePath, virtualRanges, docRevision);
            }
        }, Qt::QueuedConnection);
        future.reportResults(QVector<HighlightingResult>(results.cbegin(), results.cend()));
    }
    future.reportFinished();
}

// Unfortunately, clangd ignores almost everything except symbols when sending
// semantic token info, so we need to consult the AST for additional information.
// In particular, we inspect the following constructs:
//    - Raw string literals, because our built-in lexer does not parse them properly.
//      While we're at it, we also handle other types of literals.
//    - Ternary expressions (for the matching of "?" and ":").
//    - Template declarations and instantiations (for the matching of "<" and ">").
//    - Function declarations, to find out whether a declaration is also a definition.
//    - Function arguments, to find out whether they correspond to output parameters.
//    - We consider most other tokens to be simple enough to be handled by the built-in code model.
//      Sometimes we have no choice, as for #include directives, which appear neither
//      in the semantic tokens nor in the AST.
void ClangdClient::Private::handleSemanticTokens(TextDocument *doc,
                                                 const QList<ExpandedSemanticToken> &tokens,
                                                 int version, bool force)
{
    SubtaskTimer t(highlightingTimer);
    qCDebug(clangdLog) << "handling LSP tokens" << doc->filePath() << tokens.size();
    if (version != q->documentVersion(doc->filePath())) {
        qCDebug(clangdLogHighlight) << "LSP tokens outdated; aborting highlighting procedure"
                                    << version << q->documentVersion(doc->filePath());
        return;
    }
    force = force || isTesting;
    const auto previous = previousTokens.find(doc);
    if (previous != previousTokens.end()) {
        if (!force && previous->first == tokens && previous->second == version) {
            qCDebug(clangdLogHighlight) << "tokens and version same as last time; nothing to do";
            return;
        }
        previous->first = tokens;
        previous->second = version;
    } else {
        previousTokens.insert(doc, qMakePair(tokens, version));
    }
    for (const ExpandedSemanticToken &t : tokens)
        qCDebug(clangdLogHighlight()) << '\t' << t.line << t.column << t.length << t.type
                                      << t.modifiers;

    const auto astHandler = [this, tokens, doc, version](const AstNode &ast, const MessageId &) {
        FinalizingSubtaskTimer t(highlightingTimer);
        if (!q->documentOpen(doc))
            return;
        if (version != q->documentVersion(doc->filePath())) {
            qCDebug(clangdLogHighlight) << "AST not up to date; aborting highlighting procedure"
                                        << version << q->documentVersion(doc->filePath());
            return;
        }
        if (clangdLogAst().isDebugEnabled())
            ast.print();

        const auto runner = [tokens, filePath = doc->filePath(),
                             text = doc->document()->toPlainText(), ast,
                             doc = QPointer(doc), rev = doc->document()->revision(),
                             clangdVersion = q->versionNumber()] {
            return Utils::runAsync(semanticHighlighter, filePath, tokens, text, ast, doc, rev,
                                   clangdVersion);
        };

        if (isTesting) {
            const auto watcher = new QFutureWatcher<HighlightingResult>(q);
            connect(watcher, &QFutureWatcher<HighlightingResult>::finished,
                    q, [this, watcher, fp = doc->filePath()] {
                emit q->highlightingResultsReady(watcher->future().results(), fp);
                watcher->deleteLater();
            });
            watcher->setFuture(runner());
            return;
        }

        auto it = highlighters.find(doc);
        if (it == highlighters.end()) {
            it = highlighters.insert(std::make_pair(doc, new CppEditor::SemanticHighlighter(doc)))
                    .first;
        } else {
            it->second->updateFormatMapFromFontSettings();
        }
        it->second->setHighlightingRunner(runner);
        it->second->run();
    };
    getAndHandleAst(doc, astHandler, AstCallbackMode::SyncIfPossible);
}

void ClangdClient::VirtualFunctionAssistProcessor::cancel()
{
    resetData();
}

void ClangdClient::VirtualFunctionAssistProcessor::update()
{
    if (!m_data->followSymbolData->editorWidget)
        return;
    setAsyncProposalAvailable(createProposal(false));
}

void ClangdClient::VirtualFunctionAssistProcessor::finalize()
{
    if (!m_data->followSymbolData->editorWidget)
        return;
    const auto proposal = createProposal(true);
    if (m_data->followSymbolData->editorWidget->isInTestMode()) {
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

IAssistProposal *ClangdClient::VirtualFunctionAssistProcessor::createProposal(bool final) const
{
    QTC_ASSERT(m_data && m_data->followSymbolData, return nullptr);

    QList<AssistProposalItemInterface *> items;
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
        const auto infoItem = new CppEditor::VirtualFunctionProposalItem({}, false);
        infoItem->setText(ClangdClient::tr("collecting overrides ..."));
        infoItem->setOrder(-1);
        items << infoItem;
    }

    return new CppEditor::VirtualFunctionProposal(
                m_data->followSymbolData->cursor.position(),
                items, m_data->followSymbolData->openInSplit);
}

CppEditor::VirtualFunctionProposalItem *
ClangdClient::VirtualFunctionAssistProcessor::createEntry(const QString &name,
                                                          const Utils::Link &link) const
{
    const auto item = new CppEditor::VirtualFunctionProposalItem(
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

IAssistProcessor *ClangdClient::VirtualFunctionAssistProvider::createProcessor(
    const AssistInterface *) const
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

class ClangdClient::ClangdFunctionHintProcessor : public FunctionHintProcessor
{
public:
    ClangdFunctionHintProcessor(ClangdClient *client)
        : FunctionHintProcessor(client)
        , m_client(client)
    {}

private:
    IAssistProposal *perform(const AssistInterface *interface) override
    {
        if (m_client->d->isTesting) {
            setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal) {
                emit m_client->proposalReady(proposal);
            });
        }
        return FunctionHintProcessor::perform(interface);
    }

    ClangdClient * const m_client;
};

ClangdClient::ClangdCompletionAssistProvider::ClangdCompletionAssistProvider(ClangdClient *client)
    : LanguageClientCompletionAssistProvider(client)
    , m_client(client)
{}

IAssistProcessor *ClangdClient::ClangdCompletionAssistProvider::createProcessor(
    const AssistInterface *interface) const
{
    qCDebug(clangdLogCompletion) << "completion processor requested for" << interface->filePath();
    qCDebug(clangdLogCompletion) << "text before cursor is"
                                 << interface->textAt(interface->position(), -10);
    qCDebug(clangdLogCompletion) << "text after cursor is"
                                 << interface->textAt(interface->position(), 10);
    ClangCompletionContextAnalyzer contextAnalyzer(interface->textDocument(),
                                                   interface->position(), false, {});
    contextAnalyzer.analyze();
    switch (contextAnalyzer.completionAction()) {
    case ClangCompletionContextAnalyzer::PassThroughToLibClangAfterLeftParen:
        qCDebug(clangdLogCompletion) << "creating function hint processor";
        return new ClangdFunctionHintProcessor(m_client);
    case ClangCompletionContextAnalyzer::CompleteDoxygenKeyword:
        qCDebug(clangdLogCompletion) << "creating doxygen processor";
        return new CustomAssistProcessor(m_client,
                                         contextAnalyzer.positionForProposal(),
                                         contextAnalyzer.positionEndOfExpression(),
                                         contextAnalyzer.completionOperator(),
                                         CustomAssistMode::Doxygen);
    case ClangCompletionContextAnalyzer::CompletePreprocessorDirective:
        qCDebug(clangdLogCompletion) << "creating macro processor";
        return new CustomAssistProcessor(m_client,
                                         contextAnalyzer.positionForProposal(),
                                         contextAnalyzer.positionEndOfExpression(),
                                         contextAnalyzer.completionOperator(),
                                         CustomAssistMode::Preprocessor);
    case ClangCompletionContextAnalyzer::CompleteIncludePath:
        if (m_client->versionNumber() < QVersionNumber(14)) { // https://reviews.llvm.org/D112996
            qCDebug(clangdLogCompletion) << "creating include processor";
            return new CustomAssistProcessor(m_client,
                                             contextAnalyzer.positionForProposal(),
                                             contextAnalyzer.positionEndOfExpression(),
                                             contextAnalyzer.completionOperator(),
                                             CustomAssistMode::IncludePath);
        }
        [[fallthrough]];
    default:
        break;
    }
    const QString snippetsGroup = contextAnalyzer.addSnippets() && !isInCommentOrString(interface)
                                      ? CppEditor::Constants::CPP_SNIPPETS_GROUP_ID
                                      : QString();
    qCDebug(clangdLogCompletion) << "creating proper completion processor"
                                 << (snippetsGroup.isEmpty() ? "without" : "with") << "snippets";
    return new ClangdCompletionAssistProcessor(m_client, snippetsGroup);
}

bool ClangdClient::ClangdCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    const QChar &ch  = sequence.at(2);
    const QChar &ch2 = sequence.at(1);
    const QChar &ch3 = sequence.at(0);
    unsigned kind = T_EOF_SYMBOL;
    const int pos = CppEditor::CppCompletionAssistProvider::activationSequenceChar(
                ch, ch2, ch3, &kind, false, false);
    if (pos == 0)
        return false;

    // We want to minimize unneeded completion requests, as those trigger document updates,
    // which trigger re-highlighting and diagnostics, which we try to delay.
    // Therefore, we do not trigger on syntax elements that often occur in non-applicable
    // contexts, such as '(', '<' or '/'.
    switch (kind) {
    case T_DOT: case T_COLON_COLON: case T_ARROW: case T_DOT_STAR: case T_ARROW_STAR: case T_POUND:
        qCDebug(clangdLogCompletion) << "detected" << sequence << "as activation char sequence";
        return true;
    }
    return false;
}

bool ClangdClient::ClangdCompletionAssistProvider::isContinuationChar(const QChar &c) const
{
    return CppEditor::isValidIdentifierChar(c);
}

bool ClangdClient::ClangdCompletionAssistProvider::isInCommentOrString(
        const AssistInterface *interface) const
{
    LanguageFeatures features = LanguageFeatures::defaultFeatures();
    features.objCEnabled = CppEditor::ProjectFile::isObjC(interface->filePath().toString());
    return CppEditor::isInCommentOrString(interface, features);
}

void ClangdCompletionItem::apply(TextDocumentManipulatorInterface &manipulator,
                                 int /*basePosition*/) const
{
    const LanguageServerProtocol::CompletionItem item = this->item();
    QChar typedChar = triggeredCommitCharacter();
    const auto edit = item.textEdit();
    if (!edit)
        return;

    const int labelOpenParenOffset = item.label().indexOf('(');
    const int labelClosingParenOffset = item.label().indexOf(')');
    const auto kind = static_cast<CompletionItemKind::Kind>(
                item.kind().value_or(CompletionItemKind::Text));
    const bool isMacroCall = kind == CompletionItemKind::Text && labelOpenParenOffset != -1
            && labelClosingParenOffset > labelOpenParenOffset; // Heuristic
    const bool isFunctionLike = kind == CompletionItemKind::Function
            || kind == CompletionItemKind::Method || kind == CompletionItemKind::Constructor
            || isMacroCall;

    QString rawInsertText = edit->newText();

    // Some preparation for our magic involving (non-)insertion of parentheses and
    // cursor placement.
    if (isFunctionLike && !rawInsertText.contains('(')) {
        if (labelOpenParenOffset != -1) {
            if (labelClosingParenOffset == labelOpenParenOffset + 1) // function takes no arguments
                rawInsertText += "()";
            else                                                     // function takes arguments
                rawInsertText += "( )";
        }
    }

    const int firstParenOffset = rawInsertText.indexOf('(');
    const int lastParenOffset = rawInsertText.lastIndexOf(')');
    const QString detail = item.detail().value_or(QString());
    const CompletionSettings &completionSettings = TextEditorSettings::completionSettings();
    QString textToBeInserted = rawInsertText.left(firstParenOffset);
    QString extraCharacters;
    int extraLength = 0;
    int cursorOffset = 0;
    bool setAutoCompleteSkipPos = false;
    int currentPos = manipulator.currentPosition();
    const QTextDocument * const doc = manipulator.textCursorAt(currentPos).document();
    const Range range = edit->range();
    const int rangeStart = range.start().toPositionInDocument(doc);
    if (isFunctionLike && completionSettings.m_autoInsertBrackets) {
        // If the user typed the opening parenthesis, they'll likely also type the closing one,
        // in which case it would be annoying if we put the cursor after the already automatically
        // inserted closing parenthesis.
        const bool skipClosingParenthesis = typedChar != '(';
        QTextCursor cursor = manipulator.textCursorAt(rangeStart);

        bool abandonParen = false;
        if (matchPreviousWord(manipulator, cursor, "&")) {
            moveToPreviousWord(manipulator, cursor);
            moveToPreviousChar(manipulator, cursor);
            const QChar prevChar = manipulator.characterAt(cursor.position());
            cursor.setPosition(rangeStart);
            abandonParen = QString("(;,{}=").contains(prevChar);
        }
        if (!abandonParen)
            abandonParen = isAtUsingDeclaration(manipulator, rangeStart);
        if (!abandonParen && !isMacroCall && matchPreviousWord(manipulator, cursor, detail))
            abandonParen = true; // function definition
        if (!abandonParen) {
            if (completionSettings.m_spaceAfterFunctionName)
                extraCharacters += ' ';
            extraCharacters += '(';
            if (typedChar == '(')
                typedChar = {};

            // If the function doesn't return anything, automatically place the semicolon,
            // unless we're doing a scope completion (then it might be function definition).
            const QChar characterAtCursor = manipulator.characterAt(currentPos);
            bool endWithSemicolon = typedChar == ';';
            const QChar semicolon = typedChar.isNull() ? QLatin1Char(';') : typedChar;
            if (endWithSemicolon && characterAtCursor == semicolon) {
                endWithSemicolon = false;
                typedChar = {};
            }

            // If the function takes no arguments, automatically place the closing parenthesis
            if (firstParenOffset + 1 == lastParenOffset && skipClosingParenthesis) {
                extraCharacters += QLatin1Char(')');
                if (endWithSemicolon) {
                    extraCharacters += semicolon;
                    typedChar = {};
                }
            } else {
                const QChar lookAhead = manipulator.characterAt(currentPos + 1);
                if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                    extraCharacters += ')';
                    --cursorOffset;
                    setAutoCompleteSkipPos = true;
                    if (endWithSemicolon) {
                        extraCharacters += semicolon;
                        --cursorOffset;
                        typedChar = {};
                    }
                }
            }
        }
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!typedChar.isNull()) {
        extraCharacters += typedChar;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    // Avoid inserting characters that are already there
    QTextCursor cursor = manipulator.textCursorAt(rangeStart);
    cursor.movePosition(QTextCursor::EndOfWord);
    const QString textAfterCursor = manipulator.textAt(currentPos, cursor.position() - currentPos);
    if (currentPos < cursor.position()
            && textToBeInserted != textAfterCursor
            && textToBeInserted.indexOf(textAfterCursor, currentPos - rangeStart) >= 0) {
        currentPos = cursor.position();
    }
    for (int i = 0; i < extraCharacters.length(); ++i) {
        const QChar a = extraCharacters.at(i);
        const QChar b = manipulator.characterAt(currentPos + i);
        if (a == b)
            ++extraLength;
        else
            break;
    }

    textToBeInserted += extraCharacters;
    const int length = currentPos - rangeStart + extraLength;
    const bool isReplaced = manipulator.replace(rangeStart, length, textToBeInserted);
    manipulator.setCursorPosition(rangeStart + textToBeInserted.length());
    if (isReplaced) {
        if (cursorOffset)
            manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
        if (setAutoCompleteSkipPos)
            manipulator.setAutoCompleteSkipPosition(manipulator.currentPosition());
    }

    if (auto additionalEdits = item.additionalTextEdits()) {
        for (const auto &edit : *additionalEdits)
            applyTextEdit(manipulator, edit);
    }
}

ClangdCompletionItem::SpecialQtType ClangdCompletionItem::getQtType(const CompletionItem &item)
{
    const Utils::optional<MarkupOrString> doc = item.documentation();
    if (!doc)
        return SpecialQtType::None;
    QString docText;
    if (Utils::holds_alternative<QString>(*doc))
        docText = Utils::get<QString>(*doc);
    else if (Utils::holds_alternative<MarkupContent>(*doc))
        docText = Utils::get<MarkupContent>(*doc).content();
    if (docText.contains("Annotation: qt_signal"))
        return SpecialQtType::Signal;
    if (docText.contains("Annotation: qt_slot"))
        return SpecialQtType::Slot;
    return SpecialQtType::None;
}

QIcon ClangdCompletionItem::icon() const
{
    const SpecialQtType qtType = getQtType(item());
    switch (qtType) {
    case SpecialQtType::Signal:
        return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Signal);
    case SpecialQtType::Slot:
         // FIXME: Add visibility info to completion item tags in clangd?
        return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::SlotPublic);
    case SpecialQtType::None:
        break;
    }
    return LanguageClientCompletionItem::icon();
}

MessageId ClangdClient::Private::getAndHandleAst(const TextDocOrFile &doc,
                                                 const AstHandler &astHandler,
                                                 AstCallbackMode callbackMode, const Range &range)
{
    const auto textDocPtr = Utils::get_if<const TextDocument *>(&doc);
    const TextDocument * const textDoc = textDocPtr ? *textDocPtr : nullptr;
    const Utils::FilePath filePath = textDoc ? textDoc->filePath()
                                             : Utils::get<Utils::FilePath>(doc);

    // If the entire AST is requested and the document's AST is in the cache and it is up to date,
    // call the handler.
    const bool fullAstRequested = !range.isValid();
    if (fullAstRequested) {
        if (const auto ast = textDoc ? astCache.get(textDoc) : externalAstCache.get(filePath)) {
            qCDebug(clangdLog) << "using AST from cache";
            switch (callbackMode) {
            case AstCallbackMode::SyncIfPossible:
                astHandler(*ast, {});
                break;
            case AstCallbackMode::AlwaysAsync:
                QMetaObject::invokeMethod(q, [ast, astHandler] { astHandler(*ast, {}); },
                                      Qt::QueuedConnection);
                break;
            }
            return {};
        }
    }

    // Otherwise retrieve the AST from clangd.

    class AstParams : public JsonObject
    {
    public:
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

    class AstRequest : public Request<AstNode, std::nullptr_t, AstParams>
    {
    public:
        using Request::Request;
        explicit AstRequest(const AstParams &params) : Request("textDocument/ast", params) {}
    };

    AstRequest request(AstParams(TextDocumentIdentifier(DocumentUri::fromFilePath(filePath)),
                                 range));
    request.setResponseCallback([this, filePath, guardedTextDoc = QPointer(textDoc), astHandler,
                                fullAstRequested, docRev = textDoc ? getRevision(textDoc) : -1,
                                fileRev = getRevision(filePath), reqId = request.id()]
                                (AstRequest::Response response) {
        qCDebug(clangdLog) << "retrieved AST from clangd";
        const auto result = response.result();
        const AstNode ast = result ? *result : AstNode();
        if (fullAstRequested) {
            if (guardedTextDoc) {
                if (docRev == getRevision(guardedTextDoc))
                    astCache.insert(guardedTextDoc, ast);
            } else if (fileRev == getRevision(filePath) && !q->documentForFilePath(filePath)) {
                externalAstCache.insert(filePath, ast);
            }
        }
        astHandler(ast, reqId);
    });
    qCDebug(clangdLog) << "requesting AST for" << filePath;
    q->sendContent(request, SendDocUpdates::Ignore);
    return request.id();
}

ExtraHighlightingResultsCollector::ExtraHighlightingResultsCollector(
        QFutureInterface<HighlightingResult> &future, HighlightingResults &results,
        const Utils::FilePath &filePath, const AstNode &ast, const QTextDocument *doc,
        const QString &docContent)
    : m_future(future), m_results(results), m_filePath(filePath), m_ast(ast), m_doc(doc),
      m_docContent(docContent)
{
}

void ExtraHighlightingResultsCollector::collect()
{
    for (int i = 0; i < m_results.length(); ++i) {
        const HighlightingResult res = m_results.at(i);
        if (res.textStyles.mainStyle != C_PREPROCESSOR || res.length != 10)
            continue;
        const int pos = Utils::Text::positionInText(m_doc, res.line, res.column);
        if (subViewLen(m_docContent, pos, 10) != QLatin1String("Q_PROPERTY"))
            continue;
        int endPos;
        if (i < m_results.length() - 1) {
            const HighlightingResult nextRes = m_results.at(i + 1);
            endPos = Utils::Text::positionInText(m_doc, nextRes.line, nextRes.column);
        } else {
            endPos = m_docContent.length();
        }
        const QString qPropertyString = m_docContent.mid(pos, endPos - pos);
        QPropertyHighlighter propHighlighter(m_doc, qPropertyString, pos);
        for (const HighlightingResult &newRes : propHighlighter.highlight())
            m_results.insert(++i, newRes);
    }

    if (!m_ast.isValid())
        return;
    visitNode(m_ast);
}

bool ExtraHighlightingResultsCollector::lessThan(const HighlightingResult &r1,
                                                 const HighlightingResult &r2)
{
    return r1.line < r2.line || (r1.line == r2.line && r1.column < r2.column)
            || (r1.line == r2.line && r1.column == r2.column && r1.length < r2.length);
}

int ExtraHighlightingResultsCollector::onlyIndexOf(const QStringView &text,
                                                   const QStringView &subString, int from)
{
    const int firstIndex = text.indexOf(subString, from);
    if (firstIndex == -1)
        return -1;
    const int nextIndex = text.indexOf(subString, firstIndex + 1);

    // The second condion deals with the off-by-one error in TemplateSpecialization nodes;
    // see collectFromNode().
    return nextIndex == -1 || nextIndex == firstIndex + 1 ? firstIndex : -1;
}

// Unfortunately, the exact position of a specific token is usually not
// recorded in the AST, so if we need that, we have to search for it textually.
// In corner cases, this might get sabotaged by e.g. comments, in which case we give up.
int ExtraHighlightingResultsCollector::posForNodeStart(const AstNode &node) const
{
    return Utils::Text::positionInText(m_doc, node.range().start().line() + 1,
                                       node.range().start().character() + 1);
}

int ExtraHighlightingResultsCollector::posForNodeEnd(const AstNode &node) const
{
    return Utils::Text::positionInText(m_doc, node.range().end().line() + 1,
                                       node.range().end().character() + 1);
}

void ExtraHighlightingResultsCollector::insertResult(const HighlightingResult &result)
{
    if (!result.isValid()) // Some nodes don't have a range.
        return;
    const auto it = std::lower_bound(m_results.begin(), m_results.end(), result, lessThan);
    if (it == m_results.end() || *it != result) {

        // Prevent inserting expansions for function-like macros. For instance:
        //     #define TEST() "blubb"
        //     const char *s = TEST();
        // The macro name is always shorter than the expansion and starts at the same
        // location, so it should occur right before the insertion position.
        if (it > m_results.begin() && (it - 1)->line == result.line
                && (it - 1)->column == result.column
                && (it - 1)->textStyles.mainStyle == C_PREPROCESSOR) {
            return;
        }

        qCDebug(clangdLogHighlight) << "adding additional highlighting result"
                                    << result.line << result.column << result.length;
        m_results.insert(it, result);
        return;
    }

    // This is for conversion operators, whose type part is only reported as a type by clangd.
    if ((it->textStyles.mainStyle == C_TYPE
         || it->textStyles.mainStyle == C_PRIMITIVE_TYPE)
            && !result.textStyles.mixinStyles.empty()
            && result.textStyles.mixinStyles.at(0) == C_OPERATOR) {
        it->textStyles.mixinStyles = result.textStyles.mixinStyles;
    }
}

// For matching the "<" and ">" brackets of template declarations, specializations
// and instantiations.
void ExtraHighlightingResultsCollector::insertAngleBracketInfo(int searchStart1, int searchEnd1,
                                                               int searchStart2, int searchEnd2)
{
    const int openingAngleBracketPos = onlyIndexOf(
                subViewEnd(m_docContent, searchStart1, searchEnd1),
                QStringView(QStringLiteral("<")));
    if (openingAngleBracketPos == -1)
        return;
    const int absOpeningAngleBracketPos = searchStart1 + openingAngleBracketPos;
    if (absOpeningAngleBracketPos > searchStart2)
        searchStart2 = absOpeningAngleBracketPos + 1;
    if (searchStart2 >= searchEnd2)
        return;
    const int closingAngleBracketPos = onlyIndexOf(
                subViewEnd(m_docContent, searchStart2, searchEnd2),
                QStringView(QStringLiteral(">")));
    if (closingAngleBracketPos == -1)
        return;

    const int absClosingAngleBracketPos = searchStart2 + closingAngleBracketPos;
    if (absOpeningAngleBracketPos > absClosingAngleBracketPos)
        return;

    HighlightingResult result;
    result.useTextSyles = true;
    result.textStyles.mainStyle = C_PUNCTUATION;
    Utils::Text::convertPosition(m_doc, absOpeningAngleBracketPos, &result.line, &result.column);
    result.length = 1;
    result.kind = CppEditor::SemanticHighlighter::AngleBracketOpen;
    insertResult(result);
    Utils::Text::convertPosition(m_doc, absClosingAngleBracketPos, &result.line, &result.column);
    result.kind = CppEditor::SemanticHighlighter::AngleBracketClose;
    insertResult(result);
}

void ExtraHighlightingResultsCollector::setResultPosFromRange(HighlightingResult &result,
                                                              const Range &range)
{
    if (!range.isValid())
        return;
    const Position startPos = range.start();
    const Position endPos = range.end();
    result.line = startPos.line() + 1;
    result.column = startPos.character() + 1;
    result.length = endPos.toPositionInDocument(m_doc) - startPos.toPositionInDocument(m_doc);
}

void ExtraHighlightingResultsCollector::collectFromNode(const AstNode &node)
{
    if (node.kind() == "UserDefinedLiteral")
        return;
    if (node.kind().endsWith("Literal")) {
        HighlightingResult result;
        result.useTextSyles = true;
        const bool isStringLike = node.kind().startsWith("String")
                || node.kind().startsWith("Character");
        result.textStyles.mainStyle = isStringLike ? C_STRING : C_NUMBER;
        setResultPosFromRange(result, node.range());
        insertResult(result);
        return;
    }
    if (node.role() == "type" && node.kind() == "Builtin") {
        HighlightingResult result;
        result.useTextSyles = true;
        result.textStyles.mainStyle = C_PRIMITIVE_TYPE;
        setResultPosFromRange(result, node.range());
        insertResult(result);
        return;
    }
    if (node.role() == "attribute" && (node.kind() == "Override" || node.kind() == "Final")) {
        HighlightingResult result;
        result.useTextSyles = true;
        result.textStyles.mainStyle = C_KEYWORD;
        setResultPosFromRange(result, node.range());
        insertResult(result);
        return;
    }

    const bool isExpression = node.role() == "expression";
    const bool isDeclaration = node.role() == "declaration";

    const int nodeStartPos = posForNodeStart(node);
    const int nodeEndPos = posForNodeEnd(node);
    const QList<AstNode> children = node.children().value_or(QList<AstNode>());

    // Match question mark and colon in ternary operators.
    if (isExpression && node.kind() == "ConditionalOperator") {
        if (children.size() != 3)
            return;

        // The question mark is between sub-expressions 1 and 2, the colon is between
        // sub-expressions 2 and 3.
        const int searchStartPosQuestionMark = posForNodeEnd(children.first());
        const int searchEndPosQuestionMark = posForNodeStart(children.at(1));
        QStringView content = subViewEnd(m_docContent, searchStartPosQuestionMark,
                                         searchEndPosQuestionMark);
        const int questionMarkPos = onlyIndexOf(content, QStringView(QStringLiteral("?")));
        if (questionMarkPos == -1)
            return;
        const int searchStartPosColon = posForNodeEnd(children.at(1));
        const int searchEndPosColon = posForNodeStart(children.at(2));
        content = subViewEnd(m_docContent, searchStartPosColon, searchEndPosColon);
        const int colonPos = onlyIndexOf(content, QStringView(QStringLiteral(":")));
        if (colonPos == -1)
            return;

        const int absQuestionMarkPos = searchStartPosQuestionMark + questionMarkPos;
        const int absColonPos = searchStartPosColon + colonPos;
        if (absQuestionMarkPos > absColonPos)
            return;

        HighlightingResult result;
        result.useTextSyles = true;
        result.textStyles.mainStyle = C_PUNCTUATION;
        result.textStyles.mixinStyles.push_back(C_OPERATOR);
        Utils::Text::convertPosition(m_doc, absQuestionMarkPos, &result.line, &result.column);
        result.length = 1;
        result.kind = CppEditor::SemanticHighlighter::TernaryIf;
        insertResult(result);
        Utils::Text::convertPosition(m_doc, absColonPos, &result.line, &result.column);
        result.kind = CppEditor::SemanticHighlighter::TernaryElse;
        insertResult(result);
        return;
    }

    if (isDeclaration && (node.kind() == "FunctionTemplate"
                          || node.kind() == "ClassTemplate")) {
        // The child nodes are the template parameters and and the function or class.
        // The opening angle bracket is before the first child node, the closing angle
        // bracket is before the function child node and after the last param node.
        const QString classOrFunctionKind = QLatin1String(node.kind() == "FunctionTemplate"
                                                          ? "Function" : "CXXRecord");
        const auto functionOrClassIt = std::find_if(children.begin(), children.end(),
                                                    [&classOrFunctionKind](const AstNode &n) {
            return n.role() == "declaration" && n.kind() == classOrFunctionKind;
        });
        if (functionOrClassIt == children.end() || functionOrClassIt == children.begin())
            return;
        const int firstTemplateParamStartPos = posForNodeStart(children.first());
        const int lastTemplateParamEndPos = posForNodeEnd(*(functionOrClassIt - 1));
        const int functionOrClassStartPos = posForNodeStart(*functionOrClassIt);
        insertAngleBracketInfo(nodeStartPos, firstTemplateParamStartPos,
                               lastTemplateParamEndPos, functionOrClassStartPos);
        return;
    }

    const auto isTemplateParamDecl = [](const AstNode &node) {
        return node.isTemplateParameterDeclaration();
    };
    if (isDeclaration && node.kind() == "TypeAliasTemplate") {
        // Children are one node of type TypeAlias and the template parameters.
        // The opening angle bracket is before the first parameter and the closing
        // angle bracket is after the last parameter.
        // The TypeAlias node seems to appear first in the AST, even though lexically
        // is comes after the parameters. We don't rely on the order here.
        // Note that there is a second pair of angle brackets. That one is part of
        // a TemplateSpecialization, which is handled further below.
        const auto firstTemplateParam = std::find_if(children.begin(), children.end(),
                                                     isTemplateParamDecl);
        if (firstTemplateParam == children.end())
            return;
        const auto lastTemplateParam = std::find_if(children.rbegin(), children.rend(),
                                                    isTemplateParamDecl);
        QTC_ASSERT(lastTemplateParam != children.rend(), return);
        const auto typeAlias = std::find_if(children.begin(), children.end(),
                [](const AstNode &n) { return n.kind() == "TypeAlias"; });
        if (typeAlias == children.end())
            return;

        const int firstTemplateParamStartPos = posForNodeStart(*firstTemplateParam);
        const int lastTemplateParamEndPos = posForNodeEnd(*lastTemplateParam);
        const int searchEndPos = posForNodeStart(*typeAlias);
        insertAngleBracketInfo(nodeStartPos, firstTemplateParamStartPos,
                               lastTemplateParamEndPos, searchEndPos);
        return;
    }

    if (isDeclaration && node.kind() == "ClassTemplateSpecialization") {
        // There is one child of kind TemplateSpecialization. The first pair
        // of angle brackets comes before that.
        if (children.size() == 1) {
            const int childNodePos = posForNodeStart(children.first());
            insertAngleBracketInfo(nodeStartPos, childNodePos, nodeStartPos, childNodePos);
        }
        return;
    }

    if (isDeclaration && node.kind() == "TemplateTemplateParm") {
        // The child nodes are template arguments and template parameters.
        // Arguments seem to appear before parameters in the AST, even though they
        // come after them in the source code. We don't rely on the order here.
        const auto firstTemplateParam = std::find_if(children.begin(), children.end(),
                                                     isTemplateParamDecl);
        if (firstTemplateParam == children.end())
            return;
        const auto lastTemplateParam = std::find_if(children.rbegin(), children.rend(),
                                                    isTemplateParamDecl);
        QTC_ASSERT(lastTemplateParam != children.rend(), return);
        const auto templateArg = std::find_if(children.begin(), children.end(),
                [](const AstNode &n) { return n.role() == "template argument"; });

        const int firstTemplateParamStartPos = posForNodeStart(*firstTemplateParam);
        const int lastTemplateParamEndPos = posForNodeEnd(*lastTemplateParam);
        const int searchEndPos = templateArg == children.end()
                ? nodeEndPos : posForNodeStart(*templateArg);
        insertAngleBracketInfo(nodeStartPos, firstTemplateParamStartPos,
                               lastTemplateParamEndPos, searchEndPos);
        return;
    }

    // {static,dynamic,reinterpret}_cast<>().
    if (isExpression && node.kind().startsWith("CXX") && node.kind().endsWith("Cast")) {
        // First child is type, second child is expression.
        // The opening angle bracket is before the first child, the closing angle bracket
        // is between the two children.
        if (children.size() == 2) {
            insertAngleBracketInfo(nodeStartPos, posForNodeStart(children.first()),
                                   posForNodeEnd(children.first()),
                                   posForNodeStart(children.last()));
        }
        return;
    }

    if (node.kind() == "TemplateSpecialization") {
        // First comes the template type, then the template arguments.
        // The opening angle bracket is before the first template argument,
        // the closing angle bracket is after the last template argument.
        // The first child node has no range, so we start searching at the parent node.
        if (children.size() >= 2) {
            int searchStart2 = posForNodeEnd(children.last());
            int searchEnd2 = nodeEndPos;

            // There is a weird off-by-one error on the clang side: If there is a
            // nested template instantiation *and* there is no space between
            // the closing angle brackets, then the inner TemplateSpecialization node's range
            // will extend one character too far, covering the outer's closing angle bracket.
            // This is what we are correcting for here.
            // This issue is tracked at https://github.com/clangd/clangd/issues/871.
            if (searchStart2 == searchEnd2)
                --searchStart2;
            insertAngleBracketInfo(nodeStartPos, posForNodeStart(children.at(1)),
                                   searchStart2, searchEnd2);
        }
        return;
    }

    if (!isExpression && !isDeclaration)
        return;

    // Operators, overloaded ones in particular.
    static const QString operatorPrefix = "operator";
    QString detail = node.detail().value_or(QString());
    const bool isCallToNew = node.kind() == "CXXNew";
    const bool isCallToDelete = node.kind() == "CXXDelete";
    if (!isCallToNew && !isCallToDelete
            && (!detail.startsWith(operatorPrefix) || detail == operatorPrefix)) {
        return;
    }

    if (!isCallToNew && !isCallToDelete)
        detail.remove(0, operatorPrefix.length());

    HighlightingResult result;
    result.useTextSyles = true;
    const bool isConversionOp = node.kind() == "CXXConversion";
    const bool isOverloaded = !isConversionOp
            && (isDeclaration || ((!isCallToNew && !isCallToDelete)
                                  || node.arcanaContains("CXXMethod")));
    result.textStyles.mainStyle = isConversionOp
            ? C_PRIMITIVE_TYPE
            : isCallToNew || isCallToDelete || detail.at(0).isSpace()
              ? C_KEYWORD : C_PUNCTUATION;
    result.textStyles.mixinStyles.push_back(C_OPERATOR);
    if (isOverloaded)
        result.textStyles.mixinStyles.push_back(C_OVERLOADED_OPERATOR);
    if (isDeclaration)
        result.textStyles.mixinStyles.push_back(C_DECLARATION);

    const QStringView nodeText = subViewEnd(m_docContent, nodeStartPos, nodeEndPos);

    if (isCallToNew || isCallToDelete) {
        result.line = node.range().start().line() + 1;
        result.column = node.range().start().character() + 1;
        result.length = isCallToNew ? 3 : 6;
        insertResult(result);
        if (node.arcanaContains("array")) {
            const int openingBracketOffset = nodeText.indexOf('[');
            if (openingBracketOffset == -1)
                return;
            const int closingBracketOffset = nodeText.lastIndexOf(']');
            if (closingBracketOffset == -1 || closingBracketOffset < openingBracketOffset)
                return;

            result.textStyles.mainStyle = C_PUNCTUATION;
            result.length = 1;
            Utils::Text::convertPosition(m_doc,
                                         nodeStartPos + openingBracketOffset,
                                         &result.line, &result.column);
            insertResult(result);
            Utils::Text::convertPosition(m_doc,
                                         nodeStartPos + closingBracketOffset,
                                         &result.line, &result.column);
            insertResult(result);
        }
        return;
    }

    if (isExpression && (detail == QLatin1String("()") || detail == QLatin1String("[]"))) {
        result.line = node.range().start().line() + 1;
        result.column = node.range().start().character() + 1;
        result.length = 1;
        insertResult(result);
        result.line = node.range().end().line() + 1;
        result.column = node.range().end().character();
        insertResult(result);
        return;
    }

    const int opStringLen = detail.at(0).isSpace() ? detail.length() - 1 : detail.length();

    // The simple case: Call to operator+, +=, * etc.
    if (nodeEndPos - nodeStartPos == opStringLen) {
        setResultPosFromRange(result, node.range());
        insertResult(result);
        return;
    }

    const int prefixOffset = nodeText.indexOf(operatorPrefix);
    if (prefixOffset == -1)
        return;

    const bool isArray = detail == "[]";
    const bool isCall = detail == "()";
    const bool isArrayNew = detail == " new[]";
    const bool isArrayDelete = detail == " delete[]";
    const QStringView searchTerm = isArray || isCall
            ? QStringView(detail).chopped(1) : isArrayNew || isArrayDelete
              ? QStringView(detail).chopped(2) : detail;
    const int opStringOffset = nodeText.indexOf(searchTerm, prefixOffset
                                                + operatorPrefix.length());
    if (opStringOffset == -1 || nodeText.indexOf(operatorPrefix, opStringOffset) != -1)
        return;

    const int opStringOffsetInDoc = nodeStartPos + opStringOffset
            + detail.length() - opStringLen;
    Utils::Text::convertPosition(m_doc, opStringOffsetInDoc, &result.line, &result.column);
    result.length = opStringLen;
    if (isArray || isCall)
        result.length = 1;
    else if (isArrayNew || isArrayDelete)
        result.length -= 2;
    if (!isArray && !isCall)
        insertResult(result);
    if (!isArray && !isCall && !isArrayNew && !isArrayDelete)
        return;

    result.textStyles.mainStyle = C_PUNCTUATION;
    result.length = 1;
    const int openingParenOffset = nodeText.indexOf(
                isCall ? '(' : '[', prefixOffset + operatorPrefix.length());
    if (openingParenOffset == -1)
        return;
    const int closingParenOffset = nodeText.indexOf(isCall ? ')' : ']', openingParenOffset + 1);
    if (closingParenOffset == -1 || closingParenOffset < openingParenOffset)
        return;
    Utils::Text::convertPosition(m_doc, nodeStartPos + openingParenOffset,
                                 &result.line, &result.column);
    insertResult(result);
    Utils::Text::convertPosition(m_doc, nodeStartPos + closingParenOffset,
                                 &result.line, &result.column);
    insertResult(result);
}

void ExtraHighlightingResultsCollector::visitNode(const AstNode &node)
{
    if (m_future.isCanceled())
        return;
    switch (node.fileStatus(m_filePath)) {
    case AstNode::FileStatus::Ours:
    case AstNode::FileStatus::Unknown:
        collectFromNode(node);
        [[fallthrough]];
    case AstNode::FileStatus::Foreign:
    case ClangCodeModel::Internal::AstNode::FileStatus::Mixed: {
        const auto children = node.children();
        if (!children)
            return;
        for (const AstNode &childNode : *children)
            visitNode(childNode);
        break;
    }
    }
}

bool ClangdClient::FollowSymbolData::defLinkIsAmbiguous() const
{
    // Even if the call is to a virtual function, it might not be ambiguous:
    // class A { virtual void f(); }; class B : public A { void f() override { A::f(); } };
    if (!cursorNode->mightBeAmbiguousVirtualCall() && !cursorNode->isPureVirtualDeclaration())
        return false;

    // If we have up-to-date highlighting info, we know whether we are dealing with
    // a virtual call.
    if (editorWidget) {
        const auto virtualRanges = q->d->virtualRanges.constFind(editorWidget->textDocument());
        if (virtualRanges != q->d->virtualRanges.constEnd()
                && virtualRanges->second == docRevision) {
            const auto matcher = [cursorRange = cursorNode->range()](const Range &r) {
                return cursorRange.overlaps(r);
            };
            return Utils::contains(virtualRanges->first, matcher);
        }
    }

    // Otherwise, we accept potentially doing more work than needed rather than not catching
    // possible overrides.
    return true;
}

class MemoryTree : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // number of bytes used, including child components
    qint64 total() const { return qint64(typedValue<double>(totalKey())); }

    // number of bytes used, excluding child components
    qint64 self() const { return qint64(typedValue<double>(selfKey())); }

    // named child components
    using NamedComponent = std::pair<MemoryTree, QString>;
    QList<NamedComponent> children() const
    {
        QList<NamedComponent> components;
        const auto obj = operator const QJsonObject &();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it.key() == totalKey() || it.key() == selfKey())
                continue;
            components << std::make_pair(MemoryTree(it.value()), it.key());
        }
        return components;
    }

private:
    static QString totalKey() { return QLatin1String("_total"); }
    static QString selfKey() { return QLatin1String("_self"); }
};

class MemoryTreeItem : public Utils::TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(MemoryTreeItem)
public:
    MemoryTreeItem(const QString &displayName, const MemoryTree &tree)
        : m_displayName(displayName), m_bytesUsed(tree.total())
    {
        for (const MemoryTree::NamedComponent &component : tree.children())
            appendChild(new MemoryTreeItem(component.second, component.first));
    }

private:
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            if (column == 0)
                return m_displayName;
            return memString();
        case Qt::TextAlignmentRole:
            if (column == 1)
                return Qt::AlignRight;
            break;
        default:
            break;
        }
        return {};
    }

    QString memString() const
    {
        static const QList<std::pair<int, QString>> factors{
            std::make_pair(1000000000, QString("GB")),
            std::make_pair(1000000, QString("MB")),
            std::make_pair(1000, QString("KB")),
        };
        for (const auto &factor : factors) {
            if (m_bytesUsed > factor.first)
                return QString::number(qint64(std::round(double(m_bytesUsed) / factor.first)))
                        + ' ' + factor.second;
        }
        return QString::number(m_bytesUsed) + "  B";
    }

    const QString m_displayName;
    const qint64 m_bytesUsed;
};

class MemoryTreeModel : public Utils::BaseTreeModel
{
public:
    MemoryTreeModel(QObject *parent) : BaseTreeModel(parent)
    {
        setHeader({tr("Component"), tr("Total Memory")});
    }

    void update(const MemoryTree &tree)
    {
        setRootItem(new MemoryTreeItem({}, tree));
    }
};

MemoryUsageWidget::MemoryUsageWidget(ClangdClient *client)
    : m_client(client), m_model(new MemoryTreeModel(this))
{
    setupUi();
    getMemoryTree();
}

MemoryUsageWidget::~MemoryUsageWidget()
{
    if (m_currentRequest.has_value())
        m_client->cancelRequest(m_currentRequest.value());
}

void MemoryUsageWidget::setupUi()
{
    const auto layout = new QVBoxLayout(this);
    m_view.setContextMenuPolicy(Qt::CustomContextMenu);
    m_view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_view.header()->setStretchLastSection(false);
    m_view.setModel(m_model);
    layout->addWidget(&m_view);
    connect(&m_view, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu;
        menu.addAction(tr("Update"), [this] { getMemoryTree(); });
        menu.exec(m_view.mapToGlobal(pos));
    });
}

void MemoryUsageWidget::getMemoryTree()
{
    Request<MemoryTree, std::nullptr_t, JsonObject> request("$/memoryUsage", {});
    request.setResponseCallback([this](decltype(request)::Response response) {
        m_currentRequest.reset();
        qCDebug(clangdLog) << "received memory usage response";
        if (const auto result = response.result())
            m_model->update(*result);
    });
    qCDebug(clangdLog) << "sending memory usage request";
    m_currentRequest = request.id();
    m_client->sendContent(request, ClangdClient::SendDocUpdates::Ignore);
}

} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ReplacementData)
