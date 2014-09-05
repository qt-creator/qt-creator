/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppmodelmanager.h"

#include "abstracteditorsupport.h"
#include "builtinindexingsupport.h"
#include "cppcodemodelinspectordumper.h"
#include "cppcodemodelsettings.h"
#include "cppfindreferences.h"
#include "cppindexingsupport.h"
#include "cppmodelmanagersupportinternal.h"
#include "cppsourceprocessor.h"
#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"
#include "editordocumenthandle.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QTextBlock>
#include <QTimer>

#if defined(QTCREATOR_WITH_DUMP_AST) && defined(Q_CC_GNU)
#define WITH_AST_DUMP
#include <iostream>
#include <sstream>
#endif

Q_DECLARE_METATYPE(QSet<QString>)

static const bool DumpProjectInfo = qgetenv("QTC_DUMP_PROJECT_INFO") == "1";

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CPlusPlus;

#ifdef QTCREATOR_WITH_DUMP_AST

#include <cxxabi.h>

class DumpAST: protected ASTVisitor
{
public:
    int depth;

    DumpAST(Control *control)
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
        code.replace(QRegExp("\\s+"), " ");

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

static const char pp_configuration[] =
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
    "#define __forceinline inline\n";

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
        that->emitDocumentUpdated(doc);
        doc->releaseSourceAndAST();
    });
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
    \class CppTools::CppModelManager
    \brief The CppModelManager keeps tracks of the source files the code model is aware of.

    The CppModelManager manages the source files in a Snapshot object.

    The snapshot is updated in case e.g.
        * New files are opened/edited (Editor integration)
        * A project manager pushes updated project information (Project integration)
        * Files are garbage collected
*/

QMutex CppModelManager::m_instanceMutex;
CppModelManager *CppModelManager::m_instance = 0;

CppModelManager *CppModelManager::instance()
{
    if (m_instance)
        return m_instance;

    QMutexLocker locker(&m_instanceMutex);
    if (!m_instance)
        m_instance = new CppModelManager;

    return m_instance;
}

CppModelManager::CppModelManager(QObject *parent)
    : CppModelManagerInterface(parent)
    , m_indexingSupporter(0)
    , m_enableGC(true)
{
    qRegisterMetaType<QSet<QString> >();
    connect(this, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SIGNAL(globalSnapshotChanged()));
    connect(this, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SIGNAL(globalSnapshotChanged()));
    connect(this, SIGNAL(sourceFilesRefreshed(QSet<QString>)),
            this, SLOT(onSourceFilesRefreshed()));

    m_findReferences = new CppFindReferences(this);
    m_indexerEnabled = qgetenv("QTC_NO_CODE_INDEXER") != "1";

    m_dirty = true;

    m_delayedGcTimer = new QTimer(this);
    m_delayedGcTimer->setObjectName(QLatin1String("CppModelManager::m_delayedGcTimer"));
    m_delayedGcTimer->setSingleShot(true);
    connect(m_delayedGcTimer, SIGNAL(timeout()), this, SLOT(GC()));

    QObject *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(onProjectAdded(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(aboutToLoadSession(QString)),
            this, SLOT(onAboutToLoadSession()));
    connect(sessionManager, SIGNAL(aboutToUnloadSession(QString)),
            this, SLOT(onAboutToUnloadSession()));

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(onCoreAboutToClose()));

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");

    m_modelManagerSupportFallback.reset(new ModelManagerSupportInternal);
    CppToolsPlugin::instance()->codeModelSettings()->setDefaultId(
                m_modelManagerSupportFallback->id());
    addModelManagerSupport(m_modelManagerSupportFallback.data());

    m_internalIndexingSupport = new BuiltinIndexingSupport;
}

CppModelManager::~CppModelManager()
{
    delete m_internalIndexingSupport;
}

Snapshot CppModelManager::snapshot() const
{
    QMutexLocker locker(&m_snapshotMutex);
    return m_snapshot;
}

Document::Ptr CppModelManager::document(const QString &fileName) const
{
    QMutexLocker locker(&m_snapshotMutex);
    return m_snapshot.document(fileName);
}

/// Replace the document in the snapshot.
///
/// \returns true if successful, false if the new document is out-dated.
bool CppModelManager::replaceDocument(Document::Ptr newDoc)
{
    QMutexLocker locker(&m_snapshotMutex);

    Document::Ptr previous = m_snapshot.document(newDoc->fileName());
    if (previous && (newDoc->revision() != 0 && newDoc->revision() < previous->revision()))
        // the new document is outdated
        return false;

    m_snapshot.insert(newDoc);
    return true;
}

void CppModelManager::ensureUpdated()
{
    QMutexLocker locker(&m_projectMutex);
    if (!m_dirty)
        return;

    m_projectFiles = internalProjectFiles();
    m_headerPaths = internalHeaderPaths();
    m_definedMacros = internalDefinedMacros();
    m_dirty = false;
}

QStringList CppModelManager::internalProjectFiles() const
{
    QStringList files;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projectToProjectsInfo);
    while (it.hasNext()) {
        it.next();
        const ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            foreach (const ProjectFile &file, part->files)
                files += file.path;
        }
    }
    files.removeDuplicates();
    return files;
}

ProjectPart::HeaderPaths CppModelManager::internalHeaderPaths() const
{
    ProjectPart::HeaderPaths headerPaths;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projectToProjectsInfo);
    while (it.hasNext()) {
        it.next();
        const ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            foreach (const ProjectPart::HeaderPath &path, part->headerPaths) {
                const ProjectPart::HeaderPath hp(CppSourceProcessor::cleanPath(path.path),
                                                 path.type);
                if (!headerPaths.contains(hp))
                    headerPaths += hp;
            }
        }
    }
    return headerPaths;
}

static void addUnique(const QList<QByteArray> &defs, QByteArray *macros, QSet<QByteArray> *alreadyIn)
{
    Q_ASSERT(macros);
    Q_ASSERT(alreadyIn);

    foreach (const QByteArray &def, defs) {
        if (def.trimmed().isEmpty())
            continue;
        if (!alreadyIn->contains(def)) {
            macros->append(def);
            macros->append('\n');
            alreadyIn->insert(def);
        }
    }
}

QByteArray CppModelManager::internalDefinedMacros() const
{
    QByteArray macros;
    QSet<QByteArray> alreadyIn;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projectToProjectsInfo);
    while (it.hasNext()) {
        it.next();
        const ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            addUnique(part->toolchainDefines.split('\n'), &macros, &alreadyIn);
            addUnique(part->projectDefines.split('\n'), &macros, &alreadyIn);
            if (!part->projectConfigFile.isEmpty())
                macros += ProjectPart::readProjectConfigFile(part);
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
    ensureUpdated();
    dumper.dumpMergedEntities(m_headerPaths, m_definedMacros);
}

void CppModelManager::addExtraEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_extraEditorSupports.insert(editorSupport);
}

void CppModelManager::removeExtraEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_extraEditorSupports.remove(editorSupport);
}

EditorDocumentHandle *CppModelManager::editorDocument(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return 0);

    QMutexLocker locker(&m_cppEditorsMutex);
    return m_cppEditors.value(filePath, 0);
}

void CppModelManager::registerEditorDocument(EditorDocumentHandle *editorDocument)
{
    QTC_ASSERT(editorDocument, return);
    const QString filePath = editorDocument->filePath();
    QTC_ASSERT(!filePath.isEmpty(), return);

    QMutexLocker locker(&m_cppEditorsMutex);
    QTC_ASSERT(m_cppEditors.value(filePath, 0) == 0, return);
    m_cppEditors.insert(filePath, editorDocument);
}

void CppModelManager::unregisterEditorDocument(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    static short closedCppDocuments = 0;
    int openCppDocuments = 0;

    {
        QMutexLocker locker(&m_cppEditorsMutex);
        QTC_ASSERT(m_cppEditors.value(filePath, 0), return);
        QTC_CHECK(m_cppEditors.remove(filePath) == 1);
        openCppDocuments = m_cppEditors.size();
    }

    ++closedCppDocuments;
    if (openCppDocuments == 0 || closedCppDocuments == 5) {
        closedCppDocuments = 0;
        delayedGC();
    }
}

QList<int> CppModelManager::references(CPlusPlus::Symbol *symbol, const LookupContext &context)
{
    return m_findReferences->references(symbol, context);
}

void CppModelManager::findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context)
{
    if (symbol->identifier())
        m_findReferences->findUsages(symbol, context);
}

void CppModelManager::renameUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context,
                                   const QString &replacement)
{
    if (symbol->identifier())
        m_findReferences->renameUsages(symbol, context, replacement);
}

void CppModelManager::findMacroUsages(const CPlusPlus::Macro &macro)
{
    m_findReferences->findMacroUses(macro);
}

void CppModelManager::renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement)
{
    m_findReferences->renameMacroUses(macro, replacement);
}

void CppModelManager::replaceSnapshot(const CPlusPlus::Snapshot &newSnapshot)
{
    QMutexLocker snapshotLocker(&m_snapshotMutex);
    m_snapshot = newSnapshot;
}

WorkingCopy CppModelManager::buildWorkingCopyList()
{
    WorkingCopy workingCopy;

    foreach (const EditorDocumentHandle *cppEditor, cppEditors())
        workingCopy.insert(cppEditor->filePath(), cppEditor->contents(), cppEditor->revision());

    QSetIterator<AbstractEditorSupport *> it(m_extraEditorSupports);
    while (it.hasNext()) {
        AbstractEditorSupport *es = it.next();
        workingCopy.insert(es->fileName(), es->contents(), es->revision());
    }

    // Add the project configuration file
    QByteArray conf = codeModelConfiguration();
    conf += definedMacros();
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

QFuture<void> CppModelManager::updateSourceFiles(const QSet<QString> &sourceFiles,
                                                 ProgressNotificationMode mode)
{
    if (sourceFiles.isEmpty() || !m_indexerEnabled)
        return QFuture<void>();

    if (m_indexingSupporter)
        m_indexingSupporter->refreshSourceFiles(sourceFiles, mode);
    return m_internalIndexingSupport->refreshSourceFiles(sourceFiles, mode);
}

QList<ProjectInfo> CppModelManager::projectInfos() const
{
    QMutexLocker locker(&m_projectMutex);
    return m_projectToProjectsInfo.values();
}

ProjectInfo CppModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&m_projectMutex);
    return m_projectToProjectsInfo.value(project, ProjectInfo(project));
}

/// \brief Remove all files and their includes (recursively) of given ProjectInfo from the snapshot.
void CppModelManager::removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo)
{
    if (!projectInfo.isValid())
        return;

    QMutexLocker snapshotLocker(&m_snapshotMutex);
    foreach (const ProjectPart::Ptr &projectPart, projectInfo.projectParts()) {
        foreach (const ProjectFile &cxxFile, projectPart->files) {
            foreach (const QString &fileName, m_snapshot.allIncludesForDocument(cxxFile.path))
                m_snapshot.remove(fileName);
            m_snapshot.remove(cxxFile.path);
        }
    }
}

QList<EditorDocumentHandle *> CppModelManager::cppEditors() const
{
    QMutexLocker locker(&m_cppEditorsMutex);
    return m_cppEditors.values();
}

/// \brief Remove all given files from the snapshot.
void CppModelManager::removeFilesFromSnapshot(const QSet<QString> &filesToRemove)
{
    QMutexLocker snapshotLocker(&m_snapshotMutex);
    QSetIterator<QString> i(filesToRemove);
    while (i.hasNext())
        m_snapshot.remove(i.next());
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

    bool definesChanged() const
    {
        return m_new.defines() != m_old.defines();
    }

    bool configurationChanged() const
    {
        return definesChanged()
            || m_new.headerPaths() != m_old.headerPaths();
    }

    bool nothingChanged() const
    {
        return !configurationChanged() && m_new.sourceFiles() == m_old.sourceFiles();
    }

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

    /// Returns a list of common files that have a changed timestamp.
    QSet<QString> timeStampModifiedFiles(const Snapshot &snapshot) const
    {
        QSet<QString> commonSourceFiles = m_newSourceFiles;
        commonSourceFiles.intersect(m_oldSourceFiles);

        QList<Document::Ptr> documentsToCheck;
        QSetIterator<QString> i(commonSourceFiles);
        while (i.hasNext()) {
            const QString file = i.next();
            if (Document::Ptr document = snapshot.document(file))
                documentsToCheck << document;
        }

        return CppModelManager::timeStampModifiedFiles(documentsToCheck);
    }

private:
    const ProjectInfo &m_old;
    const QSet<QString> m_oldSourceFiles;

    const ProjectInfo &m_new;
    const QSet<QString> m_newSourceFiles;
};

/// Make sure that m_projectMutex is locked when calling this.
void CppModelManager::recalculateFileToProjectParts()
{
    m_projectFileToProjectPart.clear();
    m_fileToProjectParts.clear();
    foreach (const ProjectInfo &projectInfo, m_projectToProjectsInfo) {
        foreach (const ProjectPart::Ptr &projectPart, projectInfo.projectParts()) {
            m_projectFileToProjectPart[projectPart->projectFile] = projectPart;
            foreach (const ProjectFile &cxxFile, projectPart->files)
                m_fileToProjectParts[cxxFile.path].append(projectPart);

        }
    }
}

QFuture<void> CppModelManager::updateProjectInfo(const ProjectInfo &newProjectInfo)
{
    if (!newProjectInfo.isValid())
        return QFuture<void>();

    QSet<QString> filesToReindex;
    bool filesRemoved = false;

    { // Only hold the mutex for a limited scope, so the dumping afterwards does not deadlock.
        QMutexLocker projectLocker(&m_projectMutex);

        ProjectExplorer::Project *project = newProjectInfo.project().data();
        const QSet<QString> newSourceFiles = newProjectInfo.sourceFiles();

        // Check if we can avoid a full reindexing
        ProjectInfo oldProjectInfo = m_projectToProjectsInfo.value(project);
        if (oldProjectInfo.isValid()) {
            ProjectInfoComparer comparer(oldProjectInfo, newProjectInfo);
            if (comparer.nothingChanged())
                return QFuture<void>();

            // If the project configuration changed, do a full reindexing
            if (comparer.configurationChanged()) {
                removeProjectInfoFilesAndIncludesFromSnapshot(oldProjectInfo);
                filesToReindex.unite(newSourceFiles);

                // The "configuration file" includes all defines and therefore should be updated
                if (comparer.definesChanged()) {
                    QMutexLocker snapshotLocker(&m_snapshotMutex);
                    m_snapshot.remove(configurationFileName());
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
                emit aboutToRemoveFiles(removedFiles.toList());
                removeFilesFromSnapshot(removedFiles);
            }

        // A new project was opened/created, do a full indexing
        } else {
            filesToReindex.unite(newSourceFiles);
        }

        // Update Project/ProjectInfo and File/ProjectPart table
        m_dirty = true;
        m_projectToProjectsInfo.insert(project, newProjectInfo);
        recalculateFileToProjectParts();

    } // Mutex scope

    // If requested, dump everything we got
    if (DumpProjectInfo)
        dumpModelManagerConfiguration(QLatin1String("updateProjectInfo"));

    // Remove files from snapshot that are not reachable any more
    if (filesRemoved)
        GC();

    emit projectPartsUpdated(newProjectInfo.project().data());

    // Trigger reindexing
    return updateSourceFiles(filesToReindex, ForcedProgressNotification);
}

ProjectPart::Ptr CppModelManager::projectPartForProjectFile(const QString &projectFile) const
{
    return m_projectFileToProjectPart.value(projectFile);
}

QList<ProjectPart::Ptr> CppModelManager::projectPart(const QString &fileName) const
{
    QMutexLocker locker(&m_projectMutex);
    return m_fileToProjectParts.value(fileName);
}

QList<ProjectPart::Ptr> CppModelManager::projectPartFromDependencies(const QString &fileName) const
{
    QSet<ProjectPart::Ptr> parts;
    DependencyTable table;
    table.build(snapshot());
    const QStringList deps = table.filesDependingOn(fileName);
    foreach (const QString &dep, deps)
        parts.unite(QSet<ProjectPart::Ptr>::fromList(m_fileToProjectParts.value(dep)));

    return parts.values();
}

ProjectPart::Ptr CppModelManager::fallbackProjectPart() const
{
    ProjectPart::Ptr part(new ProjectPart);

    part->projectDefines = m_definedMacros;
    part->headerPaths = m_headerPaths;
    part->cVersion = ProjectPart::C11;
    part->cxxVersion = ProjectPart::CXX11;
    part->cxxExtensions = ProjectPart::AllExtensions;
    part->qtVersion = ProjectPart::Qt5;

    return part;
}

bool CppModelManager::isCppEditor(Core::IEditor *editor) const
{
    return editor->context().contains(ProjectExplorer::Constants::LANG_CXX);
}

void CppModelManager::emitDocumentUpdated(Document::Ptr doc)
{
    if (replaceDocument(doc))
        emit documentUpdated(doc);
}

void CppModelManager::onProjectAdded(ProjectExplorer::Project *)
{
    QMutexLocker locker(&m_projectMutex);
    m_dirty = true;
}

void CppModelManager::delayedGC()
{
    if (m_enableGC)
        m_delayedGcTimer->start(500);
}

void CppModelManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    do {
        QMutexLocker locker(&m_projectMutex);
        m_dirty = true;
        m_projectToProjectsInfo.remove(project);
        recalculateFileToProjectParts();
    } while (0);

    delayedGC();
}

void CppModelManager::onSourceFilesRefreshed() const
{
    if (BuiltinIndexingSupport::isFindErrorsIndexingActive()) {
        QTimer::singleShot(1, QCoreApplication::instance(), SLOT(quit()));
        qDebug("FindErrorsIndexing: Done, requesting Qt Creator to quit.");
    }
}

void CppModelManager::onAboutToLoadSession()
{
    if (m_delayedGcTimer->isActive())
        m_delayedGcTimer->stop();
    GC();
}

void CppModelManager::onAboutToUnloadSession()
{
    Core::ProgressManager::cancelTasks(CppTools::Constants::TASK_INDEX);
    do {
        QMutexLocker locker(&m_projectMutex);
        m_projectToProjectsInfo.clear();
        recalculateFileToProjectParts();
        m_dirty = true;
    } while (0);
}

void CppModelManager::onCoreAboutToClose()
{
    m_enableGC = false;
}

void CppModelManager::GC()
{
    if (!m_enableGC)
        return;

    // Collect files of CppEditorSupport and AbstractEditorSupport.
    QStringList filesInEditorSupports;
    foreach (const EditorDocumentHandle *cppEditor, cppEditors())
        filesInEditorSupports << cppEditor->filePath();

    QSetIterator<AbstractEditorSupport *> jt(m_extraEditorSupports);
    while (jt.hasNext()) {
        AbstractEditorSupport *abstractEditorSupport = jt.next();
        filesInEditorSupports << abstractEditorSupport->fileName();
    }

    Snapshot currentSnapshot = snapshot();
    QSet<QString> reachableFiles;
    // The configuration file is part of the project files, which is just fine.
    // If single files are open, without any project, then there is no need to
    // keep the configuration file around.
    QStringList todo = filesInEditorSupports + projectFiles();

    // Collect all files that are reachable from the project files
    while (!todo.isEmpty()) {
        const QString file = todo.last();
        todo.removeLast();

        if (reachableFiles.contains(file))
            continue;
        reachableFiles.insert(file);

        if (Document::Ptr doc = currentSnapshot.document(file))
            todo += doc->includedFiles();
    }

    // Find out the files in the current snapshot that are not reachable from the project files
    QStringList notReachableFiles;
    Snapshot newSnapshot;
    for (Snapshot::const_iterator it = currentSnapshot.begin(); it != currentSnapshot.end(); ++it) {
        const QString fileName = it.key();

        if (reachableFiles.contains(fileName))
            newSnapshot.insert(it.value());
        else
            notReachableFiles.append(fileName);
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

void CppModelManager::addModelManagerSupport(ModelManagerSupport *modelManagerSupport)
{
    Q_ASSERT(modelManagerSupport);
    m_idTocodeModelSupporter[modelManagerSupport->id()] = modelManagerSupport;
    QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();
    cms->setModelManagerSupports(m_idTocodeModelSupporter.values());
}

ModelManagerSupport *CppModelManager::modelManagerSupportForMimeType(const QString &mimeType) const
{
    QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();
    const QString &id = cms->modelManagerSupportId(mimeType);
    return m_idTocodeModelSupporter.value(id, m_modelManagerSupportFallback.data());
}

CppCompletionAssistProvider *CppModelManager::completionAssistProvider(const QString &mimeType) const
{
    if (mimeType.isEmpty())
        return 0;

    ModelManagerSupport *cms = modelManagerSupportForMimeType(mimeType);
    QTC_ASSERT(cms, return 0);
    return cms->completionAssistProvider();
}

BaseEditorDocumentProcessor *CppModelManager::editorDocumentProcessor(
        TextEditor::BaseTextDocument *baseTextDocument) const
{
    QTC_ASSERT(baseTextDocument, return 0);
    ModelManagerSupport *cms = modelManagerSupportForMimeType(baseTextDocument->mimeType());
    QTC_ASSERT(cms, return 0);
    return cms->editorDocumentProcessor(baseTextDocument);
}

void CppModelManager::setIndexingSupport(CppIndexingSupport *indexingSupport)
{
    if (indexingSupport)
        m_indexingSupporter = indexingSupport;
}

CppIndexingSupport *CppModelManager::indexingSupport()
{
    return m_indexingSupporter ? m_indexingSupporter : m_internalIndexingSupport;
}

void CppModelManager::enableGarbageCollector(bool enable)
{
    m_delayedGcTimer->stop();
    m_enableGC = enable;
}
