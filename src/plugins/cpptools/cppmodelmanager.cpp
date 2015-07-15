/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppmodelmanager.h"

#include "abstracteditorsupport.h"
#include "baseeditordocumentprocessor.h"
#include "builtinindexingsupport.h"
#include "cppcodemodelinspectordumper.h"
#include "cppcodemodelsettings.h"
#include "cppfindreferences.h"
#include "cppindexingsupport.h"
#include "cppmodelmanagersupportinternal.h"
#include "cpprefactoringchanges.h"
#include "cppsourceprocessor.h"
#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"
#include "cpptoolsreuse.h"
#include "editordocumenthandle.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/textdocument.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
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

namespace CppTools {
namespace Internal {

static QMutex m_instanceMutex;
static CppModelManager *m_instance;

class CppModelManagerPrivate
{
public:
    // Snapshot
    mutable QMutex m_snapshotMutex;
    Snapshot m_snapshot;

    // Project integration
    mutable QMutex m_projectMutex;
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projectToProjectsInfo;
    QMap<Utils::FileName, QList<ProjectPart::Ptr> > m_fileToProjectParts;
    QMap<QString, ProjectPart::Ptr> m_projectPartIdToProjectProjectPart;
    // The members below are cached/(re)calculated from the projects and/or their project parts
    bool m_dirty;
    QStringList m_projectFiles;
    ProjectPart::HeaderPaths m_headerPaths;
    QByteArray m_definedMacros;

    // Editor integration
    mutable QMutex m_cppEditorDocumentsMutex;
    QMap<QString, CppEditorDocumentHandle *> m_cppEditorDocuments;
    QSet<AbstractEditorSupport *> m_extraEditorSupports;

    // Completion & highlighting
    ModelManagerSupportProviderInternal m_modelManagerSupportInternalProvider;
    ModelManagerSupport::Ptr m_modelManagerSupportInternal;
    QHash<QString, ModelManagerSupportProvider *> m_availableModelManagerSupports;
    QHash<QString, ModelManagerSupport::Ptr> m_activeModelManagerSupports;

    // Indexing
    CppIndexingSupport *m_indexingSupporter;
    CppIndexingSupport *m_internalIndexingSupport;
    bool m_indexerEnabled;

    CppFindReferences *m_findReferences;

    bool m_enableGC;
    QTimer m_delayedGcTimer;
};

} // namespace Internal

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
    "#define __pragma(x)\n";

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

QString CppModelManager::configurationFileName()
{
    return Preprocessor::configurationFileName;
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
    : CppModelManagerBase(parent), d(new CppModelManagerPrivate)
{
    d->m_indexingSupporter = 0;
    d->m_enableGC = true;

    qRegisterMetaType<QSet<QString> >();
    connect(this, SIGNAL(sourceFilesRefreshed(QSet<QString>)),
            this, SLOT(onSourceFilesRefreshed()));

    d->m_findReferences = new CppFindReferences(this);
    d->m_indexerEnabled = qgetenv("QTC_NO_CODE_INDEXER") != "1";

    d->m_dirty = true;

    d->m_delayedGcTimer.setObjectName(QLatin1String("CppModelManager::m_delayedGcTimer"));
    d->m_delayedGcTimer.setSingleShot(true);
    connect(&d->m_delayedGcTimer, SIGNAL(timeout()), this, SLOT(GC()));

    QObject *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(onProjectAdded(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(aboutToLoadSession(QString)),
            this, SLOT(onAboutToLoadSession()));
    connect(sessionManager, SIGNAL(aboutToUnloadSession(QString)),
            this, SLOT(onAboutToUnloadSession()));

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &CppModelManager::onCurrentEditorChanged);

    connect(Core::DocumentManager::instance(), &Core::DocumentManager::allDocumentsRenamed,
            this, &CppModelManager::renameIncludes);

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(onCoreAboutToClose()));

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");
    qRegisterMetaType<QList<Document::DiagnosticMessage>>(
                "QList<CPlusPlus::Document::DiagnosticMessage>");

    QSharedPointer<CppCodeModelSettings> codeModelSettings
            = CppToolsPlugin::instance()->codeModelSettings();
    codeModelSettings->setDefaultId(d->m_modelManagerSupportInternalProvider.id());
    connect(codeModelSettings.data(), &CppCodeModelSettings::changed,
            this, &CppModelManager::onCodeModelSettingsChanged);

    d->m_modelManagerSupportInternal
            = d->m_modelManagerSupportInternalProvider.createModelManagerSupport();
    d->m_activeModelManagerSupports.insert(d->m_modelManagerSupportInternalProvider.id(),
                                           d->m_modelManagerSupportInternal);
    addModelManagerSupportProvider(&d->m_modelManagerSupportInternalProvider);

    d->m_internalIndexingSupport = new BuiltinIndexingSupport;
}

CppModelManager::~CppModelManager()
{
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

void CppModelManager::ensureUpdated()
{
    QMutexLocker locker(&d->m_projectMutex);
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
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(d->m_projectToProjectsInfo);
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
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(d->m_projectToProjectsInfo);
    while (it.hasNext()) {
        it.next();
        const ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            foreach (const ProjectPart::HeaderPath &path, part->headerPaths) {
                const ProjectPart::HeaderPath hp(QDir::cleanPath(path.path), path.type);
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
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(d->m_projectToProjectsInfo);
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
    dumper.dumpMergedEntities(d->m_headerPaths, d->m_definedMacros);
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
        return 0;

    QMutexLocker locker(&d->m_cppEditorDocumentsMutex);
    return d->m_cppEditorDocuments.value(filePath, 0);
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

void CppModelManager::findMacroUsages(const Macro &macro)
{
    d->m_findReferences->findMacroUses(macro);
}

void CppModelManager::renameMacroUsages(const Macro &macro, const QString &replacement)
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

    QSetIterator<AbstractEditorSupport *> it(d->m_extraEditorSupports);
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

static QSet<QString> tooBigFilesRemoved(const QSet<QString> &files, int fileSizeLimit)
{
    if (fileSizeLimit == 0)
        return files;

    QSet<QString> result;
    QFileInfo fileInfo;

    QSetIterator<QString> i(files);
    while (i.hasNext()) {
        const QString filePath = i.next();
        fileInfo.setFile(filePath);
        if (skipFileDueToSizeLimit(fileInfo), fileSizeLimit)
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

    const auto filteredFiles = tooBigFilesRemoved(sourceFiles, fileSizeLimit());

    if (d->m_indexingSupporter)
        d->m_indexingSupporter->refreshSourceFiles(filteredFiles, mode);
    return d->m_internalIndexingSupport->refreshSourceFiles(filteredFiles, mode);
}

QList<ProjectInfo> CppModelManager::projectInfos() const
{
    QMutexLocker locker(&d->m_projectMutex);
    return d->m_projectToProjectsInfo.values();
}

ProjectInfo CppModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&d->m_projectMutex);
    return d->m_projectToProjectsInfo.value(project, ProjectInfo());
}

/// \brief Remove all files and their includes (recursively) of given ProjectInfo from the snapshot.
void CppModelManager::removeProjectInfoFilesAndIncludesFromSnapshot(const ProjectInfo &projectInfo)
{
    if (!projectInfo.isValid())
        return;

    QMutexLocker snapshotLocker(&d->m_snapshotMutex);
    foreach (const ProjectPart::Ptr &projectPart, projectInfo.projectParts()) {
        foreach (const ProjectFile &cxxFile, projectPart->files) {
            foreach (const QString &fileName, d->m_snapshot.allIncludesForDocument(cxxFile.path))
                d->m_snapshot.remove(fileName);
            d->m_snapshot.remove(cxxFile.path);
        }
    }
}

void CppModelManager::handleAddedModelManagerSupports(const QSet<QString> &supportIds)
{
    foreach (const QString &id, supportIds) {
        ModelManagerSupportProvider * const provider = d->m_availableModelManagerSupports.value(id);
        if (provider) {
            QTC_CHECK(!d->m_activeModelManagerSupports.contains(id));
            d->m_activeModelManagerSupports.insert(id, provider->createModelManagerSupport());
        }
    }
}

QList<ModelManagerSupport::Ptr> CppModelManager::handleRemovedModelManagerSupports(
        const QSet<QString> &supportIds)
{
    QList<ModelManagerSupport::Ptr> removed;

    foreach (const QString &id, supportIds) {
        const ModelManagerSupport::Ptr support = d->m_activeModelManagerSupports.value(id);
        d->m_activeModelManagerSupports.remove(id);
        removed << support;
    }

    return removed;
}

void CppModelManager::closeCppEditorDocuments()
{
    QList<Core::IDocument *> cppDocumentsToClose;
    foreach (CppEditorDocumentHandle *cppDocument, cppEditorDocuments())
        cppDocumentsToClose << cppDocument->processor()->baseTextDocument();
    QTC_CHECK(Core::EditorManager::closeDocuments(cppDocumentsToClose));
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
    QSetIterator<QString> i(filesToRemove);
    while (i.hasNext())
        d->m_snapshot.remove(i.next());
}

static QSet<QString> projectPartIds(const QSet<ProjectPart::Ptr> &projectParts)
{
    return Utils::transform(projectParts, [](const ProjectPart::Ptr &projectPart) {
        return projectPart->id();
    });
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
        QSet<QString> removed = projectPartIds(m_old.projectParts().toSet());
        removed.subtract(projectPartIds(m_new.projectParts().toSet()));
        return removed.toList();
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
void CppModelManager::recalculateProjectPartMappings()
{
    d->m_projectPartIdToProjectProjectPart.clear();
    d->m_fileToProjectParts.clear();
    foreach (const ProjectInfo &projectInfo, d->m_projectToProjectsInfo) {
        foreach (const ProjectPart::Ptr &projectPart, projectInfo.projectParts()) {
            d->m_projectPartIdToProjectProjectPart[projectPart->id()] = projectPart;
            foreach (const ProjectFile &cxxFile, projectPart->files)
                d->m_fileToProjectParts[Utils::FileName::fromString(cxxFile.path)].append(
                            projectPart);

        }
    }
}

void CppModelManager::updateCppEditorDocuments() const
{
    // Refresh visible documents
    QSet<Core::IDocument *> visibleCppEditorDocuments;
    foreach (Core::IEditor *editor, Core::EditorManager::visibleEditors()) {
        if (Core::IDocument *document = editor->document()) {
            const QString filePath = document->filePath().toString();
            if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
                visibleCppEditorDocuments.insert(document);
                theCppEditorDocument->processor()->run();
            }
        }
    }

    // Mark invisible documents dirty
    QSet<Core::IDocument *> invisibleCppEditorDocuments
        = Core::DocumentModel::openedDocuments().toSet();
    invisibleCppEditorDocuments.subtract(visibleCppEditorDocuments);
    foreach (Core::IDocument *document, invisibleCppEditorDocuments) {
        const QString filePath = document->filePath().toString();
        if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath))
            theCppEditorDocument->setNeedsRefresh(true);
    }
}

QFuture<void> CppModelManager::updateProjectInfo(const ProjectInfo &newProjectInfo)
{
    if (!newProjectInfo.isValid())
        return QFuture<void>();

    QSet<QString> filesToReindex;
    bool filesRemoved = false;

    { // Only hold the mutex for a limited scope, so the dumping afterwards does not deadlock.
        QMutexLocker projectLocker(&d->m_projectMutex);

        ProjectExplorer::Project *project = newProjectInfo.project().data();
        const QSet<QString> newSourceFiles = newProjectInfo.sourceFiles();

        // Check if we can avoid a full reindexing
        ProjectInfo oldProjectInfo = d->m_projectToProjectsInfo.value(project);
        if (oldProjectInfo.isValid()) {
            ProjectInfoComparer comparer(oldProjectInfo, newProjectInfo);

            if (comparer.configurationOrFilesChanged()) {
                d->m_dirty = true;

                // If the project configuration changed, do a full reindexing
                if (comparer.configurationChanged()) {
                    removeProjectInfoFilesAndIncludesFromSnapshot(oldProjectInfo);
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
                    emit aboutToRemoveFiles(removedFiles.toList());
                    removeFilesFromSnapshot(removedFiles);
                }
            }

            // Announce removed project parts
            emit projectPartsRemoved(comparer.removedProjectParts());

        // A new project was opened/created, do a full indexing
        } else {
            d->m_dirty = true;
            filesToReindex.unite(newSourceFiles);
        }

        // Update Project/ProjectInfo and File/ProjectPart table
        d->m_projectToProjectsInfo.insert(project, newProjectInfo);
        recalculateProjectPartMappings();

    } // Mutex scope

    // If requested, dump everything we got
    if (DumpProjectInfo)
        dumpModelManagerConfiguration(QLatin1String("updateProjectInfo"));

    // Remove files from snapshot that are not reachable any more
    if (filesRemoved)
        GC();

    // Announce added project parts
    emit projectPartsUpdated(newProjectInfo.project().data());

    // Ideally, we would update all the editor documents that depend on the 'filesToReindex'.
    // However, on e.g. a session restore first the editor documents are created and then the
    // project updates come in. That is, there are no reasonable dependency tables based on
    // resolved includes that we could rely on.
    updateCppEditorDocuments();

    // Trigger reindexing
    return updateSourceFiles(filesToReindex, ForcedProgressNotification);
}

ProjectPart::Ptr CppModelManager::projectPartForId(const QString &projectPartId) const
{
    return d->m_projectPartIdToProjectProjectPart.value(projectPartId);
}

QList<ProjectPart::Ptr> CppModelManager::projectPart(const Utils::FileName &fileName) const
{
    QMutexLocker locker(&d->m_projectMutex);
    return d->m_fileToProjectParts.value(fileName);
}

QList<ProjectPart::Ptr> CppModelManager::projectPartFromDependencies(
        const Utils::FileName &fileName) const
{
    QSet<ProjectPart::Ptr> parts;
    const Utils::FileNameList deps = snapshot().filesDependingOn(fileName);

    QMutexLocker locker(&d->m_projectMutex);
    foreach (const Utils::FileName &dep, deps) {
        parts.unite(QSet<ProjectPart::Ptr>::fromList(d->m_fileToProjectParts.value(dep)));
    }

    return parts.values();
}

ProjectPart::Ptr CppModelManager::fallbackProjectPart() const
{
    ProjectPart::Ptr part(new ProjectPart);

    part->projectDefines = d->m_definedMacros;
    part->headerPaths = d->m_headerPaths;
    part->languageVersion = ProjectPart::CXX14;
    part->languageExtensions = ProjectPart::AllExtensions;
    part->qtVersion = ProjectPart::Qt5;
    part->updateLanguageFeatures();

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

void CppModelManager::emitAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                               const QByteArray &contents)
{
    emit abstractEditorSupportContentsUpdated(filePath, contents);
}

void CppModelManager::emitAbstractEditorSupportRemoved(const QString &filePath)
{
    emit abstractEditorSupportRemoved(filePath);
}

void CppModelManager::onProjectAdded(ProjectExplorer::Project *)
{
    QMutexLocker locker(&d->m_projectMutex);
    d->m_dirty = true;
}

void CppModelManager::delayedGC()
{
    if (d->m_enableGC)
        d->m_delayedGcTimer.start(500);
}

static QStringList idsOfAllProjectParts(const ProjectInfo &projectInfo)
{
    QStringList projectPaths;
    foreach (const ProjectPart::Ptr &part, projectInfo.projectParts())
        projectPaths << part->id();
    return projectPaths;
}

void CppModelManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    QStringList projectPartIds;

    {
        QMutexLocker locker(&d->m_projectMutex);
        d->m_dirty = true;

        // Save paths
        const ProjectInfo projectInfo = d->m_projectToProjectsInfo.value(project, ProjectInfo());
        projectPartIds = idsOfAllProjectParts(projectInfo);

        d->m_projectToProjectsInfo.remove(project);
        recalculateProjectPartMappings();
    }

    if (!projectPartIds.isEmpty())
        emit projectPartsRemoved(projectPartIds);

    delayedGC();
}

void CppModelManager::onSourceFilesRefreshed() const
{
    if (BuiltinIndexingSupport::isFindErrorsIndexingActive()) {
        QTimer::singleShot(1, QCoreApplication::instance(), SLOT(quit()));
        qDebug("FindErrorsIndexing: Done, requesting Qt Creator to quit.");
    }
}

void CppModelManager::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (!editor || !editor->document())
        return;

    const QString filePath = editor->document()->filePath().toString();
    if (CppEditorDocumentHandle *theCppEditorDocument = cppEditorDocument(filePath)) {
        if (theCppEditorDocument->needsRefresh()) {
            theCppEditorDocument->setNeedsRefresh(false);
            theCppEditorDocument->processor()->run();
        }
    }
}

static const QSet<QString> activeModelManagerSupportsFromSettings()
{
    QSet<QString> result;
    QSharedPointer<CppCodeModelSettings> codeModelSettings
            = CppToolsPlugin::instance()->codeModelSettings();

    const QStringList mimeTypes = codeModelSettings->supportedMimeTypes();
    foreach (const QString &mimeType, mimeTypes) {
        const QString id = codeModelSettings->modelManagerSupportIdForMimeType(mimeType);
        if (!id.isEmpty())
            result << id;
    }

    return result;
}

void CppModelManager::onCodeModelSettingsChanged()
{
    const QSet<QString> currentCodeModelSupporters = d->m_activeModelManagerSupports.keys().toSet();
    const QSet<QString> newCodeModelSupporters = activeModelManagerSupportsFromSettings();

    QSet<QString> added = newCodeModelSupporters;
    added.subtract(currentCodeModelSupporters);
    added.remove(d->m_modelManagerSupportInternalProvider.id());
    handleAddedModelManagerSupports(added);

    QSet<QString> removed = currentCodeModelSupporters;
    removed.subtract(newCodeModelSupporters);
    removed.remove(d->m_modelManagerSupportInternalProvider.id());
    const QList<ModelManagerSupport::Ptr> supportsToDelete
            = handleRemovedModelManagerSupports(removed);
    QTC_CHECK(removed.size() == supportsToDelete.size());

    if (!added.isEmpty() || !removed.isEmpty())
        closeCppEditorDocuments();

    // supportsToDelete goes out of scope and deletes the supports
}

void CppModelManager::onAboutToLoadSession()
{
    if (d->m_delayedGcTimer.isActive())
        d->m_delayedGcTimer.stop();
    GC();
}

void CppModelManager::onAboutToUnloadSession()
{
    Core::ProgressManager::cancelTasks(CppTools::Constants::TASK_INDEX);
    do {
        QMutexLocker locker(&d->m_projectMutex);
        d->m_projectToProjectsInfo.clear();
        recalculateProjectPartMappings();
        d->m_dirty = true;
    } while (0);
}

void CppModelManager::renameIncludes(const QString &oldFileName, const QString &newFileName)
{
    if (oldFileName.isEmpty() || newFileName.isEmpty())
        return;

    const QFileInfo oldFileInfo(oldFileName);
    const QFileInfo newFileInfo(newFileName);

    // We just want to handle renamings so return when the file was actually moved.
    if (oldFileInfo.absoluteDir() != newFileInfo.absoluteDir())
        return;

    const TextEditor::RefactoringChanges changes;

    foreach (Snapshot::IncludeLocation loc, snapshot().includeLocationsOfDocument(oldFileName)) {
        TextEditor::RefactoringFilePtr file = changes.file(loc.first->fileName());
        const QTextBlock &block = file->document()->findBlockByLineNumber(loc.second - 1);
        const int replaceStart = block.text().indexOf(oldFileInfo.fileName());
        if (replaceStart > -1) {
            Utils::ChangeSet changeSet;
            changeSet.replace(block.position() + replaceStart,
                              block.position() + replaceStart + oldFileInfo.fileName().length(),
                              newFileInfo.fileName());
            file->setChangeSet(changeSet);
            file->apply();
        }
    }
}

void CppModelManager::onCoreAboutToClose()
{
    d->m_enableGC = false;
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
    QSet<Utils::FileName> reachableFiles;
    // The configuration file is part of the project files, which is just fine.
    // If single files are open, without any project, then there is no need to
    // keep the configuration file around.
    QStringList todo = filesInEditorSupports + projectFiles();

    // Collect all files that are reachable from the project files
    while (!todo.isEmpty()) {
        const QString file = todo.last();
        todo.removeLast();

        const Utils::FileName fileName = Utils::FileName::fromString(file);
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
        const Utils::FileName &fileName = it.key();

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

void CppModelManager::addModelManagerSupportProvider(
        ModelManagerSupportProvider *modelManagerSupportProvider)
{
    QTC_ASSERT(modelManagerSupportProvider, return);
    d->m_availableModelManagerSupports[modelManagerSupportProvider->id()]
            = modelManagerSupportProvider;
    QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();
    cms->setModelManagerSupportProviders(d->m_availableModelManagerSupports.values());

    onCodeModelSettingsChanged();
}

ModelManagerSupport::Ptr CppModelManager::modelManagerSupportForMimeType(
        const QString &mimeType) const
{
    QSharedPointer<CppCodeModelSettings> cms = CppToolsPlugin::instance()->codeModelSettings();
    const QString &id = cms->modelManagerSupportIdForMimeType(mimeType);
    return d->m_activeModelManagerSupports.value(id, d->m_modelManagerSupportInternal);
}

CppCompletionAssistProvider *CppModelManager::completionAssistProvider(
        const QString &mimeType) const
{
    if (mimeType.isEmpty())
        return 0;

    ModelManagerSupport::Ptr cms = modelManagerSupportForMimeType(mimeType);
    QTC_ASSERT(cms, return 0);
    return cms->completionAssistProvider();
}

BaseEditorDocumentProcessor *CppModelManager::editorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument) const
{
    QTC_ASSERT(baseTextDocument, return 0);
    ModelManagerSupport::Ptr cms = modelManagerSupportForMimeType(baseTextDocument->mimeType());
    QTC_ASSERT(cms, return 0);
    return cms->editorDocumentProcessor(baseTextDocument);
}

void CppModelManager::setIndexingSupport(CppIndexingSupport *indexingSupport)
{
    if (indexingSupport) {
        if (dynamic_cast<BuiltinIndexingSupport *>(indexingSupport))
            d->m_indexingSupporter = 0;
        else
            d->m_indexingSupporter = indexingSupport;
    }
}

CppIndexingSupport *CppModelManager::indexingSupport()
{
    return d->m_indexingSupporter ? d->m_indexingSupporter : d->m_internalIndexingSupport;
}

QStringList CppModelManager::projectFiles()
{
    ensureUpdated();
    return d->m_projectFiles;
}

ProjectPart::HeaderPaths CppModelManager::headerPaths()
{
    ensureUpdated();
    return d->m_headerPaths;
}

void CppModelManager::setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths)
{
    d->m_headerPaths = headerPaths;
}

QByteArray CppModelManager::definedMacros()
{
    ensureUpdated();
    return d->m_definedMacros;
}

void CppModelManager::enableGarbageCollector(bool enable)
{
    d->m_delayedGcTimer.stop();
    d->m_enableGC = enable;
}

} // namespace CppTools
