/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cppmodelmanager.h"

#include "abstracteditorsupport.h"
#include "abstractoverviewmodel.h"
#include "baseeditordocumentprocessor.h"
#include "builtinindexingsupport.h"
#include "cppcodemodelinspectordumper.h"
#include "cppcurrentdocumentfilter.h"
#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cppfindreferences.h"
#include "cppincludesfilter.h"
#include "cppindexingsupport.h"
#include "cpplocatordata.h"
#include "cpplocatorfilter.h"
#include "cppbuiltinmodelmanagersupport.h"
#include "cpprefactoringchanges.h"
#include "cpprefactoringengine.h"
#include "cppsourceprocessor.h"
#include "cpptoolsjsextension.h"
#include "cpptoolsreuse.h"
#include "editordocumenthandle.h"
#include "stringtable.h"
#include "symbolfinder.h"
#include "symbolsfindfilter.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/TypeOfExpression.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/session.h>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFutureWatcher>
#include <QMutexLocker>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QTextBlock>
#include <QThreadPool>
#include <QTimer>
#include <QWriteLocker>

#if defined(QTCREATOR_WITH_DUMP_AST) && defined(Q_CC_GNU)
#define WITH_AST_DUMP
#include <iostream>
#include <sstream>
#endif

static const bool DumpProjectInfo = qgetenv("QTC_DUMP_PROJECT_INFO") == "1";

using namespace CPlusPlus;
using namespace ProjectExplorer;

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

using REType = RefactoringEngineType;

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
    void setupWatcher(const QFuture<void> &future, ProjectExplorer::Project *project,
                      ProjectData *projectData, CppModelManager *q);

    // Snapshot
    mutable QMutex m_snapshotMutex;
    Snapshot m_snapshot;

    // Project integration
    QReadWriteLock m_projectLock;
    QHash<ProjectExplorer::Project *, ProjectData> m_projectData;
    QMap<Utils::FilePath, QList<ProjectPart::ConstPtr> > m_fileToProjectParts;
    QMap<QString, ProjectPart::ConstPtr> m_projectPartIdToProjectProjectPart;

    // The members below are cached/(re)calculated from the projects and/or their project parts
    bool m_dirty;
    QStringList m_projectFiles;
    ProjectExplorer::HeaderPaths m_headerPaths;
    ProjectExplorer::Macros m_definedMacros;

    // Editor integration
    mutable QMutex m_cppEditorDocumentsMutex;
    QMap<QString, CppEditorDocumentHandle *> m_cppEditorDocuments;
    QSet<AbstractEditorSupport *> m_extraEditorSupports;

    // Model Manager Supports for e.g. completion and highlighting
    ModelManagerSupport::Ptr m_builtinModelManagerSupport;
    ModelManagerSupport::Ptr m_activeModelManagerSupport;

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

    // Refactoring
    using REHash = QMap<REType, RefactoringEngineInterface *>;
    REHash m_refactoringEngines;

    CppLocatorData m_locatorData;
    std::unique_ptr<Core::ILocatorFilter> m_locatorFilter;
    std::unique_ptr<Core::ILocatorFilter> m_classesFilter;
    std::unique_ptr<Core::ILocatorFilter> m_includesFilter;
    std::unique_ptr<Core::ILocatorFilter> m_functionsFilter;
    std::unique_ptr<Core::IFindFilter> m_symbolsFindFilter;
    std::unique_ptr<Core::ILocatorFilter> m_currentDocumentFilter;
};

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

QSet<QString> CppModelManager::timeStampModifiedFiles(const QList<Document::Ptr> &documentsToCheck)
{
    QSet<QString> sourceFiles;

    foreach (const Document::Ptr doc, documentsToCheck) {
        const QDateTime lastModified = doc->lastModified();

        if (!lastModified.isNull()) {
            QFileInfo fileInfo(doc->fileName());

            if (fileInfo.exists() && fileInfo.lastModified() != lastModified)
                sourceFiles.insert(doc->fileName());
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
    CppModelManager *that = instance();
    return new CppSourceProcessor(that->snapshot(), [that](const Document::Ptr &doc) {
        const Document::Ptr previousDocument = that->document(doc->fileName());
        const unsigned newRevision = previousDocument.isNull()
                ? 1U
                : previousDocument->revision() + 1;
        doc->setRevision(newRevision);
        that->emitDocumentUpdated(doc);
        doc->releaseSourceAndAST();
    });
}

QString CppModelManager::editorConfigurationFileName()
{
    return QLatin1String("<per-editor-defines>");
}

static RefactoringEngineInterface *getRefactoringEngine(CppModelManagerPrivate::REHash &engines)
{
    QTC_ASSERT(!engines.empty(), return nullptr;);
    RefactoringEngineInterface *currentEngine = engines[REType::BuiltIn];
    if (engines.find(REType::ClangCodeModel) != engines.end()) {
        currentEngine = engines[REType::ClangCodeModel];
    } else if (engines.find(REType::ClangRefactoring) != engines.end()) {
        RefactoringEngineInterface *engine = engines[REType::ClangRefactoring];
        if (engine->isRefactoringEngineAvailable())
            currentEngine = engine;
    }
    return currentEngine;
}

void CppModelManager::startLocalRenaming(const CursorInEditor &data,
                                         const ProjectPart *projectPart,
                                         RenameCallback &&renameSymbolsCallback)
{
    RefactoringEngineInterface *engine = getRefactoringEngine(d->m_refactoringEngines);
    QTC_ASSERT(engine, return;);
    engine->startLocalRenaming(data, projectPart, std::move(renameSymbolsCallback));
}

void CppModelManager::globalRename(const CursorInEditor &data, UsagesCallback &&renameCallback,
                                   const QString &replacement)
{
    RefactoringEngineInterface *engine = getRefactoringEngine(d->m_refactoringEngines);
    QTC_ASSERT(engine, return;);
    engine->globalRename(data, std::move(renameCallback), replacement);
}

void CppModelManager::findUsages(const CursorInEditor &data,
                                 UsagesCallback &&showUsagesCallback) const
{
    RefactoringEngineInterface *engine = getRefactoringEngine(d->m_refactoringEngines);
    QTC_ASSERT(engine, return;);
    engine->findUsages(data, std::move(showUsagesCallback));
}

bool CppModelManager::positionRequiresSignal(const QString &filePath, const QByteArray &content,
                                             int position) const
{
    if (content.isEmpty())
        return false;

    // Insert a dummy prefix if we don't have a real one. Otherwise the AST path will not contain
    // anything after the CallAST.
    QByteArray fixedContent = content;
    if (position > 2 && content.mid(position - 2, 2) == "::")
        fixedContent.insert(position, 'x');

    const Snapshot snapshot = this->snapshot();
    const Document::Ptr document = snapshot.preprocessedDocument(fixedContent,
                                                                 Utils::FilePath::fromString(filePath));
    document->check();
    QTextDocument textDocument(QString::fromUtf8(fixedContent));
    QTextCursor cursor(&textDocument);
    cursor.setPosition(position);

    // Are we at the second argument of a function call?
    const QList<AST *> path = ASTPath(document)(cursor);
    if (path.isEmpty() || !path.last()->asSimpleName())
        return false;
    const CallAST *callAst = nullptr;
    for (auto it = path.crbegin(); it != path.crend(); ++it) {
        if ((callAst = (*it)->asCall()))
            break;
    }
    if (!callAst)
        return false;
    if (!callAst->expression_list || !callAst->expression_list->next)
        return false;
    const ExpressionAST * const secondArg = callAst->expression_list->next->value;
    if (secondArg->firstToken() > path.last()->firstToken()
            || secondArg->lastToken() < path.last()->lastToken()) {
        return false;
    }

    // Is the function called "connect" or "disconnect"?
    if (!callAst->base_expression)
        return false;
    Scope *scope = document->globalNamespace();
    for (auto it = path.crbegin(); it != path.crend(); ++it) {
        if (const CompoundStatementAST * const stmtAst = (*it)->asCompoundStatement()) {
            scope = stmtAst->symbol;
            break;
        }
    }
    const NameAST *nameAst = nullptr;
    const LookupContext context(document, snapshot);
    if (const IdExpressionAST * const idAst = callAst->base_expression->asIdExpression()) {
        nameAst = idAst->name;
    } else if (const MemberAccessAST * const ast = callAst->base_expression->asMemberAccess()) {
        nameAst = ast->member_name;
        TypeOfExpression exprType;
        exprType.setExpandTemplates(true);
        exprType.init(document, snapshot);
        const QList<LookupItem> typeMatches = exprType(ast->base_expression, document, scope);
        if (typeMatches.isEmpty())
            return false;
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
            return false;
        const ClassOrNamespace * const result = context.lookupType(namedType->name(), scope);
        if (!result)
            return false;
        scope = result->rootClass();
        if (!scope)
            return false;
    }
    if (!nameAst || !nameAst->name)
        return false;
    const Identifier * const id = nameAst->name->identifier();
    if (!id)
        return false;
    const QString funcName = QString::fromUtf8(id->chars(), id->size());
    if (funcName != "connect" && funcName != "disconnect")
        return false;

    // Is the function a member function of QObject?
    const QList<LookupItem> matches = context.lookup(nameAst->name, scope);
    for (const LookupItem &match : matches) {
        if (!match.scope())
            continue;
        const Class *klass = match.scope()->asClass();
        if (!klass || !klass->name())
            continue;
        const Identifier * const classId = klass->name()->identifier();
        if (classId && QString::fromUtf8(classId->chars(), classId->size()) == "QObject")
            return true;
    }

    return false;
}

void CppModelManager::addRefactoringEngine(RefactoringEngineType type,
                                           RefactoringEngineInterface *refactoringEngine)
{
    instance()->d->m_refactoringEngines[type] = refactoringEngine;
}

void CppModelManager::removeRefactoringEngine(RefactoringEngineType type)
{
    instance()->d->m_refactoringEngines.remove(type);
}

RefactoringEngineInterface *CppModelManager::builtinRefactoringEngine()
{
    return instance()->d->m_refactoringEngines.value(RefactoringEngineType::BuiltIn);
}

FollowSymbolUnderCursor &CppModelManager::builtinFollowSymbol()
{
    return instance()->d->m_builtinModelManagerSupport.staticCast<BuiltinModelManagerSupport>()
            ->followSymbolInterface();
}

template<class FilterClass>
static void setFilter(std::unique_ptr<FilterClass> &filter,
                      std::unique_ptr<FilterClass> &&newFilter)
{
    QTC_ASSERT(newFilter, return;);
    filter = std::move(newFilter);
}

void CppModelManager::setLocatorFilter(std::unique_ptr<Core::ILocatorFilter> &&filter)
{
    setFilter(d->m_locatorFilter, std::move(filter));
}

void CppModelManager::setClassesFilter(std::unique_ptr<Core::ILocatorFilter> &&filter)
{
    setFilter(d->m_classesFilter, std::move(filter));
}

void CppModelManager::setIncludesFilter(std::unique_ptr<Core::ILocatorFilter> &&filter)
{
    setFilter(d->m_includesFilter, std::move(filter));
}

void CppModelManager::setFunctionsFilter(std::unique_ptr<Core::ILocatorFilter> &&filter)
{
    setFilter(d->m_functionsFilter, std::move(filter));
}

void CppModelManager::setSymbolsFindFilter(std::unique_ptr<Core::IFindFilter> &&filter)
{
    setFilter(d->m_symbolsFindFilter, std::move(filter));
}

void CppModelManager::setCurrentDocumentFilter(std::unique_ptr<Core::ILocatorFilter> &&filter)
{
    setFilter(d->m_currentDocumentFilter, std::move(filter));
}

Core::ILocatorFilter *CppModelManager::locatorFilter() const
{
    return d->m_locatorFilter.get();
}

Core::ILocatorFilter *CppModelManager::classesFilter() const
{
    return d->m_classesFilter.get();
}

Core::ILocatorFilter *CppModelManager::includesFilter() const
{
    return d->m_includesFilter.get();
}

Core::ILocatorFilter *CppModelManager::functionsFilter() const
{
    return d->m_functionsFilter.get();
}

Core::IFindFilter *CppModelManager::symbolsFindFilter() const
{
    return d->m_symbolsFindFilter.get();
}

Core::ILocatorFilter *CppModelManager::currentDocumentFilter() const
{
    return d->m_currentDocumentFilter.get();
}

std::unique_ptr<AbstractOverviewModel> CppModelManager::createOverviewModel() const
{
    return d->m_builtinModelManagerSupport->createOverviewModel();
}

QString CppModelManager::configurationFileName()
{
    return Preprocessor::configurationFileName();
}

void CppModelManager::updateModifiedSourceFiles()
{
    const Snapshot snapshot = this->snapshot();
    QList<Document::Ptr> documentsToCheck;
    foreach (const Document::Ptr document, snapshot)
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
    Core::JsExpander::registerGlobalObject("Cpp", [this] {
        return new CppToolsJsExtension(&d->m_locatorData);
    });
}

void CppModelManager::initCppTools()
{
    // Objects
    connect(Core::VcsManager::instance(), &Core::VcsManager::repositoryChanged,
            this, &CppModelManager::updateModifiedSourceFiles);
    connect(Core::DocumentManager::instance(), &Core::DocumentManager::filesChangedInternally,
            [this](const Utils::FilePaths &filePaths) {
        updateSourceFiles(Utils::transform<QSet>(filePaths, &Utils::FilePath::toString));
    });

    connect(this, &CppModelManager::documentUpdated,
            &d->m_locatorData, &CppLocatorData::onDocumentUpdated);

    connect(this, &CppModelManager::aboutToRemoveFiles,
            &d->m_locatorData, &CppLocatorData::onAboutToRemoveFiles);

    // Set up builtin filters
    setLocatorFilter(std::make_unique<CppLocatorFilter>(&d->m_locatorData));
    setClassesFilter(std::make_unique<CppClassesFilter>(&d->m_locatorData));
    setIncludesFilter(std::make_unique<CppIncludesFilter>());
    setFunctionsFilter(std::make_unique<CppFunctionsFilter>(&d->m_locatorData));
    setSymbolsFindFilter(std::make_unique<SymbolsFindFilter>(this));
    setCurrentDocumentFilter(
                std::make_unique<Internal::CppCurrentDocumentFilter>(this));
}

void CppModelManager::initializeBuiltinModelManagerSupport()
{
    d->m_builtinModelManagerSupport
            = BuiltinModelManagerSupportProvider().createModelManagerSupport();
    d->m_activeModelManagerSupport = d->m_builtinModelManagerSupport;
    d->m_refactoringEngines[RefactoringEngineType::BuiltIn] =
            &d->m_activeModelManagerSupport->refactoringEngineInterface();
}

CppModelManager::CppModelManager()
    : CppModelManagerBase(nullptr)
    , d(new CppModelManagerPrivate)
{
    m_instance = this;

    // Used for weak dependency in VcsBaseSubmitEditor
    setObjectName("CppModelManager");
    ExtensionSystem::PluginManager::addObject(this);

    d->m_enableGC = true;

    // Visual C++ has 1MiB, macOSX has 512KiB
    if (Utils::HostOsInfo::isWindowsHost() || Utils::HostOsInfo::isMacHost())
        d->m_threadPool.setStackSize(2 * 1024 * 1024);

    qRegisterMetaType<QSet<QString> >();
    connect(this, &CppModelManager::sourceFilesRefreshed,
            this, &CppModelManager::onSourceFilesRefreshed);

    d->m_findReferences = new CppFindReferences(this);
    d->m_indexerEnabled = qgetenv("QTC_NO_CODE_INDEXER") != "1";

    d->m_dirty = true;

    d->m_delayedGcTimer.setObjectName(QLatin1String("CppModelManager::m_delayedGcTimer"));
    d->m_delayedGcTimer.setSingleShot(true);
    connect(&d->m_delayedGcTimer, &QTimer::timeout, this, &CppModelManager::GC);

    auto sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::projectAdded,
            this, &CppModelManager::onProjectAdded);
    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, &CppModelManager::onAboutToRemoveProject);
    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToLoadSession,
            this, &CppModelManager::onAboutToLoadSession);
    connect(sessionManager, &ProjectExplorer::SessionManager::startupProjectChanged,
            this, &CppModelManager::onActiveProjectChanged);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &CppModelManager::onCurrentEditorChanged);

    connect(Core::DocumentManager::instance(), &Core::DocumentManager::allDocumentsRenamed,
            this, &CppModelManager::renameIncludes);

    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
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

    initializeBuiltinModelManagerSupport();

    d->m_internalIndexingSupport = new BuiltinIndexingSupport;

    initCppTools();
}

CppModelManager::~CppModelManager()
{
    ExtensionSystem::PluginManager::removeObject(this);

    delete d->m_internalIndexingSupport;
    delete d;
}

Snapshot CppModelManager::snapshot() const
{
    QMutexLocker locker(&d->m_snapshotMutex);
    return d->m_snapshot;
}

Document::Ptr CppModelManager::document(const QString &fileName) const
{
    QMutexLocker locker(&d->m_snapshotMutex);
    return d->m_snapshot.document(fileName);
}

/// Replace the document in the snapshot.
///
/// \returns true if successful, false if the new document is out-dated.
bool CppModelManager::replaceDocument(Document::Ptr newDoc)
{
    QMutexLocker locker(&d->m_snapshotMutex);

    Document::Ptr previous = d->m_snapshot.document(newDoc->fileName());
    if (previous && (newDoc->revision() != 0 && newDoc->revision() < previous->revision()))
        // the new document is outdated
        return false;

    d->m_snapshot.insert(newDoc);
    return true;
}

/// Make sure that m_projectLock is locked for writing when calling this.
void CppModelManager::ensureUpdated()
{
    if (!d->m_dirty)
        return;

    d->m_projectFiles = internalProjectFiles();
    d->m_headerPaths = internalHeaderPaths();
    d->m_definedMacros = internalDefinedMacros();
    d->m_dirty = false;
}

QStringList CppModelManager::internalProjectFiles() const
{
    QStringList files;
    for (const ProjectData &projectData : qAsConst(d->m_projectData)) {
        for (const ProjectPart::ConstPtr &part : projectData.projectInfo->projectParts()) {
            for (const ProjectFile &file : part->files)
                files += file.path;
        }
    }
    files.removeDuplicates();
    return files;
}

ProjectExplorer::HeaderPaths CppModelManager::internalHeaderPaths() const
{
    ProjectExplorer::HeaderPaths headerPaths;
    for (const ProjectData &projectData: qAsConst(d->m_projectData)) {
        for (const ProjectPart::ConstPtr &part : projectData.projectInfo->projectParts()) {
            for (const ProjectExplorer::HeaderPath &path : part->headerPaths) {
                ProjectExplorer::HeaderPath hp(QDir::cleanPath(path.path), path.type);
                if (!headerPaths.contains(hp))
                    headerPaths.push_back(std::move(hp));
            }
        }
    }
    return headerPaths;
}

static void addUnique(const ProjectExplorer::Macros &newMacros,
                      ProjectExplorer::Macros &macros,
                      QSet<ProjectExplorer::Macro> &alreadyIn)
{
    for (const ProjectExplorer::Macro &macro : newMacros) {
        if (!alreadyIn.contains(macro)) {
            macros += macro;
            alreadyIn.insert(macro);
        }
    }
}

ProjectExplorer::Macros CppModelManager::internalDefinedMacros() const
{
    ProjectExplorer::Macros macros;
    QSet<ProjectExplorer::Macro> alreadyIn;
    for (const ProjectData &projectData : qAsConst(d->m_projectData)) {
        for (const ProjectPart::ConstPtr &part : projectData.projectInfo->projectParts()) {
            addUnique(part->toolChainMacros, macros, alreadyIn);
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
                              ProjectExplorer:: Macro::toByteArray(definedMacros()));
}

QSet<AbstractEditorSupport *> CppModelManager::abstractEditorSupports() const
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

CppEditorDocumentHandle *CppModelManager::cppEditorDocument(const QString &filePath) const
{
    if (filePath.isEmpty())
        return nullptr;

    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    return d->m_cppEditorDocuments.value(filePath, 0);
}

BaseEditorDocumentProcessor *CppModelManager::cppEditorDocumentProcessor(const QString &filePath)
{
    const auto document = instance()->cppEditorDocument(filePath);
    return document ? document->processor() : nullptr;
}

void CppModelManager::registerCppEditorDocument(CppEditorDocumentHandle *editorDocument)
{
    QTC_ASSERT(editorDocument, return);
    const QString filePath = editorDocument->filePath();
    QTC_ASSERT(!filePath.isEmpty(), return);

    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    QTC_ASSERT(d->m_cppEditorDocuments.value(filePath, 0) == 0, return);
    d->m_cppEditorDocuments.insert(filePath, editorDocument);
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
                                   const QString &replacement)
{
    if (symbol->identifier())
        d->m_findReferences->renameUsages(symbol, context, replacement);
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

    foreach (const CppEditorDocumentHandle *cppEditorDocument, cppEditorDocuments()) {
        workingCopy.insert(cppEditorDocument->filePath(),
                           cppEditorDocument->contents(),
                           cppEditorDocument->revision());
    }

    for (AbstractEditorSupport *es : qAsConst(d->m_extraEditorSupports))
        workingCopy.insert(es->fileName(), es->contents(), es->revision());

    // Add the project configuration file
    QByteArray conf = codeModelConfiguration();
    conf += ProjectExplorer::Macro::toByteArray(definedMacros());
    workingCopy.insert(configurationFileName(), conf);

    return workingCopy;
}

WorkingCopy CppModelManager::workingCopy() const
{
    return const_cast<CppModelManager *>(this)->buildWorkingCopyList();
}

QByteArray CppModelManager::codeModelConfiguration() const
{
    return QByteArray::fromRawData(pp_configuration, qstrlen(pp_configuration));
}

CppLocatorData *CppModelManager::locatorData() const
{
    return &d->m_locatorData;
}

static QSet<QString> tooBigFilesRemoved(const QSet<QString> &files, int fileSizeLimitInMb)
{
    if (fileSizeLimitInMb <= 0)
        return files;

    QSet<QString> result;
    QFileInfo fileInfo;

    for (const QString &filePath : files) {
        fileInfo.setFile(filePath);
        if (fileSizeExceedsLimit(fileInfo, fileSizeLimitInMb))
            continue;

        result << filePath;
    }

    return result;
}

QFuture<void> CppModelManager::updateSourceFiles(const QSet<QString> &sourceFiles,
                                                 ProgressNotificationMode mode)
{
    if (sourceFiles.isEmpty() || !d->m_indexerEnabled)
        return QFuture<void>();

    const QSet<QString> filteredFiles = tooBigFilesRemoved(sourceFiles, indexerFileSizeLimitInMb());

    return d->m_internalIndexingSupport->refreshSourceFiles(filteredFiles, mode);
}

QList<ProjectInfo::ConstPtr> CppModelManager::projectInfos() const
{
    QReadLocker locker(&d->m_projectLock);
    return Utils::transform<QList<ProjectInfo::ConstPtr>>(d->m_projectData,
            [](const ProjectData &d) { return d.projectInfo; });
}

ProjectInfo::ConstPtr CppModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QReadLocker locker(&d->m_projectLock);
    return d->m_projectData.value(project).projectInfo;
}

/// \brief Remove all files and their includes (recursively) of given ProjectInfo from the snapshot.
void CppModelManager::removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo)
{
    QMutexLocker snapshotLocker(&d->m_snapshotMutex);
    foreach (const ProjectPart::ConstPtr &projectPart, projectInfo.projectParts()) {
        foreach (const ProjectFile &cxxFile, projectPart->files) {
            foreach (const QString &fileName, d->m_snapshot.allIncludesForDocument(cxxFile.path))
                d->m_snapshot.remove(fileName);
            d->m_snapshot.remove(cxxFile.path);
        }
    }
}

QList<CppEditorDocumentHandle *> CppModelManager::cppEditorDocuments() const
{
    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    return d->m_cppEditorDocuments.values();
}

/// \brief Remove all given files from the snapshot.
void CppModelManager::removeFilesFromSnapshot(const QSet<QString> &filesToRemove)
{
    QMutexLocker snapshotLocker(&d->m_snapshotMutex);
    for (const QString &file : filesToRemove)
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

    QSet<QString> addedFiles() const
    {
        QSet<QString> addedFilesSet = m_newSourceFiles;
        addedFilesSet.subtract(m_oldSourceFiles);
        return addedFilesSet;
    }

    QSet<QString> removedFiles() const
    {
        QSet<QString> removedFilesSet = m_oldSourceFiles;
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
    QSet<QString> timeStampModifiedFiles(const Snapshot &snapshot) const
    {
        QSet<QString> commonSourceFiles = m_newSourceFiles;
        commonSourceFiles.intersect(m_oldSourceFiles);

        QList<Document::Ptr> documentsToCheck;
        for (const QString &file : commonSourceFiles) {
            if (Document::Ptr document = snapshot.document(file))
                documentsToCheck << document;
        }

        return CppModelManager::timeStampModifiedFiles(documentsToCheck);
    }

private:
    static QSet<QString> projectPartIds(const QVector<ProjectPart::ConstPtr> &projectParts)
    {
        QSet<QString> ids;

        foreach (const ProjectPart::ConstPtr &projectPart, projectParts)
            ids.insert(projectPart->id());

        return ids;
    }

private:
    const ProjectInfo &m_old;
    const QSet<QString> m_oldSourceFiles;

    const ProjectInfo &m_new;
    const QSet<QString> m_newSourceFiles;
};

/// Make sure that m_projectLock is locked for writing when calling this.
void CppModelManager::recalculateProjectPartMappings()
{
    d->m_projectPartIdToProjectProjectPart.clear();
    d->m_fileToProjectParts.clear();
    for (const ProjectData &projectData : qAsConst(d->m_projectData)) {
        for (const ProjectPart::ConstPtr &projectPart : projectData.projectInfo->projectParts()) {
            d->m_projectPartIdToProjectProjectPart[projectPart->id()] = projectPart;
            for (const ProjectFile &cxxFile : projectPart->files)
                d->m_fileToProjectParts[Utils::FilePath::fromString(cxxFile.path)].append(
                            projectPart);
        }
    }

    d->m_symbolFinder.clearCache();
}

void CppModelManagerPrivate::setupWatcher(const QFuture<void> &future,
                                          ProjectExplorer::Project *project,
                                          ProjectData *projectData, CppModelManager *q)
{
    projectData->indexer = new QFutureWatcher<void>(q);
    const auto handleFinished = [this, project, watcher = projectData->indexer, q] {
        if (const auto it = m_projectData.find(project);
                it != m_projectData.end() && it->indexer == watcher) {
            it->indexer = nullptr;
            it->fullyIndexed = !watcher->isCanceled();
        }
        watcher->disconnect(q);
        watcher->deleteLater();
    };
    q->connect(projectData->indexer, &QFutureWatcher<void>::canceled, q, handleFinished);
    q->connect(projectData->indexer, &QFutureWatcher<void>::finished, q, handleFinished);
    projectData->indexer->setFuture(future);
}

void CppModelManager::updateCppEditorDocuments(bool projectsUpdated) const
{
    // Refresh visible documents
    QSet<Core::IDocument *> visibleCppEditorDocuments;
    foreach (Core::IEditor *editor, Core::EditorManager::visibleEditors()) {
        if (Core::IDocument *document = editor->document()) {
            const QString filePath = document->filePath().toString();
            if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
                visibleCppEditorDocuments.insert(document);
                theCppEditorDocument->processor()->run(projectsUpdated);
            }
        }
    }

    // Mark invisible documents dirty
    QSet<Core::IDocument *> invisibleCppEditorDocuments
        = Utils::toSet(Core::DocumentModel::openedDocuments());
    invisibleCppEditorDocuments.subtract(visibleCppEditorDocuments);
    foreach (Core::IDocument *document, invisibleCppEditorDocuments) {
        const QString filePath = document->filePath().toString();
        if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
            const CppEditorDocumentHandle::RefreshReason refreshReason = projectsUpdated
                    ? CppEditorDocumentHandle::ProjectUpdate
                    : CppEditorDocumentHandle::Other;
            theCppEditorDocument->setRefreshReason(refreshReason);
        }
    }
}

QFuture<void> CppModelManager::updateProjectInfo(const ProjectInfo::ConstPtr &newProjectInfo,
                                                 const QSet<QString> &additionalFiles)
{
    if (!newProjectInfo)
        return {};

    QSet<QString> filesToReindex;
    QStringList removedProjectParts;
    bool filesRemoved = false;

    ProjectExplorer::Project * const project = projectForProjectInfo(*newProjectInfo);
    if (!project)
        return {};

    ProjectData *projectData = nullptr;
    { // Only hold the lock for a limited scope, so the dumping afterwards does not deadlock.
        QWriteLocker projectLocker(&d->m_projectLock);

        const QSet<QString> newSourceFiles = newProjectInfo->sourceFiles();

        // Check if we can avoid a full reindexing
        const auto it = d->m_projectData.find(project);
        if (it != d->m_projectData.end() && it->projectInfo && it->fullyIndexed) {
            ProjectInfoComparer comparer(*it->projectInfo, *newProjectInfo);
            if (comparer.configurationOrFilesChanged()) {
                d->m_dirty = true;

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
                    const QSet<QString> addedFiles = comparer.addedFiles();
                    filesToReindex.unite(addedFiles);

                    const QSet<QString> modifiedFiles = comparer.timeStampModifiedFiles(snapshot());
                    filesToReindex.unite(modifiedFiles);
                }

                // Announce and purge the removed files from the snapshot
                const QSet<QString> removedFiles = comparer.removedFiles();
                if (!removedFiles.isEmpty()) {
                    filesRemoved = true;
                    emit aboutToRemoveFiles(Utils::toList(removedFiles));
                    removeFilesFromSnapshot(removedFiles);
                }
            }

            removedProjectParts = comparer.removedProjectParts();

        // A new project was opened/created, do a full indexing
        } else {
            d->m_dirty = true;
            filesToReindex.unite(newSourceFiles);
        }

        // Update Project/ProjectInfo and File/ProjectPart table
        if (it != d->m_projectData.end()) {
            if (it->indexer)
                it->indexer->cancel();
            it->projectInfo = newProjectInfo;
            it->fullyIndexed = false;
        }
        projectData = it == d->m_projectData.end()
                ? &(d->m_projectData[project] = ProjectData{newProjectInfo, nullptr, false})
                : &(*it);
        recalculateProjectPartMappings();
    } // Locker scope

    // If requested, dump everything we got
    if (DumpProjectInfo)
        dumpModelManagerConfiguration(QLatin1String("updateProjectInfo"));

    // Remove files from snapshot that are not reachable any more
    if (filesRemoved)
        GC();

    // Announce removed project parts
    if (!removedProjectParts.isEmpty())
        emit projectPartsRemoved(removedProjectParts);

    // Announce added project parts
    emit projectPartsUpdated(project);

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
    d->setupWatcher(indexingFuture, project, projectData, this);

    return indexingFuture;
}

ProjectPart::ConstPtr CppModelManager::projectPartForId(const QString &projectPartId) const
{
    QReadLocker locker(&d->m_projectLock);
    return d->m_projectPartIdToProjectProjectPart.value(projectPartId);
}

QList<ProjectPart::ConstPtr> CppModelManager::projectPart(const Utils::FilePath &fileName) const
{
    QReadLocker locker(&d->m_projectLock);
    return d->m_fileToProjectParts.value(fileName);
}

QList<ProjectPart::ConstPtr> CppModelManager::projectPartFromDependencies(
        const Utils::FilePath &fileName) const
{
    QSet<ProjectPart::ConstPtr> parts;
    const Utils::FilePaths deps = snapshot().filesDependingOn(fileName);

    QReadLocker locker(&d->m_projectLock);
    for (const Utils::FilePath &dep : deps)
        parts.unite(Utils::toSet(d->m_fileToProjectParts.value(dep)));

    return parts.values();
}

ProjectPart::ConstPtr CppModelManager::fallbackProjectPart()
{
    QMutexLocker locker(&d->m_fallbackProjectPartMutex);
    return d->m_fallbackProjectPart;
}

bool CppModelManager::isCppEditor(Core::IEditor *editor)
{
    return editor->context().contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
}

bool CppModelManager::supportsOutline(const TextEditor::TextDocument *document)
{
    return instance()->d->m_activeModelManagerSupport->supportsOutline(document);
}

bool CppModelManager::supportsLocalUses(const TextEditor::TextDocument *document)
{
    return instance()->d->m_activeModelManagerSupport->supportsLocalUses(document);
}

bool CppModelManager::hasSpecialHoverHandler(const TextEditor::TextDocument *document)
{
    return instance()->d->m_activeModelManagerSupport->hasSpecialHoverHandler(document);
}

bool CppModelManager::isClangCodeModelActive() const
{
    return d->m_activeModelManagerSupport != d->m_builtinModelManagerSupport;
}

void CppModelManager::emitDocumentUpdated(Document::Ptr doc)
{
    if (replaceDocument(doc))
        emit documentUpdated(doc);
}

void CppModelManager::emitAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                               const QString &sourcePath,
                                                               const QByteArray &contents)
{
    emit abstractEditorSupportContentsUpdated(filePath, sourcePath, contents);
}

void CppModelManager::emitAbstractEditorSupportRemoved(const QString &filePath)
{
    emit abstractEditorSupportRemoved(filePath);
}

void CppModelManager::onProjectAdded(ProjectExplorer::Project *)
{
    QWriteLocker locker(&d->m_projectLock);
    d->m_dirty = true;
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

void CppModelManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    QStringList idsOfRemovedProjectParts;

    {
        QWriteLocker locker(&d->m_projectLock);
        d->m_dirty = true;
        const QStringList projectPartsIdsBefore = d->m_projectPartIdToProjectProjectPart.keys();

        d->m_projectData.remove(project);
        recalculateProjectPartMappings();

        const QStringList projectPartsIdsAfter = d->m_projectPartIdToProjectProjectPart.keys();
        idsOfRemovedProjectParts = removedProjectParts(projectPartsIdsBefore, projectPartsIdsAfter);
    }

    if (!idsOfRemovedProjectParts.isEmpty())
        emit projectPartsRemoved(idsOfRemovedProjectParts);

    delayedGC();
}

void CppModelManager::onActiveProjectChanged(ProjectExplorer::Project *project)
{
    if (!project)
        return; // Last project closed.

    {
        QReadLocker locker(&d->m_projectLock);
        if (!d->m_projectData.contains(project))
            return; // Not yet known to us.
    }

    updateCppEditorDocuments();
}

void CppModelManager::onSourceFilesRefreshed() const
{
    if (BuiltinIndexingSupport::isFindErrorsIndexingActive()) {
        QTimer::singleShot(1, QCoreApplication::instance(), &QCoreApplication::quit);
        qDebug("FindErrorsIndexing: Done, requesting Qt Creator to quit.");
    }
}

void CppModelManager::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (!editor || !editor->document())
        return;

    const QString filePath = editor->document()->filePath().toString();
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

QSet<QString> CppModelManager::dependingInternalTargets(const Utils::FilePath &file) const
{
    QSet<QString> result;
    const Snapshot snapshot = this->snapshot();
    QTC_ASSERT(snapshot.contains(file), return result);
    bool wasHeader;
    const QString correspondingFile
            = correspondingHeaderOrSource(file.toString(), &wasHeader, CacheUsage::ReadOnly);
    const Utils::FilePaths dependingFiles = snapshot.filesDependingOn(
                wasHeader ? file : Utils::FilePath::fromString(correspondingFile));
    for (const Utils::FilePath &fn : qAsConst(dependingFiles)) {
        for (const ProjectPart::ConstPtr &part : projectPart(fn))
            result.insert(part->buildSystemTarget);
    }
    return result;
}

QSet<QString> CppModelManager::internalTargets(const Utils::FilePath &filePath) const
{
    const QList<ProjectPart::ConstPtr> projectParts = projectPart(filePath);
    // if we have no project parts it's most likely a header with declarations only and CMake based
    if (projectParts.isEmpty())
        return dependingInternalTargets(filePath);
    QSet<QString> targets;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        targets.insert(part->buildSystemTarget);
        if (part->buildTargetType != ProjectExplorer::BuildTargetType::Executable)
            targets.unite(dependingInternalTargets(filePath));
    }
    return targets;
}

void CppModelManager::renameIncludes(const Utils::FilePath &oldFilePath,
                                     const Utils::FilePath &newFilePath)
{
    if (oldFilePath.isEmpty() || newFilePath.isEmpty())
        return;

    // We just want to handle renamings so return when the file was actually moved.
    if (oldFilePath.absolutePath() != newFilePath.absolutePath())
        return;

    const TextEditor::RefactoringChanges changes;

    foreach (Snapshot::IncludeLocation loc,
             snapshot().includeLocationsOfDocument(oldFilePath.toString())) {
        TextEditor::RefactoringFilePtr file = changes.file(
            Utils::FilePath::fromString(loc.first->fileName()));
        const QTextBlock &block = file->document()->findBlockByNumber(loc.second - 1);
        const int replaceStart = block.text().indexOf(oldFilePath.fileName());
        if (replaceStart > -1) {
            Utils::ChangeSet changeSet;
            changeSet.replace(block.position() + replaceStart,
                              block.position() + replaceStart + oldFilePath.fileName().length(),
                              newFilePath.fileName());
            file->setChangeSet(changeSet);
            file->apply();
        }
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

QSet<QString> CppModelManager::symbolsInFiles(const QSet<Utils::FilePath> &files) const
{
    QSet<QString> uniqueSymbols;
    const Snapshot cppSnapShot = snapshot();

    // Iterate over the files and get interesting symbols
    for (const Utils::FilePath &file : files) {
        // Add symbols from the C++ code model
        const CPlusPlus::Document::Ptr doc = cppSnapShot.document(file);
        if (!doc.isNull() && doc->control()) {
            const CPlusPlus::Control *ctrl = doc->control();
            CPlusPlus::Symbol **symPtr = ctrl->firstSymbol(); // Read-only
            while (symPtr != ctrl->lastSymbol()) {
                const CPlusPlus::Symbol *sym = *symPtr;

                const CPlusPlus::Identifier *symId = sym->identifier();
                // Add any class, function or namespace identifiers
                if ((sym->isClass() || sym->isFunction() || sym->isNamespace()) && symId
                    && symId->chars()) {
                    uniqueSymbols.insert(QString::fromUtf8(symId->chars()));
                }

                // Handle specific case : get "Foo" in "void Foo::function() {}"
                if (sym->isFunction() && !sym->asFunction()->isDeclaration()) {
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
    Core::ProgressManager::cancelTasks(Constants::TASK_INDEX);
    d->m_enableGC = false;
}

void CppModelManager::setupFallbackProjectPart()
{
    ToolChainInfo tcInfo;
    RawProjectPart rpp;
    rpp.setMacros(definedMacros());
    rpp.setHeaderPaths(headerPaths());
    rpp.setQtVersion(Utils::QtMajorVersion::Qt5);

    // Do not activate ObjectiveCExtensions since this will lead to the
    // "objective-c++" language option for a project-less *.cpp file.
    Utils::LanguageExtensions langExtensions = Utils::LanguageExtension::All;
    langExtensions &= ~Utils::LanguageExtensions(Utils::LanguageExtension::ObjectiveC);

    // TODO: Use different fallback toolchain for different kinds of files?
    const Kit * const defaultKit = KitManager::isLoaded() ? KitManager::defaultKit() : nullptr;
    const ToolChain * const defaultTc = defaultKit
            ? ToolChainKitAspect::cxxToolChain(defaultKit) : nullptr;
    if (defaultKit && defaultTc) {
        Utils::FilePath sysroot = SysRootKitAspect::sysRoot(defaultKit);
        if (sysroot.isEmpty())
            sysroot = Utils::FilePath::fromString(defaultTc->sysRoot());
        Utils::Environment env = defaultKit->buildEnvironment();
        tcInfo = ToolChainInfo(defaultTc, sysroot.toString(), env);
        const auto macroInspectionWrapper = [runner = tcInfo.macroInspectionRunner](
                const QStringList &flags) {
            ToolChain::MacroInspectionReport report = runner(flags);
            report.languageVersion = Utils::LanguageVersion::LatestCxx;
            return report;
        };
        tcInfo.macroInspectionRunner = macroInspectionWrapper;
    }

    const auto part = ProjectPart::create({}, rpp, {}, {}, {}, langExtensions, {}, tcInfo);
    {
        QMutexLocker locker(&d->m_fallbackProjectPartMutex);
        d->m_fallbackProjectPart = part;
    }
    emit fallbackProjectPartUpdated();
}

void CppModelManager::GC()
{
    if (!d->m_enableGC)
        return;

    // Collect files of opened editors and editor supports (e.g. ui code model)
    QStringList filesInEditorSupports;
    foreach (const CppEditorDocumentHandle *editorDocument, cppEditorDocuments())
        filesInEditorSupports << editorDocument->filePath();

    foreach (AbstractEditorSupport *abstractEditorSupport, abstractEditorSupports())
        filesInEditorSupports << abstractEditorSupport->fileName();

    Snapshot currentSnapshot = snapshot();
    QSet<Utils::FilePath> reachableFiles;
    // The configuration file is part of the project files, which is just fine.
    // If single files are open, without any project, then there is no need to
    // keep the configuration file around.
    QStringList todo = filesInEditorSupports + projectFiles();

    // Collect all files that are reachable from the project files
    while (!todo.isEmpty()) {
        const QString file = todo.last();
        todo.removeLast();

        const Utils::FilePath fileName = Utils::FilePath::fromString(file);
        if (reachableFiles.contains(fileName))
            continue;
        reachableFiles.insert(fileName);

        if (Document::Ptr doc = currentSnapshot.document(file))
            todo += doc->includedFiles();
    }

    // Find out the files in the current snapshot that are not reachable from the project files
    QStringList notReachableFiles;
    Snapshot newSnapshot;
    for (Snapshot::const_iterator it = currentSnapshot.begin(); it != currentSnapshot.end(); ++it) {
        const Utils::FilePath &fileName = it.key();

        if (reachableFiles.contains(fileName))
            newSnapshot.insert(it.value());
        else
            notReachableFiles.append(fileName.toString());
    }

    // Announce removing files and replace the snapshot
    emit aboutToRemoveFiles(notReachableFiles);
    replaceSnapshot(newSnapshot);
    emit gcFinished();
}

void CppModelManager::finishedRefreshingSourceFiles(const QSet<QString> &files)
{
    emit sourceFilesRefreshed(files);
}

void CppModelManager::activateClangCodeModel(
        ModelManagerSupportProvider *modelManagerSupportProvider)
{
    QTC_ASSERT(modelManagerSupportProvider, return);

    d->m_activeModelManagerSupport = modelManagerSupportProvider->createModelManagerSupport();
    d->m_refactoringEngines[RefactoringEngineType::ClangCodeModel] =
            &d->m_activeModelManagerSupport->refactoringEngineInterface();
}

CppCompletionAssistProvider *CppModelManager::completionAssistProvider() const
{
    return d->m_builtinModelManagerSupport->completionAssistProvider();
}

CppCompletionAssistProvider *CppModelManager::functionHintAssistProvider() const
{
    return d->m_builtinModelManagerSupport->functionHintAssistProvider();
}

TextEditor::BaseHoverHandler *CppModelManager::createHoverHandler() const
{
    return d->m_builtinModelManagerSupport->createHoverHandler();
}

void CppModelManager::followSymbol(const CursorInEditor &data,
                                   Utils::ProcessLinkCallback &&processLinkCallback,
                                   bool resolveTarget, bool inNextSplit)
{
    d->m_activeModelManagerSupport->followSymbol(data, std::move(processLinkCallback),
                                                 resolveTarget, inNextSplit);
}

void CppModelManager::switchDeclDef(const CursorInEditor &data,
                                    Utils::ProcessLinkCallback &&processLinkCallback)
{
    d->m_activeModelManagerSupport->switchDeclDef(data, std::move(processLinkCallback));
}

Core::ILocatorFilter *CppModelManager::createAuxiliaryCurrentDocumentFilter()
{
    const auto filter = new Internal::CppCurrentDocumentFilter(instance());
    filter->makeAuxiliary();
    return filter;
}

BaseEditorDocumentProcessor *CppModelManager::createEditorDocumentProcessor(
    TextEditor::TextDocument *baseTextDocument) const
{
    return d->m_activeModelManagerSupport->createEditorDocumentProcessor(baseTextDocument);
}

CppIndexingSupport *CppModelManager::indexingSupport()
{
    return d->m_internalIndexingSupport;
}

QStringList CppModelManager::projectFiles()
{
    QWriteLocker locker(&d->m_projectLock);
    ensureUpdated();

    return d->m_projectFiles;
}

ProjectExplorer::HeaderPaths CppModelManager::headerPaths()
{
    QWriteLocker locker(&d->m_projectLock);
    ensureUpdated();

    return d->m_headerPaths;
}

void CppModelManager::setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths)
{
    QWriteLocker locker(&d->m_projectLock);
    d->m_headerPaths = headerPaths;
}

ProjectExplorer::Macros CppModelManager::definedMacros()
{
    QWriteLocker locker(&d->m_projectLock);
    ensureUpdated();

    return d->m_definedMacros;
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

} // namespace CppEditor
