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

#include "cppfindreferences.h"
#include "cpptoolsconstants.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/basefilefind.h>
#include <find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>

#include <ASTVisitor.h>
#include <AST.h>
#include <Control.h>
#include <Literals.h>
#include <TranslationUnit.h>
#include <Symbols.h>
#include <Names.h>
#include <Scope.h>

#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/FindUsages.h>

#include <QTime>
#include <QTimer>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QDir>
#include <QApplication>
#include <utils/runextensions.h>
#include <utils/textfileformat.h>

#include <functional>

using namespace CppTools::Internal;
using namespace CPlusPlus;

static QString getSource(const QString &fileName,
                         const CppModelManagerInterface::WorkingCopy &workingCopy)
{
    if (workingCopy.contains(fileName)) {
        return workingCopy.source(fileName);
    } else {
        QString fileContents;
        Utils::TextFileFormat format;
        QString error;
        QTextCodec *defaultCodec = Core::EditorManager::instance()->defaultTextCodec();
        Utils::TextFileFormat::ReadResult result = Utils::TextFileFormat::readFile(
                    fileName, defaultCodec, &fileContents, &format, &error);
        if (result != Utils::TextFileFormat::ReadSuccess)
            qWarning() << "Could not read " << fileName << ". Error: " << error;

        return fileContents;
    }
}

namespace {

class ProcessFile: public std::unary_function<QString, QList<Usage> >
{
    const CppModelManagerInterface::WorkingCopy workingCopy;
    const Snapshot snapshot;
    Document::Ptr symbolDocument;
    Symbol *symbol;
    QFutureInterface<Usage> *future;

public:
    ProcessFile(const CppModelManagerInterface::WorkingCopy &workingCopy,
                const Snapshot snapshot,
                Document::Ptr symbolDocument,
                Symbol *symbol,
                QFutureInterface<Usage> *future)
        : workingCopy(workingCopy),
          snapshot(snapshot),
          symbolDocument(symbolDocument),
          symbol(symbol),
          future(future)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;
        const Identifier *symbolId = symbol->identifier();

        if (Document::Ptr previousDoc = snapshot.document(fileName)) {
            Control *control = previousDoc->control();
            if (! control->findIdentifier(symbolId->chars(), symbolId->size()))
                return usages; // skip this document, it's not using symbolId.
        }
        Document::Ptr doc;
        const QString unpreprocessedSource = getSource(fileName, workingCopy);

        if (symbolDocument && fileName == symbolDocument->fileName()) {
            doc = symbolDocument;
        } else {
            doc = snapshot.preprocessedDocument(unpreprocessedSource, fileName);
            doc->tokenize();
        }

        Control *control = doc->control();
        if (control->findIdentifier(symbolId->chars(), symbolId->size()) != 0) {
            if (doc != symbolDocument)
                doc->check();

            FindUsages process(unpreprocessedSource.toUtf8(), doc, snapshot);
            process(symbol);

            usages = process.usages();
        }

        if (future->isPaused())
            future->waitForResume();
        return usages;
    }
};

class UpdateUI: public std::binary_function<QList<Usage> &, QList<Usage>, void>
{
    QFutureInterface<Usage> *future;

public:
    UpdateUI(QFutureInterface<Usage> *future): future(future) {}

    void operator()(QList<Usage> &, const QList<Usage> &usages)
    {
        foreach (const Usage &u, usages)
            future->reportResult(u);

        future->setProgressValue(future->progressValue() + 1);
    }
};

} // end of anonymous namespace

CppFindReferences::CppFindReferences(CppModelManagerInterface *modelManager)
    : QObject(modelManager),
      _modelManager(modelManager)
{
}

CppFindReferences::~CppFindReferences()
{
}

QList<int> CppFindReferences::references(Symbol *symbol, const LookupContext &context) const
{
    QList<int> references;

    FindUsages findUsages(context);
    findUsages(symbol);
    references = findUsages.references();

    return references;
}

static void find_helper(QFutureInterface<Usage> &future,
                        const CppModelManagerInterface::WorkingCopy workingCopy,
                        const LookupContext context,
                        CppFindReferences *findRefs,
                        Symbol *symbol)
{
    const Identifier *symbolId = symbol->identifier();
    QTC_ASSERT(symbolId != 0, return);

    const Snapshot snapshot = context.snapshot();

    const QString sourceFile = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    QStringList files(sourceFile);

    if (symbol->isClass() || symbol->isForwardClassDeclaration() || (symbol->enclosingScope() && ! symbol->isStatic() &&
                                                                     symbol->enclosingScope()->isNamespace())) {
        foreach (const Document::Ptr &doc, context.snapshot()) {
            if (doc->fileName() == sourceFile)
                continue;

            Control *control = doc->control();

            if (control->findIdentifier(symbolId->chars(), symbolId->size()))
                files.append(doc->fileName());
        }
    } else {
        DependencyTable dependencyTable = findRefs->updateDependencyTable(snapshot);
        files += dependencyTable.filesDependingOn(sourceFile);
    }
    files.removeDuplicates();

    future.setProgressRange(0, files.size());

    ProcessFile process(workingCopy, snapshot, context.thisDocument(), symbol, &future);
    UpdateUI reduce(&future);
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<QList<Usage> > (files, process, reduce);
    QThreadPool::globalInstance()->reserveThread();
    future.setProgressValue(files.size());
}

void CppFindReferences::findUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context)
{
    findUsages(symbol, context, QString(), false);
}

void CppFindReferences::findUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context,
                                   const QString &replacement,
                                   bool replace)
{
    Overview overview;
    Find::SearchResult *search = Find::SearchResultWindow::instance()->startNewSearch(tr("C++ Usages:"),
                                                QString(),
                                                overview.prettyName(context.fullyQualifiedName(symbol)),
                                                replace ? Find::SearchResultWindow::SearchAndReplace
                                                        : Find::SearchResultWindow::SearchOnly,
                                                QLatin1String("CppEditor"));
    search->setTextToReplace(replacement);
    connect(search, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>,bool)),
            SLOT(onReplaceButtonClicked(QString,QList<Find::SearchResultItem>,bool)));
    connect(search, SIGNAL(paused(bool)), this, SLOT(setPaused(bool)));
    search->setSearchAgainSupported(true);
    connect(search, SIGNAL(searchAgainRequested()), this, SLOT(searchAgain()));
    CppFindReferencesParameters parameters;
    parameters.context = context;
    parameters.symbol = symbol;
    search->setUserData(qVariantFromValue(parameters));
    findAll_helper(search);
}

void CppFindReferences::renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                                     const QString &replacement)
{
    if (const Identifier *id = symbol->identifier()) {
        const QString textToReplace = replacement.isEmpty()
                ? QString::fromUtf8(id->chars(), id->size()) : replacement;
        findUsages(symbol, context, textToReplace, true);
    }
}

void CppFindReferences::findAll_helper(Find::SearchResult *search)
{
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    if (!(parameters.symbol && parameters.symbol->identifier())) {
        search->finishSearch(false);
        return;
    }
    connect(search, SIGNAL(cancelled()), this, SLOT(cancel()));
    connect(search, SIGNAL(activated(Find::SearchResultItem)),
            this, SLOT(openEditor(Find::SearchResultItem)));

    Find::SearchResultWindow::instance()->popup(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus);
    const CppModelManagerInterface::WorkingCopy workingCopy = _modelManager->workingCopy();
    QFuture<Usage> result;
    result = QtConcurrent::run(&find_helper, workingCopy,
                               parameters.context, this, parameters.symbol);
    createWatcher(result, search);

    Core::ProgressManager *progressManager = Core::ICore::progressManager();
    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching"),
                                                              QLatin1String(CppTools::Constants::TASK_SEARCH));

    connect(progress, SIGNAL(clicked()), search, SLOT(popup()));
}

void CppFindReferences::onReplaceButtonClicked(const QString &text,
                                               const QList<Find::SearchResultItem> &items,
                                               bool preserveCase)
{
    const QStringList fileNames = TextEditor::BaseFileFind::replaceAll(text, items, preserveCase);
    if (!fileNames.isEmpty()) {
        _modelManager->updateSourceFiles(fileNames);
        Find::SearchResultWindow::instance()->hide();
    }
}

void CppFindReferences::searchAgain()
{
    Find::SearchResult *search = qobject_cast<Find::SearchResult *>(sender());
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    Snapshot snapshot = CppModelManagerInterface::instance()->snapshot();
    search->restart();
    if (!findSymbol(&parameters, snapshot)) {
        search->finishSearch(false);
        return;
    }
    search->setUserData(qVariantFromValue(parameters));
    findAll_helper(search);
}

static QByteArray typeId(Symbol *symbol)
{
    if (symbol->asEnum()) {
        return QByteArray("e");
    } else if (symbol->asFunction()) {
        return QByteArray("f");
    } else if (symbol->asNamespace()) {
        return QByteArray("n");
    } else if (symbol->asTemplate()) {
        return QByteArray("t");
    } else if (symbol->asNamespaceAlias()) {
        return QByteArray("na");
    } else if (symbol->asClass()) {
        return QByteArray("c");
    } else if (symbol->asBlock()) {
        return QByteArray("b");
    } else if (symbol->asUsingNamespaceDirective()) {
        return QByteArray("u");
    } else if (symbol->asUsingDeclaration()) {
        return QByteArray("ud");
    } else if (symbol->asDeclaration()) {
        QByteArray temp("d,");
        Overview pretty;
        temp.append(pretty.prettyType(symbol->type()).toLatin1());
        return temp;
    } else if (symbol->asArgument()) {
        return QByteArray("a");
    } else if (symbol->asTypenameArgument()) {
        return QByteArray("ta");
    } else if (symbol->asBaseClass()) {
        return QByteArray("bc");
    } else if (symbol->asForwardClassDeclaration()) {
        return QByteArray("fcd");
    } else if (symbol->asQtPropertyDeclaration()) {
        return QByteArray("qpd");
    } else if (symbol->asQtEnum()) {
        return QByteArray("qe");
    } else if (symbol->asObjCBaseClass()) {
        return QByteArray("ocbc");
    } else if (symbol->asObjCBaseProtocol()) {
        return QByteArray("ocbp");
    } else if (symbol->asObjCClass()) {
        return QByteArray("occ");
    } else if (symbol->asObjCForwardClassDeclaration()) {
        return QByteArray("ocfd");
    } else if (symbol->asObjCProtocol()) {
        return QByteArray("ocp");
    } else if (symbol->asObjCForwardProtocolDeclaration()) {
        return QByteArray("ocfpd");
    } else if (symbol->asObjCMethod()) {
        return QByteArray("ocm");
    } else if (symbol->asObjCPropertyDeclaration()) {
        return QByteArray("ocpd");
    }
    return QByteArray("unknown");
}

static QByteArray idForSymbol(Symbol *symbol)
{
    QByteArray uid(typeId(symbol));
    if (const Identifier *id = symbol->identifier()) {
        uid.append("|");
        uid.append(QByteArray(id->chars(), id->size()));
    } else if (Scope *scope = symbol->enclosingScope()) {
        // add the index of this symbol within its enclosing scope
        // (counting symbols without identifier of the same type)
        int count = 0;
        Scope::iterator it = scope->firstMember();
        while (it != scope->lastMember() && *it != symbol) {
            Symbol *val = *it;
            ++it;
            if (val->identifier() || typeId(val) != uid)
                continue;
            ++count;
        }
        uid.append(QString::number(count).toLocal8Bit());
    }
    return uid;
}

namespace {
class SymbolFinder : public SymbolVisitor
{
public:
    SymbolFinder(const QList<QByteArray> &uid) : m_uid(uid), m_index(0), m_result(0) { }
    Symbol *result() const { return m_result; }

    bool preVisit(Symbol *symbol)
    {
        if (m_result)
            return false;
        int index = m_index;
        if (symbol->asScope())
            ++m_index;
        if (index >= m_uid.size())
            return false;
        if (idForSymbol(symbol) != m_uid.at(index))
            return false;
        if (index == m_uid.size() - 1) {
            // symbol found
            m_result = symbol;
            return false;
        }
        return true;
    }

    void postVisit(Symbol *symbol)
    {
        if (symbol->asScope())
            --m_index;
    }

private:
    QList<QByteArray> m_uid;
    int m_index;
    Symbol *m_result;
};
}

bool CppFindReferences::findSymbol(CppFindReferencesParameters *parameters,
                                      const Snapshot &snapshot)
{
    QString symbolFile = QLatin1String(parameters->symbol->fileName());
    if (!snapshot.contains(symbolFile))
        return false;

    Document::Ptr newSymbolDocument = snapshot.document(symbolFile);
    // document is not parsed and has no bindings yet, do it
    QString source = getSource(newSymbolDocument->fileName(), _modelManager->workingCopy());
    Document::Ptr doc =
            snapshot.preprocessedDocument(source, newSymbolDocument->fileName());
    doc->check();

    // construct id of old symbol
    QList<QByteArray> uid;
    Symbol *current = parameters->symbol;
    do {
        uid.prepend(idForSymbol(current));
        current = current->enclosingScope();
    } while (current);
    // find matching symbol in new document and return the new parameters
    SymbolFinder finder(uid);
    finder.accept(doc->globalNamespace());
    if (finder.result()) {
        parameters->symbol = finder.result();
        parameters->context = LookupContext(doc, snapshot);
        return true;
    }
    return false;
}

void CppFindReferences::displayResults(int first, int last)
{
    QFutureWatcher<Usage> *watcher = static_cast<QFutureWatcher<Usage> *>(sender());
    Find::SearchResult *search = m_watchers.value(watcher);
    if (!search) {
        // search was deleted while it was running
        watcher->cancel();
        return;
    }
    for (int index = first; index != last; ++index) {
        Usage result = watcher->future().resultAt(index);
        search->addResult(result.path,
                          result.line,
                          result.lineText,
                          result.col,
                          result.len);
    }
}

void CppFindReferences::searchFinished()
{
    QFutureWatcher<Usage> *watcher = static_cast<QFutureWatcher<Usage> *>(sender());
    Find::SearchResult *search = m_watchers.value(watcher);
    if (search)
        search->finishSearch(watcher->isCanceled());
    m_watchers.remove(watcher);
}

void CppFindReferences::cancel()
{
    Find::SearchResult *search = qobject_cast<Find::SearchResult *>(sender());
    QTC_ASSERT(search, return);
    QFutureWatcher<Usage> *watcher = m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    watcher->cancel();
}

void CppFindReferences::setPaused(bool paused)
{
    Find::SearchResult *search = qobject_cast<Find::SearchResult *>(sender());
    QTC_ASSERT(search, return);
    QFutureWatcher<Usage> *watcher = m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
        watcher->setPaused(paused);
}

void CppFindReferences::openEditor(const Find::SearchResultItem &item)
{
    if (item.path.size() > 0) {
        TextEditor::BaseTextEditorWidget::openEditorAt(QDir::fromNativeSeparators(item.path.first()),
                                                       item.lineNumber, item.textMarkPos,
                                                       Core::Id(),
                                                       Core::EditorManager::ModeSwitch);
    } else {
        Core::EditorManager::openEditor(QDir::fromNativeSeparators(item.text),
                                        Core::Id(), Core::EditorManager::ModeSwitch);
    }
}


namespace {

class FindMacroUsesInFile: public std::unary_function<QString, QList<Usage> >
{
    const CppModelManagerInterface::WorkingCopy workingCopy;
    const Snapshot snapshot;
    const Macro &macro;
    QFutureInterface<Usage> *future;

public:
    FindMacroUsesInFile(const CppModelManagerInterface::WorkingCopy &workingCopy,
                        const Snapshot snapshot,
                        const Macro &macro,
                        QFutureInterface<Usage> *future)
        : workingCopy(workingCopy), snapshot(snapshot), macro(macro), future(future)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;
        Document::Ptr doc = snapshot.document(fileName);
        QString source;

restart_search:
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;

        usages.clear();
        foreach (const Document::MacroUse &use, doc->macroUses()) {
            const Macro &useMacro = use.macro();

            if (useMacro.fileName() == macro.fileName()) { // Check if this is a match, but possibly against an outdated document.
                if (source.isEmpty())
                    source = getSource(fileName, workingCopy);

                if (macro.fileRevision() > useMacro.fileRevision()) {
                    // yes, it is outdated, so re-preprocess and start from scratch for this file.
                    doc = snapshot.preprocessedDocument(source, fileName);
                    usages.clear();
                    goto restart_search;
                }

                if (macro.name() == useMacro.name()) {
                    unsigned lineStart;
                    const QString &lineSource = matchingLine(use.begin(), source, &lineStart);
                    usages.append(Usage(fileName, lineSource, use.beginLine(),
                                        use.begin() - lineStart, useMacro.name().length()));
                }
            }
        }

        if (future->isPaused())
            future->waitForResume();
        return usages;
    }

    static QString matchingLine(unsigned position, const QString &source,
                                unsigned *lineStart = 0)
    {
        int lineBegin = source.lastIndexOf(QLatin1Char('\n'), position) + 1;
        int lineEnd = source.indexOf(QLatin1Char('\n'), position);
        if (lineEnd == -1)
            lineEnd = source.length();

        if (lineStart)
            *lineStart = lineBegin;

        const QString matchingLine = source.mid(lineBegin, lineEnd - lineBegin);
        return matchingLine;
    }
};

} // end of anonymous namespace

static void findMacroUses_helper(QFutureInterface<Usage> &future,
                                 const CppModelManagerInterface::WorkingCopy workingCopy,
                                 const Snapshot snapshot,
                                 CppFindReferences *findRefs,
                                 const Macro macro)
{
    // ensure the dependency table is updated
    DependencyTable dependencies = findRefs->updateDependencyTable(snapshot);

    const QString& sourceFile = macro.fileName();
    QStringList files(sourceFile);
    files += dependencies.filesDependingOn(sourceFile);
    files.removeDuplicates();

    future.setProgressRange(0, files.size());
    FindMacroUsesInFile process(workingCopy, snapshot, macro, &future);
    UpdateUI reduce(&future);
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<QList<Usage> > (files, process, reduce);
    QThreadPool::globalInstance()->reserveThread();
    future.setProgressValue(files.size());
}

void CppFindReferences::findMacroUses(const Macro &macro)
{
    findMacroUses(macro, QString(), false);
}

void CppFindReferences::findMacroUses(const Macro &macro, const QString &replacement, bool replace)
{
    Find::SearchResult *search = Find::SearchResultWindow::instance()->startNewSearch(
                tr("C++ Macro Usages:"),
                QString(),
                QString::fromUtf8(macro.name()),
                replace ? Find::SearchResultWindow::SearchAndReplace
                        : Find::SearchResultWindow::SearchOnly,
                QLatin1String("CppEditor"));

    search->setTextToReplace(replacement);
    connect(search, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>,bool)),
            SLOT(onReplaceButtonClicked(QString,QList<Find::SearchResultItem>,bool)));

    Find::SearchResultWindow::instance()->popup(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus);

    connect(search, SIGNAL(activated(Find::SearchResultItem)),
            this, SLOT(openEditor(Find::SearchResultItem)));
    connect(search, SIGNAL(cancelled()), this, SLOT(cancel()));
    connect(search, SIGNAL(paused(bool)), this, SLOT(setPaused(bool)));

    const Snapshot snapshot = _modelManager->snapshot();
    const CppModelManagerInterface::WorkingCopy workingCopy = _modelManager->workingCopy();

    // add the macro definition itself
    {
        const QString &source = getSource(macro.fileName(), workingCopy);
        unsigned lineStart;
        const QString line = FindMacroUsesInFile::matchingLine(macro.offset(), source, &lineStart);
        search->addResult(macro.fileName(), macro.line(), line,
                          macro.offset() - lineStart, macro.name().length());
    }

    QFuture<Usage> result;
    result = QtConcurrent::run(&findMacroUses_helper, workingCopy, snapshot, this, macro);
    createWatcher(result, search);

    Core::ProgressManager *progressManager = Core::ICore::progressManager();
    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching"),
                                                              QLatin1String(CppTools::Constants::TASK_SEARCH));
    connect(progress, SIGNAL(clicked()), search, SLOT(popup()));
}

void CppFindReferences::renameMacroUses(const Macro &macro, const QString &replacement)
{
    const QString textToReplace = replacement.isEmpty() ? QString::fromUtf8(macro.name()) : replacement;
    findMacroUses(macro, textToReplace, true);
}

DependencyTable CppFindReferences::updateDependencyTable(CPlusPlus::Snapshot snapshot)
{
    DependencyTable oldDeps = dependencyTable();
    if (oldDeps.isValidFor(snapshot))
        return oldDeps;

    DependencyTable newDeps;
    newDeps.build(snapshot);
    setDependencyTable(newDeps);
    return newDeps;
}

DependencyTable CppFindReferences::dependencyTable() const
{
    QMutexLocker locker(&m_depsLock);
    Q_UNUSED(locker);
    return m_deps;
}

void CppFindReferences::setDependencyTable(const CPlusPlus::DependencyTable &newTable)
{
    QMutexLocker locker(&m_depsLock);
    Q_UNUSED(locker);
    m_deps = newTable;
}

void CppFindReferences::createWatcher(const QFuture<Usage> &future, Find::SearchResult *search)
{
    QFutureWatcher<Usage> *watcher = new QFutureWatcher<Usage>();
    watcher->setPendingResultsLimit(1);
    connect(watcher, SIGNAL(resultsReadyAt(int,int)), this, SLOT(displayResults(int,int)));
    connect(watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
    m_watchers.insert(watcher, search);
    watcher->setFuture(future);
}
