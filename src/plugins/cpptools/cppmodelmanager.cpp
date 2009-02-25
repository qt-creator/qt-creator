/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include <cplusplus/pp.h>

#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpptoolseditorsupport.h"

#include <qtconcurrent/runextensions.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <TranslationUnit.h>
#include <Semantic.h>
#include <AST.h>
#include <Scope.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <NameVisitor.h>
#include <TypeVisitor.h>
#include <Lexer.h>
#include <Token.h>

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QTime>
#include <QtCore/QTimer>

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CPlusPlus;

static const char pp_configuration_file[] = "<configuration>";

static const char pp_configuration[] =
    "# 1 \"<configuration>\"\n"
    "#define __GNUC_MINOR__ 0\n"
    "#define __GNUC__ 4\n"
    "#define __GNUG__ 4\n"
    "#define __STDC_HOSTED__ 1\n"
    "#define __VERSION__ \"4.0.1 (fake)\"\n"
    "#define __cplusplus 1\n"

    "#define __extension__\n"
    "#define __context__\n"
    "#define __range__\n"
    "#define __asm(a...)\n"
    "#define __asm__(a...)\n"
    "#define   restrict\n"
    "#define __restrict\n"

    // ### add macros for win32
    "#define __cdecl\n"
    "#define QT_WA(x) x\n"
    "#define API\n"
    "#define WINAPI\n"
    "#define CALLBACK\n"
    "#define STDMETHODCALLTYPE\n"
    "#define __RPC_FAR\n"
    "#define APIENTRY\n"
    "#define __declspec(a)\n"
    "#define STDMETHOD(method) virtual HRESULT STDMETHODCALLTYPE method\n";

namespace CppTools {
namespace Internal {

class CppPreprocessor: public CPlusPlus::Client
{
public:
    CppPreprocessor(QPointer<CppModelManager> modelManager);

    void setWorkingCopy(const QMap<QString, QByteArray> &workingCopy);
    void setIncludePaths(const QStringList &includePaths);
    void setFrameworkPaths(const QStringList &frameworkPaths);
    void setProjectFiles(const QStringList &files);
    void run(QString &fileName);
    void operator()(QString &fileName);

protected:
    CPlusPlus::Document::Ptr switchDocument(CPlusPlus::Document::Ptr doc);

    bool includeFile(const QString &absoluteFilePath, QByteArray *result);
    QByteArray tryIncludeFile(QString &fileName, IncludeType type);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);
    void mergeEnvironment(CPlusPlus::Document::Ptr doc, QSet<QString> *processed);

    virtual void macroAdded(const Macro &macro);
    virtual void startExpandingMacro(unsigned offset,
                                     const Macro &macro,
                                     const QByteArray &originalText);
    virtual void stopExpandingMacro(unsigned offset, const Macro &macro);
    virtual void startSkippingBlocks(unsigned offset);
    virtual void stopSkippingBlocks(unsigned offset);
    virtual void sourceNeeded(QString &fileName, IncludeType type,
                              unsigned line);

private:
    QPointer<CppModelManager> m_modelManager;
    Snapshot m_snapshot;
    Environment env;
    Preprocessor m_proc;
    QStringList m_includePaths;
    QStringList m_systemIncludePaths;
    QMap<QString, QByteArray> m_workingCopy;
    QStringList m_projectFiles;
    QStringList m_frameworkPaths;
    QSet<QString> m_included;
    CPlusPlus::Document::Ptr m_currentDoc;
};

} // namespace Internal
} // namespace CppTools

CppPreprocessor::CppPreprocessor(QPointer<CppModelManager> modelManager)
    : m_modelManager(modelManager),
    m_snapshot(modelManager->snapshot()),
    m_proc(this, env)
{ }

void CppPreprocessor::setWorkingCopy(const QMap<QString, QByteArray> &workingCopy)
{ m_workingCopy = workingCopy; }

void CppPreprocessor::setIncludePaths(const QStringList &includePaths)
{ m_includePaths = includePaths; }

void CppPreprocessor::setFrameworkPaths(const QStringList &frameworkPaths)
{ m_frameworkPaths = frameworkPaths; }

void CppPreprocessor::setProjectFiles(const QStringList &files)
{ m_projectFiles = files; }

void CppPreprocessor::run(QString &fileName)
{ sourceNeeded(fileName, IncludeGlobal, /*line = */ 0); }

void CppPreprocessor::operator()(QString &fileName)
{ run(fileName); }

bool CppPreprocessor::includeFile(const QString &absoluteFilePath, QByteArray *result)
{
    if (absoluteFilePath.isEmpty() || m_included.contains(absoluteFilePath)) {
        return true;
    }

    if (m_workingCopy.contains(absoluteFilePath)) {
        m_included.insert(absoluteFilePath);
        *result = m_workingCopy.value(absoluteFilePath);
        return true;
    }

    QFileInfo fileInfo(absoluteFilePath);
    if (! fileInfo.isFile())
        return false;

    QFile file(absoluteFilePath);
    if (file.open(QFile::ReadOnly)) {
        m_included.insert(absoluteFilePath);
        QTextStream stream(&file);
        const QString contents = stream.readAll();
        *result = contents.toUtf8();
        file.close();
        return true;
    }

    return false;
}

QByteArray CppPreprocessor::tryIncludeFile(QString &fileName, IncludeType type)
{
    QFileInfo fileInfo(fileName);
    if (fileName == QLatin1String(pp_configuration_file) || fileInfo.isAbsolute()) {
        QByteArray contents;
        includeFile(fileName, &contents);
        return contents;
    }

    if (type == IncludeLocal && m_currentDoc) {
        QFileInfo currentFileInfo(m_currentDoc->fileName());
        QString path = currentFileInfo.absolutePath();
        path += QLatin1Char('/');
        path += fileName;
        path = QDir::cleanPath(path);
        QByteArray contents;
        if (includeFile(path, &contents)) {
            fileName = path;
            return contents;
        }
    }

    foreach (const QString &includePath, m_includePaths) {
        QString path = includePath;
        path += QLatin1Char('/');
        path += fileName;
        path = QDir::cleanPath(path);
        QByteArray contents;
        if (includeFile(path, &contents)) {
            fileName = path;
            return contents;
        }
    }

    // look in the system include paths
    foreach (const QString &includePath, m_systemIncludePaths) {
        QString path = includePath;
        path += QLatin1Char('/');
        path += fileName;
        path = QDir::cleanPath(path);
        QByteArray contents;
        if (includeFile(path, &contents)) {
            fileName = path;
            return contents;
        }
    }

    int index = fileName.indexOf(QLatin1Char('/'));
    if (index != -1) {
        QString frameworkName = fileName.left(index);
        QString name = fileName.mid(index + 1);

        foreach (const QString &frameworkPath, m_frameworkPaths) {
            QString path = frameworkPath;
            path += QLatin1Char('/');
            path += frameworkName;
            path += QLatin1String(".framework/Headers/");
            path += name;
            QByteArray contents;
            if (includeFile(path, &contents)) {
                fileName = path;
                return contents;
            }
        }
    }

    QString path = fileName;
    if (path.at(0) != QLatin1Char('/'))
        path.prepend(QLatin1Char('/'));

    foreach (const QString &projectFile, m_projectFiles) {
        if (projectFile.endsWith(path)) {
            fileName = projectFile;
            QByteArray contents;
            includeFile(fileName, &contents);
            return contents;
        }
    }

    //qDebug() << "**** file" << fileName << "not found!";
    return QByteArray();
}

void CppPreprocessor::macroAdded(const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->appendMacro(macro);
}

void CppPreprocessor::startExpandingMacro(unsigned offset,
                                          const Macro &macro,
                                          const QByteArray &originalText)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "start expanding:" << macro.name << "text:" << originalText;
    m_currentDoc->addMacroUse(macro, offset, originalText.length());
}

void CppPreprocessor::stopExpandingMacro(unsigned, const Macro &)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "stop expanding:" << macro.name;
}

void CppPreprocessor::mergeEnvironment(Document::Ptr doc)
{
    QSet<QString> processed;
    mergeEnvironment(doc, &processed);
}

void CppPreprocessor::mergeEnvironment(Document::Ptr doc, QSet<QString> *processed)
{
    if (! doc)
        return;

    const QString fn = doc->fileName();

    if (processed->contains(fn))
        return;

    processed->insert(fn);

    foreach (QString includedFile, doc->includedFiles()) {
        mergeEnvironment(m_snapshot.value(includedFile), processed);
    }

    foreach (const Macro macro, doc->definedMacros()) {
        env.bind(macro);
    }
}

void CppPreprocessor::startSkippingBlocks(unsigned offset)
{
    //qDebug() << "start skipping blocks:" << offset;
    if (m_currentDoc)
        m_currentDoc->startSkippingBlocks(offset);
}

void CppPreprocessor::stopSkippingBlocks(unsigned offset)
{
    //qDebug() << "stop skipping blocks:" << offset;
    if (m_currentDoc)
        m_currentDoc->stopSkippingBlocks(offset);
}

void CppPreprocessor::sourceNeeded(QString &fileName, IncludeType type,
                                   unsigned line)
{
    if (fileName.isEmpty())
        return;

    QByteArray contents = tryIncludeFile(fileName, type);

    if (m_currentDoc) {
        m_currentDoc->addIncludeFile(fileName, line);
        if (contents.isEmpty() && ! QFileInfo(fileName).isAbsolute()) {
            QString msg;
            msg += fileName;
            msg += QLatin1String(": No such file or directory");
            Document::DiagnosticMessage d(Document::DiagnosticMessage::Warning,
                                          m_currentDoc->fileName(),
                                          env.currentLine, /*column = */ 0,
                                          msg);
            m_currentDoc->addDiagnosticMessage(d);
            //qWarning() << "file not found:" << fileName << m_currentDoc->fileName() << env.current_line;
        }
    }

    if (! contents.isEmpty()) {
        Document::Ptr cachedDoc = m_snapshot.value(fileName);
        if (cachedDoc && m_currentDoc) {
            mergeEnvironment(cachedDoc);
        } else {
            Document::Ptr previousDoc = switchDocument(Document::create(fileName));

            const QByteArray previousFile = env.currentFile;
            const unsigned previousLine = env.currentLine;

            env.currentFile = QByteArray(m_currentDoc->translationUnit()->fileName(),
                                         m_currentDoc->translationUnit()->fileNameLength());

            QByteArray preprocessedCode;
            m_proc(contents, &preprocessedCode);
            //qDebug() << preprocessedCode;

            env.currentFile = previousFile;
            env.currentLine = previousLine;

            m_currentDoc->setSource(preprocessedCode);
            m_currentDoc->parse();
            m_currentDoc->check();
            m_currentDoc->releaseTranslationUnit(); // release the AST and the token stream.

            if (m_modelManager)
                m_modelManager->emitDocumentUpdated(m_currentDoc);
            (void) switchDocument(previousDoc);
        }
    }
}

Document::Ptr CppPreprocessor::switchDocument(Document::Ptr doc)
{
    Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}



/*!
    \class CppTools::CppModelManager
    \brief The CppModelManager keeps track of one CppCodeModel instance
           for each project and all related CppCodeModelPart instances.

    It also takes care of updating the code models when C++ files are
    modified within Workbench.
*/

CppModelManager::CppModelManager(QObject *parent)
    : CppModelManagerInterface(parent)
{
    m_core = Core::ICore::instance(); // FIXME
    m_dirty = true;

    m_projectExplorer = ExtensionSystem::PluginManager::instance()
                        ->getObject<ProjectExplorer::ProjectExplorerPlugin>();

    QTC_ASSERT(m_projectExplorer, return);

    ProjectExplorer::SessionManager *session = m_projectExplorer->session();
    QTC_ASSERT(session, return);

    m_updateEditorSelectionsTimer = new QTimer(this);
    m_updateEditorSelectionsTimer->setInterval(500);
    m_updateEditorSelectionsTimer->setSingleShot(true);
    connect(m_updateEditorSelectionsTimer, SIGNAL(timeout()),
            this, SLOT(updateEditorSelections()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(onProjectAdded(ProjectExplorer::Project*)));

    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
            this, SLOT(onAboutToRemoveProject(ProjectExplorer::Project *)));

    connect(session, SIGNAL(sessionUnloaded()),
            this, SLOT(onSessionUnloaded()));

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");

    // thread connections
    connect(this, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    // Listen for editor closed and opened events so that we can keep track of changing files
    connect(m_core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
        this, SLOT(editorOpened(Core::IEditor *)));

    connect(m_core->editorManager(), SIGNAL(editorAboutToClose(Core::IEditor *)),
        this, SLOT(editorAboutToClose(Core::IEditor *)));
}

CppModelManager::~CppModelManager()
{ }

Snapshot CppModelManager::snapshot() const
{ return m_snapshot; }

void CppModelManager::ensureUpdated()
{
    QMutexLocker locker(&mutex);
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
        files += pinfo.sourceFiles;
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
        includePaths += pinfo.includePaths;
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
        frameworkPaths += pinfo.frameworkPaths;
    }
    frameworkPaths.removeDuplicates();
    return frameworkPaths;
}

QByteArray CppModelManager::internalDefinedMacros() const
{
    QByteArray macros;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        macros += pinfo.defines;
    }
    return macros;
}

QMap<QString, QByteArray> CppModelManager::buildWorkingCopyList()
{
    QMap<QString, QByteArray> workingCopy;
    QMapIterator<TextEditor::ITextEditor *, CppEditorSupport *> it(m_editorSupport);
    while (it.hasNext()) {
        it.next();
        TextEditor::ITextEditor *textEditor = it.key();
        CppEditorSupport *editorSupport = it.value();
        QString fileName = textEditor->file()->fileName();
        workingCopy[fileName] = editorSupport->contents().toUtf8();
    }

    // add the project configuration file
    QByteArray conf(pp_configuration);
    conf += definedMacros();
    workingCopy[pp_configuration_file] = conf;

    return workingCopy;
}

void CppModelManager::updateSourceFiles(const QStringList &sourceFiles)
{ (void) refreshSourceFiles(sourceFiles); }

QList<CppModelManager::ProjectInfo> CppModelManager::projectInfos() const
{
    QMutexLocker locker(&mutex);

    return m_projects.values();
}

CppModelManager::ProjectInfo CppModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&mutex);

    return m_projects.value(project, ProjectInfo(project));
}

void CppModelManager::updateProjectInfo(const ProjectInfo &pinfo)
{
    QMutexLocker locker(&mutex);

    if (! pinfo.isValid())
        return;

    m_projects.insert(pinfo.project, pinfo);
    m_dirty = true;
}

QFuture<void> CppModelManager::refreshSourceFiles(const QStringList &sourceFiles)
{
    if (! sourceFiles.isEmpty() && qgetenv("QTCREATOR_NO_CODE_INDEXER").isNull()) {
        const QMap<QString, QByteArray> workingCopy = buildWorkingCopyList();

        CppPreprocessor *preproc = new CppPreprocessor(this);
        preproc->setProjectFiles(projectFiles());
        preproc->setIncludePaths(includePaths());
        preproc->setFrameworkPaths(frameworkPaths());
        preproc->setWorkingCopy(workingCopy);

        QFuture<void> result = QtConcurrent::run(&CppModelManager::parse,
                                                 preproc, sourceFiles);

        if (sourceFiles.count() > 1) {
            m_core->progressManager()->addTask(result, tr("Indexing"),
                            CppTools::Constants::TASK_INDEX,
                            Core::ProgressManager::CloseOnSuccess);
        }
        return result;
    }
    return QFuture<void>();
}

/*!
    \fn    void CppModelManager::editorOpened(Core::IEditor *editor)
    \brief If a C++ editor is opened, the model manager listens to content changes
           in order to update the CppCodeModel accordingly. It also updates the
           CppCodeModel for the first time with this editor.

    \sa    void CppModelManager::editorContentsChanged()
 */
void CppModelManager::editorOpened(Core::IEditor *editor)
{
    if (isCppEditor(editor)) {
        TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
        QTC_ASSERT(textEditor, return);

        CppEditorSupport *editorSupport = new CppEditorSupport(this);
        editorSupport->setTextEditor(textEditor);
        m_editorSupport[textEditor] = editorSupport;
    }
}

void CppModelManager::editorAboutToClose(Core::IEditor *editor)
{
    if (isCppEditor(editor)) {
        TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
        QTC_ASSERT(textEditor, return);

        CppEditorSupport *editorSupport = m_editorSupport.value(textEditor);
        m_editorSupport.remove(textEditor);
        delete editorSupport;
    }
}

bool CppModelManager::isCppEditor(Core::IEditor *editor) const
{
    Core::UniqueIDManager *uidm = m_core->uniqueIDManager();
    const int uid = uidm->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);
    return editor->context().contains(uid);
}

void CppModelManager::emitDocumentUpdated(Document::Ptr doc)
{ emit documentUpdated(doc); }

void CppModelManager::onDocumentUpdated(Document::Ptr doc)
{
    const QString fileName = doc->fileName();
    m_snapshot[fileName] = doc;
    QList<Core::IEditor *> openedEditors = m_core->editorManager()->openedEditors();
    foreach (Core::IEditor *editor, openedEditors) {
        if (editor->file()->fileName() == fileName) {
            TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
            if (! textEditor)
                continue;

            TextEditor::BaseTextEditor *ed = qobject_cast<TextEditor::BaseTextEditor *>(textEditor->widget());
            if (! ed)
                continue;

            QList<TextEditor::BaseTextEditor::BlockRange> blockRanges;

            foreach (const Document::Block block, doc->skippedBlocks()) {
                blockRanges.append(TextEditor::BaseTextEditor::BlockRange(block.begin(), block.end()));
            }
            ed->setIfdefedOutBlocks(blockRanges);

            QList<QTextEdit::ExtraSelection> selections;

#ifdef QTCREATOR_WITH_MACRO_HIGHLIGHTING
            // set up the format for the macros
            QTextCharFormat macroFormat;
            macroFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

            QTextCursor c = ed->textCursor();
            foreach (const Document::Block block, doc->macroUses()) {
                QTextEdit::ExtraSelection sel;
                sel.cursor = c;
                sel.cursor.setPosition(block.begin());
                sel.cursor.setPosition(block.end(), QTextCursor::KeepAnchor);
                sel.format = macroFormat;
                selections.append(sel);
            }
#endif // QTCREATOR_WITH_MACRO_HIGHLIGHTING

            // set up the format for the errors
            QTextCharFormat errorFormat;
            errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            errorFormat.setUnderlineColor(Qt::red);

            // set up the format for the warnings.
            QTextCharFormat warningFormat;
            warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            warningFormat.setUnderlineColor(Qt::darkYellow);

            QSet<int> lines;
            foreach (const Document::DiagnosticMessage m, doc->diagnosticMessages()) {
                if (m.fileName() != fileName)
                    continue;
                else if (lines.contains(m.line()))
                    continue;

                lines.insert(m.line());

                QTextEdit::ExtraSelection sel;
                if (m.isWarning())
                    sel.format = warningFormat;
                else
                    sel.format = errorFormat;

                QTextCursor c(ed->document()->findBlockByNumber(m.line() - 1));
                const QString text = c.block().text();
                for (int i = 0; i < text.size(); ++i) {
                    if (! text.at(i).isSpace()) {
                        c.setPosition(c.position() + i);
                        break;
                    }
                }
                c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                sel.cursor = c;
                selections.append(sel);
            }

            QList<Editor> todo;
            foreach (Editor e, todo) {
                if (e.widget != ed)
                    todo.append(e);
            }

            Editor e;
            e.widget = ed;
            e.selections = selections;
            todo.append(e);
            m_todo = todo;
            postEditorUpdate();
            break;
        }
    }
}

void CppModelManager::postEditorUpdate()
{
    m_updateEditorSelectionsTimer->start(500);
}

void CppModelManager::updateEditorSelections()
{
    foreach (Editor ed, m_todo) {
        if (! ed.widget)
            continue;

        ed.widget->setExtraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection,
                                      ed.selections);
    }

    m_todo.clear();
}

void CppModelManager::onProjectAdded(ProjectExplorer::Project *)
{
    QMutexLocker locker(&mutex);
    m_dirty = true;
}

void CppModelManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    do {
        QMutexLocker locker(&mutex);
        m_dirty = true;
        m_projects.remove(project);
    } while (0);

    GC();
}

void CppModelManager::onSessionUnloaded()
{
    if (m_core->progressManager()) {
        m_core->progressManager()->cancelTasks(CppTools::Constants::TASK_INDEX);
    }

    do {
        QMutexLocker locker(&mutex);
        m_projects.clear();
        m_dirty = true;
    } while (0);

    GC();
}

void CppModelManager::parse(QFutureInterface<void> &future,
                            CppPreprocessor *preproc,
                            QStringList files)
{
    QTC_ASSERT(!files.isEmpty(), return);

    // Change the priority of the background parser thread to idle.
    QThread::currentThread()->setPriority(QThread::IdlePriority);

    future.setProgressRange(0, files.size());

    QString conf = QLatin1String(pp_configuration_file);
    (void) preproc->run(conf);

    const int STEP = 10;

    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            break;

        future.setProgressValue(i);

#ifdef CPPTOOLS_DEBUG_PARSING_TIME
        QTime tm;
        tm.start();
#endif

        QString fileName = files.at(i);
        preproc->run(fileName);

        if (! (i % STEP)) // Yields execution of the current thread.
            QThread::yieldCurrentThread();

#ifdef CPPTOOLS_DEBUG_PARSING_TIME
        qDebug() << fileName << "parsed in:" << tm.elapsed();
#endif
    }

    future.setProgressValue(files.size());

    // Restore the previous thread priority.
    QThread::currentThread()->setPriority(QThread::NormalPriority);

    delete preproc;
}

void CppModelManager::GC()
{
    Snapshot documents = m_snapshot;

    QSet<QString> processed;
    QStringList todo = projectFiles();

    while (! todo.isEmpty()) {
        QString fn = todo.last();
        todo.removeLast();

        if (processed.contains(fn))
            continue;

        processed.insert(fn);

        if (Document::Ptr doc = documents.value(fn)) {
            todo += doc->includedFiles();
        }
    }

    QStringList removedFiles;
    QMutableMapIterator<QString, Document::Ptr> it(documents);
    while (it.hasNext()) {
        it.next();
        const QString fn = it.key();
        if (! processed.contains(fn)) {
            removedFiles.append(fn);
            it.remove();
        }
    }

    emit aboutToRemoveFiles(removedFiles);
    m_snapshot = documents;
}


