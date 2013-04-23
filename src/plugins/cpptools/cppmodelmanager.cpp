/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "cpppreprocessor.h"
#include "cpptoolsconstants.h"

#include "builtinindexingsupport.h"
#include "cppcompletionassist.h"
#include "cpphighlightingsupport.h"
#include "cpphighlightingsupportinternal.h"
#include "cppindexingsupport.h"
#include "abstracteditorsupport.h"
#include "cpptoolseditorsupport.h"
#include "cppfindreferences.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>
#include <QTextBlock>

#if defined(QTCREATOR_WITH_DUMP_AST) && defined(Q_CC_GNU)
#define WITH_AST_DUMP
#include <iostream>
#include <sstream>
#endif

namespace CppTools {

uint qHash(const ProjectPart &p)
{
    uint h = qHash(p.defines) ^ p.cVersion ^ p.cxxVersion ^ p.cxxExtensions ^ p.qtVersion;

    foreach (const QString &i, p.includePaths)
        h ^= qHash(i);

    foreach (const QString &f, p.frameworkPaths)
        h ^= qHash(f);

    return h;
}

bool operator==(const ProjectPart &p1,
                const ProjectPart &p2)
{
    if (p1.defines != p2.defines)
        return false;
    if (p1.cVersion != p2.cVersion)
        return false;
    if (p1.cxxVersion != p2.cxxVersion)
        return false;
    if (p1.cxxExtensions != p2.cxxExtensions)
        return false;
    if (p1.qtVersion!= p2.qtVersion)
        return false;
    if (p1.includePaths != p2.includePaths)
        return false;
    return p1.frameworkPaths == p2.frameworkPaths;
}

} // namespace CppTools

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CPlusPlus;

#ifdef WITH_AST_DUMP

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

void CppModelManager::updateModifiedSourceFiles()
{
    const Snapshot snapshot = this->snapshot();
    QStringList sourceFiles;

    foreach (const Document::Ptr doc, snapshot) {
        const QDateTime lastModified = doc->lastModified();

        if (! lastModified.isNull()) {
            QFileInfo fileInfo(doc->fileName());

            if (fileInfo.exists() && fileInfo.lastModified() != lastModified)
                sourceFiles.append(doc->fileName());
        }
    }

    updateSourceFiles(sourceFiles);
}

/*!
    \class CppTools::CppModelManager
    \brief The CppModelManager keeps track of one CppCodeModel instance
           for each project and all related CppCodeModelPart instances.

    It also takes care of updating the code models when C++ files are
    modified within Qt Creator.
*/

QMutex CppModelManager::m_modelManagerMutex;
CppModelManager *CppModelManager::m_modelManagerInstance = 0;

CppModelManager *CppModelManager::instance()
{
    if (m_modelManagerInstance)
        return m_modelManagerInstance;
    QMutexLocker locker(&m_modelManagerMutex);
    if (!m_modelManagerInstance)
        m_modelManagerInstance = new CppModelManager;
    return m_modelManagerInstance;
}

CppModelManager::CppModelManager(QObject *parent)
    : CppModelManagerInterface(parent)
    , m_enableGC(true)
    , m_completionAssistProvider(0)
    , m_highlightingFactory(0)
    , m_indexingSupporter(0)
{
    m_findReferences = new CppFindReferences(this);
    m_indexerEnabled = qgetenv("QTCREATOR_NO_CODE_INDEXER").isNull();

    m_dirty = true;

    ProjectExplorer::ProjectExplorerPlugin *pe =
       ProjectExplorer::ProjectExplorerPlugin::instance();

    QTC_ASSERT(pe, return);

    ProjectExplorer::SessionManager *session = pe->session();
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(onProjectAdded(ProjectExplorer::Project*)));

    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));

    connect(session, SIGNAL(aboutToUnloadSession(QString)),
            this, SLOT(onAboutToUnloadSession()));

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(onCoreAboutToClose()));

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");

    // Listen for editor closed events so that we can keep track of changing files
    connect(Core::ICore::editorManager(), SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));

    m_completionFallback = new InternalCompletionAssistProvider;
    m_completionAssistProvider = m_completionFallback;
    ExtensionSystem::PluginManager::addObject(m_completionAssistProvider);
    m_highlightingFallback = new CppHighlightingSupportInternalFactory;
    m_highlightingFactory = m_highlightingFallback;
    m_internalIndexingSupport = new BuiltinIndexingSupport;
}

CppModelManager::~CppModelManager()
{
    ExtensionSystem::PluginManager::removeObject(m_completionAssistProvider);
    delete m_completionFallback;
    delete m_highlightingFallback;
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
    if (! m_dirty)
        return;

    m_projectFiles = internalProjectFiles();
    m_includePaths = internalIncludePaths();
    m_frameworkPaths = internalFrameworkPaths();
    m_definedMacros = internalDefinedMacros();
    m_dirty = false;
}

QStringList CppModelManager::internalProjectFiles() const
{
    QStringList files;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            foreach (const ProjectFile &file, part->files)
                files += file.path;
        }
    }
    files.removeDuplicates();
    return files;
}

QStringList CppModelManager::internalIncludePaths() const
{
    QStringList includePaths;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts())
            foreach (const QString &path, part->includePaths)
                includePaths.append(CppPreprocessor::cleanPath(path));
    }
    includePaths.removeDuplicates();
    return includePaths;
}

QStringList CppModelManager::internalFrameworkPaths() const
{
    QStringList frameworkPaths;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts())
            foreach (const QString &path, part->frameworkPaths)
                frameworkPaths.append(CppPreprocessor::cleanPath(path));
    }
    frameworkPaths.removeDuplicates();
    return frameworkPaths;
}

QByteArray CppModelManager::internalDefinedMacros() const
{
    QByteArray macros;
    QSet<QByteArray> alreadyIn;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            const QList<QByteArray> defs = part->defines.split('\n');
            foreach (const QByteArray &def, defs) {
                if (!alreadyIn.contains(def)) {
                    macros += def;
                    macros.append('\n');
                    alreadyIn.insert(def);
                }
            }
        }
    }
    return macros;
}

/// This method will aquire the mutex!
void CppModelManager::dumpModelManagerConfiguration()
{
    // Tons of debug output...
    qDebug()<<"========= CppModelManager::dumpModelManagerConfiguration ======";
    foreach (const ProjectInfo &pinfo, m_projects) {
        qDebug()<<" for project:"<< pinfo.project().data()->document()->fileName();
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
            qDebug() << "=== part ===";
            const char* cVersion;
            switch (part->cVersion) {
            case ProjectPart::C89: cVersion = "C89"; break;
            case ProjectPart::C99: cVersion = "C99"; break;
            case ProjectPart::C11: cVersion = "C11"; break;
            default: cVersion = "INVALID";
            }
            const char* cxxVersion;
            switch (part->cxxVersion) {
            case ProjectPart::CXX98: cxxVersion = "CXX98"; break;
            case ProjectPart::CXX11: cxxVersion = "CXX11"; break;
            default: cxxVersion = "INVALID";
            }
            QStringList cxxExtensions;
            if (part->cxxExtensions & ProjectPart::GnuExtensions)
                cxxExtensions << QLatin1String("GnuExtensions");
            if (part->cxxExtensions & ProjectPart::MicrosoftExtensions)
                cxxExtensions << QLatin1String("MicrosoftExtensions");
            if (part->cxxExtensions & ProjectPart::BorlandExtensions)
                cxxExtensions << QLatin1String("BorlandExtensions");
            if (part->cxxExtensions & ProjectPart::OpenMP)
                cxxExtensions << QLatin1String("OpenMP");

            qDebug() << "cVersion:" << cVersion;
            qDebug() << "cxxVersion:" << cxxVersion;
            qDebug() << "cxxExtensions:" << cxxExtensions;
            qDebug() << "Qt version:" << part->qtVersion;
            qDebug() << "precompiled header:" << part->precompiledHeaders;
            qDebug() << "defines:" << part->defines;
            qDebug() << "includes:" << part->includePaths;
            qDebug() << "frameworkPaths:" << part->frameworkPaths;
            qDebug() << "files:" << part->files;
            qDebug() << "";
        }
    }

    ensureUpdated();
    qDebug() << "=== Merged include paths ===";
    foreach (const QString &inc, m_includePaths)
        qDebug() << inc;
    qDebug() << "=== Merged framework paths ===";
    foreach (const QString &inc, m_frameworkPaths)
        qDebug() << inc;
    qDebug() << "=== Merged defined macros ===";
    qDebug() << m_definedMacros;
    qDebug()<<"========= End of dump ======";
}

void CppModelManager::addEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.insert(editorSupport);
}

void CppModelManager::removeEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.remove(editorSupport);
}

/// \brief Returns the \c CppEditorSupport for the given text editor. It will
///        create one when none exists yet.
CppEditorSupport *CppModelManager::cppEditorSupport(TextEditor::BaseTextEditor *editor)
{
    Q_ASSERT(editor);

    QMutexLocker locker(&m_editorSupportMutex);

    CppEditorSupport *editorSupport = m_editorSupport.value(editor, 0);
    if (!editorSupport) {
        editorSupport = new CppEditorSupport(this, editor);
        m_editorSupport.insert(editor, editorSupport);
    }
    return editorSupport;
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

void CppModelManager::renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
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

CppModelManager::WorkingCopy CppModelManager::buildWorkingCopyList()
{
    QList<CppEditorSupport *> supporters;

    {
        QMutexLocker locker(&m_editorSupportMutex);
        supporters = m_editorSupport.values();
    }

    WorkingCopy workingCopy;
    foreach (const CppEditorSupport *editorSupport, supporters) {
        workingCopy.insert(editorSupport->fileName(), editorSupport->contents(),
                           editorSupport->editorRevision());
    }

    QSetIterator<AbstractEditorSupport *> jt(m_addtionalEditorSupport);
    while (jt.hasNext()) {
        AbstractEditorSupport *es =  jt.next();
        workingCopy.insert(es->fileName(), QString::fromUtf8(es->contents()));
    }

    // add the project configuration file
    QByteArray conf(pp_configuration);
    conf += definedMacros();
    workingCopy.insert(configurationFileName(), QString::fromUtf8(conf));

    return workingCopy;
}

CppModelManager::WorkingCopy CppModelManager::workingCopy() const
{
    return const_cast<CppModelManager *>(this)->buildWorkingCopyList();
}

QFuture<void> CppModelManager::updateSourceFiles(const QStringList &sourceFiles)
{
    if (sourceFiles.isEmpty() || !m_indexerEnabled)
        return QFuture<void>();

    if (m_indexingSupporter)
        m_indexingSupporter->refreshSourceFiles(sourceFiles);
    return m_internalIndexingSupport->refreshSourceFiles(sourceFiles);
}

QList<CppModelManager::ProjectInfo> CppModelManager::projectInfos() const
{
    QMutexLocker locker(&m_projectMutex);

    return m_projects.values();
}

CppModelManager::ProjectInfo CppModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&m_projectMutex);

    return m_projects.value(project, ProjectInfo(project));
}

void CppModelManager::updateProjectInfo(const ProjectInfo &pinfo)
{
    { // only hold the mutex for a limited scope, so the dumping afterwards can aquire it without deadlocking.
        QMutexLocker locker(&m_projectMutex);

        if (! pinfo.isValid())
            return;

        ProjectExplorer::Project *project = pinfo.project().data();
        m_projects.insert(project, pinfo);
        m_dirty = true;

        m_srcToProjectPart.clear();

        foreach (const ProjectInfo &projectInfo, m_projects) {
            foreach (const ProjectPart::Ptr &projectPart, projectInfo.projectParts()) {
                foreach (const ProjectFile &cxxFile, projectPart->files) {
                    m_srcToProjectPart[cxxFile.path].append(projectPart);
                    foreach (const QString &fileName, m_snapshot.allIncludesForDocument(cxxFile.path))
                        m_snapshot.remove(fileName);
                    m_snapshot.remove(cxxFile.path);
                }
            }
        }

        m_snapshot.remove(configurationFileName());
    }

    if (!qgetenv("QTCREATOR_DUMP_PROJECT_INFO").isEmpty())
        dumpModelManagerConfiguration();

    emit projectPartsUpdated(pinfo.project().data());
}

QList<ProjectPart::Ptr> CppModelManager::projectPart(const QString &fileName) const
{
    QList<ProjectPart::Ptr> parts = m_srcToProjectPart.value(fileName);
    if (!parts.isEmpty())
        return parts;

    DependencyTable table;
    table.build(snapshot());
    QStringList deps = table.filesDependingOn(fileName);
    foreach (const QString &dep, deps) {
        parts = m_srcToProjectPart.value(dep);
        if (!parts.isEmpty())
            return parts;
    }

    return parts;
}

/// \brief Removes the CppEditorSupport for the closed editor.
void CppModelManager::editorAboutToClose(Core::IEditor *editor)
{
    if (!isCppEditor(editor))
        return;

    TextEditor::BaseTextEditor *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QTC_ASSERT(textEditor, return);

    QMutexLocker locker(&m_editorSupportMutex);
    CppEditorSupport *editorSupport = m_editorSupport.value(textEditor, 0);
    m_editorSupport.remove(textEditor);
    delete editorSupport;
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

void CppModelManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    do {
        QMutexLocker locker(&m_projectMutex);
        m_dirty = true;
        m_projects.remove(project);
    } while (0);

    GC();
}

void CppModelManager::onAboutToUnloadSession()
{
    if (Core::ProgressManager *pm = Core::ICore::progressManager())
        pm->cancelTasks(QLatin1String(CppTools::Constants::TASK_INDEX));
    do {
        QMutexLocker locker(&m_projectMutex);
        m_projects.clear();
        m_dirty = true;
    } while (0);

    GC();
}

void CppModelManager::onCoreAboutToClose()
{
    m_enableGC = false;
}

void CppModelManager::GC()
{
    if (!m_enableGC)
        return;

    Snapshot currentSnapshot = snapshot();
    QSet<QString> processed;
    QStringList todo = projectFiles();

    while (! todo.isEmpty()) {
        QString fn = todo.last();
        todo.removeLast();

        if (processed.contains(fn))
            continue;

        processed.insert(fn);

        if (Document::Ptr doc = currentSnapshot.document(fn))
            todo += doc->includedFiles();
    }

    QStringList removedFiles;

    Snapshot newSnapshot;
    for (Snapshot::const_iterator it = currentSnapshot.begin(); it != currentSnapshot.end(); ++it) {
        const QString fileName = it.key();

        if (processed.contains(fileName))
            newSnapshot.insert(it.value());
        else
            removedFiles.append(fileName);
    }

    emit aboutToRemoveFiles(removedFiles);

    replaceSnapshot(newSnapshot);
}

void CppModelManager::finishedRefreshingSourceFiles(const QStringList &files)
{
    emit sourceFilesRefreshed(files);
}

CppCompletionSupport *CppModelManager::completionSupport(Core::IEditor *editor) const
{
    if (TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor))
        return m_completionAssistProvider->completionSupport(textEditor);
    else
        return 0;
}

void CppModelManager::setCppCompletionAssistProvider(CppCompletionAssistProvider *completionAssistProvider)
{
    ExtensionSystem::PluginManager::removeObject(m_completionAssistProvider);
    if (completionAssistProvider)
        m_completionAssistProvider = completionAssistProvider;
    else
        m_completionAssistProvider = m_completionFallback;
    ExtensionSystem::PluginManager::addObject(m_completionAssistProvider);
}

CppHighlightingSupport *CppModelManager::highlightingSupport(Core::IEditor *editor) const
{
    if (TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor))
        return m_highlightingFactory->highlightingSupport(textEditor);
    else
        return 0;
}

void CppModelManager::setHighlightingSupportFactory(CppHighlightingSupportFactory *highlightingFactory)
{
    if (highlightingFactory)
        m_highlightingFactory = highlightingFactory;
    else
        m_highlightingFactory = m_highlightingFallback;
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

void CppModelManager::setExtraDiagnostics(const QString &fileName, const QString &kind,
                                          const QList<Document::DiagnosticMessage> &diagnostics)
{
    QList<CppEditorSupport *> supporters;

    {
        QMutexLocker locker(&m_editorSupportMutex);
        supporters = m_editorSupport.values();
    }

    foreach (CppEditorSupport *supporter, supporters) {
        if (supporter->fileName() == fileName) {
            supporter->setExtraDiagnostics(kind, diagnostics);
            break;
        }
    }
}
