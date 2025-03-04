// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppmodelmanager.h"

#include "abstracteditorsupport.h"
#include "baseeditordocumentprocessor.h"
#include "compileroptionsbuilder.h"
#include "cppbuiltinmodelmanagersupport.h"
#include "cppcanonicalsymbol.h"
#include "cppcodemodelinspectordumper.h"
#include "cppcodemodelsettings.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppfindreferences.h"
#include "cppincludesfilter.h"
#include "cppindexingsupport.h"
#include "cpplocatordata.h"
#include "cpplocatorfilter.h"
#include "cppprojectfile.h"
#include "cppsourceprocessor.h"
#include "cpptoolsjsextension.h"
#include "cpptoolsreuse.h"
#include "editordocumenthandle.h"
#include "symbolfinder.h"
#include "symbolsfindfilter.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/TypeOfExpression.h>

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/session.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/savefile.h>
#include <utils/synchronizedvalue.h>
#include <utils/temporarydirectory.h>

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFutureWatcher>
#include <QHash>
#include <QMutexLocker>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextBlock>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QWriteLocker>

#include <memory>
#include <unordered_map>

#if defined(QTCREATOR_WITH_DUMP_AST) && defined(Q_CC_GNU)
#define WITH_AST_DUMP
#include <iostream>
#include <sstream>
#endif

using namespace Core;
using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace Utils;

static const bool DumpProjectInfo = qtcEnvironmentVariable("QTC_DUMP_PROJECT_INFO") == "1";

#ifdef QTCREATOR_WITH_DUMP_AST

#include <cxxabi.h>

class DumpAST: protected ASTVisitor
{
public:
    int depth;

    explicit DumpAST(Control *control)
        : ASTVisitor(control), depth(0)
    { }

    void operator()(AST *ast)
    { accept(ast); }

protected:
    virtual bool preVisit(AST *ast)
    {
        std::ostringstream s;
        PrettyPrinter pp(control(), s);
        pp(ast);
        QString code = QString::fromStdString(s.str());
        code.replace('\n', ' ');
        code.replace(QRegularExpression("\\s+"), " ");

        const char *name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 11;

        QByteArray ind(depth, ' ');
        ind += name;

        printf("%-40s %s\n", ind.constData(), qPrintable(code));
        ++depth;
        return true;
    }

    virtual void postVisit(AST *)
    { --depth; }
};

#endif // QTCREATOR_WITH_DUMP_AST

namespace CppEditor {

namespace Internal {

static CppModelManager *m_instance;

class ProjectData
{
public:
    ProjectInfo::ConstPtr projectInfo;
    QFutureWatcher<void> *indexer = nullptr;
    bool fullyIndexed = false;
};

class CppModelManagerPrivate
{
public:
    void setupWatcher(const QFuture<void> &future, Project *project,
                      ProjectData *projectData, CppModelManager *q);

    // Snapshot
    mutable QMutex m_snapshotMutex;
    Snapshot m_snapshot;

    // Project integration
    struct SyncedProjectData
    {
        QHash<Project *, ProjectData> m_projectData;
        QMap<FilePath, QList<ProjectPart::ConstPtr>> m_fileToProjectParts;
        QMap<QString, ProjectPart::ConstPtr> m_projectPartIdToProjectProjectPart;

        // The members below are cached/(re)calculated from the projects and/or their project parts
        bool m_dirty{true};
        FilePaths m_projectFiles;
        HeaderPaths m_headerPaths;
        Macros m_definedMacros;
    };

    Utils::SynchronizedValue<SyncedProjectData> m_lockedProjectData;

    // Editor integration
    mutable QMutex m_cppEditorDocumentsMutex;
    QMap<QString, CppEditorDocumentHandle *> m_cppEditorDocuments;
    QSet<AbstractEditorSupport *> m_extraEditorSupports;

    // Model Manager Supports for e.g. completion and highlighting
    BuiltinModelManagerSupport m_builtinModelManagerSupport;
    std::unique_ptr<ModelManagerSupport> m_extendedModelManagerSupport;
    ModelManagerSupport *m_activeModelManagerSupport = &m_builtinModelManagerSupport;

    // Indexing
    CppIndexingSupport *m_internalIndexingSupport;
    bool m_indexerEnabled;

    QMutex m_fallbackProjectPartMutex;
    ProjectPart::ConstPtr m_fallbackProjectPart;

    CppFindReferences *m_findReferences;

    SymbolFinder m_symbolFinder;
    QThreadPool m_threadPool;

    bool m_enableGC;
    QTimer m_delayedGcTimer;
    QTimer m_fallbackProjectPartTimer;

    CppLocatorData m_locatorData;
    std::unique_ptr<ILocatorFilter> m_locatorFilter;
    std::unique_ptr<ILocatorFilter> m_classesFilter;
    std::unique_ptr<ILocatorFilter> m_includesFilter;
    std::unique_ptr<ILocatorFilter> m_functionsFilter;
    std::unique_ptr<IFindFilter> m_symbolsFindFilter;
    std::unique_ptr<ILocatorFilter> m_currentDocumentFilter;

    QList<Document::DiagnosticMessage> m_diagnosticMessages;

    static void recalculateProjectPartMappings(SyncedProjectData &ld);
    static void ensureUpdated(SyncedProjectData &ld);
    static Utils::FilePaths internalProjectFiles(SyncedProjectData &ld);
    static ProjectExplorer::HeaderPaths internalHeaderPaths(SyncedProjectData &ld);
    static ProjectExplorer::Macros internalDefinedMacros(SyncedProjectData &ld);
};

static CppModelManagerPrivate *d;

} // namespace Internal
using namespace Internal;

const char pp_configuration[] =
    "# 1 \"<configuration>\"\n"
    "#define Q_CREATOR_RUN 1\n"
    "#define __cplusplus 1\n"
    "#define __extension__\n"
    "#define __context__\n"
    "#define __range__\n"
    "#define   restrict\n"
    "#define __restrict\n"
    "#define __restrict__\n"

    "#define __complex__\n"
    "#define __imag__\n"
    "#define __real__\n"

    "#define __builtin_va_arg(a,b) ((b)0)\n"

    "#define _Pragma(x)\n" // C99 _Pragma operator

    "#define __func__ \"\"\n"

    // ### add macros for gcc
    "#define __PRETTY_FUNCTION__ \"\"\n"
    "#define __FUNCTION__ \"\"\n"

    // ### add macros for win32
    "#define __cdecl\n"
    "#define __stdcall\n"
    "#define __thiscall\n"
    "#define QT_WA(x) x\n"
    "#define CALLBACK\n"
    "#define STDMETHODCALLTYPE\n"
    "#define __RPC_FAR\n"
    "#define __declspec(a)\n"
    "#define STDMETHOD(method) virtual HRESULT STDMETHODCALLTYPE method\n"
    "#define __try try\n"
    "#define __except catch\n"
    "#define __finally\n"
    "#define __inline inline\n"
    "#define __forceinline inline\n"
    "#define __pragma(x)\n"
    "#define __w64\n"
    "#define __int64 long long\n"
    "#define __int32 long\n"
    "#define __int16 short\n"
    "#define __int8 char\n"
    "#define __ptr32\n"
    "#define __ptr64\n";

QSet<FilePath> CppModelManager::timeStampModifiedFiles(const QList<Document::Ptr> &documentsToCheck)
{
    QSet<FilePath> sourceFiles;

    for (const Document::Ptr &doc : documentsToCheck) {
        const QDateTime lastModified = doc->lastModified();

        if (!lastModified.isNull()) {
            const FilePath filePath = doc->filePath();

            if (filePath.exists() && filePath.lastModified() != lastModified)
                sourceFiles.insert(filePath);
        }
    }

    return sourceFiles;
}

/*!
 * \brief createSourceProcessor Create a new source processor, which will signal the
 * model manager when a document has been processed.
 *
 * Indexed file is truncated version of fully parsed document: copy of source
 * code and full AST will be dropped when indexing is done.
 *
 * \return a new source processor object, which the caller needs to delete when finished.
 */
CppSourceProcessor *CppModelManager::createSourceProcessor()
{
    return new CppSourceProcessor(snapshot(), [](const Document::Ptr &doc) {
        const Document::Ptr previousDocument = document(doc->filePath());
        const unsigned newRevision = previousDocument.isNull()
                ? 1U
                : previousDocument->revision() + 1;
        doc->setRevision(newRevision);
        emitDocumentUpdated(doc);
        doc->releaseSourceAndAST();
    });
}

const FilePath &CppModelManager::editorConfigurationFileName()
{
    static const FilePath config = FilePath::fromPathPart(u"<per-editor-defines>");
    return config;
}

ModelManagerSupport *CppModelManager::modelManagerSupport(Backend backend)
{
    return backend == Backend::Builtin
            ? &d->m_builtinModelManagerSupport : d->m_activeModelManagerSupport;
}

void CppModelManager::startLocalRenaming(const CursorInEditor &data,
                                         const ProjectPart *projectPart,
                                         RenameCallback &&renameSymbolsCallback,
                                         Backend backend)
{
    modelManagerSupport(backend)
            ->startLocalRenaming(data, projectPart, std::move(renameSymbolsCallback));
}

void CppModelManager::globalRename(const CursorInEditor &data, const QString &replacement,
                                   const std::function<void()> &callback, Backend backend)
{
    modelManagerSupport(backend)->globalRename(data, replacement, callback);
}

void CppModelManager::findUsages(const CursorInEditor &data, Backend backend)
{
    modelManagerSupport(backend)->findUsages(data);
}

void CppModelManager::switchHeaderSource(bool inNextSplit, Backend backend)
{
    const IDocument *currentDocument = EditorManager::currentDocument();
    QTC_ASSERT(currentDocument, return);
    modelManagerSupport(backend)->switchHeaderSource(currentDocument->filePath(), inNextSplit);
}

void CppModelManager::showPreprocessedFile(bool inNextSplit)
{
    const IDocument *doc = EditorManager::currentDocument();
    QTC_ASSERT(doc, return);

    static const auto showError = [](const QString &reason) {
        MessageManager::writeFlashing(Tr::tr("Cannot show preprocessed file: %1")
                                          .arg(reason));
    };
    static const auto showFallbackWarning = [](const QString &reason) {
        MessageManager::writeSilently(Tr::tr("Falling back to built-in preprocessor: %1")
                                          .arg(reason));
    };
    static const auto saveAndOpen = [](const FilePath &filePath, const QByteArray &contents,
                                       bool inNextSplit) {
        SaveFile f(filePath);
        if (!f.open()) {
            showError(Tr::tr("Failed to open output file \"%1\".").arg(filePath.toUserOutput()));
            return;
        }
        f.write(contents);
        if (!f.commit()) {
            showError(Tr::tr("Failed to write output file \"%1\".").arg(filePath.toUserOutput()));
            return;
        }
        f.close();
        openEditor(filePath, inNextSplit, Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    };

    const FilePath &filePath = doc->filePath();
    const QString outFileName = filePath.completeBaseName() + "_preprocessed." + filePath.suffix();
    const auto outFilePath = FilePath::fromString(
                TemporaryDirectory::masterTemporaryDirectory()->filePath(outFileName));
    const auto useBuiltinPreprocessor = [filePath, outFilePath, inNextSplit,
                                         contents = doc->contents()] {
        const Document::Ptr preprocessedDoc = snapshot()
                .preprocessedDocument(contents, filePath);
        QByteArray content = R"(/* Created using Qt Creator's built-in preprocessor. */
/* See Tools -> Debug Qt Creator -> Inspect C++ Code Model for the parameters used.
 * Adapt the respective setting in Edit -> Preferences -> C++ -> Code Model to invoke
 * the actual compiler instead.
 */
)";
        saveAndOpen(outFilePath, content.append(preprocessedDoc->utf8Source()), inNextSplit);
    };

    if (CppCodeModelSettings::settingsForFile(filePath).useBuiltinPreprocessor) {
        useBuiltinPreprocessor();
        return;
    }

    const Project * const project = ProjectTree::currentProject();
    if (!project || !project->activeBuildConfiguration()) {
        showFallbackWarning(Tr::tr("Could not determine which compiler to invoke."));
        useBuiltinPreprocessor();
        return;
    }

    const Toolchain * tc = nullptr;
    const ProjectFile classifier(filePath, ProjectFile::classify(filePath.toUrlishString()));
    if (classifier.isC()) {
        tc = ToolchainKitAspect::cToolchain(project->activeKit());
    } else if (classifier.isCxx() || classifier.isHeader()) {
        tc = ToolchainKitAspect::cxxToolchain(project->activeKit());
    } else {
        showFallbackWarning(Tr::tr("Could not determine which compiler to invoke."));
        useBuiltinPreprocessor();
        return;
    }

    const bool isGcc = dynamic_cast<const GccToolchain *>(tc);
    const bool isMsvc = !isGcc
            && (tc->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
                || tc->typeId() == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID);
    if (!isGcc && !isMsvc) {
        showFallbackWarning(Tr::tr("Could not determine compiler command line."));
        useBuiltinPreprocessor();
        return;
    }

    const ProjectPart::ConstPtr projectPart = Utils::findOrDefault(
                CppModelManager::projectPart(filePath), [](const ProjectPart::ConstPtr &pp) {
        return pp->belongsToProject(ProjectTree::currentProject());
    });
    if (!projectPart) {
        showFallbackWarning(Tr::tr("Could not determine compiler command line."));
        useBuiltinPreprocessor();
        return;
    }

    CompilerOptionsBuilder optionsBuilder(*projectPart);
    optionsBuilder.setNativeMode();
    optionsBuilder.setClStyle(isMsvc);
    optionsBuilder.build(classifier.kind, UsePrecompiledHeaders::No);
    QStringList compilerArgs = optionsBuilder.options();
    if (isGcc)
        compilerArgs.append({"-E", "-o", outFilePath.toUserOutput()});
    else
        compilerArgs.append("/E");
    compilerArgs.append(filePath.toUserOutput());
    const CommandLine compilerCommandLine(tc->compilerCommand(), compilerArgs);
    const auto compiler = new Process(instance());
    compiler->setCommand(compilerCommandLine);
    compiler->setEnvironment(project->activeBuildConfiguration()->environment());
    connect(compiler, &Process::done, instance(), [compiler, outFilePath, inNextSplit,
                                                      useBuiltinPreprocessor, isMsvc] {
        compiler->deleteLater();
        if (compiler->result() != ProcessResult::FinishedWithSuccess) {
            showFallbackWarning("Compiler failed to run");
            useBuiltinPreprocessor();
            return;
        }
        if (isMsvc)
            saveAndOpen(outFilePath, compiler->rawStdOut(), inNextSplit);
        else
            openEditor(outFilePath, inNextSplit, Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    });
    compiler->start();
}

static void foldOrUnfoldComments(bool unfold)
{
    IEditor * const currentEditor = EditorManager::currentEditor();
    if (!currentEditor)
        return;
    const auto editorWidget = qobject_cast<CppEditorWidget*>(currentEditor->widget());
    if (!editorWidget)
        return;
    TextEditor::TextDocument * const textDoc = editorWidget->textDocument();
    QTC_ASSERT(textDoc, return);

    const Document::Ptr cppDoc = CppModelManager::snapshot().preprocessedDocument(
        textDoc->contents(), textDoc->filePath());
    QTC_ASSERT(cppDoc, return);
    cppDoc->tokenize();
    TranslationUnit * const tu = cppDoc->translationUnit();
    if (!tu || !tu->isTokenized())
        return;

    for (int commentTokIndex = 0; commentTokIndex < tu->commentCount(); ++commentTokIndex) {
        const Token &tok = tu->commentAt(commentTokIndex);
        if (tok.kind() != T_COMMENT && tok.kind() != T_DOXY_COMMENT)
            continue;
        const int tokenPos = tu->getTokenPositionInDocument(tok, textDoc->document());
        const int tokenEndPos = tu->getTokenEndPositionInDocument(tok, textDoc->document());
        const QTextBlock tokenBlock = textDoc->document()->findBlock(tokenPos);
        if (!tokenBlock.isValid())
            continue;
        const QTextBlock nextBlock = tokenBlock.next();
        if (!nextBlock.isValid())
            continue;
        if (tokenEndPos < nextBlock.position())
            continue;
        if (TextEditor::TextDocumentLayout::foldingIndent(tokenBlock)
            >= TextEditor::TextDocumentLayout::foldingIndent(nextBlock)) {
            continue;
        }
        if (unfold)
            editorWidget->unfold(tokenBlock);
        else
            editorWidget->fold(tokenBlock);
    }
}

void CppModelManager::foldComments() { foldOrUnfoldComments(false); }
void CppModelManager::unfoldComments() { foldOrUnfoldComments(true); }

class FindUnusedActionsEnabledSwitcher
{
public:
    FindUnusedActionsEnabledSwitcher()
        : actions{ActionManager::command("CppTools.FindUnusedFunctions"),
                  ActionManager::command("CppTools.FindUnusedFunctionsInSubProject")}
    {
        for (Command * const action : actions)
            action->action()->setEnabled(false);
    }
    ~FindUnusedActionsEnabledSwitcher()
    {
        for (Command * const action : actions)
            action->action()->setEnabled(true);
    }
private:
    const QList<Command *> actions;
};
using FindUnusedActionsEnabledSwitcherPtr = std::shared_ptr<FindUnusedActionsEnabledSwitcher>;

static void checkNextFunctionForUnused(
        const QPointer<SearchResult> &search,
        const std::shared_ptr<QFutureInterface<void>> &findRefsFuture,
        const FindUnusedActionsEnabledSwitcherPtr &actionsSwitcher)
{
    if (!search || findRefsFuture->isCanceled())
        return;
    QVariantMap data = search->userData().toMap();
    QVariant &remainingLinks = data["remaining"];
    QVariantList remainingLinksList = remainingLinks.toList();
    QVariant &activeLinks = data["active"];
    QVariantList activeLinksList = activeLinks.toList();
    if (remainingLinksList.isEmpty()) {
        if (activeLinksList.isEmpty()) {
            search->finishSearch(false);
            findRefsFuture->reportFinished();
        }
        return;
    }
    const auto link = qvariant_cast<Link>(remainingLinksList.takeFirst());
    activeLinksList << QVariant::fromValue(link);
    remainingLinks = remainingLinksList;
    activeLinks = activeLinksList;
    search->setUserData(data);
    CppModelManager::modelManagerSupport(CppModelManager::Backend::Best)
            ->checkUnused(link, search, [search, link, findRefsFuture, actionsSwitcher](const Link &) {
        if (!search || findRefsFuture->isCanceled())
            return;
        const int newProgress = findRefsFuture->progressValue() + 1;
        findRefsFuture->setProgressValueAndText(newProgress,
                                                Tr::tr("Checked %1 of %n function(s)",
                                                       nullptr,
                                                       findRefsFuture->progressMaximum())
                                                    .arg(newProgress));
        QVariantMap data = search->userData().toMap();
        QVariant &activeLinks = data["active"];
        QVariantList activeLinksList = activeLinks.toList();
        QTC_CHECK(activeLinksList.removeOne(QVariant::fromValue(link)));
        activeLinks = activeLinksList;
        search->setUserData(data);
        checkNextFunctionForUnused(search, findRefsFuture, actionsSwitcher);
    });
}

void CppModelManager::findUnusedFunctions(const FilePath &folder)
{
    const auto actionsSwitcher = std::make_shared<FindUnusedActionsEnabledSwitcher>();

    // Step 1: Employ locator to find all functions
    LocatorMatcher *matcher = new LocatorMatcher;
    matcher->setTasks(LocatorMatcher::matchers(MatcherType::Functions));
    const QPointer<SearchResult> search
        = SearchResultWindow::instance()->startNewSearch(Tr::tr("Find Unused Functions"),
                             {},
                             {},
                             SearchResultWindow::SearchOnly,
                             SearchResultWindow::PreserveCaseDisabled,
                             "CppEditor");
    matcher->setParent(search);
    connect(search, &SearchResult::activated, [](const SearchResultItem &item) {
        EditorManager::openEditorAtSearchResult(item);
    });
    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    connect(search, &SearchResult::canceled, matcher, [matcher] { delete matcher; });
    connect(matcher, &LocatorMatcher::done, search,
            [matcher, search, folder, actionsSwitcher](bool success) {
        matcher->deleteLater();
        if (!success) {
            search->finishSearch(true);
            return;
        }
        Links links;
        const LocatorFilterEntries entries = matcher->outputData();
        for (const LocatorFilterEntry &entry : entries) {
            static const QStringList prefixBlacklist{"main(", "~", "qHash(", "begin()", "end()",
                    "cbegin()", "cend()", "constBegin()", "constEnd()"};
            if (Utils::anyOf(prefixBlacklist, [&entry](const QString &prefix) {
                    return entry.displayName.startsWith(prefix); })) {
                continue;
            }
            if (!entry.linkForEditor)
                continue;
            const Link link = *entry.linkForEditor;
            if (link.hasValidTarget() && link.targetFilePath.isReadableFile()
                    && (folder.isEmpty() || link.targetFilePath.isChildOf(folder))
                    && ProjectManager::projectForFile(link.targetFilePath)) {
                links << link;
            }
        }
        if (links.isEmpty()) {
            search->finishSearch(false);
            return;
        }
        QVariantMap remainingAndActiveLinks;
        remainingAndActiveLinks.insert("active", QVariantList());
        remainingAndActiveLinks.insert("remaining",
            Utils::transform<QVariantList>(links, [](const Link &l) { return QVariant::fromValue(l);
        }));
        search->setUserData(remainingAndActiveLinks);
        const auto findRefsFuture = std::make_shared<QFutureInterface<void>>();
        FutureProgress *const progress = ProgressManager::addTask(findRefsFuture->future(),
                                                          Tr::tr("Finding Unused Functions"),
                                                          "CppEditor.FindUnusedFunctions");
        connect(progress,
                &FutureProgress::canceled,
                search,
                [search, future = std::weak_ptr<QFutureInterface<void>>(findRefsFuture)] {
            search->finishSearch(true);
            if (const auto f = future.lock()) {
                f->cancel();
                f->reportFinished();
            }
        });
        findRefsFuture->setProgressRange(0, links.size());
        connect(search, &SearchResult::canceled, [findRefsFuture] {
            findRefsFuture->cancel();
            findRefsFuture->reportFinished();
        });

        // Step 2: Forward search results one by one to backend to check which functions are unused.
        //         We keep several requests in flight for decent throughput.
        const int inFlightCount = std::min(QThread::idealThreadCount() / 2 + 1, int(links.size()));
        for (int i = 0; i < inFlightCount; ++i)
            checkNextFunctionForUnused(search, findRefsFuture, actionsSwitcher);
    });
    matcher->start();
}

void CppModelManager::checkForUnusedSymbol(SearchResult *search,
                                           const Link &link,
                                           CPlusPlus::Symbol *symbol,
                                           const CPlusPlus::LookupContext &context,
                                           const LinkHandler &callback)
{
    d->m_findReferences->checkUnused(search, link, symbol, context, callback);
}

int argumentPositionOf(const AST *last, const CallAST *callAst)
{
    if (!callAst || !callAst->expression_list)
        return false;

    int num = 0;
    for (ExpressionListAST *it = callAst->expression_list; it; it = it->next) {
        ++num;
        const ExpressionAST *const arg = it->value;
        if (arg->firstToken() <= last->firstToken()
            && arg->lastToken() >= last->lastToken()) {
            return num;
        }
    }
    return 0;
}

SignalSlotType CppModelManager::getSignalSlotType(const FilePath &filePath,
                                                  const QByteArray &content,
                                                  int position)
{
    if (content.isEmpty())
        return SignalSlotType::None;

    // Insert a dummy prefix if we don't have a real one. Otherwise the AST path will not contain
    // anything after the CallAST.
    QByteArray fixedContent = content;
    if (position > 2 && content.mid(position - 2, 2) == "::")
        fixedContent.insert(position, 'x');

    const Snapshot snapshot = CppModelManager::snapshot();
    const Document::Ptr document = snapshot.preprocessedDocument(fixedContent, filePath);
    document->check();
    QTextDocument textDocument(QString::fromUtf8(fixedContent));
    QTextCursor cursor(&textDocument);
    cursor.setPosition(position);

    const QList<AST *> path = ASTPath(document)(cursor);
    if (path.isEmpty())
        return SignalSlotType::None;
    const CallAST *callAst = nullptr;
    for (auto it = path.crbegin(); it != path.crend(); ++it) {
        if ((callAst = (*it)->asCall()))
            break;
    }

    if (!callAst || !callAst->base_expression)
        return SignalSlotType::None;

    const int argumentPosition = argumentPositionOf(path.last(), callAst);
    if (argumentPosition != 2 && argumentPosition != 4)
        return SignalSlotType::None;

    const NameAST *nameAst = nullptr;
    if (const IdExpressionAST * const idAst = callAst->base_expression->asIdExpression())
        nameAst = idAst->name;
    else if (const MemberAccessAST * const ast = callAst->base_expression->asMemberAccess())
        nameAst = ast->member_name;
    if (!nameAst || !nameAst->name)
        return SignalSlotType::None;
    const Identifier * const id = nameAst->name->identifier();
    if (!id)
        return SignalSlotType::None;
    const QString funcName = QString::fromUtf8(id->chars(), id->size());
    if (funcName != "connect" && funcName != "disconnect")
        return SignalSlotType::None;

    Scope *scope = document->globalNamespace();
    for (auto it = path.crbegin(); it != path.crend(); ++it) {
        if (const CompoundStatementAST * const stmtAst = (*it)->asCompoundStatement()) {
            scope = stmtAst->symbol;
            break;
        }
    }
    const LookupContext context(document, snapshot);
    if (const MemberAccessAST * const ast = callAst->base_expression->asMemberAccess()) {
        TypeOfExpression exprType;
        exprType.setExpandTemplates(true);
        exprType.init(document, snapshot);
        const QList<LookupItem> typeMatches = exprType(ast->base_expression, document, scope);
        if (typeMatches.isEmpty())
            return SignalSlotType::None;
        const std::function<const NamedType *(const FullySpecifiedType &)> getNamedType
                = [&getNamedType](const FullySpecifiedType &type ) -> const NamedType * {
            Type * const t = type.type();
            if (const auto namedType = t->asNamedType())
                return namedType;
            if (const auto pointerType = t->asPointerType())
                return getNamedType(pointerType->elementType());
            if (const auto refType = t->asReferenceType())
                return getNamedType(refType->elementType());
            return nullptr;
        };
        const NamedType *namedType = getNamedType(typeMatches.first().type());
        if (!namedType && typeMatches.first().declaration())
            namedType = getNamedType(typeMatches.first().declaration()->type());
        if (!namedType)
            return SignalSlotType::None;
        const ClassOrNamespace * const result = context.lookupType(namedType->name(), scope);
        if (!result)
            return SignalSlotType::None;
        scope = result->rootClass();
        if (!scope)
            return SignalSlotType::None;
    }

    // Is the function a member function of QObject?
    const QList<LookupItem> matches = context.lookup(nameAst->name, scope);
    for (const LookupItem &match : matches) {
        if (!match.scope())
            continue;
        const Class *klass = match.scope()->asClass();
        if (!klass || !klass->name())
            continue;
        const Identifier * const classId = klass->name()->identifier();
        if (classId && QString::fromUtf8(classId->chars(), classId->size()) == "QObject") {
            QString expression;
            LanguageFeatures features = LanguageFeatures::defaultFeatures();
            CPlusPlus::ExpressionUnderCursor expressionUnderCursor(features);
            for (int i = cursor.position(); i > 0; --i)
                if (textDocument.characterAt(i) == '(') {
                    cursor.setPosition(i);
                    break;
                }

            expression = expressionUnderCursor(cursor);

            if (expression.endsWith(QLatin1String("SIGNAL"))
                    || (expression.endsWith(QLatin1String("SLOT")) && argumentPosition == 4))
                return SignalSlotType::OldStyleSignal;

            if (argumentPosition == 2)
                return SignalSlotType::NewStyleSignal;
        }
    }
    return SignalSlotType::None;
}

FollowSymbolUnderCursor &CppModelManager::builtinFollowSymbol()
{
    return d->m_builtinModelManagerSupport.followSymbolInterface();
}

template<class FilterClass>
static void setFilter(std::unique_ptr<FilterClass> &filter,
                      std::unique_ptr<FilterClass> &&newFilter)
{
    QTC_ASSERT(newFilter, return;);
    filter = std::move(newFilter);
}

void CppModelManager::setLocatorFilter(std::unique_ptr<ILocatorFilter> &&filter)
{
    setFilter(d->m_locatorFilter, std::move(filter));
}

void CppModelManager::setClassesFilter(std::unique_ptr<ILocatorFilter> &&filter)
{
    setFilter(d->m_classesFilter, std::move(filter));
}

void CppModelManager::setIncludesFilter(std::unique_ptr<ILocatorFilter> &&filter)
{
    setFilter(d->m_includesFilter, std::move(filter));
}

void CppModelManager::setFunctionsFilter(std::unique_ptr<ILocatorFilter> &&filter)
{
    setFilter(d->m_functionsFilter, std::move(filter));
}

void CppModelManager::setSymbolsFindFilter(std::unique_ptr<IFindFilter> &&filter)
{
    setFilter(d->m_symbolsFindFilter, std::move(filter));
}

void CppModelManager::setCurrentDocumentFilter(std::unique_ptr<ILocatorFilter> &&filter)
{
    setFilter(d->m_currentDocumentFilter, std::move(filter));
}

ILocatorFilter *CppModelManager::locatorFilter()
{
    return d->m_locatorFilter.get();
}

ILocatorFilter *CppModelManager::classesFilter()
{
    return d->m_classesFilter.get();
}

ILocatorFilter *CppModelManager::includesFilter()
{
    return d->m_includesFilter.get();
}

ILocatorFilter *CppModelManager::functionsFilter()
{
    return d->m_functionsFilter.get();
}

IFindFilter *CppModelManager::symbolsFindFilter()
{
    return d->m_symbolsFindFilter.get();
}

ILocatorFilter *CppModelManager::currentDocumentFilter()
{
    return d->m_currentDocumentFilter.get();
}

const FilePath &CppModelManager::configurationFileName()
{
    return Preprocessor::configurationFileName();
}

void CppModelManager::updateModifiedSourceFiles()
{
    const Snapshot snapshot = CppModelManager::snapshot();
    QList<Document::Ptr> documentsToCheck;
    for (const Document::Ptr &document : snapshot)
        documentsToCheck << document;

    updateSourceFiles(timeStampModifiedFiles(documentsToCheck));
}

/*!
    \class CppEditor::CppModelManager
    \brief The CppModelManager keeps tracks of the source files the code model is aware of.

    The CppModelManager manages the source files in a Snapshot object.

    The snapshot is updated in case e.g.
        * New files are opened/edited (Editor integration)
        * A project manager pushes updated project information (Project integration)
        * Files are garbage collected
*/

CppModelManager *CppModelManager::instance()
{
    QTC_ASSERT(m_instance, return nullptr;);
    return m_instance;
}

void CppModelManager::registerJsExtension()
{
    JsExpander::registerGlobalObject("Cpp", [] {
        return new CppToolsJsExtension(&d->m_locatorData);
    });
}

void CppModelManager::initCppTools()
{
    // Objects
    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            m_instance, &CppModelManager::updateModifiedSourceFiles);
    connect(DocumentManager::instance(), &DocumentManager::filesChangedInternally,
            m_instance, [](const FilePaths &filePaths) {
        updateSourceFiles(toSet(filePaths));
    });

    connect(m_instance, &CppModelManager::documentUpdated,
            &d->m_locatorData, &CppLocatorData::onDocumentUpdated);

    connect(m_instance, &CppModelManager::aboutToRemoveFiles,
            &d->m_locatorData, &CppLocatorData::onAboutToRemoveFiles);

    // Set up builtin filters
    setLocatorFilter(std::make_unique<CppAllSymbolsFilter>());
    setClassesFilter(std::make_unique<CppClassesFilter>());
    setIncludesFilter(std::make_unique<CppIncludesFilter>());
    setFunctionsFilter(std::make_unique<CppFunctionsFilter>());
    setSymbolsFindFilter(std::make_unique<SymbolsFindFilter>());
    setCurrentDocumentFilter(std::make_unique<CppCurrentDocumentFilter>());
    // Setup matchers
    LocatorMatcher::addMatcherCreator(MatcherType::AllSymbols, [] {
        return cppMatchers(MatcherType::AllSymbols);
    });
    LocatorMatcher::addMatcherCreator(MatcherType::Classes, [] {
        return cppMatchers(MatcherType::Classes);
    });
    LocatorMatcher::addMatcherCreator(MatcherType::Functions, [] {
        return cppMatchers(MatcherType::Functions);
    });
    LocatorMatcher::addMatcherCreator(MatcherType::CurrentDocumentSymbols, [] {
        return cppMatchers(MatcherType::CurrentDocumentSymbols);
    });
}

CppModelManager::CppModelManager()
{
    d = new CppModelManagerPrivate;
    m_instance = this;

    CppModelManagerBase::registerSetExtraDiagnosticsCallback(&CppModelManager::setExtraDiagnostics);
    CppModelManagerBase::registerSnapshotCallback(&CppModelManager::snapshot);

    // Used for weak dependency in VcsBaseSubmitEditor
    setObjectName("CppModelManager");
    ExtensionSystem::PluginManager::addObject(this);

    d->m_enableGC = true;

    // Visual C++ has 1MiB, macOSX has 512KiB
    if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
        d->m_threadPool.setStackSize(2 * 1024 * 1024);

    qRegisterMetaType<QSet<QString> >();
    connect(this, &CppModelManager::sourceFilesRefreshed,
            this, &CppModelManager::onSourceFilesRefreshed);

    d->m_findReferences = new CppFindReferences(this);
    d->m_indexerEnabled = qtcEnvironmentVariable("QTC_NO_CODE_INDEXER") != "1";

    d->m_delayedGcTimer.setObjectName(QLatin1String("CppModelManager::m_delayedGcTimer"));
    d->m_delayedGcTimer.setSingleShot(true);
    connect(&d->m_delayedGcTimer, &QTimer::timeout, this, &CppModelManager::GC);

    auto projectManager = ProjectManager::instance();
    connect(projectManager, &ProjectManager::projectAdded,
            this, &CppModelManager::onProjectAdded);
    connect(projectManager, &ProjectManager::aboutToRemoveProject,
            this, &CppModelManager::onAboutToRemoveProject);
    connect(SessionManager::instance(), &SessionManager::aboutToLoadSession,
            this, &CppModelManager::onAboutToLoadSession);
    connect(projectManager, &ProjectManager::startupProjectChanged,
            this, &CppModelManager::onActiveProjectChanged);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &CppModelManager::onCurrentEditorChanged);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::filesRenamed,
            this, &CppModelManager::renameIncludes);

    connect(ICore::instance(), &ICore::coreAboutToClose,
            this, &CppModelManager::onCoreAboutToClose);

    d->m_fallbackProjectPartTimer.setSingleShot(true);
    d->m_fallbackProjectPartTimer.setInterval(5000);
    connect(&d->m_fallbackProjectPartTimer, &QTimer::timeout,
            this, &CppModelManager::setupFallbackProjectPart);
    connect(KitManager::instance(), &KitManager::kitsChanged,
            &d->m_fallbackProjectPartTimer, qOverload<>(&QTimer::start));
    connect(this, &CppModelManager::projectPartsRemoved,
            &d->m_fallbackProjectPartTimer, qOverload<>(&QTimer::start));
    connect(this, &CppModelManager::projectPartsUpdated,
            &d->m_fallbackProjectPartTimer, qOverload<>(&QTimer::start));
    setupFallbackProjectPart();

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");
    qRegisterMetaType<QList<Document::DiagnosticMessage>>(
                "QList<CPlusPlus::Document::DiagnosticMessage>");

    d->m_internalIndexingSupport = new CppIndexingSupport;

    initCppTools();
}

CppModelManager::~CppModelManager()
{
    ExtensionSystem::PluginManager::removeObject(this);

    delete d->m_internalIndexingSupport;
    delete d;
}

Snapshot CppModelManager::snapshot()
{
    QMutexLocker locker(&d->m_snapshotMutex);
    return d->m_snapshot;
}

Document::Ptr CppModelManager::document(const FilePath &filePath)
{
    QMutexLocker locker(&d->m_snapshotMutex);
    return d->m_snapshot.document(filePath);
}

/// Replace the document in the snapshot.
///
/// Returns true if successful, false if the new document is out-dated.
bool CppModelManager::replaceDocument(Document::Ptr newDoc)
{
    QMutexLocker locker(&d->m_snapshotMutex);

    Document::Ptr previous = d->m_snapshot.document(newDoc->filePath());
    if (previous && (newDoc->revision() != 0 && newDoc->revision() < previous->revision()))
        // the new document is outdated
        return false;

    d->m_snapshot.insert(newDoc);
    return true;
}

void CppModelManagerPrivate::ensureUpdated(SyncedProjectData &ld)
{
    if (!ld.m_dirty)
        return;

    ld.m_projectFiles = internalProjectFiles(ld);
    ld.m_headerPaths = internalHeaderPaths(ld);
    ld.m_definedMacros = internalDefinedMacros(ld);
    ld.m_dirty = false;
}

FilePaths CppModelManagerPrivate::internalProjectFiles(SyncedProjectData &ld)
{
    FilePaths files;
    for (const ProjectData &projectData : std::as_const(ld.m_projectData)) {
        for (const ProjectPart::ConstPtr &part : projectData.projectInfo->projectParts()) {
            for (const ProjectFile &file : part->files)
                files += file.path;
        }
    }
    FilePath::removeDuplicates(files);
    return files;
}

HeaderPaths CppModelManagerPrivate::internalHeaderPaths(SyncedProjectData &ld)
{
    HeaderPaths headerPaths;
    for (const ProjectData &projectData : std::as_const(ld.m_projectData)) {
        for (const ProjectPart::ConstPtr &part : projectData.projectInfo->projectParts()) {
            for (const HeaderPath &path : part->headerPaths) {
                HeaderPath hp(QDir::cleanPath(path.path), path.type);
                if (!headerPaths.contains(hp))
                    headerPaths.push_back(std::move(hp));
            }
        }
    }
    return headerPaths;
}

static void addUnique(const Macros &newMacros, Macros &macros,
                      QSet<ProjectExplorer::Macro> &alreadyIn)
{
    for (const ProjectExplorer::Macro &macro : newMacros) {
        if (Utils::insert(alreadyIn, macro))
            macros += macro;
    }
}

Macros CppModelManagerPrivate::internalDefinedMacros(SyncedProjectData &ld)
{
    Macros macros;
    QSet<ProjectExplorer::Macro> alreadyIn;
    for (const ProjectData &projectData : std::as_const(ld.m_projectData)) {
        for (const ProjectPart::ConstPtr &part : projectData.projectInfo->projectParts()) {
            addUnique(part->toolchainMacros, macros, alreadyIn);
            addUnique(part->projectMacros, macros, alreadyIn);
        }
    }
    return macros;
}

/// This function will acquire mutexes!
void CppModelManager::dumpModelManagerConfiguration(const QString &logFileId)
{
    const Snapshot globalSnapshot = snapshot();
    const QString globalSnapshotTitle
        = QString::fromLatin1("Global/Indexing Snapshot (%1 Documents)").arg(globalSnapshot.size());

    CppCodeModelInspector::Dumper dumper(globalSnapshot, logFileId);
    dumper.dumpProjectInfos(projectInfos());
    dumper.dumpSnapshot(globalSnapshot, globalSnapshotTitle, /*isGlobalSnapshot=*/ true);
    dumper.dumpWorkingCopy(workingCopy());
    dumper.dumpMergedEntities(headerPaths(),
                              ProjectExplorer::Macro::toByteArray(definedMacros()));
}

QSet<AbstractEditorSupport *> CppModelManager::abstractEditorSupports()
{
    return d->m_extraEditorSupports;
}

void CppModelManager::addExtraEditorSupport(AbstractEditorSupport *editorSupport)
{
    d->m_extraEditorSupports.insert(editorSupport);
}

void CppModelManager::removeExtraEditorSupport(AbstractEditorSupport *editorSupport)
{
    d->m_extraEditorSupports.remove(editorSupport);
}

CppEditorDocumentHandle *CppModelManager::cppEditorDocument(const FilePath &filePath)
{
    if (filePath.isEmpty())
        return nullptr;

    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    return d->m_cppEditorDocuments.value(filePath.toUrlishString(), 0);
}

BaseEditorDocumentProcessor *CppModelManager::cppEditorDocumentProcessor(const FilePath &filePath)
{
    const auto document = cppEditorDocument(filePath);
    return document ? document->processor() : nullptr;
}

void CppModelManager::registerCppEditorDocument(CppEditorDocumentHandle *editorDocument)
{
    QTC_ASSERT(editorDocument, return);
    const FilePath filePath = editorDocument->filePath();
    QTC_ASSERT(!filePath.isEmpty(), return);

    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    QTC_ASSERT(d->m_cppEditorDocuments.value(filePath.toUrlishString(), 0) == 0, return);
    d->m_cppEditorDocuments.insert(filePath.toUrlishString(), editorDocument);
}

void CppModelManager::unregisterCppEditorDocument(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    static short closedCppDocuments = 0;
    int openCppDocuments = 0;

    {
        QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
        QTC_ASSERT(d->m_cppEditorDocuments.value(filePath, 0), return);
        QTC_CHECK(d->m_cppEditorDocuments.remove(filePath) == 1);
        openCppDocuments = d->m_cppEditorDocuments.size();
    }

    ++closedCppDocuments;
    if (openCppDocuments == 0 || closedCppDocuments == 5) {
        closedCppDocuments = 0;
        delayedGC();
    }
}

QList<int> CppModelManager::references(Symbol *symbol, const LookupContext &context)
{
    return d->m_findReferences->references(symbol, context);
}

void CppModelManager::findUsages(Symbol *symbol, const LookupContext &context)
{
    if (symbol->identifier())
        d->m_findReferences->findUsages(symbol, context);
}

void CppModelManager::renameUsages(Symbol *symbol,
                                   const LookupContext &context,
                                   const QString &replacement,
                                   const std::function<void()> &callback)
{
    if (symbol->identifier())
        d->m_findReferences->renameUsages(symbol, context, replacement, callback);
}

void CppModelManager::renameUsages(const Document::Ptr &doc, const QTextCursor &cursor,
                                   const Snapshot &snapshot, const QString &replacement,
                                   const std::function<void ()> &callback)
{
    Internal::CanonicalSymbol cs(doc, snapshot);
    CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
    if (canonicalSymbol)
        renameUsages(canonicalSymbol, cs.context(), replacement, callback);
}

void CppModelManager::findMacroUsages(const CPlusPlus::Macro &macro)
{
    d->m_findReferences->findMacroUses(macro);
}

void CppModelManager::renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement)
{
    d->m_findReferences->renameMacroUses(macro, replacement);
}

void CppModelManager::replaceSnapshot(const Snapshot &newSnapshot)
{
    QMutexLocker snapshotLocker(&d->m_snapshotMutex);
    d->m_snapshot = newSnapshot;
}

WorkingCopy CppModelManager::buildWorkingCopyList()
{
    WorkingCopy workingCopy;

    const QList<CppEditorDocumentHandle *> cppEditorDocumentList = cppEditorDocuments();
    for (const CppEditorDocumentHandle *cppEditorDocument : cppEditorDocumentList) {
        workingCopy.insert(cppEditorDocument->filePath(),
                           cppEditorDocument->contents(),
                           cppEditorDocument->revision());
    }

    for (AbstractEditorSupport *es : std::as_const(d->m_extraEditorSupports))
        workingCopy.insert(es->filePath(), es->contents(), es->revision());

    // Add the project configuration file
    QByteArray conf = codeModelConfiguration();
    conf += ProjectExplorer::Macro::toByteArray(definedMacros());
    workingCopy.insert(configurationFileName(), conf);

    return workingCopy;
}

WorkingCopy CppModelManager::workingCopy()
{
    return buildWorkingCopyList();
}

QByteArray CppModelManager::codeModelConfiguration()
{
    return QByteArray::fromRawData(pp_configuration, qstrlen(pp_configuration));
}

CppLocatorData *CppModelManager::locatorData()
{
    return &d->m_locatorData;
}

static QSet<QString> filteredFilesRemoved(const QSet<QString> &files,
                                          const CppCodeModelSettings &settings)
{
    if (!settings.enableIndexing)
        return {};

    const int fileSizeLimitInMb = settings.effectiveIndexerFileSizeLimitInMb();
    if (fileSizeLimitInMb <= 0 && !settings.ignoreFiles)
        return files;

    QSet<QString> result;
    QList<QRegularExpression> regexes;
    const QStringList wildcards = settings.ignorePattern.split('\n');

    for (const QString &wildcard : wildcards)
        regexes.append(QRegularExpression::fromWildcard(wildcard, Qt::CaseInsensitive,
                                                        QRegularExpression::UnanchoredWildcardConversion));

    for (const QString &file : files) {
        const FilePath filePath = FilePath::fromString(file);
        if (fileSizeLimitInMb > 0 && fileSizeExceedsLimit(filePath, fileSizeLimitInMb))
            continue;
        bool skip = false;
        if (settings.ignoreFiles) {
            for (const QRegularExpression &rx: std::as_const(regexes)) {
                QRegularExpressionMatch match = rx.match(filePath.absoluteFilePath().path());
                if (match.hasMatch()) {
                    MessageManager::writeSilently(Tr::tr("C++ Indexer: Skipping file \"%1\" "
                        "because its path matches the ignore pattern.").arg(filePath.displayName()));
                    skip = true;
                    break;
                }
            }
        }

        if (!skip)
            result << filePath.toUrlishString();
    }

    return result;
}

QFuture<void> CppModelManager::updateSourceFiles(const QSet<FilePath> &sourceFiles,
                                                 ProgressNotificationMode mode)
{
    if (sourceFiles.isEmpty() || !d->m_indexerEnabled)
        return QFuture<void>();

    std::unordered_map<Project *, QSet<QString>> sourcesPerProject;
    for (const FilePath &fp : sourceFiles)
        sourcesPerProject[ProjectManager::projectForFile(fp)] << fp.toUrlishString();
    std::vector<std::pair<QSet<QString>, CppCodeModelSettings>> sourcesAndSettings;
    for (const auto &it : sourcesPerProject) {
        sourcesAndSettings
            .emplace_back(it.second, CppCodeModelSettings::settingsForProject(it.first));
    }

    const auto filteredFiles = [sourcesAndSettings = std::move(sourcesAndSettings)] {
        QSet<QString> result;
        for (const auto &it : sourcesAndSettings)
            result.unite(filteredFilesRemoved(it.first, it.second));
        return result;
    };

    // "ReservedProgressNotification" should be shown if there is more than one source file.
    if (sourceFiles.size() > 1)
        mode = ForcedProgressNotification;
    return d->m_internalIndexingSupport->refreshSourceFiles(filteredFiles, mode);
}

ProjectInfoList CppModelManager::projectInfos()
{
    return Utils::transform<QList<ProjectInfo::ConstPtr>>(d->m_lockedProjectData.readLocked()
                                                              ->m_projectData,
                                                          [](const ProjectData &d) {
                                                              return d.projectInfo;
                                                          });
}

ProjectInfo::ConstPtr CppModelManager::projectInfo(Project *project)
{
    return d->m_lockedProjectData.get<ProjectInfo::ConstPtr>(
        [project](const CppModelManagerPrivate::SyncedProjectData &ld) {
            return ld.m_projectData.value(project).projectInfo;
        });
}

/// \brief Remove all files and their includes (recursively) of given ProjectInfo from the snapshot.
void CppModelManager::removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo)
{
    QMutexLocker snapshotLocker(&d->m_snapshotMutex);
    QStringList removedFiles;
    for (const ProjectPart::ConstPtr &projectPart : projectInfo.projectParts()) {
        for (const ProjectFile &cxxFile : std::as_const(projectPart->files)) {
            const QSet<FilePath> filePaths = d->m_snapshot.allIncludesForDocument(cxxFile.path);
            for (const FilePath &filePath : filePaths) {
                d->m_snapshot.remove(filePath);
                removedFiles << filePath.toUrlishString();
            }
            d->m_snapshot.remove(cxxFile.path);
            removedFiles << cxxFile.path.toUrlishString();
        }
    }

    emit m_instance->aboutToRemoveFiles(removedFiles);
}

const QList<CppEditorDocumentHandle *> CppModelManager::cppEditorDocuments()
{
    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    return d->m_cppEditorDocuments.values();
}

/// \brief Remove all given files from the snapshot.
void CppModelManager::removeFilesFromSnapshot(const QSet<FilePath> &filesToRemove)
{
    QMutexLocker snapshotLocker(&d->m_snapshotMutex);
    for (const FilePath &file : filesToRemove)
        d->m_snapshot.remove(file);
}

class ProjectInfoComparer
{
public:
    ProjectInfoComparer(const ProjectInfo &oldProjectInfo,
                        const ProjectInfo &newProjectInfo)
        : m_old(oldProjectInfo)
        , m_oldSourceFiles(oldProjectInfo.sourceFiles())
        , m_new(newProjectInfo)
        , m_newSourceFiles(newProjectInfo.sourceFiles())
    {}

    bool definesChanged() const { return m_new.definesChanged(m_old); }
    bool configurationChanged() const { return m_new.configurationChanged(m_old); }
    bool configurationOrFilesChanged() const { return m_new.configurationOrFilesChanged(m_old); }

    QSet<FilePath> addedFiles() const
    {
        QSet<FilePath> addedFilesSet = m_newSourceFiles;
        addedFilesSet.subtract(m_oldSourceFiles);
        return addedFilesSet;
    }

    QSet<FilePath> removedFiles() const
    {
        QSet<FilePath> removedFilesSet = m_oldSourceFiles;
        removedFilesSet.subtract(m_newSourceFiles);
        return removedFilesSet;
    }

    QStringList removedProjectParts()
    {
        QSet<QString> removed = projectPartIds(m_old.projectParts());
        removed.subtract(projectPartIds(m_new.projectParts()));
        return Utils::toList(removed);
    }

    /// Returns a list of common files that have a changed timestamp.
    QSet<FilePath> timeStampModifiedFiles(const Snapshot &snapshot) const
    {
        QSet<FilePath> commonSourceFiles = m_newSourceFiles;
        commonSourceFiles.intersect(m_oldSourceFiles);

        QList<Document::Ptr> documentsToCheck;
        for (const FilePath &file : commonSourceFiles) {
            if (Document::Ptr document = snapshot.document(file))
                documentsToCheck << document;
        }

        return CppModelManager::timeStampModifiedFiles(documentsToCheck);
    }

private:
    static QSet<QString> projectPartIds(const QVector<ProjectPart::ConstPtr> &projectParts)
    {
        QSet<QString> ids;

        for (const ProjectPart::ConstPtr &projectPart : projectParts)
            ids.insert(projectPart->id());

        return ids;
    }

private:
    const ProjectInfo &m_old;
    const QSet<FilePath> m_oldSourceFiles;

    const ProjectInfo &m_new;
    const QSet<FilePath> m_newSourceFiles;
};

void CppModelManagerPrivate::recalculateProjectPartMappings(SyncedProjectData &ld)
{
    ld.m_projectPartIdToProjectProjectPart.clear();
    ld.m_fileToProjectParts.clear();
    for (const ProjectData &projectData : std::as_const(ld.m_projectData)) {
        for (const ProjectPart::ConstPtr &projectPart : projectData.projectInfo->projectParts()) {
            ld.m_projectPartIdToProjectProjectPart[projectPart->id()] = projectPart;
            for (const ProjectFile &cxxFile : projectPart->files) {
                ld.m_fileToProjectParts[cxxFile.path].append(projectPart);
                if (FilePath canonical = cxxFile.path.canonicalPath(); canonical != cxxFile.path)
                    ld.m_fileToProjectParts[canonical].append(projectPart);
            }
        }
    }

    d->m_symbolFinder.clearCache();
}

void CppModelManagerPrivate::setupWatcher(const QFuture<void> &future, Project *project,
                                          ProjectData *projectData, CppModelManager *q)
{
    projectData->indexer = new QFutureWatcher<void>(q);
    const auto handleFinished = [project, watcher = projectData->indexer, q] {
        d->m_lockedProjectData.write([watcher, project](auto &ld) {
            const auto it = ld.m_projectData.find(project);
            if (it != ld.m_projectData.end() && it->indexer == watcher) {
                it->indexer = nullptr;
                it->fullyIndexed = !watcher->isCanceled();
            }
        });

        watcher->disconnect(q);
        watcher->deleteLater();
    };
    q->connect(projectData->indexer, &QFutureWatcher<void>::canceled, q, handleFinished);
    q->connect(projectData->indexer, &QFutureWatcher<void>::finished, q, handleFinished);
    projectData->indexer->setFuture(future);
}

void CppModelManager::updateCppEditorDocuments(bool projectsUpdated)
{
    // Refresh visible documents
    QSet<IDocument *> visibleCppEditorDocuments;
    const QList<IEditor *> editors = EditorManager::visibleEditors();
    for (IEditor *editor: editors) {
        if (IDocument *document = editor->document()) {
            const FilePath filePath = document->filePath();
            if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
                visibleCppEditorDocuments.insert(document);
                theCppEditorDocument->processor()->run(projectsUpdated);
            }
        }
    }

    // Mark invisible documents dirty
    QSet<IDocument *> invisibleCppEditorDocuments
        = Utils::toSet(DocumentModel::openedDocuments());
    invisibleCppEditorDocuments.subtract(visibleCppEditorDocuments);
    for (IDocument *document : std::as_const(invisibleCppEditorDocuments)) {
        const FilePath filePath = document->filePath();
        if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
            const CppEditorDocumentHandle::RefreshReason refreshReason = projectsUpdated
                    ? CppEditorDocumentHandle::ProjectUpdate
                    : CppEditorDocumentHandle::Other;
            theCppEditorDocument->setRefreshReason(refreshReason);
        }
    }
}

QFuture<void> CppModelManager::updateProjectInfo(const ProjectInfo::ConstPtr &newProjectInfo,
                                                 const QSet<FilePath> &additionalFiles)
{
    if (!newProjectInfo)
        return {};

    QSet<FilePath> filesToReindex;
    QStringList removedProjectParts;
    bool filesRemoved = false;

    Project * const project = newProjectInfo->project();
    if (!project)
        return {};

    ProjectData *projectData = nullptr;

    d->m_lockedProjectData.write([&newProjectInfo,
                                  project,
                                  &filesToReindex,
                                  &removedProjectParts,
                                  &filesRemoved,
                                  &projectData](auto &ld) {
        const QSet<FilePath> newSourceFiles = newProjectInfo->sourceFiles();

        // Check if we can avoid a full reindexing
        const auto it = ld.m_projectData.find(project);
        if (it != ld.m_projectData.end() && it->projectInfo && it->fullyIndexed) {
            ProjectInfoComparer comparer(*it->projectInfo, *newProjectInfo);
            if (comparer.configurationOrFilesChanged()) {
                ld.m_dirty = true;

                // If the project configuration changed, do a full reindexing
                if (comparer.configurationChanged()) {
                    removeProjectInfoFilesAndIncludesFromSnapshot(*it->projectInfo);
                    filesToReindex.unite(newSourceFiles);

                    // The "configuration file" includes all defines and therefore should be updated
                    if (comparer.definesChanged()) {
                        QMutexLocker snapshotLocker(&d->m_snapshotMutex);
                        d->m_snapshot.remove(configurationFileName());
                    }

                // Otherwise check for added and modified files
                } else {
                    const QSet<FilePath> addedFiles = comparer.addedFiles();
                    filesToReindex.unite(addedFiles);

                    const QSet<FilePath> modifiedFiles = comparer.timeStampModifiedFiles(snapshot());
                    filesToReindex.unite(modifiedFiles);
                }

                // Announce and purge the removed files from the snapshot
                const QSet<FilePath> removedFiles = comparer.removedFiles();
                if (!removedFiles.isEmpty()) {
                    filesRemoved = true;
                    emit m_instance->aboutToRemoveFiles(transform<QStringList>(removedFiles, &FilePath::toUrlishString));
                    removeFilesFromSnapshot(removedFiles);
                }
            }

            removedProjectParts = comparer.removedProjectParts();

        // A new project was opened/created, do a full indexing
        } else {
            ld.m_dirty = true;
            filesToReindex.unite(newSourceFiles);
        }

        // Update Project/ProjectInfo and File/ProjectPart table
        if (it != ld.m_projectData.end()) {
            if (it->indexer)
                it->indexer->cancel();
            it->projectInfo = newProjectInfo;
            it->fullyIndexed = false;
        }
        projectData = it == ld.m_projectData.end() ? &(
                          ld.m_projectData[project] = ProjectData{newProjectInfo, nullptr, false})
                                                   : &(*it);
        CppModelManagerPrivate::recalculateProjectPartMappings(ld);
    });

    // If requested, dump everything we got
    if (DumpProjectInfo)
        dumpModelManagerConfiguration(QLatin1String("updateProjectInfo"));

    // Remove files from snapshot that are not reachable any more
    if (filesRemoved)
        GC();

    // Announce removed project parts
    if (!removedProjectParts.isEmpty())
        emit m_instance->projectPartsRemoved(removedProjectParts);

    // Announce added project parts
    emit m_instance->projectPartsUpdated(project);

    // Ideally, we would update all the editor documents that depend on the 'filesToReindex'.
    // However, on e.g. a session restore first the editor documents are created and then the
    // project updates come in. That is, there are no reasonable dependency tables based on
    // resolved includes that we could rely on.
    updateCppEditorDocuments(/*projectsUpdated = */ true);

    filesToReindex.unite(additionalFiles);
    // Trigger reindexing
    const QFuture<void> indexingFuture = updateSourceFiles(filesToReindex,
                                                           ForcedProgressNotification);

    // It's safe to do this here, as only the UI thread writes to the map and no other thread
    // uses the indexer value.
    d->setupWatcher(indexingFuture, project, projectData, m_instance);

    return indexingFuture;
}

ProjectPart::ConstPtr CppModelManager::projectPartForId(const QString &projectPartId)
{
    return d->m_lockedProjectData.get<ProjectPart::ConstPtr>(
        [projectPartId](const CppModelManagerPrivate::SyncedProjectData &ld) {
            return ld.m_projectPartIdToProjectProjectPart.value(projectPartId);
        });
}

QList<ProjectPart::ConstPtr> CppModelManager::projectPart(const FilePath &fileName)
{
    std::optional<QList<ProjectPart::ConstPtr>> result;

    d->m_lockedProjectData.read(
        [&fileName, &result](const CppModelManagerPrivate::SyncedProjectData &sd) {
            const auto it = sd.m_fileToProjectParts.constFind(fileName);
            if (it != sd.m_fileToProjectParts.constEnd())
                result = *it;
        });

    if (result)
        return *result;

    const FilePath canonicalPath = fileName.canonicalPath();

    d->m_lockedProjectData.write(
        [&fileName, &canonicalPath, &result](CppModelManagerPrivate::SyncedProjectData &sd) {
            const auto it = sd.m_fileToProjectParts.constFind(canonicalPath);
            if (it == sd.m_fileToProjectParts.constEnd())
                return;
            sd.m_fileToProjectParts.insert(fileName, it.value());
            result = it.value();
        });

    return result.value_or(QList<ProjectPart::ConstPtr>{});
}

QList<ProjectPart::ConstPtr> CppModelManager::projectPartFromDependencies(
        const FilePath &fileName)
{
    QSet<ProjectPart::ConstPtr> parts;
    const FilePaths deps = snapshot().filesDependingOn(fileName);

    for (const FilePath &dep : deps)
        parts.unite(Utils::toSet(projectPart(dep)));

    return parts.values();
}

ProjectPart::ConstPtr CppModelManager::fallbackProjectPart()
{
    QMutexLocker locker(&d->m_fallbackProjectPartMutex);
    return d->m_fallbackProjectPart;
}

bool CppModelManager::isCppEditor(IEditor *editor)
{
    return editor->context().contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
}

std::optional<QVersionNumber> CppModelManager::usesClangd(const TextEditor::TextDocument *document)
{
    return d->m_activeModelManagerSupport->usesClangd(document);
}

bool CppModelManager::isClangCodeModelActive()
{
    return d->m_activeModelManagerSupport != &d->m_builtinModelManagerSupport;
}

void CppModelManager::emitDocumentUpdated(Document::Ptr doc)
{
    if (replaceDocument(doc))
        emit m_instance->documentUpdated(doc);
}

void CppModelManager::emitAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                               const QString &sourcePath,
                                                               const QByteArray &contents)
{
    emit m_instance->abstractEditorSupportContentsUpdated(filePath, sourcePath, contents);
}

void CppModelManager::emitAbstractEditorSupportRemoved(const QString &filePath)
{
    emit m_instance->abstractEditorSupportRemoved(filePath);
}

void CppModelManager::onProjectAdded(Project *)
{
    d->m_lockedProjectData.writeLocked()->m_dirty = true;
}

void CppModelManager::delayedGC()
{
    if (d->m_enableGC)
        d->m_delayedGcTimer.start(500);
}

static QStringList removedProjectParts(const QStringList &before, const QStringList &after)
{
    QSet<QString> b = Utils::toSet(before);
    b.subtract(Utils::toSet(after));

    return Utils::toList(b);
}

void CppModelManager::onAboutToRemoveProject(Project *project)
{
    QStringList idsOfRemovedProjectParts;

    d->m_lockedProjectData.write([project, &idsOfRemovedProjectParts](
                                     CppModelManagerPrivate::SyncedProjectData &sd) {
        sd.m_dirty = true;
        const QStringList projectPartsIdsBefore = sd.m_projectPartIdToProjectProjectPart.keys();

        sd.m_projectData.remove(project);
        CppModelManagerPrivate::recalculateProjectPartMappings(sd);

        const QStringList projectPartsIdsAfter = sd.m_projectPartIdToProjectProjectPart.keys();
        idsOfRemovedProjectParts = removedProjectParts(projectPartsIdsBefore, projectPartsIdsAfter);
    });

    if (!idsOfRemovedProjectParts.isEmpty())
        emit m_instance->projectPartsRemoved(idsOfRemovedProjectParts);

    delayedGC();
}

void CppModelManager::onActiveProjectChanged(Project *project)
{
    if (!project)
        return; // Last project closed.

    if (!d->m_lockedProjectData.readLocked()->m_projectData.contains(project))
        return; // Not yet known to us.

    updateCppEditorDocuments();
}

void CppModelManager::onSourceFilesRefreshed()
{
    if (CppIndexingSupport::isFindErrorsIndexingActive()) {
        QTimer::singleShot(1, QCoreApplication::instance(), &QCoreApplication::quit);
        qDebug("FindErrorsIndexing: Done, requesting Qt Creator to quit.");
    }
}

void CppModelManager::onCurrentEditorChanged(IEditor *editor)
{
    if (!editor || !editor->document())
        return;

    const FilePath filePath = editor->document()->filePath();
    if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
        const CppEditorDocumentHandle::RefreshReason refreshReason
                = theCppEditorDocument->refreshReason();
        if (refreshReason != CppEditorDocumentHandle::None) {
            const bool projectsChanged = refreshReason == CppEditorDocumentHandle::ProjectUpdate;
            theCppEditorDocument->setRefreshReason(CppEditorDocumentHandle::None);
            theCppEditorDocument->processor()->run(projectsChanged);
        }
    }
}

void CppModelManager::onAboutToLoadSession()
{
    if (d->m_delayedGcTimer.isActive())
        d->m_delayedGcTimer.stop();
    GC();
}

QSet<QString> CppModelManager::dependingInternalTargets(const FilePath &file)
{
    QSet<QString> result;
    const Snapshot snapshot = CppModelManager::snapshot();
    QTC_ASSERT(snapshot.contains(file), return result);
    bool wasHeader;
    const FilePath correspondingFile
            = correspondingHeaderOrSource(file, &wasHeader, CacheUsage::ReadOnly);
    const FilePaths dependingFiles = snapshot.filesDependingOn(
                wasHeader ? file : correspondingFile);
    for (const FilePath &fn : std::as_const(dependingFiles)) {
        for (const ProjectPart::ConstPtr &part : projectPart(fn))
            result.insert(part->buildSystemTarget);
    }
    return result;
}

QSet<QString> CppModelManager::internalTargets(const FilePath &filePath)
{
    QTC_ASSERT(m_instance, return {});

    const QList<ProjectPart::ConstPtr> projectParts = projectPart(filePath);
    // if we have no project parts it's most likely a header with declarations only and CMake based
    if (projectParts.isEmpty())
        return dependingInternalTargets(filePath);
    QSet<QString> targets;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        targets.insert(part->buildSystemTarget);
        if (part->buildTargetType != BuildTargetType::Executable)
            targets.unite(dependingInternalTargets(filePath));
    }
    return targets;
}

// Note: This function assumes that neither the project tree nor the code model are aware of
//       the renamings yet, i.e. they still refer to the old file paths.
void CppModelManager::renameIncludes(const QList<std::pair<FilePath, FilePath>> &oldAndNewPaths)
{
    using IncludingFile = std::pair<FilePath, FilePath>; // old and new path; might be identical
    struct RewriteCandidate {
        FilePath oldHeaderFilePath;
        FilePath newHeaderFilePath;
        QString oldHeaderFileName;
        QString newHeaderFileName;
        int includeLine;
        bool isUiHeader;
    };
    QHash<IncludingFile, QList<RewriteCandidate>> rewriteCandidates;

    const Snapshot theSnapshot = snapshot();
    for (const auto &[oldFilePath, newFilePath] : oldAndNewPaths) {
        if (oldFilePath.isEmpty() || newFilePath.isEmpty())
            continue;

        QString oldFileName = oldFilePath.fileName();
        QString newFileName = newFilePath.fileName();
        const bool isUiFile = oldFilePath.suffix() == "ui" && newFilePath.suffix() == "ui";
        const bool moved = oldFilePath.absolutePath() != newFilePath.absolutePath();
        if (isUiFile) {
            if (moved)
                return; // This is out of scope.
            oldFileName = "ui_" + oldFilePath.baseName() + ".h";
            newFileName = "ui_" + newFilePath.baseName() + ".h";
        }
        static const auto getProductNode = [](const FilePath &filePath) -> const Node * {
            const Node * const fileNode = ProjectTree::nodeForFile(filePath);
            if (!fileNode)
                return nullptr;
            const ProjectNode *productNode = fileNode->parentProjectNode();
            while (productNode && !productNode->isProduct())
                productNode = productNode->parentProjectNode();
            if (!productNode)
                productNode = fileNode->getProject()->rootProjectNode();
            return productNode;
        };
        const Node * const productNodeForUiFile = isUiFile ? getProductNode(oldFilePath) : nullptr;
        if (isUiFile && !productNodeForUiFile)
            continue;

        // Handle locations where this file is included.
        const QList<Snapshot::IncludeLocation> locations = theSnapshot.includeLocationsOfDocument(
            isUiFile ? FilePath::fromString(oldFileName) : oldFilePath);
        for (const Snapshot::IncludeLocation &loc : locations) {
            const FilePath includingFileOld = loc.first->filePath();
            FilePath includingFileNew = includingFileOld;
            for (const auto &pathPair : oldAndNewPaths) {
                if (includingFileNew == pathPair.first) {
                    includingFileNew = pathPair.second;
                    break;
                }
            }

            // Larger projects can easily have more than one ui file with the same name.
            // Replace only if ui file and including source file belong to the same product.
            if (isUiFile && getProductNode(includingFileOld) != productNodeForUiFile)
                continue;

            rewriteCandidates[std::make_pair(includingFileOld, includingFileNew)].append(
                {oldFilePath, newFilePath, oldFileName, newFileName, loc.second, isUiFile});
        }

        // Handle the includes of this file: If its location changed, its own local includes
        // need to get adapted as well.
        if (isUiFile || !moved)
            continue;
        const CPlusPlus::Document::Ptr cppDoc = theSnapshot.document(oldFilePath);
        if (!cppDoc)
            continue;
        const auto key = std::make_pair(oldFilePath, newFilePath);
        for (const CPlusPlus::Document::Include &include : cppDoc->resolvedIncludes()) {
            const FilePath oldHeaderPath = include.resolvedFileName();
            const QString oldHeaderName = oldHeaderPath.fileName();
            const bool isUiHeader = oldHeaderName.startsWith("ui_") && oldHeaderName.endsWith(".h");
            if (!isUiHeader && include.type() != CPlusPlus::Client::IncludeLocal)
                continue;
            FilePath newHeaderPath = oldHeaderPath;
            for (const auto &pathPair : oldAndNewPaths) {
                if (oldHeaderPath == pathPair.first) {
                    newHeaderPath = pathPair.second;
                    break;
                }
            }
            rewriteCandidates[key].append(
                {oldHeaderPath,
                 newHeaderPath,
                 oldHeaderName,
                 newHeaderPath.fileName(),
                 include.line(),
                 isUiHeader});
        }
    }

    const TextEditor::PlainRefactoringFileFactory changes;
    for (auto it = rewriteCandidates.cbegin(); it != rewriteCandidates.cend(); ++it) {
        const FilePath &includingFileOld = it.key().first;
        const FilePath &includingFileNew = it.key().second;
        TextEditor::RefactoringFilePtr file = changes.file(includingFileNew);
        ChangeSet changeSet;
        for (const RewriteCandidate &candidate : it.value()) {
            const QTextBlock &block = file->document()->findBlockByNumber(
                candidate.includeLine - 1);
            const FilePath relPathOld = candidate.oldHeaderFilePath.relativePathFromDir(
                includingFileOld.parentDir());
            const FilePath relPathNew = candidate.newHeaderFilePath.relativePathFromDir(
                includingFileNew.parentDir());
            int replaceStart = block.text().indexOf(relPathOld.toUrlishString());
            QString oldString;
            QString newString;
            if (candidate.isUiHeader || replaceStart == -1) {
                replaceStart = block.text().indexOf(candidate.oldHeaderFileName);
                oldString = candidate.oldHeaderFileName;
                newString = candidate.newHeaderFileName;
            } else {
                oldString = relPathOld.toUrlishString();
                newString = relPathNew.toUrlishString();
            }
            if (replaceStart > -1 && oldString != newString) {
                changeSet.replace(block.position() + replaceStart,
                                  block.position() + replaceStart + oldString.length(),
                                  newString);
            }
        }
        file->apply(changeSet);
    }
}

// Return the class name which function belongs to
static const char *belongingClassName(const Function *function)
{
    if (!function)
        return nullptr;

    if (auto funcName = function->name()) {
        if (auto qualifiedNameId = funcName->asQualifiedNameId()) {
            if (const Name *funcBaseName = qualifiedNameId->base()) {
                if (auto identifier = funcBaseName->identifier())
                    return identifier->chars();
            }
        }
    }

    return nullptr;
}

QSet<QString> CppModelManager::symbolsInFiles(const QSet<FilePath> &files) const
{
    QSet<QString> uniqueSymbols;
    const Snapshot cppSnapShot = snapshot();

    // Iterate over the files and get interesting symbols
    for (const FilePath &file : files) {
        // Add symbols from the C++ code model
        const CPlusPlus::Document::Ptr doc = cppSnapShot.document(file);
        if (!doc.isNull() && doc->control()) {
            const CPlusPlus::Control *ctrl = doc->control();
            CPlusPlus::Symbol **symPtr = ctrl->firstSymbol(); // Read-only
            while (symPtr != ctrl->lastSymbol()) {
                const CPlusPlus::Symbol *sym = *symPtr;

                const CPlusPlus::Identifier *symId = sym->identifier();
                // Add any class, function or namespace identifiers
                if ((sym->asClass() || sym->asFunction() || sym->asNamespace()) && symId
                    && symId->chars()) {
                    uniqueSymbols.insert(QString::fromUtf8(symId->chars()));
                }

                // Handle specific case : get "Foo" in "void Foo::function() {}"
                if (sym->asFunction() && !sym->asFunction()->asDeclaration()) {
                    const char *className = belongingClassName(sym->asFunction());
                    if (className)
                        uniqueSymbols.insert(QString::fromUtf8(className));
                }

                ++symPtr;
            }
        }
    }
    return uniqueSymbols;
}

void CppModelManager::onCoreAboutToClose()
{
    d->m_fallbackProjectPartTimer.disconnect();
    d->m_fallbackProjectPartTimer.stop();
    ProgressManager::cancelTasks(Constants::TASK_INDEX);
    d->m_enableGC = false;
}

void CppModelManager::handleSettingsChange(Project *project)
{
    ProjectInfoList info;
    if (project)
        info << projectInfo(project);
    else
        info << projectInfos();
    for (const ProjectInfo::ConstPtr &pi : std::as_const(info)) {
        const CppCodeModelSettings newSettings = CppCodeModelSettings::settingsForProject(
            pi->projectFilePath());
        if (pi->settings() != newSettings)
            updateProjectInfo(ProjectInfo::cloneWithNewSettings(pi, newSettings));
    }
}

void CppModelManager::setupFallbackProjectPart()
{
    ToolchainInfo tcInfo;
    RawProjectPart rpp;
    rpp.setMacros(definedMacros());
    rpp.setHeaderPaths(headerPaths());
    rpp.setQtVersion(QtMajorVersion::Qt5);

    // Do not activate ObjectiveCExtensions since this will lead to the
    // "objective-c++" language option for a project-less *.cpp file.
    LanguageExtensions langExtensions = LanguageExtension::All;
    langExtensions &= ~LanguageExtensions(LanguageExtension::ObjectiveC);

    // TODO: Use different fallback toolchain for different kinds of files?
    const Kit * const defaultKit = KitManager::isLoaded() ? KitManager::defaultKit() : nullptr;
    const Toolchain * const defaultTc = defaultKit
            ? ToolchainKitAspect::cxxToolchain(defaultKit) : nullptr;
    if (defaultKit && defaultTc) {
        FilePath sysroot = SysRootKitAspect::sysRoot(defaultKit);
        if (sysroot.isEmpty())
            sysroot = FilePath::fromString(defaultTc->sysRoot());
        Utils::Environment env = defaultKit->buildEnvironment();
        tcInfo = ToolchainInfo(defaultTc, sysroot, env);
        const auto macroInspectionWrapper = [runner = tcInfo.macroInspectionRunner](
                const QStringList &flags) {
            Toolchain::MacroInspectionReport report = runner(flags);
            report.languageVersion = LanguageVersion::LatestCxx;
            return report;
        };
        tcInfo.macroInspectionRunner = macroInspectionWrapper;
    }

    const auto part = ProjectPart::create({}, rpp, {}, {}, {}, langExtensions, {}, tcInfo);
    {
        QMutexLocker locker(&d->m_fallbackProjectPartMutex);
        d->m_fallbackProjectPart = part;
    }
    emit m_instance->fallbackProjectPartUpdated();
}

void CppModelManager::GC()
{
    if (!d->m_enableGC)
        return;

    // Collect files of opened editors and editor supports (e.g. ui code model)
    FilePaths filesInEditorSupports;
    const QList<CppEditorDocumentHandle *> editorDocuments = cppEditorDocuments();
    for (const CppEditorDocumentHandle *editorDocument : editorDocuments)
        filesInEditorSupports << editorDocument->filePath();

    const QSet<AbstractEditorSupport *> abstractEditorSupportList = abstractEditorSupports();
    for (AbstractEditorSupport *abstractEditorSupport : abstractEditorSupportList)
        filesInEditorSupports << abstractEditorSupport->filePath();

    Snapshot currentSnapshot = snapshot();
    QSet<FilePath> reachableFiles;
    // The configuration file is part of the project files, which is just fine.
    // If single files are open, without any project, then there is no need to
    // keep the configuration file around.
    FilePaths todo = filesInEditorSupports + projectFiles();

    // Collect all files that are reachable from the project files
    while (!todo.isEmpty()) {
        const FilePath filePath = todo.last();
        todo.removeLast();

        if (!Utils::insert(reachableFiles, filePath))
            continue;

        if (Document::Ptr doc = currentSnapshot.document(filePath))
            todo += doc->includedFiles();
    }

    // Find out the files in the current snapshot that are not reachable from the project files
    QStringList notReachableFiles;
    Snapshot newSnapshot;
    for (Snapshot::const_iterator it = currentSnapshot.begin(); it != currentSnapshot.end(); ++it) {
        const FilePath &fileName = it.key();

        if (reachableFiles.contains(fileName))
            newSnapshot.insert(it.value());
        else
            notReachableFiles.append(fileName.toUrlishString());
    }

    // Announce removing files and replace the snapshot
    emit m_instance->aboutToRemoveFiles(notReachableFiles);
    replaceSnapshot(newSnapshot);
    emit m_instance->gcFinished();
}

void CppModelManager::finishedRefreshingSourceFiles(const QSet<QString> &files)
{
    emit m_instance->sourceFilesRefreshed(files);
}

void CppModelManager::activateClangCodeModel(
        std::unique_ptr<ModelManagerSupport> &&modelManagerSupport)
{
    d->m_extendedModelManagerSupport = std::move(modelManagerSupport);
    d->m_activeModelManagerSupport = d->m_extendedModelManagerSupport.get();
}

CppCompletionAssistProvider *CppModelManager::completionAssistProvider()
{
    return d->m_builtinModelManagerSupport.completionAssistProvider();
}

TextEditor::BaseHoverHandler *CppModelManager::createHoverHandler()
{
    return d->m_builtinModelManagerSupport.createHoverHandler();
}

void CppModelManager::followSymbol(const CursorInEditor &data,
                                   const LinkHandler &processLinkCallback,
                                   bool resolveTarget, bool inNextSplit,
                                   FollowSymbolMode mode, Backend backend)
{
    modelManagerSupport(backend)->followSymbol(data, processLinkCallback, mode,
                                               resolveTarget, inNextSplit);
}

void CppModelManager::followSymbolToType(const CursorInEditor &data,
                                         const LinkHandler &processLinkCallback,
                                         bool inNextSplit, Backend backend)
{
    modelManagerSupport(backend)->followSymbolToType(data, processLinkCallback,
                                                                 inNextSplit);
}

void CppModelManager::switchDeclDef(const CursorInEditor &data,
                                    const LinkHandler &processLinkCallback,
                                    Backend backend)
{
    modelManagerSupport(backend)->switchDeclDef(data, processLinkCallback);
}

BaseEditorDocumentProcessor *CppModelManager::createEditorDocumentProcessor(
    TextEditor::TextDocument *baseTextDocument)
{
    return d->m_activeModelManagerSupport->createEditorDocumentProcessor(baseTextDocument);
}

CppIndexingSupport *CppModelManager::indexingSupport()
{
    return d->m_internalIndexingSupport;
}

FilePaths CppModelManager::projectFiles()
{
    return d->m_lockedProjectData.update<FilePaths>(
        [](CppModelManagerPrivate::SyncedProjectData &sd) {
            CppModelManagerPrivate::ensureUpdated(sd);
            return sd.m_projectFiles;
        });
}

HeaderPaths CppModelManager::headerPaths()
{
    return d->m_lockedProjectData.update<HeaderPaths>(
        [](CppModelManagerPrivate::SyncedProjectData &sd) {
            CppModelManagerPrivate::ensureUpdated(sd);
            return sd.m_headerPaths;
        });
}

void CppModelManager::setHeaderPaths(const HeaderPaths &headerPaths)
{
    d->m_lockedProjectData.writeLocked()->m_headerPaths = headerPaths;
}

Macros CppModelManager::definedMacros()
{
    return d->m_lockedProjectData.update<Macros>([](CppModelManagerPrivate::SyncedProjectData &sd) {
        CppModelManagerPrivate::ensureUpdated(sd);
        return sd.m_definedMacros;
    });
}

void CppModelManager::enableGarbageCollector(bool enable)
{
    d->m_delayedGcTimer.stop();
    d->m_enableGC = enable;
}

SymbolFinder *CppModelManager::symbolFinder()
{
    return &d->m_symbolFinder;
}

QThreadPool *CppModelManager::sharedThreadPool()
{
    return &d->m_threadPool;
}

bool CppModelManager::setExtraDiagnostics(const FilePath &filePath,
                                          const QString &kind,
                                          const QList<Document::DiagnosticMessage> &diagnostics)
{
    d->m_diagnosticMessages = diagnostics;
    emit m_instance->diagnosticsChanged(filePath, kind);
    return true;
}

const QList<Document::DiagnosticMessage> CppModelManager::diagnosticMessages()
{
    return d->m_diagnosticMessages;
}

} // namespace CppEditor
