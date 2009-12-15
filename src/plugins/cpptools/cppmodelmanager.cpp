/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <cplusplus/pp.h>
#include <cplusplus/CppBindings.h>
#include <cplusplus/Overview.h>
#include <cplusplus/CheckUndefinedSymbols.h>

#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpptoolseditorsupport.h"
#include "cppfindreferences.h"

#include <functional>
#include <QtConcurrentRun>
#include <QFutureSynchronizer>
#include <qtconcurrent/runextensions.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/mimedatabase.h>
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
#include <ASTVisitor.h>
#include <Lexer.h>
#include <Token.h>

#include <cplusplus/LookupContext.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtConcurrentMap>
#include <iostream>
#include <sstream>

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CPlusPlus;

#if defined(QTCREATOR_WITH_DUMP_AST) && defined(Q_CC_GNU)

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

static const char pp_configuration_file[] = "<configuration>";

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
    virtual ~CppPreprocessor();

    void setRevision(unsigned revision);
    void setWorkingCopy(const QHash<QString, QString> &workingCopy);
    void setIncludePaths(const QStringList &includePaths);
    void setFrameworkPaths(const QStringList &frameworkPaths);
    void setProjectFiles(const QStringList &files);
    void setTodo(const QStringList &files);

    void run(const QString &fileName);

    void resetEnvironment();

    const QSet<QString> &todo() const
    { return m_todo; }

public: // attributes
    Snapshot snapshot;

protected:
    CPlusPlus::Document::Ptr switchDocument(CPlusPlus::Document::Ptr doc);

    bool includeFile(const QString &absoluteFilePath, QString *result);
    QString tryIncludeFile(QString &fileName, IncludeType type);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);

    virtual void macroAdded(const Macro &macro);
    virtual void passedMacroDefinitionCheck(unsigned offset, const Macro &macro);
    virtual void failedMacroDefinitionCheck(unsigned offset, const QByteArray &name);
    virtual void startExpandingMacro(unsigned offset,
                                     const Macro &macro,
                                     const QByteArray &originalText,
                                     bool inCondition,
                                     const QVector<MacroArgumentReference> &actuals);
    virtual void stopExpandingMacro(unsigned offset, const Macro &macro);
    virtual void startSkippingBlocks(unsigned offset);
    virtual void stopSkippingBlocks(unsigned offset);
    virtual void sourceNeeded(QString &fileName, IncludeType type,
                              unsigned line);

private:
    QPointer<CppModelManager> m_modelManager;
    Environment env;
    Preprocessor preprocess;
    QStringList m_includePaths;
    QStringList m_systemIncludePaths;
    QHash<QString, QString> m_workingCopy;
    QStringList m_projectFiles;
    QStringList m_frameworkPaths;
    QSet<QString> m_included;
    Document::Ptr m_currentDoc;
    QSet<QString> m_todo;
    QSet<QString> m_processed;
    unsigned m_revision;
};

} // namespace Internal
} // namespace CppTools

CppPreprocessor::CppPreprocessor(QPointer<CppModelManager> modelManager)
    : snapshot(modelManager->snapshot()),
      m_modelManager(modelManager),
      preprocess(this, &env),
      m_revision(0)
{ }

CppPreprocessor::~CppPreprocessor()
{ }

void CppPreprocessor::setRevision(unsigned revision)
{ m_revision = revision; }

void CppPreprocessor::setWorkingCopy(const QHash<QString, QString> &workingCopy)
{ m_workingCopy = workingCopy; }

void CppPreprocessor::setIncludePaths(const QStringList &includePaths)
{ m_includePaths = includePaths; }

void CppPreprocessor::setFrameworkPaths(const QStringList &frameworkPaths)
{ m_frameworkPaths = frameworkPaths; }

void CppPreprocessor::setProjectFiles(const QStringList &files)
{ m_projectFiles = files; }

void CppPreprocessor::setTodo(const QStringList &files)
{ m_todo = QSet<QString>::fromList(files); }


namespace {

class Process: public std::unary_function<Document::Ptr, void>
{
    QPointer<CppModelManager> _modelManager;
    Snapshot _snapshot;
    QHash<QString, QString> _workingCopy;
    Document::Ptr _doc;

public:
    Process(QPointer<CppModelManager> modelManager,
            Snapshot snapshot,
            const QHash<QString, QString> &workingCopy)
        : _modelManager(modelManager),
          _snapshot(snapshot),
          _workingCopy(workingCopy)
    { }

    LookupContext lookupContext(unsigned line, unsigned column) const
    { return lookupContext(_doc->findSymbolAt(line, column)); }

    LookupContext lookupContext(Symbol *symbol) const
    {
        LookupContext context(symbol, Document::create(QLatin1String("<none>")), _doc, _snapshot);
        return context;
    }

    void operator()(Document::Ptr doc)
    {
        _doc = doc;

        Document::CheckMode mode = Document::FastCheck;

        if (_workingCopy.contains(doc->fileName()))
            mode = Document::FullCheck;

        doc->parse();
        doc->check(mode);

        if (mode == Document::FullCheck) {
            // run the binding pass
            NamespaceBindingPtr ns = bind(doc, _snapshot);

            // check for undefined symbols.
            CheckUndefinedSymbols checkUndefinedSymbols(doc);
            checkUndefinedSymbols.setGlobalNamespaceBinding(ns);

            checkUndefinedSymbols(doc->translationUnit()->ast()); // ### FIXME
        }

        doc->releaseTranslationUnit();

        if (_modelManager)
            _modelManager->emitDocumentUpdated(doc); // ### TODO: compress
    }
};

} // end of anonymous namespace

void CppPreprocessor::run(const QString &fileName)
{
    QString absoluteFilePath = fileName;
    sourceNeeded(absoluteFilePath, IncludeGlobal, /*line = */ 0);
}

void CppPreprocessor::resetEnvironment()
{
    env.reset();
    m_processed.clear();
}

bool CppPreprocessor::includeFile(const QString &absoluteFilePath, QString *result)
{
    if (absoluteFilePath.isEmpty() || m_included.contains(absoluteFilePath))
        return true;

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

QString CppPreprocessor::tryIncludeFile(QString &fileName, IncludeType type)
{
    QFileInfo fileInfo(fileName);
    if (fileName == QLatin1String(pp_configuration_file) || fileInfo.isAbsolute()) {
        QString contents;
        includeFile(fileName, &contents);
        return contents;
    }

    if (type == IncludeLocal && m_currentDoc) {
        QFileInfo currentFileInfo(m_currentDoc->fileName());
        QString path = currentFileInfo.absolutePath();
        path += QLatin1Char('/');
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
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
        QString contents;
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
        QString contents;
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
            path = QDir::cleanPath(path);
            QString contents;
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
            QString contents;
            includeFile(fileName, &contents);
            return contents;
        }
    }

    //qDebug() << "**** file" << fileName << "not found!";
    return QString();
}

void CppPreprocessor::macroAdded(const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->appendMacro(macro);
}

void CppPreprocessor::passedMacroDefinitionCheck(unsigned offset, const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(macro, offset, macro.name().length(),
                              QVector<MacroArgumentReference>(), true);
}

void CppPreprocessor::failedMacroDefinitionCheck(unsigned offset, const QByteArray &name)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addUndefinedMacroUse(name, offset);
}

void CppPreprocessor::startExpandingMacro(unsigned offset,
                                          const Macro &macro,
                                          const QByteArray &originalText,
                                          bool inCondition,
                                          const QVector<MacroArgumentReference> &actuals)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "start expanding:" << macro.name() << "text:" << originalText;
    m_currentDoc->addMacroUse(macro, offset, originalText.length(), actuals, inCondition);
}

void CppPreprocessor::stopExpandingMacro(unsigned, const Macro &)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "stop expanding:" << macro.name;
}

void CppPreprocessor::mergeEnvironment(Document::Ptr doc)
{
    if (! doc)
        return;

    const QString fn = doc->fileName();

    if (m_processed.contains(fn))
        return;

    m_processed.insert(fn);

    foreach (const Document::Include &incl, doc->includes()) {
        QString includedFile = incl.fileName();

        if (Document::Ptr includedDoc = snapshot.document(includedFile))
            mergeEnvironment(includedDoc);
        else
            run(includedFile);
    }

    env.addMacros(doc->definedMacros());
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

    QString contents = tryIncludeFile(fileName, type);
    fileName = QDir::cleanPath(fileName);
    if (m_currentDoc) {
        m_currentDoc->addIncludeFile(fileName, line);

        if (contents.isEmpty() && ! QFileInfo(fileName).isAbsolute()) {
            QString msg = QCoreApplication::translate(
                    "CppPreprocessor", "%1: No such file or directory").arg(fileName);

            Document::DiagnosticMessage d(Document::DiagnosticMessage::Warning,
                                          m_currentDoc->fileName(),
                                          env.currentLine, /*column = */ 0,
                                          msg);

            m_currentDoc->addDiagnosticMessage(d);

            //qWarning() << "file not found:" << fileName << m_currentDoc->fileName() << env.current_line;
        }
    }

    //qDebug() << "parse file:" << fileName << "contents:" << contents.size();

    Document::Ptr doc = snapshot.document(fileName);
    if (doc) {
        mergeEnvironment(doc);
        return;
    }

    doc = Document::create(fileName);
    doc->setRevision(m_revision);

    QFileInfo info(fileName);
    if (info.exists())
        doc->setLastModified(info.lastModified());

    Document::Ptr previousDoc = switchDocument(doc);

    const QByteArray preprocessedCode = preprocess(fileName, contents);

    doc->setSource(preprocessedCode);
    doc->tokenize();
    doc->releaseSource();

    snapshot.insert(doc);
    m_todo.remove(fileName);

    Process process(m_modelManager, snapshot, m_workingCopy);

    process(doc);

    (void) switchDocument(previousDoc);
}

Document::Ptr CppPreprocessor::switchDocument(Document::Ptr doc)
{
    Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}

void CppTools::CppModelManagerInterface::updateModifiedSourceFiles()
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

CppTools::CppModelManagerInterface *CppTools::CppModelManagerInterface::instance()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    return pluginManager->getObject<CppTools::CppModelManagerInterface>();

}


/*!
    \class CppTools::CppModelManager
    \brief The CppModelManager keeps track of one CppCodeModel instance
           for each project and all related CppCodeModelPart instances.

    It also takes care of updating the code models when C++ files are
    modified within Qt Creator.
*/

CppModelManager::CppModelManager(QObject *parent)
    : CppModelManagerInterface(parent)
{
    m_findReferences = new CppFindReferences(this);
    m_indexerEnabled = qgetenv("QTCREATOR_NO_CODE_INDEXER").isNull();

    m_revision = 0;
    m_synchronizer.setCancelOnWait(true);

    m_core = Core::ICore::instance(); // FIXME
    m_dirty = true;

    ProjectExplorer::ProjectExplorerPlugin *pe =
       ProjectExplorer::ProjectExplorerPlugin::instance();

    QTC_ASSERT(pe, return);

    ProjectExplorer::SessionManager *session = pe->session();
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

    connect(session, SIGNAL(aboutToUnloadSession()),
            this, SLOT(onAboutToUnloadSession()));

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
{
    QMutexLocker locker(&protectSnapshot);
    return m_snapshot;
}

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

void CppModelManager::setIncludesInPaths(const QMap<QString, QStringList> &includesInPaths)
{
    QMutexLocker locker(&mutex);
    QMapIterator<QString, QStringList> i(includesInPaths);
    while (i.hasNext()) {
        i.next();
        m_includesInPaths.insert(i.key(), i.value());
    }
}

void CppModelManager::addEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.insert(editorSupport);
}

void CppModelManager::removeEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.remove(editorSupport);
}

QList<int> CppModelManager::references(CPlusPlus::Symbol *symbol,
                                       CPlusPlus::Document::Ptr doc,
                                       const CPlusPlus::Snapshot &snapshot)
{
    NamespaceBindingPtr glo = bind(doc, snapshot);
    return m_findReferences->references(LookupContext::canonicalSymbol(symbol, glo.data()), doc, snapshot);
}

void CppModelManager::findUsages(CPlusPlus::Symbol *symbol)
{
    if (symbol->identifier())
        m_findReferences->findUsages(symbol);
}

void CppModelManager::renameUsages(CPlusPlus::Symbol *symbol)
{
    if (symbol->identifier())
        m_findReferences->renameUsages(symbol);
}

CppModelManager::WorkingCopy CppModelManager::buildWorkingCopyList()
{
    WorkingCopy workingCopy;
    QMapIterator<TextEditor::ITextEditor *, CppEditorSupport *> it(m_editorSupport);
    while (it.hasNext()) {
        it.next();
        TextEditor::ITextEditor *textEditor = it.key();
        CppEditorSupport *editorSupport = it.value();
        QString fileName = textEditor->file()->fileName();
        workingCopy[fileName] = editorSupport->contents();
    }

    QSetIterator<AbstractEditorSupport *> jt(m_addtionalEditorSupport);
    while (jt.hasNext()) {
        AbstractEditorSupport *es =  jt.next();
        workingCopy[es->fileName()] = es->contents();
    }

    // add the project configuration file
    QByteArray conf(pp_configuration);
    conf += definedMacros();
    workingCopy[pp_configuration_file] = conf;

    return workingCopy;
}

CppModelManager::WorkingCopy CppModelManager::workingCopy() const
{
    return const_cast<CppModelManager *>(this)->buildWorkingCopyList();
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

    if (m_indexerEnabled) {
        QFuture<void> result = QtConcurrent::run(&CppModelManager::updateIncludesInPaths,
                                                 this,
                                                 pinfo.includePaths,
                                                 pinfo.frameworkPaths,
                                                 m_headerSuffixes);

        if (pinfo.includePaths.size() > 1) {
            m_core->progressManager()->addTask(result, tr("Scanning"),
                                               CppTools::Constants::TASK_INDEX);
        }
    }
}

QStringList CppModelManager::includesInPath(const QString &path) const
{
    QMutexLocker locker(&mutex);
    return m_includesInPaths.value(path);
}

QFuture<void> CppModelManager::refreshSourceFiles(const QStringList &sourceFiles)
{
    if (! sourceFiles.isEmpty() && m_indexerEnabled) {
        const WorkingCopy workingCopy = buildWorkingCopyList();

        CppPreprocessor *preproc = new CppPreprocessor(this);
        preproc->setRevision(++m_revision);
        preproc->setProjectFiles(projectFiles());
        preproc->setIncludePaths(includePaths());
        preproc->setFrameworkPaths(frameworkPaths());
        preproc->setWorkingCopy(workingCopy);

        QFuture<void> result = QtConcurrent::run(&CppModelManager::parse,
                                                 preproc, sourceFiles);

        if (m_synchronizer.futures().size() > 10) {
            QList<QFuture<void> > futures = m_synchronizer.futures();

            m_synchronizer.clearFutures();

            foreach (QFuture<void> future, futures) {
                if (! (future.isFinished() || future.isCanceled()))
                    m_synchronizer.addFuture(future);
            }
        }

        m_synchronizer.addFuture(result);

        if (sourceFiles.count() > 1) {
            m_core->progressManager()->addTask(result, tr("Indexing"),
                            CppTools::Constants::TASK_INDEX);
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

    bool outdated = false;

    protectSnapshot.lock();

    Document::Ptr previous = m_snapshot.document(fileName);

    if (previous && (doc->revision() != 0 && doc->revision() < previous->revision()))
        outdated = true;
    else
        m_snapshot.insert(doc);

    protectSnapshot.unlock();

    if (outdated)
        return;

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

            foreach (const Document::Block &block, doc->skippedBlocks()) {
                blockRanges.append(TextEditor::BaseTextEditor::BlockRange(block.begin(), block.end()));
            }

            QList<QTextEdit::ExtraSelection> selections;

#ifdef QTCREATOR_WITH_MACRO_HIGHLIGHTING
            // set up the format for the macros
            QTextCharFormat macroFormat;
            macroFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

            QTextCursor c = ed->textCursor();
            foreach (const Document::MacroUse &block, doc->macroUses()) {
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

#ifdef QTCREATOR_WITH_ADVANCED_HIGHLIGHTER
            QSet<QPair<unsigned, unsigned> > lines;
            foreach (const Document::DiagnosticMessage &m, doc->diagnosticMessages()) {
                if (m.fileName() != fileName)
                    continue;

                const QPair<unsigned, unsigned> coordinates = qMakePair(m.line(), m.column());

                if (lines.contains(coordinates))
                    continue;

                lines.insert(coordinates);

                QTextEdit::ExtraSelection sel;
                if (m.isWarning())
                    sel.format = warningFormat;
                else
                    sel.format = errorFormat;

                QTextCursor c(ed->document()->findBlockByNumber(m.line() - 1));

                // ### check for generated tokens.

                int column = m.column();

                if (column > c.block().length()) {
                    column = 0;

                    const QString text = c.block().text();
                    for (int i = 0; i < text.size(); ++i) {
                        if (! text.at(i).isSpace()) {
                            ++column;
                            break;
                        }
                    }
                }

                if (column != 0)
                    --column;

                c.setPosition(c.position() + column);
                c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
                sel.cursor = c;
                selections.append(sel);
            }
#else
            QSet<int> lines;
            foreach (const Document::DiagnosticMessage &m, doc->diagnosticMessages()) {
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
#endif
            QList<Editor> todo;
            foreach (const Editor &e, todo) {
                if (e.textEditor != textEditor)
                    todo.append(e);
            }

            Editor e;
            e.revision = ed->document()->revision();
            e.textEditor = textEditor;
            e.selections = selections;
            e.ifdefedOutBlocks = blockRanges;
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
    foreach (const Editor &ed, m_todo) {
        if (! ed.textEditor)
            continue;

        TextEditor::ITextEditor *textEditor = ed.textEditor;
        TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(textEditor->widget());

        if (! editor)
            continue;
        else if (editor->document()->revision() != ed.revision)
            continue; // outdated

        editor->setExtraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection,
                                   ed.selections);

        editor->setIfdefedOutBlocks(ed.ifdefedOutBlocks);
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

void CppModelManager::onAboutToUnloadSession()
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

void CppModelManager::updateIncludesInPaths(QFutureInterface<void> &future,
                                            CppModelManager *manager,
                                            QStringList paths,
                                            QStringList frameworkPaths,
                                            QStringList suffixes)
{
    QMap<QString, QStringList> entriesInPaths;
    typedef QPair<QString, QString> SymLink;
    typedef QList<SymLink> SymLinks;
    SymLinks symlinks;
    int processed = 0;

    future.setProgressRange(0, paths.size());

    // Add framework header directories to path list
    QStringList frameworkFilter;
    frameworkFilter << QLatin1String("*.framework");
    QStringListIterator fwPathIt(frameworkPaths);
    while (fwPathIt.hasNext()) {
        const QString &fwPath = fwPathIt.next();
        QStringList entriesInFrameworkPath;
        const QStringList &frameworks = QDir(fwPath).entryList(frameworkFilter, QDir::Dirs | QDir::NoDotAndDotDot);
        QStringListIterator fwIt(frameworks);
        while (fwIt.hasNext()) {
            QString framework = fwIt.next();
            paths.append(fwPath + QLatin1Char('/') + framework + QLatin1String("/Headers"));
            framework.chop(10); // remove the ".framework"
            entriesInFrameworkPath.append(framework + QLatin1Char('/'));
        }
        entriesInPaths.insert(fwPath, entriesInFrameworkPath);
    }

    while (!paths.isEmpty()) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            return;

        const QString path = paths.takeFirst();

        // Skip already scanned paths
        if (entriesInPaths.contains(path))
            continue;

        QStringList entries;

        QDirIterator i(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        while (i.hasNext()) {
            const QString fileName = i.next();
            const QFileInfo fileInfo = i.fileInfo();
            const QString suffix = fileInfo.suffix();
            if (suffix.isEmpty() || suffixes.contains(suffix)) {
                QString text = fileInfo.fileName();
                if (fileInfo.isDir()) {
                    text += QLatin1Char('/');

                    // Also scan subdirectory, but avoid endless recursion with symbolic links
                    if (fileInfo.isSymLink()) {
                        QString target = fileInfo.symLinkTarget();

                        // Don't add broken symlinks
                        if (!QFileInfo(target).exists())
                            continue;

                        QMap<QString, QStringList>::const_iterator result = entriesInPaths.find(target);
                        if (result != entriesInPaths.constEnd()) {
                            entriesInPaths.insert(fileName, result.value());
                        } else {
                            paths.append(target);
                            symlinks.append(SymLink(fileName, target));
                        }
                    } else {
                        paths.append(fileName);
                    }
                }
                entries.append(text);
            }
        }

        entriesInPaths.insert(path, entries);

        ++processed;
        future.setProgressRange(0, processed + paths.size());
        future.setProgressValue(processed);
    }
    // link symlinks
    QListIterator<SymLink> it(symlinks);
    it.toBack();
    while (it.hasPrevious()) {
        SymLink v = it.previous();
        QMap<QString, QStringList>::const_iterator result = entriesInPaths.find(v.second);
        entriesInPaths.insert(v.first, result.value());
    }

    manager->setIncludesInPaths(entriesInPaths);

    future.reportFinished();
}

void CppModelManager::parse(QFutureInterface<void> &future,
                            CppPreprocessor *preproc,
                            QStringList files)
{
    if (files.isEmpty())
        return;

    Core::MimeDatabase *db = Core::ICore::instance()->mimeDatabase();
    QStringList headers, sources;
    Core::MimeType cSourceTy = db->findByType(QLatin1String("text/x-csrc"));
    Core::MimeType cppSourceTy = db->findByType(QLatin1String("text/x-c++src"));
    Core::MimeType mSourceTy = db->findByType(QLatin1String("text/x-objcsrc"));

    Core::MimeType cHeaderTy = db->findByType(QLatin1String("text/x-hdr"));
    Core::MimeType cppHeaderTy = db->findByType(QLatin1String("text/x-c++hdr"));

    foreach (const QString &file, files) {
        const QFileInfo fileInfo(file);

        if (cSourceTy.matchesFile(fileInfo) || cppSourceTy.matchesFile(fileInfo) || mSourceTy.matchesFile(fileInfo))
            sources.append(file);

        else if (cHeaderTy.matchesFile(fileInfo) || cppHeaderTy.matchesFile(fileInfo))
            headers.append(file);
    }

    foreach (const QString &file, files) {
        preproc->snapshot.remove(file);
    }

    files = sources;
    files += headers;

    preproc->setTodo(files);

    future.setProgressRange(0, files.size());

    QString conf = QLatin1String(pp_configuration_file);

    bool processingHeaders = false;

    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            break;

        // Change the priority of the background parser thread to idle.
        QThread::currentThread()->setPriority(QThread::IdlePriority);

        QString fileName = files.at(i);

        bool isSourceFile = false;
        if (cppSourceTy.matchesFile(fileName) || cSourceTy.matchesFile(fileName))
            isSourceFile = true;

        if (isSourceFile)
            (void) preproc->run(conf);

        else if (! processingHeaders) {
            (void) preproc->run(conf);

            processingHeaders = true;
        }

        preproc->run(fileName);

        future.setProgressValue(files.size() - preproc->todo().size());

        if (isSourceFile)
            preproc->resetEnvironment();

        // Restore the previous thread priority.
        QThread::currentThread()->setPriority(QThread::NormalPriority);
    }

    future.setProgressValue(files.size());

    delete preproc;
}

void CppModelManager::GC()
{
    protectSnapshot.lock();
    Snapshot currentSnapshot = m_snapshot;
    protectSnapshot.unlock();

    QSet<QString> processed;
    QStringList todo = projectFiles();

    while (! todo.isEmpty()) {
        QString fn = todo.last();
        todo.removeLast();

        if (processed.contains(fn))
            continue;

        processed.insert(fn);

        if (Document::Ptr doc = currentSnapshot.document(fn)) {
            todo += doc->includedFiles();
        }
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

    protectSnapshot.lock();
    m_snapshot = newSnapshot;
    protectSnapshot.unlock();
}


