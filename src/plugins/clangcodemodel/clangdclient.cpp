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

#include "clangcompletioncontextanalyzer.h"
#include "clangconstants.h"
#include "clangdast.h"
#include "clangdfollowsymbol.h"
#include "clangdlocatorfilters.h"
#include "clangdswitchdecldef.h"
#include "clangpreprocessorassistproposalitem.h"
#include "clangtextmark.h"
#include "clangutils.h"
#include "clangdsemantichighlighting.h"
#include "tasktimers.h"

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
#include <cppeditor/cppsemanticinfo.h>
#include <languageclient/diagnosticmanager.h>
#include <languageclient/languageclientcompletionassist.h>
#include <languageclient/languageclientfunctionhint.h>
#include <languageclient/languageclienthoverhandler.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientquickfix.h>
#include <languageclient/languageclientsymbolsupport.h>
#include <languageclient/languageclientutils.h>
#include <languageserverprotocol/clientcapabilities.h>
#include <languageserverprotocol/progresssupport.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/taskhub.h>
#include <texteditor/basefilefind.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/codeassist/textdocumentmanipulatorinterface.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/itemviews.h>
#include <utils/runextensions.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QHeaderView>
#include <QMenu>
#include <QPair>
#include <QPointer>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QWidget>

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

Q_LOGGING_CATEGORY(clangdLog, "qtc.clangcodemodel.clangd", QtWarningMsg);
Q_LOGGING_CATEGORY(clangdLogAst, "qtc.clangcodemodel.clangd.ast", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogServer, "qtc.clangcodemodel.clangd.server", QtWarningMsg);
static Q_LOGGING_CATEGORY(clangdLogCompletion, "qtc.clangcodemodel.clangd.completion",
                          QtWarningMsg);
static QString indexingToken() { return "backgroundIndexProgress"; }

static Usage::Type getUsageType(const ClangdAstPath &path)
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

    static constexpr char16_t usrKey[] = u"usr";

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
    const Utils::FilePath targetConfigFile = CppEditor::ClangdSettings::clangdUserConfigFilePath();
    const Utils::FilePath baseDir = targetConfigFile.parentDir();
    baseDir.ensureWritableDir();
    Utils::FileReader configReader;
    const QByteArray firstLine = "# This file was generated by Qt Creator and will be overwritten "
                                 "unless you remove this line.";
    if (!configReader.fetch(targetConfigFile) || configReader.data().startsWith(firstLine)) {
        Utils::FileSaver saver(targetConfigFile);
        saver.write(firstLine + '\n');
        saver.write("Hover:\n");
        saver.write("  ShowAKA: Yes\n");
        saver.write("Diagnostics:\n");
        saver.write("  UnusedIncludes: Strict\n");
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

    bool ok = false;
    const int userValue
        = Utils::Environment::systemEnvironment().value("QTC_CLANGD_COMPLETION_RESULTS").toInt(&ok);
    const QString limitResults = QString("--limit-results=%1").arg(ok ? userValue : 0);

    Utils::CommandLine cmd{settings.clangdFilePath(), {indexingOption, headerInsertionOption,
            limitResults, "--limit-references=0", "--clang-tidy=0"}};
    if (settings.workerThreadLimit() != 0)
        cmd.addArg("-j=" + QString::number(settings.workerThreadLimit()));
    if (!jsonDbDir.isEmpty())
        cmd.addArg("--compile-commands-dir=" + jsonDbDir.toString());
    if (clangdLogServer().isDebugEnabled())
        cmd.addArgs({"--log=verbose", "--pretty"});
    cmd.addArg("--use-dirty-headers");
    const auto interface = new StdIOClientInterface;
    interface->setCommandLine(cmd);
    return interface;
}

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

class LocalRefsData {
public:
    LocalRefsData(quint64 id, TextDocument *doc, const QTextCursor &cursor,
                  CppEditor::RenameCallback &&callback)
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
    CppEditor::RenameCallback callback;
    const DocumentUri uri;
    const int revision;
};

class DiagnosticsCapabilities : public JsonObject
{
public:
    using JsonObject::JsonObject;
    void enableCategorySupport() { insert(u"categorySupport", true); }
    void enableCodeActionsInline() {insert(u"codeActionsInline", true);}
};

class ClangdTextDocumentClientCapabilities : public TextDocumentClientCapabilities
{
public:
    using TextDocumentClientCapabilities::TextDocumentClientCapabilities;


    void setPublishDiagnostics(const DiagnosticsCapabilities &caps)
    { insert(u"publishDiagnostics", caps); }
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
            completions = completeInclude(m_endPos, m_completionOperator, interface, headerPaths);
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

    /**
     * @brief Creates completion proposals for #include and given cursor
     * @param position - cursor placed after opening bracked or quote
     * @param completionOperator - the type of token
     * @param interface - relevant document data
     * @param headerPaths - the include paths
     * @return the list of completion items
     */
    static QList<AssistProposalItemInterface *> completeInclude(
            int position, unsigned completionOperator, const TextEditor::AssistInterface *interface,
            const ProjectExplorer::HeaderPaths &headerPaths)
    {
        QTextCursor cursor(interface->textDocument());
        cursor.setPosition(position);
        QString directoryPrefix;
        if (completionOperator == T_SLASH) {
            QTextCursor c = cursor;
            c.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString sel = c.selectedText();
            int startCharPos = sel.indexOf(QLatin1Char('"'));
            if (startCharPos == -1) {
                startCharPos = sel.indexOf(QLatin1Char('<'));
                completionOperator = T_ANGLE_STRING_LITERAL;
            } else {
                completionOperator = T_STRING_LITERAL;
            }
            if (startCharPos != -1)
                directoryPrefix = sel.mid(startCharPos + 1, sel.length() - 1);
        }

        // Make completion for all relevant includes
        ProjectExplorer::HeaderPaths allHeaderPaths = headerPaths;
        const auto currentFilePath = ProjectExplorer::HeaderPath::makeUser(
                    interface->filePath().toFileInfo().path());
        if (!allHeaderPaths.contains(currentFilePath))
            allHeaderPaths.append(currentFilePath);

        const ::Utils::MimeType mimeType = ::Utils::mimeTypeForName("text/x-c++hdr");
        const QStringList suffixes = mimeType.suffixes();

        QList<AssistProposalItemInterface *> completions;
        for (const ProjectExplorer::HeaderPath &headerPath : qAsConst(allHeaderPaths)) {
            QString realPath = headerPath.path;
            if (!directoryPrefix.isEmpty()) {
                realPath += QLatin1Char('/');
                realPath += directoryPrefix;
                if (headerPath.type == ProjectExplorer::HeaderPathType::Framework)
                    realPath += QLatin1String(".framework/Headers");
            }
            completions << completeIncludePath(realPath, suffixes, completionOperator);
        }

        QList<QPair<AssistProposalItemInterface *, QString>> completionsForSorting;
        for (AssistProposalItemInterface * const item : qAsConst(completions)) {
            QString s = item->text();
            s.replace('/', QChar(0)); // The dir separator should compare less than anything else.
            completionsForSorting << qMakePair(item, s);
        }
        Utils::sort(completionsForSorting, [](const auto &left, const auto &right) {
            return left.second < right.second;
        });
        for (int i = 0; i < completionsForSorting.count(); ++i)
            completions[i] = completionsForSorting[i].first;

        return completions;
    }

    /**
     * @brief Finds #include completion proposals using given include path
     * @param realPath - one of directories where compiler searches includes
     * @param suffixes - file suffixes for C/C++ header files
     * @return a list of matching completion items
     */
    static QList<AssistProposalItemInterface *> completeIncludePath(
            const QString &realPath, const QStringList &suffixes, unsigned completionOperator)
    {
        QList<AssistProposalItemInterface *> completions;
        QDirIterator i(realPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        //: Parent folder for proposed #include completion
        const QString hint = ClangdClient::tr("Location: %1")
                .arg(QDir::toNativeSeparators(QDir::cleanPath(realPath)));
        while (i.hasNext()) {
            const QString fileName = i.next();
            const QFileInfo fileInfo = i.fileInfo();
            const QString suffix = fileInfo.suffix();
            if (suffix.isEmpty() || suffixes.contains(suffix)) {
                QString text = fileName.mid(realPath.length() + 1);
                if (fileInfo.isDir())
                    text += QLatin1Char('/');

                auto *item = new ClangPreprocessorAssistProposalItem;
                item->setText(text);
                item->setDetail(hint);
                item->setIcon(CPlusPlus::Icons::keywordIcon());
                item->setCompletionOperator(completionOperator);
                completions.append(item);
            }
        }
        return completions;
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

class HighlightingData
{
public:
    // For all QPairs, the int member is the corresponding document version.
    QPair<QList<ExpandedSemanticToken>, int> previousTokens;

    // The ranges of symbols referring to virtual functions,
    // as extracted by the highlighting procedure.
    QPair<QList<Range>, int> virtualRanges;

    // The highlighter is owned by its document.
    CppEditor::SemanticHighlighter *highlighter = nullptr;
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

    void handleDeclDefSwitchReplies();

    Utils::optional<QString> getContainingFunctionName(const ClangdAstPath &astPath, const Range& range);

    static CppEditor::CppEditorWidget *widgetFromDocument(const TextDocument *doc);
    QString searchTermFromCursor(const QTextCursor &cursor) const;
    QTextCursor adjustedCursor(const QTextCursor &cursor, const TextDocument *doc);

    void setHelpItemForTooltip(const MessageId &token, const QString &fqn = {},
                               HelpItem::Category category = HelpItem::Unknown,
                               const QString &type = {});

    void handleSemanticTokens(TextDocument *doc, const QList<ExpandedSemanticToken> &tokens,
                              int version, bool force);

    MessageId getAndHandleAst(const TextDocOrFile &doc, const AstHandler &astHandler,
                              AstCallbackMode callbackMode, const Range &range = {});

    ClangdClient * const q;
    const CppEditor::ClangdSettings::Data settings;
    QHash<quint64, ReferencesData> runningFindUsages;
    ClangdFollowSymbol *followSymbol = nullptr;
    ClangdSwitchDeclDef *switchDeclDef = nullptr;
    Utils::optional<LocalRefsData> localRefsData;
    Utils::optional<QVersionNumber> versionNumber;

    QHash<TextDocument *, HighlightingData> highlightingData;
    QHash<Utils::FilePath, CppEditor::BaseEditorDocumentParser::Configuration> parserConfigs;
    QHash<Utils::FilePath, Tasks> issuePaneEntries;

    VersionedDataCache<const TextDocument *, ClangdAstNode> astCache;
    VersionedDataCache<Utils::FilePath, ClangdAstNode> externalAstCache;
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
        insert(u"editsNearCursor", true); // For dot-to-arrow correction.
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
    QString text() const override;
};

class ClangdClient::ClangdCompletionAssistProcessor : public LanguageClientCompletionAssistProcessor
{
public:
    ClangdCompletionAssistProcessor(ClangdClient *client, const QString &snippetsGroup)
        : LanguageClientCompletionAssistProcessor(client, snippetsGroup)
        , m_client(client)
    {
        m_timer.start();
    }

    ~ClangdCompletionAssistProcessor()
    {
        qCDebug(clangdLogTiming).noquote().nospace()
            << "ClangdCompletionAssistProcessor took: " << m_timer.elapsed() << " ms";
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
    QElapsedTimer m_timer;
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
                                    ->getSignalSlotType(filePath().toString(), content.toUtf8(), pos)
                                == CppEditor::SignalSlotType::NewStyleSignal;
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

    TextEditor::GenericProposal *handleCodeActionResult(const CodeActionResult &result) override
    {
        auto toOperation =
            [=](const Utils::variant<Command, CodeAction> &item) -> QuickFixOperation * {
            if (auto action = Utils::get_if<CodeAction>(&item)) {
                const Utils::optional<QList<Diagnostic>> diagnostics = action->diagnostics();
                if (!diagnostics.has_value() || diagnostics->isEmpty())
                    return new CodeActionQuickFixOperation(*action, client());
            }
            if (auto command = Utils::get_if<Command>(&item))
                return new CommandQuickFixOperation(*command, client());
            return nullptr;
        };

        if (auto list = Utils::get_if<QList<Utils::variant<Command, CodeAction>>>(&result)) {
            QuickFixOperations ops;
            for (const Utils::variant<Command, CodeAction> &item : *list) {
                if (QuickFixOperation *op = toOperation(item)) {
                    op->setDescription("clangd: " + op->description());
                    ops << op;
                }
            }
            return GenericProposal::createProposal(m_interface, ops + m_builtinOps);
        }
        return nullptr;
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

static void addToCompilationDb(QJsonObject &cdb,
                               const CppEditor::ProjectPart &projectPart,
                               CppEditor::UsePrecompiledHeaders usePch,
                               const QJsonArray &projectPartOptions,
                               const Utils::FilePath &workingDir,
                               const CppEditor::ProjectFile &sourceFile,
                               bool clStyle)
{
    QJsonArray args = clangOptionsForFile(sourceFile, projectPart, projectPartOptions, usePch,
                                          clStyle);

    // TODO: clangd seems to apply some heuristics depending on what we put here.
    //       Should we make use of them or keep using our own?
    args.prepend("clang");

    const QString fileString = Utils::FilePath::fromString(sourceFile.path).toUserOutput();
    args.append(fileString);
    QJsonObject value;
    value.insert("workingDirectory", workingDir.toString());
    value.insert("compilationCommand", args);
    cdb.insert(fileString, value);
}

static void addCompilationDb(QJsonObject &parentObject, const QJsonObject &cdb)
{
    parentObject.insert("compilationDatabaseChanges", cdb);
}

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
        const Utils::FilePath includeDir
                = CppEditor::ClangdSettings(d->settings).clangdIncludePath();
        CppEditor::CompilerOptionsBuilder optionsBuilder = clangOptionsBuilder(
                    *CppEditor::CppModelManager::instance()->fallbackProjectPart(),
                    warningsConfigForProject(nullptr), includeDir);
        const CppEditor::UsePrecompiledHeaders usePch = CppEditor::getPchUsage();
        const QJsonArray projectPartOptions = fullProjectPartOptions(
                    optionsBuilder, globalClangOptions());
        const QJsonArray clangOptions = clangOptionsForFile({}, optionsBuilder.projectPart(),
                                                            projectPartOptions, usePch,
                                                            optionsBuilder.isClStyle());
        initOptions.insert("fallbackFlags", clangOptions);
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
    if (project) {
        setProgressTitleForToken(indexingToken(),
                                 tr("Indexing %1 with clangd").arg(project->displayName()));
    }
    setCurrentProject(project);
    setDocumentChangeUpdateThreshold(d->settings.documentUpdateThreshold);
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
        auto currentDocumentFilter = static_cast<ClangdCurrentDocumentFilter *>(
            CppEditor::CppModelManager::instance()->currentDocumentFilter());
        currentDocumentFilter->updateCurrentClient();
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
    if (d->followSymbol)
        d->followSymbol->clear();
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
    sendMessage(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)),
                SendDocUpdates::Ignore);
}

void ClangdClient::closeExtraFile(const Utils::FilePath &filePath)
{
    sendMessage(DidCloseTextDocumentNotification(DidCloseTextDocumentParams(
            TextDocumentIdentifier{DocumentUri::fromFilePath(filePath)})),
                SendDocUpdates::Ignore);
}

void ClangdClient::findUsages(TextDocument *document, const QTextCursor &cursor,
                              const Utils::optional<QString> &replacement)
{
    // Quick check: Are we even on anything searchable?
    const QTextCursor adjustedCursor = d->adjustedCursor(cursor, document);
    const QString searchTerm = d->searchTermFromCursor(adjustedCursor);
    if (searchTerm.isEmpty())
        return;

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
    const auto symbolInfoHandler = [this, doc = QPointer(document), adjustedCursor, replacement, categorize]
            (const QString &name, const QString &, const MessageId &) {
        if (!doc)
            return;
        if (name.isEmpty())
            return;
        d->findUsages(doc.data(), adjustedCursor, name, replacement, categorize);
    };
    requestSymbolInfo(document->filePath(), Range(adjustedCursor).start(), symbolInfoHandler);
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
        auto codeActions = clangdDiagnostic.codeActions();
        if (codeActions && !codeActions->isEmpty()) {
            for (CodeAction &action : *codeActions)
                action.setDiagnostics({diagnostic});
            LanguageClient::updateCodeActionRefactoringMarker(this, *codeActions, uri);
        } else {
            // We know that there's only one kind of diagnostic for which clangd has
            // a quickfix tweak, so let's not be wasteful.
            const Diagnostic::Code code = diagnostic.code().value_or(Diagnostic::Code());
            const QString * const codeString = Utils::get_if<QString>(&code);
            if (codeString && *codeString == "-Wswitch")
                requestCodeActions(uri, diagnostic);
        }
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
    d->highlightingData.remove(doc);
    d->astCache.remove(doc);
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

class ClangdDiagnosticManager : public LanguageClient::DiagnosticManager
{
    using LanguageClient::DiagnosticManager::DiagnosticManager;

    ClangdClient *getClient() const { return qobject_cast<ClangdClient *>(client()); }

    bool isCurrentDocument(const Utils::FilePath &filePath) const
    {
        const IDocument * const doc = EditorManager::currentDocument();
        return doc && doc->filePath() == filePath;
    }

    void showDiagnostics(const DocumentUri &uri, int version) override
    {
        const Utils::FilePath filePath = uri.toFilePath();
        getClient()->clearTasks(filePath);
        DiagnosticManager::showDiagnostics(uri, version);
        if (isCurrentDocument(filePath))
            getClient()->switchIssuePaneEntries(filePath);
    }

    void hideDiagnostics(const Utils::FilePath &filePath) override
    {
        DiagnosticManager::hideDiagnostics(filePath);
        if (isCurrentDocument(filePath))
            TaskHub::clearTasks(Constants::TASK_CATEGORY_DIAGNOSTICS);
    }

    QList<Diagnostic> filteredDiagnostics(const QList<Diagnostic> &diagnostics) const override
    {
        return Utils::filtered(diagnostics, [](const Diagnostic &diag){
            const Diagnostic::Code code = diag.code().value_or(Diagnostic::Code());
            const QString * const codeString = Utils::get_if<QString>(&code);
            return !codeString || *codeString != "drv_unknown_argument";
        });
    }

    TextMark *createTextMark(const Utils::FilePath &filePath,
                             const Diagnostic &diagnostic,
                             bool isProjectFile) const override
    {
        return new ClangdTextMark(filePath, diagnostic, isProjectFile, getClient());
    }
};

DiagnosticManager *ClangdClient::createDiagnosticManager()
{
    auto diagnosticManager = new ClangdDiagnosticManager(this);
    if (d->isTesting) {
        connect(diagnosticManager, &DiagnosticManager::textMarkCreated,
                this, &ClangdClient::textMarkCreated);
    }
    return diagnosticManager;
}

bool ClangdClient::referencesShadowFile(const TextEditor::TextDocument *doc,
                                        const Utils::FilePath &candidate)
{
    const QRegularExpression includeRex("#include.*" + candidate.fileName() + R"([>"])");
    const QTextCursor includePos = doc->document()->find(includeRex);
    return !includePos.isNull();
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
    case SymbolKind::Function: {
        const int lastParenOffset = detail.lastIndexOf(')');
        if (lastParenOffset == -1)
            return name;
        int leftParensNeeded = 1;
        int i = -1;
        for (i = lastParenOffset - 1; i >= 0; --i) {
            switch (detail.at(i).toLatin1()) {
            case ')':
                ++leftParensNeeded;
                break;
            case '(':
                --leftParensNeeded;
                break;
            default:
                break;
            }
            if (leftParensNeeded == 0)
                break;
        }
        if (leftParensNeeded > 0)
            return name;
        return name + detail.mid(i) + " -> " + detail.left(i);
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
    const QList<Client *> &allClients = LanguageClientManager::clients();
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
    QJsonObject cdbChanges;
    const Utils::FilePath includeDir = CppEditor::ClangdSettings(d->settings).clangdIncludePath();
    CppEditor::CompilerOptionsBuilder optionsBuilder = clangOptionsBuilder(
                *projectPart, warningsConfigForProject(project()), includeDir);
    const CppEditor::ProjectFile file(filePath.toString(),
                                      CppEditor::ProjectFile::classify(filePath.toString()));
    const QJsonArray projectPartOptions = fullProjectPartOptions(
                optionsBuilder, globalClangOptions());
    addToCompilationDb(cdbChanges, *projectPart, CppEditor::getPchUsage(), projectPartOptions,
                       filePath.parentDir(), file, optionsBuilder.isClStyle());
    QJsonObject settings;
    addCompilationDb(settings, cdbChanges);
    DidChangeConfigurationParams configChangeParams;
    configChangeParams.setSettings(settings);
    sendMessage(DidChangeConfigurationNotification(configChangeParams));
}

void ClangdClient::switchIssuePaneEntries(const Utils::FilePath &filePath)
{
    TaskHub::clearTasks(Constants::TASK_CATEGORY_DIAGNOSTICS);
    const Tasks tasks = d->issuePaneEntries.value(filePath);
    for (const Task &t : tasks)
        TaskHub::addTask(t);
}

void ClangdClient::addTask(const ProjectExplorer::Task &task)
{
    d->issuePaneEntries[task.file] << task;
}

void ClangdClient::clearTasks(const Utils::FilePath &filePath)
{
    d->issuePaneEntries[filePath].clear();
}

Utils::optional<bool> ClangdClient::hasVirtualFunctionAt(TextDocument *doc, int revision,
                                                         const Range &range)
{
    const auto highlightingData = d->highlightingData.constFind(doc);
    if (highlightingData == d->highlightingData.constEnd()
            || highlightingData->virtualRanges.second != revision) {
        return {};
    }
    const auto matcher = [range](const Range &r) { return range.overlaps(r); };
    return Utils::contains(highlightingData->virtualRanges.first, matcher);
}

MessageId ClangdClient::getAndHandleAst(const TextDocOrFile &doc, const AstHandler &astHandler,
                                        AstCallbackMode callbackMode, const Range &range)
{
    return d->getAndHandleAst(doc, astHandler, callbackMode, range);
}

MessageId ClangdClient::requestSymbolInfo(const Utils::FilePath &filePath, const Position &position,
                                          const SymbolInfoHandler &handler)
{
    const TextDocumentIdentifier docId(DocumentUri::fromFilePath(filePath));
    const TextDocumentPositionParams params(docId, position);
    SymbolInfoRequest symReq(params);
    symReq.setResponseCallback([handler, reqId = symReq.id()]
                               (const SymbolInfoRequest::Response &response) {
        const auto result = response.result();
        if (!result) {
            handler({}, {}, reqId);
            return;
        }

        // According to the documentation, we should receive a single
        // object here, but it's a list. No idea what it means if there's
        // more than one entry. We choose the first one.
        const auto list = Utils::get_if<QList<SymbolDetails>>(&result.value());
        if (!list || list->isEmpty()) {
            handler({}, {}, reqId);
            return;
        }

        const SymbolDetails &sd = list->first();
        handler(sd.name(), sd.containerName(), reqId);
    });
    sendMessage(symReq);
    return symReq.id();
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
    if (refData->replacementData || !refData->categorize) {
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
        const auto astHandler = [this, key, loc = it.key()](const ClangdAstNode &ast,
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
        const ClangdAstPath astPath = getAstPath(fileData.ast, range);
        const Usage::Type usageType = fileData.ast.isValid() ? getUsageType(astPath)
                                                             : Usage::Type::Other;

        SearchResultItem item;
        item.setUserData(int(usageType));
        item.setStyle(CppEditor::colorStyleForUsageType(usageType));
        item.setFilePath(file);
        item.setMainRange(SymbolSupport::convertRange(range));
        item.setUseTextEditorFont(true);
        item.setLineText(rangeWithText.second);
        item.setContainingFunctionName(getContainingFunctionName(astPath, range));

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
        const Utils::LinkHandler &callback,
        bool resolveTarget,
        bool openInSplit
        )
{
    QTC_ASSERT(documentOpen(document), openDocument(document));

    delete d->followSymbol;
    d->followSymbol = nullptr;

    const QTextCursor adjustedCursor = d->adjustedCursor(cursor, document);
    if (!resolveTarget) {
        symbolSupport().findLinkAt(document, adjustedCursor, callback, false);
        return;
    }

    qCDebug(clangdLog) << "follow symbol requested" << document->filePath()
                       << adjustedCursor.blockNumber() << adjustedCursor.positionInBlock();
    d->followSymbol = new ClangdFollowSymbol(this, adjustedCursor, editorWidget, document, callback,
                                             openInSplit);
    connect(d->followSymbol, &ClangdFollowSymbol::done, this, [this] {
        d->followSymbol->deleteLater();
        d->followSymbol = nullptr;
    });
}

void ClangdClient::switchDeclDef(TextDocument *document, const QTextCursor &cursor,
                                 CppEditor::CppEditorWidget *editorWidget,
                                 const Utils::LinkHandler &callback)
{
    QTC_ASSERT(documentOpen(document), openDocument(document));

    qCDebug(clangdLog) << "switch decl/dev requested" << document->filePath()
                       << cursor.blockNumber() << cursor.positionInBlock();
    if (d->switchDeclDef)
        delete d->switchDeclDef;
    d->switchDeclDef = new ClangdSwitchDeclDef(this, document, cursor, editorWidget, callback);
    connect(d->switchDeclDef, &ClangdSwitchDeclDef::done, this, [this] {
        d->switchDeclDef->deleteLater();
        d->switchDeclDef = nullptr;
    });
}

void ClangdClient::switchHeaderSource(const Utils::FilePath &filePath, bool inNextSplit)
{
    class SwitchSourceHeaderRequest : public Request<QJsonValue, std::nullptr_t, TextDocumentIdentifier>
    {
    public:
        using Request::Request;
        explicit SwitchSourceHeaderRequest(const Utils::FilePath &filePath)
            : Request("textDocument/switchSourceHeader",
                      TextDocumentIdentifier(DocumentUri::fromFilePath(filePath))) {}
    };
    SwitchSourceHeaderRequest req(filePath);
    req.setResponseCallback([inNextSplit](const SwitchSourceHeaderRequest::Response &response) {
        if (const Utils::optional<QJsonValue> result = response.result()) {
            const DocumentUri uri = DocumentUri::fromProtocol(result->toString());
            const Utils::FilePath filePath = uri.toFilePath();
            if (!filePath.isEmpty())
                CppEditor::openEditor(filePath, inNextSplit);
        }
    });
    sendMessage(req);
}

void ClangdClient::findLocalUsages(TextDocument *document, const QTextCursor &cursor,
        CppEditor::RenameCallback &&callback)
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
        if (!link.hasValidTarget() || !d->localRefsData->document
                || d->localRefsData->document->filePath() != link.targetFilePath) {
            d->localRefsData.reset();
            return;
        }

        // Step 2: Get AST and check whether it's a local variable.
        const auto astHandler = [this, link, id](const ClangdAstNode &ast, const MessageId &) {
            qCDebug(clangdLog) << "received ast response";
            if (!d->localRefsData || id != d->localRefsData->id)
                return;
            if (!ast.isValid() || !d->localRefsData->document) {
                d->localRefsData.reset();
                return;
            }

            const Position linkPos(link.targetLine - 1, link.targetColumn);
            const ClangdAstPath astPath = getAstPath(ast, linkPos);
            bool isVar = false;
            for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
                if (it->role() == "declaration"
                        && (it->kind() == "Function" || it->kind() == "CXXMethod"
                            || it->kind() == "CXXConstructor" || it->kind() == "CXXDestructor"
                            || it->kind() == "Lambda")) {
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
                        const Utils::Links links = Utils::transform(locations, &Location::toLink);

                        // The callback only uses the symbol length, so we just create a dummy.
                        // Note that the calculation will be wrong for identifiers with
                        // embedded newlines, but we've never supported that.
                        QString symbol;
                        if (!locations.isEmpty()) {
                            const Range r = locations.first().range();
                            symbol = QString(r.end().character() - r.start().character(), 'x');
                        }
                        d->localRefsData->callback(symbol, links, d->localRefsData->revision);
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
                           AstCallbackMode::SyncIfPossible);
    };
    symbolSupport().findLinkAt(document, cursor, std::move(gotoDefCallback), true);
}

void ClangdClient::gatherHelpItemForTooltip(const HoverRequest::Response &hoverResponse,
                                            const DocumentUri &uri)
{
    if (const Utils::optional<HoverResult> result = hoverResponse.result()) {
        if (auto hover = Utils::get_if<Hover>(&(*result))) {
            const HoverContent content = hover->content();
            const MarkupContent *const markup = Utils::get_if<MarkupContent>(&content);
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
                        d->setHelpItemForTooltip(hoverResponse.id(),
                                                 filePath.fileName(),
                                                 HelpItem::Brief);
                        return;
                    }
                }
            }
        }
    }

    const TextDocument * const doc = documentForFilePath(uri.toFilePath());
    QTC_ASSERT(doc, return);
    const auto astHandler = [this, uri, hoverResponse](const ClangdAstNode &ast, const MessageId &) {
        const MessageId id = hoverResponse.id();
        Range range;
        if (const Utils::optional<HoverResult> result = hoverResponse.result()) {
            if (auto hover = Utils::get_if<Hover>(&(*result)))
                range = hover->range().value_or(Range());
        }
        const ClangdAstPath path = getAstPath(ast, range);
        if (path.isEmpty()) {
            d->setHelpItemForTooltip(id);
            return;
        }
        ClangdAstNode node = path.last();
        if (node.role() == "expression" && node.kind() == "ImplicitCast") {
            const Utils::optional<QList<ClangdAstNode>> children = node.children();
            if (children && !children->isEmpty())
                node = children->first();
        }
        while (node.kind() == "Qualified") {
            const Utils::optional<QList<ClangdAstNode>> children = node.children();
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
            const auto symbolInfoHandler = [this, id, type, isFunction]
                    (const QString &name, const QString &prefix, const MessageId &) {
                qCDebug(clangdLog) << "handling symbol info reply";
                const QString fqn = prefix + name;

                // Unfortunately, the arcana string contains the signature only for
                // free functions, so we can't distinguish member function overloads.
                // But since HtmlDocExtractor::getFunctionDescription() is always called
                // with mainOverload = true, such information would get ignored anyway.
                if (!fqn.isEmpty())
                    d->setHelpItemForTooltip(id, fqn, HelpItem::Function, isFunction ? type : "()");
            };
            requestSymbolInfo(uri.toFilePath(), range.start(), symbolInfoHandler);
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
    d->getAndHandleAst(doc, astHandler, AstCallbackMode::SyncIfPossible);
}

void ClangdClient::setVirtualRanges(const Utils::FilePath &filePath, const QList<Range> &ranges,
                                    int revision)
{
    TextDocument * const doc = documentForFilePath(filePath);
    if (doc && doc->document()->revision() == revision)
        d->highlightingData[doc].virtualRanges = {ranges, revision};
}

Utils::optional<QString> ClangdClient::Private::getContainingFunctionName(
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
        return Utils::nullopt;

    return containingFuncNode->detail();
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
    if (builtinAstPath.isEmpty())
        return cursor;
    const TranslationUnit * const tu = cppDoc->translationUnit();
    const auto posForToken = [doc, tu](int tok) {
        int line, column;
        tu->getTokenPosition(tok, &line, &column);
        return Utils::Text::positionInText(doc->document(), line, column);
    };
    const auto endPosForToken = [doc, tu](int tok) {
        int line, column;
        tu->getTokenEndPosition(tok, &line, &column);
        return Utils::Text::positionInText(doc->document(), line, column);
    };
    const auto leftMovedCursor = [cursor] {
        QTextCursor c = cursor;
        c.setPosition(cursor.position() - 1);
        return c;
    };

    // enum E { v1|, v2 };
    if (const EnumeratorAST * const enumAst = builtinAstPath.last()->asEnumerator()) {
        if (endPosForToken(enumAst->identifier_token) == cursor.position())
            return leftMovedCursor();
        return cursor;
    }

    for (auto it = builtinAstPath.rbegin(); it != builtinAstPath.rend(); ++it) {

        // s|.x or s|->x
        if (const MemberAccessAST * const memberAccess = (*it)->asMemberAccess()) {
            switch (tu->tokenAt(memberAccess->access_token).kind()) {
            case T_DOT:
                break;
            case T_ARROW: {
                const Utils::optional<ClangdAstNode> clangdAst = astCache.get(doc);
                if (!clangdAst)
                    return cursor;
                const ClangdAstPath clangdAstPath = getAstPath(*clangdAst, Range(cursor));
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

        // QVector<QString|>
        if (const TemplateIdAST * const templAst = (*it)->asTemplateId()) {
            if (posForToken(templAst->greater_token) == cursor.position())
                return leftMovedCursor();
            return cursor;
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
    qCInfo(clangdLogHighlight) << "handling LSP tokens" << doc->filePath()
                               << version << tokens.size();
    if (version != q->documentVersion(doc->filePath())) {
        qCInfo(clangdLogHighlight) << "LSP tokens outdated; aborting highlighting procedure"
                                    << version << q->documentVersion(doc->filePath());
        return;
    }
    force = force || isTesting;
    const auto data = highlightingData.find(doc);
    if (data != highlightingData.end()) {
        if (!force && data->previousTokens.first == tokens
                && data->previousTokens.second == version) {
            qCInfo(clangdLogHighlight) << "tokens and version same as last time; nothing to do";
            return;
        }
        data->previousTokens.first = tokens;
        data->previousTokens.second = version;
    } else {
        HighlightingData data;
        data.previousTokens = qMakePair(tokens, version);
        highlightingData.insert(doc, data);
    }
    for (const ExpandedSemanticToken &t : tokens)
        qCDebug(clangdLogHighlight()) << '\t' << t.line << t.column << t.length << t.type
                                      << t.modifiers;

    const auto astHandler = [this, tokens, doc, version](const ClangdAstNode &ast, const MessageId &) {
        FinalizingSubtaskTimer t(highlightingTimer);
        if (!q->documentOpen(doc))
            return;
        if (version != q->documentVersion(doc->filePath())) {
            qCInfo(clangdLogHighlight) << "AST not up to date; aborting highlighting procedure"
                                        << version << q->documentVersion(doc->filePath());
            return;
        }
        if (clangdLogAst().isDebugEnabled())
            ast.print();

        const auto runner = [tokens, filePath = doc->filePath(),
                             text = doc->document()->toPlainText(), ast,
                             doc = QPointer(doc), rev = doc->document()->revision(),
                             clangdVersion = q->versionNumber(),
                             this] {
            return Utils::runAsync(doSemanticHighlighting, filePath, tokens, text, ast, doc, rev,
                                   clangdVersion, highlightingTimer);
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

        auto &data = highlightingData[doc];
        if (!data.highlighter)
            data.highlighter = new CppEditor::SemanticHighlighter(doc);
        else
            data.highlighter->updateFormatMapFromFontSettings();
        data.highlighter->setHighlightingRunner(runner);
        data.highlighter->run();
    };
    getAndHandleAst(doc, astHandler, AstCallbackMode::SyncIfPossible);
}

Utils::optional<QList<CodeAction> > ClangdDiagnostic::codeActions() const
{
    auto actions = optionalArray<LanguageServerProtocol::CodeAction>(u"codeActions");
    if (!actions)
        return actions;
    static const QStringList badCodeActions{
        "remove constant to silence this warning", // QTCREATORBUG-18593
    };
    for (auto it = actions->begin(); it != actions->end();) {
        if (badCodeActions.contains(it->title()))
            it = actions->erase(it);
        else
            ++it;
    }
    return actions;
}

QString ClangdDiagnostic::category() const
{
    return typedValue<QString>(u"category");
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
    case ClangCompletionContextAnalyzer::CompleteSignal:
    case ClangCompletionContextAnalyzer::CompleteSlot:
        if (!interface->isBaseObject())
            return CppEditor::getCppCompletionAssistProcessor();
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
    if (isDeprecated())
        return Utils::Icons::WARNING.icon();
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
    if (item().kind().value_or(CompletionItemKind::Text) == CompletionItemKind::Property)
        return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::VarPublicStatic);
    return LanguageClientCompletionItem::icon();
}

QString ClangdCompletionItem::text() const
{
    const QString clangdValue = LanguageClientCompletionItem::text();
    if (isDeprecated())
        return "[[deprecated]]" + clangdValue;
    return clangdValue;
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
    const auto wrapperHandler = [this, filePath, guardedTextDoc = QPointer(textDoc), astHandler,
            fullAstRequested, docRev = textDoc ? getRevision(textDoc) : -1,
            fileRev = getRevision(filePath)](const ClangdAstNode &ast, const MessageId &reqId) {
        qCDebug(clangdLog) << "retrieved AST from clangd";
        if (fullAstRequested) {
            if (guardedTextDoc) {
                if (docRev == getRevision(guardedTextDoc))
                    astCache.insert(guardedTextDoc, ast);
            } else if (fileRev == getRevision(filePath) && !q->documentForFilePath(filePath)) {
                externalAstCache.insert(filePath, ast);
            }
        }
        astHandler(ast, reqId);
    };
    qCDebug(clangdLog) << "requesting AST for" << filePath;
    return requestAst(q, filePath, range, wrapperHandler);
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
        setHeader({MemoryUsageWidget::tr("Component"), MemoryUsageWidget::tr("Total Memory")});
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
    m_client->sendMessage(request, ClangdClient::SendDocUpdates::Ignore);
}

} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ReplacementData)
