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

#include "cppfindreferences.h"

#include "cpptoolsconstants.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <texteditor/basefilefind.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/textfileformat.h>

#include <cplusplus/Overview.h>
#include <QtConcurrentMap>
#include <QDir>

#include <functional>

using namespace Core;
using namespace CppTools::Internal;
using namespace CppTools;
using namespace CPlusPlus;

static QByteArray getSource(const Utils::FileName &fileName,
                            const WorkingCopy &workingCopy)
{
    if (workingCopy.contains(fileName)) {
        return workingCopy.source(fileName);
    } else {
        QString fileContents;
        Utils::TextFileFormat format;
        QString error;
        QTextCodec *defaultCodec = EditorManager::defaultTextCodec();
        Utils::TextFileFormat::ReadResult result = Utils::TextFileFormat::readFile(
                    fileName.toString(), defaultCodec, &fileContents, &format, &error);
        if (result != Utils::TextFileFormat::ReadSuccess)
            qWarning() << "Could not read " << fileName << ". Error: " << error;

        return fileContents.toUtf8();
    }
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
        temp.append(pretty.prettyType(symbol->type()).toUtf8());
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
        Scope::iterator it = scope->memberBegin();
        while (it != scope->memberEnd() && *it != symbol) {
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

static QList<QByteArray> fullIdForSymbol(Symbol *symbol)
{
    QList<QByteArray> uid;
    Symbol *current = symbol;
    do {
        uid.prepend(idForSymbol(current));
        current = current->enclosingScope();
    } while (current);
    return uid;
}

namespace {

class ProcessFile: public std::unary_function<QString, QList<Usage> >
{
    const WorkingCopy workingCopy;
    const Snapshot snapshot;
    Document::Ptr symbolDocument;
    Symbol *symbol;
    QFutureInterface<Usage> *future;

public:
    ProcessFile(const WorkingCopy &workingCopy,
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

    QList<Usage> operator()(const Utils::FileName &fileName)
    {
        QList<Usage> usages;
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;
        const Identifier *symbolId = symbol->identifier();

        if (Document::Ptr previousDoc = snapshot.document(fileName)) {
            Control *control = previousDoc->control();
            if (!control->findIdentifier(symbolId->chars(), symbolId->size()))
                return usages; // skip this document, it's not using symbolId.
        }
        Document::Ptr doc;
        const QByteArray unpreprocessedSource = getSource(fileName, workingCopy);

        if (symbolDocument && fileName == Utils::FileName::fromString(symbolDocument->fileName())) {
            doc = symbolDocument;
        } else {
            doc = snapshot.preprocessedDocument(unpreprocessedSource, fileName);
            doc->tokenize();
        }

        Control *control = doc->control();
        if (control->findIdentifier(symbolId->chars(), symbolId->size()) != 0) {
            if (doc != symbolDocument)
                doc->check();

            FindUsages process(unpreprocessedSource, doc, snapshot);
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

CppFindReferences::CppFindReferences(CppModelManager *modelManager)
    : QObject(modelManager),
      m_modelManager(modelManager)
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
                        const WorkingCopy workingCopy,
                        const LookupContext context,
                        Symbol *symbol)
{
    const Identifier *symbolId = symbol->identifier();
    QTC_ASSERT(symbolId != 0, return);

    const Snapshot snapshot = context.snapshot();

    const Utils::FileName sourceFile = Utils::FileName::fromUtf8(symbol->fileName(),
                                                                 symbol->fileNameLength());
    Utils::FileNameList files{sourceFile};

    if (symbol->isClass()
        || symbol->isForwardClassDeclaration()
        || (symbol->enclosingScope()
            && !symbol->isStatic()
            && symbol->enclosingScope()->isNamespace())) {
        const Snapshot snapshotFromContext = context.snapshot();
        for (auto i = snapshotFromContext.begin(), ei = snapshotFromContext.end(); i != ei; ++i) {
            if (i.key() == sourceFile)
                continue;

            const Control *control = i.value()->control();

            if (control->findIdentifier(symbolId->chars(), symbolId->size()))
                files.append(i.key());
        }
    } else {
        files += snapshot.filesDependingOn(sourceFile);
    }
    files = Utils::filteredUnique(files);

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

void CppFindReferences::findUsages(Symbol *symbol, const LookupContext &context)
{
    findUsages(symbol, context, QString(), false);
}

void CppFindReferences::findUsages(Symbol *symbol,
                                   const LookupContext &context,
                                   const QString &replacement,
                                   bool replace)
{
    Overview overview;
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(tr("C++ Usages:"),
                                                QString(),
                                                overview.prettyName(context.fullyQualifiedName(symbol)),
                                                replace ? SearchResultWindow::SearchAndReplace
                                                        : SearchResultWindow::SearchOnly,
                                                SearchResultWindow::PreserveCaseDisabled,
                                                QLatin1String("CppEditor"));
    search->setTextToReplace(replacement);
    connect(search, &SearchResult::replaceButtonClicked,
            this, &CppFindReferences::onReplaceButtonClicked);
    search->setSearchAgainSupported(true);
    connect(search, &SearchResult::searchAgainRequested, this, &CppFindReferences::searchAgain);
    CppFindReferencesParameters parameters;
    parameters.symbolId = fullIdForSymbol(symbol);
    parameters.symbolFileName = QByteArray(symbol->fileName());
    search->setUserData(qVariantFromValue(parameters));
    findAll_helper(search, symbol, context);
}

void CppFindReferences::renameUsages(Symbol *symbol, const LookupContext &context,
                                     const QString &replacement)
{
    if (const Identifier *id = symbol->identifier()) {
        const QString textToReplace = replacement.isEmpty()
                ? QString::fromUtf8(id->chars(), id->size()) : replacement;
        findUsages(symbol, context, textToReplace, true);
    }
}

void CppFindReferences::findAll_helper(SearchResult *search, Symbol *symbol,
                                       const LookupContext &context)
{
    if (!(symbol && symbol->identifier())) {
        search->finishSearch(false);
        return;
    }
    connect(search, &SearchResult::activated,
            this, &CppFindReferences::openEditor);

    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    const WorkingCopy workingCopy = m_modelManager->workingCopy();
    QFuture<Usage> result;
    result = Utils::runAsync(m_modelManager->sharedThreadPool(), find_helper,
                             workingCopy, context, symbol);
    createWatcher(result, search);

    FutureProgress *progress = ProgressManager::addTask(result, tr("Searching for Usages"),
                                                              CppTools::Constants::TASK_SEARCH);

    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void CppFindReferences::onReplaceButtonClicked(const QString &text,
                                               const QList<SearchResultItem> &items,
                                               bool preserveCase)
{
    const QStringList fileNames = TextEditor::BaseFileFind::replaceAll(text, items, preserveCase);
    if (!fileNames.isEmpty()) {
        m_modelManager->updateSourceFiles(fileNames.toSet());
        SearchResultWindow::instance()->hide();
    }
}

void CppFindReferences::searchAgain()
{
    SearchResult *search = qobject_cast<SearchResult *>(sender());
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    Snapshot snapshot = CppModelManager::instance()->snapshot();
    search->restart();
    LookupContext context;
    Symbol *symbol = findSymbol(parameters, snapshot, &context);
    if (!symbol) {
        search->finishSearch(false);
        return;
    }
    findAll_helper(search, symbol, context);
}

namespace {
class UidSymbolFinder : public SymbolVisitor
{
public:
    UidSymbolFinder(const QList<QByteArray> &uid) : m_uid(uid), m_index(0), m_result(0) { }
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

Symbol *CppFindReferences::findSymbol(const CppFindReferencesParameters &parameters,
                                      const Snapshot &snapshot, LookupContext *context)
{
    QTC_ASSERT(context, return 0);
    QString symbolFile = QLatin1String(parameters.symbolFileName);
    if (!snapshot.contains(symbolFile))
        return 0;

    Document::Ptr newSymbolDocument = snapshot.document(symbolFile);
    // document is not parsed and has no bindings yet, do it
    QByteArray source = getSource(Utils::FileName::fromString(newSymbolDocument->fileName()),
                                  m_modelManager->workingCopy());
    Document::Ptr doc =
            snapshot.preprocessedDocument(source, newSymbolDocument->fileName());
    doc->check();

    // find matching symbol in new document and return the new parameters
    UidSymbolFinder finder(parameters.symbolId);
    finder.accept(doc->globalNamespace());
    if (finder.result()) {
        *context = LookupContext(doc, snapshot);
        return finder.result();
    }
    return 0;
}

static void displayResults(SearchResult *search, QFutureWatcher<Usage> *watcher,
                           int first, int last)
{
    for (int index = first; index != last; ++index) {
        Usage result = watcher->future().resultAt(index);
        search->addResult(result.path,
                          result.line,
                          result.lineText,
                          result.col,
                          result.len);
    }
}

void CppFindReferences::openEditor(const SearchResultItem &item)
{
    if (item.path.size() > 0) {
        EditorManager::openEditorAt(QDir::fromNativeSeparators(item.path.first()),
                                    item.mainRange.begin.line,
                                    item.mainRange.begin.column);
    } else {
        EditorManager::openEditor(QDir::fromNativeSeparators(item.text));
    }
}


namespace {

class FindMacroUsesInFile: public std::unary_function<QString, QList<Usage> >
{
    const WorkingCopy workingCopy;
    const Snapshot snapshot;
    const Macro &macro;
    QFutureInterface<Usage> *future;

public:
    FindMacroUsesInFile(const WorkingCopy &workingCopy,
                        const Snapshot snapshot,
                        const Macro &macro,
                        QFutureInterface<Usage> *future)
        : workingCopy(workingCopy), snapshot(snapshot), macro(macro), future(future)
    { }

    QList<Usage> operator()(const Utils::FileName &fileName)
    {
        QList<Usage> usages;
        Document::Ptr doc = snapshot.document(fileName);
        QByteArray source;

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
                    unsigned column;
                    const QString &lineSource = matchingLine(use.bytesBegin(), source, &column);
                    usages.append(Usage(fileName.toString(), lineSource, use.beginLine(), column,
                                        useMacro.nameToQString().size()));
                }
            }
        }

        if (future->isPaused())
            future->waitForResume();
        return usages;
    }

    static QString matchingLine(unsigned bytesOffsetOfUseStart, const QByteArray &utf8Source,
                                unsigned *columnOfUseStart = 0)
    {
        int lineBegin = utf8Source.lastIndexOf('\n', bytesOffsetOfUseStart) + 1;
        int lineEnd = utf8Source.indexOf('\n', bytesOffsetOfUseStart);
        if (lineEnd == -1)
            lineEnd = utf8Source.length();

        if (columnOfUseStart) {
            *columnOfUseStart = 0;
            const char *startOfUse = utf8Source.constData() + bytesOffsetOfUseStart;
            QTC_ASSERT(startOfUse < utf8Source.constData() + lineEnd, return QString());
            const char *currentSourceByte = utf8Source.constData() + lineBegin;
            unsigned char yychar = *currentSourceByte;
            while (currentSourceByte != startOfUse)
                Lexer::yyinp_utf8(currentSourceByte, yychar, *columnOfUseStart);
        }

        const QByteArray matchingLine = utf8Source.mid(lineBegin, lineEnd - lineBegin);
        return QString::fromUtf8(matchingLine, matchingLine.size());
    }
};

} // end of anonymous namespace

static void findMacroUses_helper(QFutureInterface<Usage> &future,
                                 const WorkingCopy workingCopy,
                                 const Snapshot snapshot,
                                 const Macro macro)
{
    const Utils::FileName sourceFile = Utils::FileName::fromString(macro.fileName());
    Utils::FileNameList files{sourceFile};
    files = Utils::filteredUnique(files + snapshot.filesDependingOn(sourceFile));

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
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
                tr("C++ Macro Usages:"),
                QString(),
                macro.nameToQString(),
                replace ? SearchResultWindow::SearchAndReplace
                        : SearchResultWindow::SearchOnly,
                SearchResultWindow::PreserveCaseDisabled,
                QLatin1String("CppEditor"));

    search->setTextToReplace(replacement);
    connect(search, &SearchResult::replaceButtonClicked,
            this, &CppFindReferences::onReplaceButtonClicked);

    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);

    connect(search, &SearchResult::activated,
            this, &CppFindReferences::openEditor);

    const Snapshot snapshot = m_modelManager->snapshot();
    const WorkingCopy workingCopy = m_modelManager->workingCopy();

    // add the macro definition itself
    {
        const QByteArray &source = getSource(Utils::FileName::fromString(macro.fileName()),
                                             workingCopy);
        unsigned column;
        const QString line = FindMacroUsesInFile::matchingLine(macro.bytesOffset(), source,
                                                               &column);
        search->addResult(macro.fileName(), macro.line(), line, column,
                          macro.nameToQString().length());
    }

    QFuture<Usage> result;
    result = Utils::runAsync(m_modelManager->sharedThreadPool(), findMacroUses_helper,
                             workingCopy, snapshot, macro);
    createWatcher(result, search);

    FutureProgress *progress = ProgressManager::addTask(result, tr("Searching for Usages"),
                                                              CppTools::Constants::TASK_SEARCH);
    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void CppFindReferences::renameMacroUses(const Macro &macro, const QString &replacement)
{
    const QString textToReplace = replacement.isEmpty() ? macro.nameToQString() : replacement;
    findMacroUses(macro, textToReplace, true);
}

void CppFindReferences::createWatcher(const QFuture<Usage> &future, SearchResult *search)
{
    QFutureWatcher<Usage> *watcher = new QFutureWatcher<Usage>();
    // auto-delete:
    connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);

    connect(watcher, &QFutureWatcherBase::resultsReadyAt, search,
            [search, watcher](int first, int last) {
                displayResults(search, watcher, first, last);
            });
    connect(watcher, &QFutureWatcherBase::finished, search, [search, watcher]() {
        search->finishSearch(watcher->isCanceled());
    });
    connect(search, &SearchResult::cancelled, watcher, [watcher]() { watcher->cancel(); });
    connect(search, &SearchResult::paused, watcher, [watcher](bool paused) {
        if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
            watcher->setPaused(paused);
    });
    watcher->setPendingResultsLimit(1);
    watcher->setFuture(future);
}
