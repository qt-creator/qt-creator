/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <cplusplus/pp.h>
#include <cplusplus/Overview.h>

#include "cppmodelmanager.h"
#include "cppcompletionassist.h"
#include "cpphighlightingsupport.h"
#include "cpphighlightingsupportinternal.h"
#include "abstracteditorsupport.h"
#ifndef ICHECK_BUILD
#  include "cpptoolsconstants.h"
#  include "cpptoolseditorsupport.h"
#  include "cppfindreferences.h"
#endif

#include <functional>
#include <QtConcurrentRun>
#ifndef ICHECK_BUILD
#  include <QFutureSynchronizer>
#  include <utils/runextensions.h>
#  include <texteditor/itexteditor.h>
#  include <texteditor/basetexteditor.h>
#  include <projectexplorer/project.h>
#  include <projectexplorer/projectexplorer.h>
#  include <projectexplorer/projectexplorerconstants.h>
#  include <projectexplorer/session.h>
#  include <coreplugin/icore.h>
#  include <coreplugin/mimedatabase.h>
#  include <coreplugin/editormanager/editormanager.h>
#  include <coreplugin/progressmanager/progressmanager.h>
#  include <extensionsystem/pluginmanager.h>
#else
#  include <QDir>
#endif

#include <utils/qtcassert.h>

#include <TranslationUnit.h>
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
#include <Parser.h>
#include <Control.h>
#include <CoreTypes.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QTime>
#include <QTimer>
#include <QtConcurrentMap>

#include <QTextBlock>

#include <iostream>
#include <sstream>

namespace CPlusPlus {
uint qHash(const CppModelManagerInterface::ProjectPart &p)
{
    uint h = qHash(p.defines) ^ p.language ^ ((int) p.cxx11Enabled);

    foreach (const QString &i, p.includePaths)
        h ^= qHash(i);

    foreach (const QString &f, p.frameworkPaths)
        h ^= qHash(f);

    return h;
}
bool operator==(const CppModelManagerInterface::ProjectPart &p1,
                const CppModelManagerInterface::ProjectPart &p2)
{
    if (p1.defines != p2.defines)
        return false;
    if (p1.language != p2.language)
        return false;
    if (p1.cxx11Enabled != p2.cxx11Enabled)
        return false;
    if (p1.includePaths != p2.includePaths)
        return false;
    return p1.frameworkPaths == p2.frameworkPaths;
}
} // namespace CPlusPlus

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

#ifndef ICHECK_BUILD
CppPreprocessor::CppPreprocessor(QPointer<CppModelManager> modelManager, bool dumpFileNameWhileParsing)
    : snapshot(modelManager->snapshot()),
      m_modelManager(modelManager),
      m_dumpFileNameWhileParsing(dumpFileNameWhileParsing),
      preprocess(this, &env),
      m_revision(0)
{
    preprocess.setKeepComments(true);
}

#else

CppPreprocessor::CppPreprocessor(QPointer<CPlusPlus::ParseManager> modelManager)
    : preprocess(this, &env),
      m_dumpFileNameWhileParsing(false),
      m_revision(0)
{
}
#endif

CppPreprocessor::~CppPreprocessor()
{ }

void CppPreprocessor::setRevision(unsigned revision)
{ m_revision = revision; }

void CppPreprocessor::setWorkingCopy(const CppModelManagerInterface::WorkingCopy &workingCopy)
{ m_workingCopy = workingCopy; }

void CppPreprocessor::setIncludePaths(const QStringList &includePaths)
{
    m_includePaths.clear();

    for (int i = 0; i < includePaths.size(); ++i) {
        const QString &path = includePaths.at(i);

#ifdef Q_OS_DARWIN
        if (i + 1 < includePaths.size() && path.endsWith(QLatin1String(".framework/Headers"))) {
            const QFileInfo pathInfo(path);
            const QFileInfo frameworkFileInfo(pathInfo.path());
            const QString frameworkName = frameworkFileInfo.baseName();

            const QFileInfo nextIncludePath = includePaths.at(i + 1);
            if (nextIncludePath.fileName() == frameworkName) {
                // We got a QtXXX.framework/Headers followed by $QTDIR/include/QtXXX.
                // In this case we prefer to include files from $QTDIR/include/QtXXX.
                continue;
            }
        }
        m_includePaths.append(path);
#else
        m_includePaths.append(path);
#endif
    }
}

void CppPreprocessor::setFrameworkPaths(const QStringList &frameworkPaths)
{
    m_frameworkPaths.clear();

    foreach (const QString &frameworkPath, frameworkPaths) {
        addFrameworkPath(frameworkPath);
    }
}

// Add the given framework path, and expand private frameworks.
//
// Example:
//  <framework-path>/ApplicationServices.framework
// has private frameworks in:
//  <framework-path>/ApplicationServices.framework/Frameworks
// if the "Frameworks" folder exists inside the top level framework.
void CppPreprocessor::addFrameworkPath(const QString &frameworkPath)
{
    // The algorithm below is a bit too eager, but that's because we're not getting
    // in the frameworks we're linking against. If we would have that, then we could
    // add only those private frameworks.
    if (!m_frameworkPaths.contains(frameworkPath)) {
        m_frameworkPaths.append(frameworkPath);
    }

    const QDir frameworkDir(frameworkPath);
    const QStringList filter = QStringList() << QLatin1String("*.framework");
    foreach (const QFileInfo &framework, frameworkDir.entryInfoList(filter)) {
        if (!framework.isDir())
            continue;
        const QFileInfo privateFrameworks(framework.absoluteFilePath(), QLatin1String("Frameworks"));
        if (privateFrameworks.exists() && privateFrameworks.isDir()) {
            addFrameworkPath(privateFrameworks.absoluteFilePath());
        }
    }
}

void CppPreprocessor::setProjectFiles(const QStringList &files)
{ m_projectFiles = files; }

void CppPreprocessor::setTodo(const QStringList &files)
{ m_todo = QSet<QString>::fromList(files); }

#ifndef ICHECK_BUILD
namespace {
class Process: public std::unary_function<Document::Ptr, void>
{
    QPointer<CppModelManager> _modelManager;
    Snapshot _snapshot;
    Document::Ptr _doc;
    Document::CheckMode _mode;

public:
    Process(QPointer<CppModelManager> modelManager,
            Document::Ptr doc,
            const Snapshot &snapshot,
            const CppModelManager::WorkingCopy &workingCopy)
        : _modelManager(modelManager),
          _snapshot(snapshot),
          _doc(doc),
          _mode(Document::FastCheck)
    {

        if (workingCopy.contains(_doc->fileName()))
            _mode = Document::FullCheck;
    }

    void operator()()
    {
        _doc->check(_mode);

        if (_modelManager)
            _modelManager->emitDocumentUpdated(_doc); // ### TODO: compress

        _doc->releaseSourceAndAST();
    }
};
} // end of anonymous namespace
#endif

void CppPreprocessor::run(const QString &fileName)
{
    QString absoluteFilePath = fileName;
    sourceNeeded(0, absoluteFilePath, IncludeGlobal);
}

void CppPreprocessor::resetEnvironment()
{
    env.reset();
    m_processed.clear();
}

bool CppPreprocessor::includeFile(const QString &absoluteFilePath, QString *result, unsigned *revision)
{
    if (absoluteFilePath.isEmpty() || m_included.contains(absoluteFilePath))
        return true;

    if (m_workingCopy.contains(absoluteFilePath)) {
        m_included.insert(absoluteFilePath);
        const QPair<QString, unsigned> r = m_workingCopy.get(absoluteFilePath);
        *result = r.first;
        *revision = r.second;
        return true;
    }

    QFileInfo fileInfo(absoluteFilePath);
    if (! fileInfo.isFile())
        return false;

    QFile file(absoluteFilePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        m_included.insert(absoluteFilePath);
        QTextCodec *defaultCodec = Core::EditorManager::instance()->defaultTextCodec();
        QTextStream stream(&file);
        stream.setCodec(defaultCodec);
        if (result)
            *result = stream.readAll();
        file.close();
        return true;
    }

    return false;
}

QString CppPreprocessor::tryIncludeFile(QString &fileName, IncludeType type, unsigned *revision)
{
    if (type == IncludeGlobal) {
        const QString fn = m_fileNameCache.value(fileName);

        if (! fn.isEmpty()) {
            fileName = fn;

            if (revision)
                *revision = 0;

            return QString();
        }
    }

    const QString originalFileName = fileName;
    const QString contents = tryIncludeFile_helper(fileName, type, revision);
    if (type == IncludeGlobal)
        m_fileNameCache.insert(originalFileName, fileName);
    return contents;
}

static inline void appendDirSeparatorIfNeeded(QString &path)
{
    if (!path.endsWith(QLatin1Char('/'), Qt::CaseInsensitive))
        path += QLatin1Char('/');
}

QString CppPreprocessor::tryIncludeFile_helper(QString &fileName, IncludeType type, unsigned *revision)
{
    QFileInfo fileInfo(fileName);
    if (fileName == QLatin1String(pp_configuration_file) || fileInfo.isAbsolute()) {
        QString contents;
        includeFile(fileName, &contents, revision);
        return contents;
    }

    if (type == IncludeLocal && m_currentDoc) {
        QFileInfo currentFileInfo(m_currentDoc->fileName());
        QString path = currentFileInfo.absolutePath();
        appendDirSeparatorIfNeeded(path);
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
        if (includeFile(path, &contents, revision)) {
            fileName = path;
            return contents;
        }
    }

    foreach (const QString &includePath, m_includePaths) {
        QString path = includePath;
        appendDirSeparatorIfNeeded(path);
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
        if (includeFile(path, &contents, revision)) {
            fileName = path;
            return contents;
        }
    }

    // look in the system include paths
    foreach (const QString &includePath, m_systemIncludePaths) {
        QString path = includePath;
        appendDirSeparatorIfNeeded(path);
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
        if (includeFile(path, &contents, revision)) {
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
            appendDirSeparatorIfNeeded(path);
            path += frameworkName;
            path += QLatin1String(".framework/Headers/");
            path += name;
            path = QDir::cleanPath(path);
            QString contents;
            if (includeFile(path, &contents, revision)) {
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
            includeFile(fileName, &contents, revision);
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

static inline const Macro revision(const CppModelManagerInterface::WorkingCopy &s, const Macro &macro)
{
    Macro newMacro(macro);
    newMacro.setFileRevision(s.get(macro.fileName()).second);
    return newMacro;
}

void CppPreprocessor::passedMacroDefinitionCheck(unsigned offset, unsigned line, const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro), offset, macro.name().length(), line,
                              QVector<MacroArgumentReference>());
}

void CppPreprocessor::failedMacroDefinitionCheck(unsigned offset, const ByteArrayRef &name)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()), offset);
}

void CppPreprocessor::notifyMacroReference(unsigned offset, unsigned line, const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro), offset, macro.name().length(), line,
                              QVector<MacroArgumentReference>());
}

void CppPreprocessor::startExpandingMacro(unsigned offset, unsigned line,
                                          const Macro &macro,
                                          const QVector<MacroArgumentReference> &actuals)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro), offset, macro.name().length(), line, actuals);
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

void CppPreprocessor::sourceNeeded(unsigned line, QString &fileName, IncludeType type)
{
    if (fileName.isEmpty())
        return;

    unsigned editorRevision = 0;
    QString contents = tryIncludeFile(fileName, type, &editorRevision);
    fileName = QDir::cleanPath(fileName);
    if (m_currentDoc) {
        m_currentDoc->addIncludeFile(fileName, line);

        if (contents.isEmpty() && ! QFileInfo(fileName).isAbsolute()) {
            QString msg = QCoreApplication::translate(
                    "CppPreprocessor", "%1: No such file or directory").arg(fileName);

            Document::DiagnosticMessage d(Document::DiagnosticMessage::Warning,
                                          m_currentDoc->fileName(),
                                          line, /*column = */ 0,
                                          msg);

            m_currentDoc->addDiagnosticMessage(d);

            //qWarning() << "file not found:" << fileName << m_currentDoc->fileName() << env.current_line;
        }
    }

    if (m_dumpFileNameWhileParsing) {
        qDebug() << "Parsing file:" << fileName
//             << "contents:" << contents.size()
                    ;
    }

    Document::Ptr doc = snapshot.document(fileName);
    if (doc) {
        mergeEnvironment(doc);
        return;
    }

    doc = Document::create(fileName);
    doc->setRevision(m_revision);
    doc->setEditorRevision(editorRevision);

    QFileInfo info(fileName);
    if (info.exists())
        doc->setLastModified(info.lastModified());

    Document::Ptr previousDoc = switchDocument(doc);

    const QByteArray preprocessedCode = preprocess.run(fileName, contents);

//    { QByteArray b(preprocessedCode); b.replace("\n", "<<<\n"); qDebug("Preprocessed code for \"%s\": [[%s]]", fileName.toUtf8().constData(), b.constData()); }

    doc->setUtf8Source(preprocessedCode);
    doc->keepSourceAndAST();
    doc->tokenize();

    snapshot.insert(doc);
    m_todo.remove(fileName);

#ifndef ICHECK_BUILD
    Process process(m_modelManager, doc, snapshot, m_workingCopy);

    process();

    (void) switchDocument(previousDoc);
#else
    doc->releaseSource();
    Document::CheckMode mode = Document::FastCheck;
    mode = Document::FullCheck;
    doc->parse();
    doc->check(mode);

    (void) switchDocument(previousDoc);
#endif
}

Document::Ptr CppPreprocessor::switchDocument(Document::Ptr doc)
{
    Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}

#ifndef ICHECK_BUILD
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

CppModelManager *CppModelManager::instance()
{
    // TODO this is pretty stupid. use regular singleton pattern.
    return ExtensionSystem::PluginManager::getObject<CppModelManager>();
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
    m_dumpFileNameWhileParsing = !qgetenv("QTCREATOR_DUMP_FILENAME_WHILE_PARSING").isNull();

    m_revision = 0;
    m_synchronizer.setCancelOnWait(true);

    m_dirty = true;

    ProjectExplorer::ProjectExplorerPlugin *pe =
       ProjectExplorer::ProjectExplorerPlugin::instance();

    QTC_ASSERT(pe, return);

    ProjectExplorer::SessionManager *session = pe->session();
    m_updateEditorSelectionsTimer = new QTimer(this);
    m_updateEditorSelectionsTimer->setInterval(500);
    m_updateEditorSelectionsTimer->setSingleShot(true);
    connect(m_updateEditorSelectionsTimer, SIGNAL(timeout()),
            this, SLOT(updateEditorSelections()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(onProjectAdded(ProjectExplorer::Project*)));

    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));

    connect(session, SIGNAL(aboutToUnloadSession(QString)),
            this, SLOT(onAboutToUnloadSession()));

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");

    // thread connections
    connect(this, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    connect(this, SIGNAL(extraDiagnosticsUpdated(QString)),
            this, SLOT(onExtraDiagnosticsUpdated(QString)));

    // Listen for editor closed and opened events so that we can keep track of changing files
    connect(Core::ICore::editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    connect(Core::ICore::editorManager(), SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));

    m_completionFallback = new InternalCompletionAssistProvider;
    m_completionAssistProvider = m_completionFallback;
    ExtensionSystem::PluginManager::addObject(m_completionAssistProvider);
    m_highlightingFallback = new CppHighlightingSupportInternalFactory;
    m_highlightingFactory = m_highlightingFallback;
}

CppModelManager::~CppModelManager()
{
    ExtensionSystem::PluginManager::removeObject(m_completionAssistProvider);
    delete m_completionFallback;
    delete m_highlightingFallback;
}

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
        foreach (const ProjectPart::Ptr &part, pinfo.projectParts())
            files += part->sourceFiles;
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
            includePaths += part->includePaths;
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
            frameworkPaths += part->frameworkPaths;
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

void CppModelManager::addEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.insert(editorSupport);
}

void CppModelManager::removeEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.remove(editorSupport);
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

CppModelManager::WorkingCopy CppModelManager::buildWorkingCopyList()
{
    WorkingCopy workingCopy;
    QMapIterator<TextEditor::ITextEditor *, CppEditorSupport *> it(m_editorSupport);
    while (it.hasNext()) {
        it.next();
        TextEditor::ITextEditor *textEditor = it.key();
        CppEditorSupport *editorSupport = it.value();
        QString fileName = textEditor->document()->fileName();
        workingCopy.insert(fileName, editorSupport->contents(), editorSupport->editorRevision());
    }

    QSetIterator<AbstractEditorSupport *> jt(m_addtionalEditorSupport);
    while (jt.hasNext()) {
        AbstractEditorSupport *es =  jt.next();
        workingCopy.insert(es->fileName(), es->contents());
    }

    // add the project configuration file
    QByteArray conf(pp_configuration);
    conf += definedMacros();
    workingCopy.insert(pp_configuration_file, conf);

    return workingCopy;
}

CppModelManager::WorkingCopy CppModelManager::workingCopy() const
{
    return const_cast<CppModelManager *>(this)->buildWorkingCopyList();
}

QFuture<void> CppModelManager::updateSourceFiles(const QStringList &sourceFiles)
{ return refreshSourceFiles(sourceFiles); }

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
#if 0
    // Tons of debug output...
    qDebug()<<"========= CppModelManager::updateProjectInfo ======";
    qDebug()<<" for project:"<< pinfo.project().data()->document()->fileName();
    foreach (const ProjectPart::Ptr &part, pinfo.projectParts()) {
        qDebug() << "=== part ===";
        qDebug() << "language:" << (part->language == CXX ? "C++" : "ObjC++");
        qDebug() << "C++11:" << part->cxx11Enabled;
        qDebug() << "Qt version:" << part->qtVersion;
        qDebug() << "precompiled header:" << part->precompiledHeaders;
        qDebug() << "defines:" << part->defines;
        qDebug() << "includes:" << part->includePaths;
        qDebug() << "frameworkPaths:" << part->frameworkPaths;
        qDebug() << "sources:" << part->sourceFiles;
        qDebug() << "";
    }

    qDebug() << "";
#endif
    QMutexLocker locker(&mutex);

    if (! pinfo.isValid())
        return;

    ProjectExplorer::Project *project = pinfo.project().data();
    m_projects.insert(project, pinfo);
    m_dirty = true;

    m_srcToProjectPart.clear();

    foreach (const ProjectInfo &projectInfo, m_projects.values())
        foreach (const ProjectPart::Ptr &projectPart, projectInfo.projectParts())
            foreach (const QString &sourceFile, projectPart->sourceFiles)
                m_srcToProjectPart[sourceFile].append(projectPart);
}

QList<CppModelManager::ProjectPart::Ptr> CppModelManager::projectPart(const QString &fileName) const
{
    QList<CppModelManager::ProjectPart::Ptr> parts = m_srcToProjectPart.value(fileName);
    if (!parts.isEmpty())
        return parts;

    //### FIXME: This is a DIRTY hack!
    if (fileName.endsWith(".h")) {
        QString cppFile = fileName.mid(0, fileName.length() - 2) + QLatin1String(".cpp");
        parts = m_srcToProjectPart.value(cppFile);
        if (!parts.isEmpty())
            return parts;
    }

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

QFuture<void> CppModelManager::refreshSourceFiles(const QStringList &sourceFiles)
{
    if (! sourceFiles.isEmpty() && m_indexerEnabled) {
        const WorkingCopy workingCopy = buildWorkingCopyList();

        CppPreprocessor *preproc = new CppPreprocessor(this, m_dumpFileNameWhileParsing);
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

            foreach (const QFuture<void> &future, futures) {
                if (! (future.isFinished() || future.isCanceled()))
                    m_synchronizer.addFuture(future);
            }
        }

        m_synchronizer.addFuture(result);

        if (sourceFiles.count() > 1) {
            Core::ICore::progressManager()->addTask(result, tr("Parsing"),
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
    return editor->context().contains(ProjectExplorer::Constants::LANG_CXX);
}

void CppModelManager::emitDocumentUpdated(Document::Ptr doc)
{
    emit documentUpdated(doc);
}

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

    updateEditor(doc);
}

void CppModelManager::onExtraDiagnosticsUpdated(const QString &fileName)
{
    protectSnapshot.lock();
    Document::Ptr doc = m_snapshot.document(fileName);
    protectSnapshot.unlock();
    if (doc)
        updateEditor(doc);
}

void CppModelManager::updateEditor(Document::Ptr doc)
{
    const QString fileName = doc->fileName();

    QList<Core::IEditor *> openedEditors = Core::ICore::editorManager()->openedEditors();
    foreach (Core::IEditor *editor, openedEditors) {
        if (editor->document()->fileName() == fileName) {
            TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
            if (! textEditor)
                continue;

            TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(textEditor->widget());
            if (! ed)
                continue;

            QList<TextEditor::BaseTextEditorWidget::BlockRange> blockRanges;

            foreach (const Document::Block &block, doc->skippedBlocks()) {
                blockRanges.append(TextEditor::BaseTextEditorWidget::BlockRange(block.begin(), block.end()));
            }

            // set up the format for the errors
            QTextCharFormat errorFormat;
            errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            errorFormat.setUnderlineColor(Qt::red);

            // set up the format for the warnings.
            QTextCharFormat warningFormat;
            warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            warningFormat.setUnderlineColor(Qt::darkYellow);

            QList<Editor> todo;
            foreach (const Editor &e, m_todo) {
                if (e.textEditor != textEditor)
                    todo.append(e);
            }
            Editor e;

            if (m_highlightingFactory->hightlighterHandlesDiagnostics()) {
                e.updateSelections = false;
            } else {
                QSet<int> lines;
                QList<Document::DiagnosticMessage> messages = doc->diagnosticMessages();
                messages += extraDiagnostics(doc->fileName());
                foreach (const Document::DiagnosticMessage &m, messages) {
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
                    if (m.length() > 0 && m.column() + m.length() < (unsigned)text.size()) {
                        int column = m.column() > 0 ? m.column() - 1 : 0;
                        c.setPosition(c.position() + column);
                        c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, m.length());
                    } else {
                        for (int i = 0; i < text.size(); ++i) {
                            if (! text.at(i).isSpace()) {
                                c.setPosition(c.position() + i);
                                break;
                            }
                        }
                        c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    }
                    sel.cursor = c;
                    sel.format.setToolTip(m.text());
                    e.selections.append(sel);
                }
            }


            e.revision = ed->document()->revision();
            e.textEditor = textEditor;
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
        TextEditor::BaseTextEditorWidget *editor = qobject_cast<TextEditor::BaseTextEditorWidget *>(textEditor->widget());

        if (! editor)
            continue;
        else if (editor->document()->revision() != ed.revision)
            continue; // outdated

        if (ed.updateSelections)
            editor->setExtraSelections(TextEditor::BaseTextEditorWidget::CodeWarningsSelection,
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
    if (Core::ProgressManager *pm = Core::ICore::progressManager()) {
        pm->cancelTasks(CppTools::Constants::TASK_INDEX);
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
    if (files.isEmpty())
        return;

    const Core::MimeDatabase *mimeDb = Core::ICore::mimeDatabase();
    Core::MimeType cSourceTy = mimeDb->findByType(QLatin1String("text/x-csrc"));
    Core::MimeType cppSourceTy = mimeDb->findByType(QLatin1String("text/x-c++src"));
    Core::MimeType mSourceTy = mimeDb->findByType(QLatin1String("text/x-objcsrc"));

    QStringList sources;
    QStringList headers;

    QStringList suffixes = cSourceTy.suffixes();
    suffixes += cppSourceTy.suffixes();
    suffixes += mSourceTy.suffixes();

    foreach (const QString &file, files) {
        QFileInfo info(file);

        preproc->snapshot.remove(file);

        if (suffixes.contains(info.suffix()))
            sources.append(file);
        else
            headers.append(file);
    }

    const int sourceCount = sources.size();
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

        const QString fileName = files.at(i);

        const bool isSourceFile = i < sourceCount;
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
    }

    future.setProgressValue(files.size());
    preproc->modelManager()->finishedRefreshingSourceFiles(files);

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

void CppModelManager::setExtraDiagnostics(const QString &fileName, int kind,
                                          const QList<Document::DiagnosticMessage> &diagnostics)
{
    {
        QMutexLocker locker(&protectExtraDiagnostics);
        if (m_extraDiagnostics[fileName][kind] == diagnostics)
            return;
        m_extraDiagnostics[fileName].insert(kind, diagnostics);
    }
    emit extraDiagnosticsUpdated(fileName);
}

QList<Document::DiagnosticMessage> CppModelManager::extraDiagnostics(const QString &fileName, int kind) const
{
    QMutexLocker locker(&protectExtraDiagnostics);
    if (kind == -1) {
        QList<Document::DiagnosticMessage> messages;
        foreach (const QList<Document::DiagnosticMessage> &list, m_extraDiagnostics.value(fileName))
            messages += list;
        return messages;
    }
    return m_extraDiagnostics.value(fileName).value(kind);
}

#endif

