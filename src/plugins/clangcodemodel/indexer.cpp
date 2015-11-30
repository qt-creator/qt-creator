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

#include "clangutils.h"
#include "indexer.h"
#include "index.h"
#include "cxraii.h"
#include "sourcelocation.h"
#include "utils_p.h"
#include "pchmanager.h"
#include "raii/scopedclangoptions.h"

#include <clang-c/Index.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/fileutils.h>
#include <utils/QtConcurrentTools>

#include <QDebug>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QDir>
#include <QFuture>
#include <QTime>
#include <QRunnable>
#include <QThreadPool>
#include <QDateTime>
#include <QStringBuilder>

using namespace Utils;
using namespace ClangCodeModel;
using namespace Internal;

namespace ClangCodeModel {

// The indexing result, containing the symbols found, reported by the indexer processor.
struct IndexingResult
{
    typedef CppTools::ProjectPart ProjectPart;

    IndexingResult()
        : m_unit(Unit::create())
    {}

    IndexingResult(const QVector<Symbol> &symbol,
                   const QSet<QString> &processedFiles,
                   const Unit::Ptr &unit,
                   const ProjectPart::Ptr &projectPart)
        : m_symbolsInfo(symbol)
        , m_processedFiles(processedFiles)
        , m_unit(unit)
        , m_projectPart(projectPart)
    {}

    QVector<Symbol> m_symbolsInfo;
    QSet<QString> m_processedFiles;
    Unit::Ptr m_unit;
    ProjectPart::Ptr m_projectPart;
};

class LibClangIndexer;

class IndexerPrivate : public QObject
{
    Q_OBJECT

public:
    typedef CppTools::ProjectPart ProjectPart;

public:
    IndexerPrivate(Indexer *indexer);
    ~IndexerPrivate()
    { cancel(true); }

    // This enumeration is used to index a vector. So be careful when changing.
    enum FileType {
        ImplementationFile = 0,
        HeaderFile,
        TotalFileTypes
    };

    struct FileData
    {
        FileData() : m_upToDate(false) {}
        FileData(const QString &fileName,
                 const ProjectPart::Ptr &projectPart,
                 bool upToDate = false)
            : m_fileName(fileName)
            , m_projectPart(projectPart)
            , m_upToDate(upToDate)
            , m_managementOptions(CXTranslationUnit_DetailedPreprocessingRecord
                                  | CXTranslationUnit_Incomplete)
        {}

        QString m_fileName;
        ProjectPart::Ptr m_projectPart;
        bool m_upToDate;
        unsigned m_managementOptions;
    };

    void synchronize(const QVector<IndexingResult> &results);
    void finished(LibClangIndexer *indexer);
    bool noIndexersRunning() const;

private:
    mutable QMutex m_mutex;

    void indexingFinished();
    void cancelIndexing();
    int runningIndexerCount() const;

public slots:
    void dependencyGraphComputed();
    void restoredSymbolsAnalysed();

public:
    enum IndexingMode {
        RelaxedIndexing,        // Index symbols from any file.
        ConstrainedIndexing     // Index symbols only from the requested files.
    };

    void startLoading();
    void concludeLoading();

    void computeDependencyGraph();
    void analyzeRestoredSymbols();

    void runQuickIndexing(const Unit::Ptr &unit, const ProjectPart::Ptr &part);
    void run();
    void run(const QStringList &fileNames);
    void runCore(const QHash<QString, FileData> &headers,
                          const QHash<QString, FileData> &impls,
                          IndexingMode mode);
    void watchIndexingThreads(QFutureInterface<void> &future);
    bool isBusy() const;
    void cancel(bool wait);
    void reset();

    bool addFile(const QString &fileName,
                 ProjectPart::Ptr projectPart);
    void addOrUpdateFileData(const QString &fileName,
                             ProjectPart::Ptr projectPart,
                             bool upToDate);
    QStringList allFiles() const;
    bool isTrackingFile(const QString &fileName, FileType type) const;
    static FileType identifyFileType(const QString &fileName);
    static void populateFileNames(QStringList *all, const QList<FileData> &data);
    QStringList compilationOptions(const QString &fileName) const;

    bool deserealizeSymbols();
    void serializeSymbols() const;

    QList<Symbol> symbols(Symbol::Kind kind) const;
    QList<Symbol> symbols(const QString &fileName, const Symbol::Kind kind) const;
    void match(ClangSymbolSearcher *searcher) const;

    Indexer *m_q;
    QVector<QHash<QString, FileData> > m_files;
    Index m_index;
    bool m_hasQueuedFullRun;
    QSet<QString> m_queuedFilesRun;
    QString m_storagePath;
    bool m_isLoaded;
//    DependencyGraph m_dependencyGraph;
    QScopedPointer<QFutureWatcher<void> >m_loadingWatcher;
    QScopedPointer<QFutureWatcher<void> >m_indexingWatcher;
    QThreadPool m_indexingPool;
    QSet<LibClangIndexer *> m_runningIndexers;
};

} // ClangCodeModel

Q_DECLARE_METATYPE(IndexingResult)

namespace {

struct ScopepTimer
{
    ScopepTimer(int id = 0) : m_id(id) { m_t.start(); }
    ~ScopepTimer() { qDebug() << "\t#Timer" << m_id << ":" << m_t.elapsed() << "ms"; }
    int m_id;
    QTime m_t;
};

} // Anonymous

namespace ClangCodeModel {

class LibClangIndexer: public QRunnable
{
protected:
    typedef CppTools::ProjectPart ProjectPart;

public:
    LibClangIndexer(IndexerPrivate *indexer)
        : m_indexer(indexer)
        , m_isCanceled(false)
    {}

    virtual ~LibClangIndexer()
    {}

    void cancel()
    { m_isCanceled = true; }

protected:
    void propagateResults(const ProjectPart::Ptr &projectPart)
    {
        if (!isCanceled()) {
            QVector<IndexingResult> indexingResults;
            indexingResults.reserve(m_allFiles.size());

            foreach (const QString &fn, m_allFiles.keys()) {
                QVector<Symbol> symbols; unfoldSymbols(symbols, fn);
                QSet<QString> processedFiles = QSet<QString>::fromList(m_allFiles.keys());
                Unit::Ptr unit = Unit::create(fn);
                IndexingResult indexingResult(symbols, processedFiles, unit, projectPart);
                indexingResults.append(indexingResult);

                // TODO: includes need to be propagated to the dependency table.
            }
            m_indexer->synchronize(indexingResults);
        }

        qDeleteAll(m_allFiles.values());
        m_allFiles.clear();
        qDeleteAll(m_allSymbols);
        m_allSymbols.clear();
    }

protected:
    static inline LibClangIndexer *indexer(CXClientData d)
    { return static_cast<LibClangIndexer *>(d); }

    static int abortQuery(CXClientData client_data, void *reserved) {
        Q_UNUSED(reserved);

        return indexer(client_data)->isCanceled();
    }

    static void diagnostic(CXClientData client_data, CXDiagnosticSet diagSet, void *reserved) {
        Q_UNUSED(client_data);
        Q_UNUSED(diagSet);
        Q_UNUSED(reserved);
    }

    static CXIdxClientFile enteredMainFile(CXClientData client_data, CXFile file, void *reserved) {
        Q_UNUSED(client_data);
        Q_UNUSED(reserved);

        const QString fileName = getQString(clang_getFileName(file));
//        qDebug() << "enteredMainFile:" << fileName;
        LibClangIndexer *lci = indexer(client_data);
        File *f = lci->file(fileName);
        f->setMainFile();

        return f;
    }

    static CXIdxClientFile includedFile(CXClientData client_data, const CXIdxIncludedFileInfo *info) {
        Q_UNUSED(client_data);

        File *includingFile = 0;
        clang_indexLoc_getFileLocation(info->hashLoc, reinterpret_cast<CXIdxClientFile*>(&includingFile), 0, 0, 0, 0);

        const QString fileName = getQString(clang_getFileName(info->file));
        File *f = indexer(client_data)->file(fileName);

        if (includingFile)
            includingFile->addInclude(f);

        return f;
    }

    static CXIdxClientFile importedASTFile(CXClientData client_data, const CXIdxImportedASTFileInfo *info) {
        const QString fileName = getQString(clang_getFileName(info->file));

//        qDebug() << "importedASTFile:" << fileName;

        indexer(client_data)->m_importedASTs.insert(fileName, false);

        return info->file;
    }

    static CXIdxClientContainer startedTranslationUnit(CXClientData client_data, void *reserved) {
        Q_UNUSED(client_data);
        Q_UNUSED(reserved);

//        qDebug() << "startedTranslationUnit";
        return 0;
    }

    static void indexDeclaration(CXClientData client_data, const CXIdxDeclInfo *info) {
        LibClangIndexer *lci = indexer(client_data);

        File *includingFile = 0;
        unsigned line = 0, column = 0, offset = 0;
        clang_indexLoc_getFileLocation(info->loc, reinterpret_cast<CXIdxClientFile*>(&includingFile), 0, &line, &column, &offset);

        QString kind = getQString(clang_getCursorKindSpelling(info->cursor.kind));
        QString displayName = getQString(clang_getCursorDisplayName(info->cursor));
        QString spellingName = getQString(clang_getCursorSpelling(info->cursor));
//        qDebug() << (includingFile ? includingFile->name() : QLatin1String("<UNKNOWN FILE>")) << ":"<<line<<":"<<column<<": display name ="<<displayName<<"spelling name ="<<spellingName<<"of kind"<<kind;

        Symbol *sym = lci->newSymbol(info->cursor.kind, displayName, spellingName, includingFile, line, column, offset);

        // TODO: add to decl container...
        if (includingFile) // TODO: check why includingFile can be null...
            includingFile->addSymbol(sym);

        if (const CXIdxContainerInfo *semanticContainer = info->semanticContainer) {
            if (Symbol *container = static_cast<Symbol *>(clang_index_getClientContainer(semanticContainer))) {
                sym->semanticContainer = container;
                container->addSymbol(sym);
            }
        }

        // TODO: ObjC containers
        // TODO: index forward decls too?

        if (info->declAsContainer)
            clang_index_setClientContainer(info->declAsContainer, sym);
    }

    static void indexEntityReference(CXClientData client_data, const CXIdxEntityRefInfo *info) {
        Q_UNUSED(client_data);
        Q_UNUSED(info);

        // TODO: well, we do get the info, so why not (optionally?) remember all references?
    }

protected:
    struct File;
    struct Symbol;

    typedef QHash<QString, File *> FilesByName;
    struct File
    {
        File(const QString &fileName)
            : m_fileName(fileName)
        {}

        void addInclude(File *f)
        {
//            assert(f);
            m_includes.insert(f->name(), f);
        }

        QList<File *> includes() const
        { return m_includes.values(); }

        QString name() const
        { return m_fileName; }

        void setMainFile(bool isMainFile = true)
        { m_isMainFile = isMainFile; }

        bool isMainFile() const
        { return m_isMainFile; }

        void addSymbol(Symbol *symbol)
        {
//            assert(symbol);
            m_symbols.append(symbol);
        }

        QVector<Symbol *> symbols() const
        { return m_symbols; }

    private:
        QString m_fileName;
        FilesByName m_includes;
        bool m_isMainFile;
        QVector<Symbol *> m_symbols;
    };

    struct Symbol
    {
        Symbol(enum CXCursorKind kind, const QString &displayName, const QString &spellingName, File *file, unsigned line, unsigned column, unsigned offset)
            : kind(kind)
            , displayName(displayName)
            , spellingName(spellingName)
            , file(file)
            , line(line)
            , column(column)
            , offset(offset)
            , semanticContainer(0)
        {}

        QString spellKind() const
        { return getQString(clang_getCursorKindSpelling(kind)); }

        void addSymbol(Symbol *symbol)
        { symbols.append(symbol); }

        enum CXCursorKind kind;
        QString displayName, spellingName;
        File *file;
        unsigned line, column, offset;
        Symbol *semanticContainer;
        QVector<Symbol *> symbols;
    };

protected:
    bool isCanceled() const
    { return m_isCanceled; }

    void finish()
    { m_indexer->finished(this); }

    File *file(const QString &fileName)
    {
        File *f = m_allFiles[fileName];
        if (!f) {
            f = new File(fileName);
            m_allFiles.insert(fileName, f);
        }
        return f;
    }

    Symbol *newSymbol(enum CXCursorKind kind, const QString &displayName, const QString &spellingName, File *file, unsigned line, unsigned column, unsigned offset)
    {
        Symbol *s = new Symbol(kind, displayName, spellingName, file, line, column, offset);
        m_allSymbols.append(s);
        return s;
    }

    void dumpInfo()
    {
        qDebug() << "=== indexing info dump ===";
        qDebug() << "indexed" << m_allFiles.size() << "files. Main files:";
        foreach (const File *f, m_allFiles) {
            if (!f->isMainFile())
                continue;
            qDebug() << f->name() << ":";
            foreach (const File *inc, f->includes())
                qDebug() << "  includes" << inc->name();
            dumpSymbols(f->symbols(), QByteArray("  "));
        }

        qDebug() << "=== end of dump ===";
    }

    void dumpSymbols(const QVector<Symbol *> &symbols, const QByteArray &indent)
    {
        if (symbols.isEmpty())
            return;

        qDebug("%scontained symbols:", indent.constData());
        QByteArray newIndent = indent + "  ";
        foreach (const Symbol *s, symbols) {
            qDebug("%s%s (%s)", newIndent.constData(), s->spellingName.toUtf8().constData(), s->spellKind().toUtf8().constData());
            dumpSymbols(s->symbols, newIndent);
        }
    }

    void unfoldSymbols(QVector<Symbol> &result, const QString &fileName) {
        const QVector<Symbol *> symbolsForFile = file(fileName)->symbols();
        foreach (const Symbol *s, symbolsForFile) {
            unfoldSymbols(s, result);
        }
    }

    void unfoldSymbols(const Symbol *s, QVector<Symbol> &result) {
        if (!s->file)
            return;

        Symbol sym;
        sym.m_name = s->spellingName;
        sym.m_qualification = s->spellingName;

        static QLatin1String sep("::");
        for (Symbol *parent = s->semanticContainer; parent; parent = parent->semanticContainer)
            sym.m_qualification = parent->spellingName + sep + sym.m_qualification;

        sym.m_location = SourceLocation(s->file->name(), s->line, s->column, s->offset);

        switch (s->kind) {
        case CXCursor_EnumDecl: sym.m_kind = Symbol::Enum; break;
        case CXCursor_StructDecl:
        case CXCursor_ClassDecl: sym.m_kind = Symbol::Class; break;
        case CXCursor_CXXMethod: sym.m_kind = Symbol::Method; break;
        case CXCursor_FunctionTemplate:
        case CXCursor_FunctionDecl: sym.m_kind = Symbol::Function; break;
        case CXCursor_DeclStmt: sym.m_kind = Symbol::Declaration; break;
        case CXCursor_Constructor: sym.m_kind = Symbol::Constructor; break;
        case CXCursor_Destructor: sym.m_kind = Symbol::Destructor; break;
        default: sym.m_kind = Symbol::Unknown; break;
        }

        result.append(sym);
    }

protected:
    static IndexerCallbacks IndexCB;

protected:
    IndexerPrivate *m_indexer;
    bool m_isCanceled;
    QHash<QString, bool> m_importedASTs;
    FilesByName  m_allFiles;
    QVector<Symbol *> m_allSymbols;
};

IndexerCallbacks LibClangIndexer::IndexCB = {
    abortQuery,
    diagnostic,
    enteredMainFile,
    includedFile,
    importedASTFile,
    startedTranslationUnit,
    indexDeclaration,
    indexEntityReference
};

class ProjectPartIndexer: public LibClangIndexer
{
public:
    ProjectPartIndexer(IndexerPrivate *indexer, const QList<IndexerPrivate::FileData> &todo)
        : LibClangIndexer(indexer)
        , m_todo(todo)
    {}

    void run()
    {
        if (isCanceled() || m_todo.isEmpty()) {
            finish();
            return;
        }

        const ProjectPart::Ptr &pPart = m_todo[0].m_projectPart;

restart:
        CXIndex idx;
        if (!(idx = clang_createIndex(/* excludeDeclsFromPCH */ 1,
                                      /* displayDiagnosics=*/1))) {
          qDebug() << "Could not create Index";
          return;
        }

        CXIndexAction idxAction = clang_IndexAction_create(idx);
        const unsigned index_opts = CXIndexOpt_SuppressWarnings;

        PchManager *pchManager = PchManager::instance();
        PchInfo::Ptr pchInfo = pchManager->pchInfo(pPart);

        for (int i = 0, ei = m_todo.size(); i < ei; ++i) {
            const IndexerPrivate::FileData &fd = m_todo.at(i);
            if (fd.m_upToDate)
                continue;

            if (pchManager->pchInfo(pPart) != pchInfo) {
                clang_IndexAction_dispose(idxAction);
                clang_disposeIndex(idx);
                goto restart;
            }

            QStringList opts = Utils::createClangOptions(pPart, fd.m_fileName);
            if (!pchInfo.isNull())
                opts.append(Utils::createPCHInclusionOptions(pchInfo->fileName()));

            ScopedClangOptions scopedOpts(opts);
            QByteArray fileName = fd.m_fileName.toUtf8();

//            qDebug() << "Indexing file" << fd.m_fileName << "with options" << opts;
            unsigned parsingOptions = fd.m_managementOptions;
            parsingOptions |= CXTranslationUnit_SkipFunctionBodies;

            /*int result =*/ clang_indexSourceFile(idxAction, this,
                                                   &IndexCB, sizeof(IndexCB),
                                                   index_opts, fileName.constData(),
                                                   scopedOpts.data(), scopedOpts.size(), 0, 0, 0,
                                                   parsingOptions);

            // index imported ASTs:
            foreach (const QString &astFile, m_importedASTs.keys()) {
                if (m_importedASTs.value(astFile))
                    continue;

                if (CXTranslationUnit TU = clang_createTranslationUnit(
                            idx, astFile.toUtf8().constData())) {
                    /*result =*/ clang_indexTranslationUnit(idxAction, this,
                                                            &IndexCB,
                                                            sizeof(IndexCB),
                                                            index_opts, TU);
                    clang_disposeTranslationUnit(TU);
                }

                m_importedASTs[astFile] = true;
            }

            propagateResults(fd.m_projectPart);
            if (isCanceled())
                break;
        }

//        dumpInfo();

        clang_IndexAction_dispose(idxAction);
        clang_disposeIndex(idx);

        finish();
    }

private:
    QList<IndexerPrivate::FileData> m_todo;
};

class QuickIndexer: public LibClangIndexer
{
public:
    QuickIndexer(IndexerPrivate *indexer, const Unit::Ptr &unit, const ProjectPart::Ptr&projectPart)
        : LibClangIndexer(indexer)
        , m_unit(unit)
        , m_projectPart(projectPart)
    {}

    void run()
    {
        if (isCanceled() || !m_unit->isLoaded()) {
            finish();
            return;
        }

        CXIndexAction idxAction = clang_IndexAction_create(m_unit->clangIndex());
        const unsigned index_opts = CXIndexOpt_SuppressWarnings;

//        qDebug() << "Indexing TU" << m_unit.fileName() << "...";
        /*int result =*/ clang_indexTranslationUnit(idxAction, this,
                                                    &IndexCB, sizeof(IndexCB),
                                                    index_opts,
                                                    m_unit->clangTranslationUnit());

        propagateResults(m_projectPart);

        clang_IndexAction_dispose(idxAction);
        finish();
    }

private:
    Unit::Ptr m_unit;
    ProjectPart::Ptr m_projectPart;
};

} // ClangCodeModel

IndexerPrivate::IndexerPrivate(Indexer *indexer)
    : m_mutex(QMutex::Recursive)
    , m_q(indexer)
    , m_files(TotalFileTypes)
    , m_hasQueuedFullRun(false)
    , m_isLoaded(false)
    , m_loadingWatcher(new QFutureWatcher<void>)
    , m_indexingWatcher(new QFutureWatcher<void>)
{
//    const int magicThreadCount = QThread::idealThreadCount() * 4 / 3;
    const int magicThreadCount = QThread::idealThreadCount() - 1;
    m_indexingPool.setMaxThreadCount(std::max(magicThreadCount, 1));
    m_indexingPool.setExpiryTimeout(1000);
}

void IndexerPrivate::runCore(const QHash<QString, FileData> & /*headers*/,
                             const QHash<QString, FileData> &impls,
                             IndexingMode /*mode*/)
{
    QMutexLocker locker(&m_mutex);

    typedef QHash<QString, FileData>::const_iterator FileContIt;
    QHash<ProjectPart::Ptr, QList<IndexerPrivate::FileData> > parts;
    typedef QHash<ProjectPart::Ptr, QList<IndexerPrivate::FileData> >::Iterator PartIter;

    QList<Core::IDocument *> docs = Core::DocumentModel::openedDocuments();
    QSet<QString> openDocs;
    foreach (Core::IDocument *doc, docs)
        openDocs.insert(doc->filePath().toString());

    for (FileContIt tit = impls.begin(), eit = impls.end(); tit != eit; ++tit) {
        if (!tit->m_upToDate && openDocs.contains(tit.key())) {
            const IndexerPrivate::FileData &fd = tit.value();
            parts[fd.m_projectPart].append(fd);
        }
    }

    if (parts.isEmpty())
        return;

    for (PartIter i = parts.begin(), ei = parts.end(); i != ei; ++i) {
        ProjectPartIndexer *ppi = new ProjectPartIndexer(this, i.value());
        m_runningIndexers.insert(ppi);
        m_indexingPool.start(ppi);
    }

    QFuture<void> task = QtConcurrent::run(&IndexerPrivate::watchIndexingThreads, this);
    m_indexingWatcher->setFuture(task);
    emit m_q->indexingStarted(task);
}

void IndexerPrivate::watchIndexingThreads(QFutureInterface<void> &future)
{
    int maxTodo = runningIndexerCount();
    future.setProgressRange(0, maxTodo);

    int todo = -1;
    while (todo) {
        int newTodo = runningIndexerCount();
        if (todo != newTodo)
            future.setProgressValue(maxTodo - newTodo);
        todo = newTodo;
        if (future.isCanceled()) {
            cancelIndexing();
            return;
        }
        m_indexingPool.waitForDone(500);
    }
}

void IndexerPrivate::run()
{
    Q_ASSERT(m_isLoaded);

    QMutexLocker locker(&m_mutex);

    if (m_runningIndexers.isEmpty()) {
        runCore(m_files.value(HeaderFile),
                m_files.value(ImplementationFile),
                RelaxedIndexing);
    } else {
        m_hasQueuedFullRun = true;
        cancelIndexing();
    }
}

void IndexerPrivate::run(const QStringList &fileNames)
{
    Q_ASSERT(m_isLoaded);
    QMutexLocker locker(&m_mutex);

    if (noIndexersRunning()) {
        QVector<QHash<QString, FileData> > files(TotalFileTypes);
        foreach (const QString &fileName, fileNames) {
            FileType type = identifyFileType(fileName);
            if (!isTrackingFile(fileName, type)) {
                // @TODO
                continue;
            }

            FileData *data = &m_files[type][fileName];
            data->m_upToDate = false;
            files[type].insert(fileName, *data);
            m_index.removeFile(fileName);
        }
        runCore(files.value(HeaderFile),
                files.value(ImplementationFile),
                ConstrainedIndexing);
    } else {
        m_queuedFilesRun.unite(fileNames.toSet());
    }
}

bool IndexerPrivate::isBusy() const
{
    return !noIndexersRunning() || m_loadingWatcher->isRunning();
}

void IndexerPrivate::cancel(bool wait)
{
//    m_dependencyGraph.discard();

    m_loadingWatcher->cancel();
    cancelIndexing();
    if (wait) {
        m_loadingWatcher->waitForFinished();
        m_indexingWatcher->waitForFinished();
        while (!noIndexersRunning())
            m_indexingPool.waitForDone(100);
    }
}

void IndexerPrivate::reset()
{
    cancel(true);
    serializeSymbols();

    for (int i = 0; i < TotalFileTypes; ++i)
        m_files[i].clear();
    m_hasQueuedFullRun = false;
    m_queuedFilesRun.clear();
    m_storagePath.clear();
    m_index.clear();
    m_isLoaded = false;
}

void IndexerPrivate::synchronize(const QVector<IndexingResult> &results)
{
    Q_UNUSED(results);
#if 0
    foreach (const IndexingResult &result, results) {
        QMutexLocker locker(&m_mutex);

        foreach (const Symbol &symbol, result.m_symbolsInfo) {
            addOrUpdateFileData(symbol.m_location.fileName(),
                                result.m_projectPart,
                                true);

            // Make the symbol available in the database.
            m_index.insertSymbol(symbol, result.m_unit->timeStamp());
        }

        // There might be files which were processed but did not "generate" any indexable symbol,
        // but we still need to make the index aware of them.
        result.m_processedFiles.insert(result.m_unit->fileName());
        foreach (const QString &fileName, result.m_processedFiles) {
            if (!m_index.containsFile(fileName))
                m_index.insertFile(fileName, result.m_unit.timeStamp());
        }

        // If this unit is being kept alive, update in the manager.
        if (LiveUnitsManager::instance()->isTracking(result.m_unit.fileName()))
            LiveUnitsManager::instance()->updateUnit(result.m_unit.fileName(), result.m_unit);
    }
#endif
}

void IndexerPrivate::finished(LibClangIndexer *indexer)
{
    QMutexLocker locker(&m_mutex);

    m_runningIndexers.remove(indexer);
    if (noIndexersRunning())
        indexingFinished();
}

bool IndexerPrivate::noIndexersRunning() const
{
    QMutexLocker locker(&m_mutex);

    return m_runningIndexers.isEmpty();
}

void IndexerPrivate::indexingFinished()
{
    if (m_hasQueuedFullRun) {
        m_hasQueuedFullRun = false;
        run();
    } else if (!m_queuedFilesRun.isEmpty()) {
        const QStringList &files = m_queuedFilesRun.toList();
        m_queuedFilesRun.clear();
        run(files);
    }

    emit m_q->indexingFinished();
}

void IndexerPrivate::cancelIndexing()
{
    QMutexLocker locker(&m_mutex);

    foreach (LibClangIndexer* partIndexer, m_runningIndexers) {
        partIndexer->cancel();
    }
}

int IndexerPrivate::runningIndexerCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_runningIndexers.size();
}

void IndexerPrivate::addOrUpdateFileData(const QString &fileName,
                                         ProjectPart::Ptr projectPart,
                                         bool upToDate)
{
    Q_ASSERT(QDir::isAbsolutePath(fileName));

    QString cleanFileName(normalizeFileName(fileName));

    FileType fileType = identifyFileType(cleanFileName);
    if (isTrackingFile(cleanFileName, fileType)) {
        m_files[fileType][cleanFileName].m_projectPart = projectPart;
        m_files[fileType][cleanFileName].m_upToDate = upToDate;
    } else {
        m_files[fileType].insert(cleanFileName,
                                 FileData(cleanFileName, projectPart, upToDate));
    }

    if (!upToDate)
        m_index.removeFile(cleanFileName);
}

bool IndexerPrivate::addFile(const QString &fileName,
                             ProjectPart::Ptr projectPart)
{
    if (isBusy()
            || fileName.trimmed().isEmpty()
            || !QFileInfo(fileName).isFile())
        return false;

    addOrUpdateFileData(fileName, projectPart, false);

    return true;
}

QStringList IndexerPrivate::allFiles() const
{
    QStringList all;
    populateFileNames(&all, m_files.at(ImplementationFile).values());
    populateFileNames(&all, m_files.at(HeaderFile).values());
    return all;
}

bool IndexerPrivate::isTrackingFile(const QString &fileName, FileType type) const
{
    return m_files.value(type).contains(normalizeFileName(fileName));
}

QStringList IndexerPrivate::compilationOptions(const QString &fileName) const
{
    FileType type = identifyFileType(fileName);
    return Utils::createClangOptions(m_files.value(type).value(normalizeFileName(fileName)).m_projectPart);
}

IndexerPrivate::FileType IndexerPrivate::identifyFileType(const QString &fileName)
{
    const QString fn = fileName.toLower();
    if (fn.endsWith(QLatin1String(".cpp"))
            || fn.endsWith(QLatin1String(".cxx"))
            || fn.endsWith(QLatin1String(".cc"))
            || fn.endsWith(QLatin1String(".c"))
            || fn.endsWith(QLatin1String(".m"))
            || fn.endsWith(QLatin1String(".mm"))) {
        return ImplementationFile;
    }

    // Everything that is not an implementation file is treated as a header. This makes things
    // easier when handling standard library files and any other file that does not use
    // conventional suffixes.
    return HeaderFile;
}

void IndexerPrivate::populateFileNames(QStringList *all, const QList<FileData> &data)
{
    foreach (const FileData &fileData, data)
        all->append(fileData.m_fileName);
}


namespace {

struct DepedencyVisitor
{
    DepedencyVisitor(IndexerPrivate *indexer) : m_indexer(indexer) {}

    bool acceptFile(const QString &includer)
    {
        IndexerPrivate::FileType fileType = IndexerPrivate::identifyFileType(includer);
        if (m_indexer->isTrackingFile(includer, fileType)) {
            m_match = m_indexer->m_files.at(fileType).value(includer);
            return true;
        }
        return false;
    }

    IndexerPrivate *m_indexer;
    IndexerPrivate::FileData m_match;
};

} // Anonymous

void IndexerPrivate::startLoading()
{
    // In the case of existent persisted symbols, we restore them and make them visible
    // to the indexer. However, we need a dependency graph in order to identify the proper
    // options.

    if (deserealizeSymbols() && !m_index.isEmpty())
        computeDependencyGraph();
    else
        concludeLoading();
}

void IndexerPrivate::concludeLoading()
{
    m_isLoaded = true;
    run();
}

void IndexerPrivate::computeDependencyGraph()
{
    // FIXME
//    for (int fileType = ImplementationFile; fileType < TotalFileTypes; ++fileType) {
//        QHash<QString, FileData>::iterator it = m_files[fileType].begin();
//        for (; it != m_files[fileType].end(); ++it)
//            m_dependencyGraph.addFile(it.value().m_fileName, it.value().m_compilationOptions);
//    }

    m_loadingWatcher.reset(new QFutureWatcher<void>);
    connect(m_loadingWatcher.data(), SIGNAL(finished()), this, SLOT(dependencyGraphComputed()));
//    m_loadingWatcher->setFuture(m_dependencyGraph.compute());
}

void IndexerPrivate::dependencyGraphComputed()
{
    if (m_loadingWatcher->isCanceled())
        return;

    m_loadingWatcher.reset(new QFutureWatcher<void>);
    connect(m_loadingWatcher.data(), SIGNAL(finished()), this, SLOT(restoredSymbolsAnalysed()));
    m_loadingWatcher->setFuture(QtConcurrent::run(this, &IndexerPrivate::analyzeRestoredSymbols));
}

void IndexerPrivate::analyzeRestoredSymbols()
{
    // @TODO: We only check for time stamps, so we still need to handle somehow the case in
    // which the project options (for example a .pro file) changed while "outside" a Creator
    // session.

    foreach (const QString &fileName, m_index.files()) {
        bool upToDate = m_index.validate(fileName);

        FileType fileType = identifyFileType(fileName);
        if (isTrackingFile(fileName, fileType)) {
            // When the file is already being tracked we simply need to update its state.
            if (upToDate)
                m_files[fileType][fileName].m_upToDate = true;
        } else {
            // If it's not being tracked we need to find at least one tracked dependency
            // so we can use its options.
            DepedencyVisitor visitor(this);
//            m_dependencyGraph.collectDependencies(fileName,
//                                                  DependencyGraph::FilesWhichInclude,
//                                                  &visitor);
            if (!visitor.m_match.m_fileName.isEmpty()) {
                addOrUpdateFileData(fileName,
                                    visitor.m_match.m_projectPart,
                                    upToDate);
            } else {
                m_index.removeFile(fileName);
            }
        }

        if (!upToDate && m_index.containsFile(fileName))
            m_index.removeFile(fileName);
    }
}

void IndexerPrivate::runQuickIndexing(const Unit::Ptr &unit, const CppTools::ProjectPart::Ptr &part)
{
    QMutexLocker locker(&m_mutex);

    addOrUpdateFileData(unit->fileName(), part, false);

    QuickIndexer indexer(this, unit, part);
    indexer.run();
}

void IndexerPrivate::restoredSymbolsAnalysed()
{
    if (m_loadingWatcher->isCanceled())
        return;

    concludeLoading();
}

bool IndexerPrivate::deserealizeSymbols()
{
    if (m_storagePath.isEmpty())
        return false;

    ::Utils::FileReader reader;
    if (reader.fetch(m_storagePath)) {
        m_index.deserialize(reader.data());
        return true;
    }

    return false;
}

void IndexerPrivate::serializeSymbols() const
{
    if (m_storagePath.isEmpty())
        return;

    ::Utils::FileSaver saver(m_storagePath);
    saver.write(m_index.serialize());
    if (!saver.finalize())
        qWarning("Failed to serialize index");
}

QList<Symbol> IndexerPrivate::symbols(Symbol::Kind kind) const
{
    if (m_loadingWatcher->isRunning())
        return QList<Symbol>();

    return m_index.symbols(kind);
}

QList<Symbol> IndexerPrivate::symbols(const QString &fileName, const Symbol::Kind kind) const
{
    if (m_loadingWatcher->isRunning())
        return QList<Symbol>();

    if (kind == Symbol::Unknown)
        return m_index.symbols(fileName);

    return m_index.symbols(fileName, kind);
}

void IndexerPrivate::match(ClangSymbolSearcher *searcher) const
{
    if (m_loadingWatcher->isRunning())
        return;

    m_index.match(searcher);
}

Indexer::Indexer(QObject *parent)
    : QObject(parent)
    , m_d(new IndexerPrivate(this))
{}

Indexer::~Indexer()
{}

void Indexer::regenerate()
{
    if (!m_d->m_isLoaded) {
        if (m_d->m_loadingWatcher->isRunning())
            return;
        m_d->startLoading();
    } else {
        m_d->run();
    }
}

void Indexer::initialize(const QString &storagePath)
{
    Q_ASSERT(!m_d->m_isLoaded);

    m_d->m_storagePath = storagePath;
}

void Indexer::evaluateFile(const QString &fileName)
{
    if (!m_d->m_isLoaded)
        return;

    m_d->run(QStringList(normalizeFileName(fileName)));
}

bool Indexer::isBusy() const
{
    return m_d->isBusy();
}

void Indexer::cancel(bool waitForFinished)
{
    return m_d->cancel(waitForFinished);
}

void Indexer::finalize()
{
    m_d->reset();
}

bool Indexer::addFile(const QString &fileName, ProjectPart::Ptr projectPart)
{
    return m_d->addFile(fileName, projectPart);
}

QStringList Indexer::allFiles() const
{
    return m_d->allFiles();
}

QStringList Indexer::compilationOptions(const QString &fileName) const
{
    return m_d->compilationOptions(fileName);
}

QList<Symbol> Indexer::allFunctions() const
{
    return m_d->symbols(Symbol::Function);
}

QList<Symbol> Indexer::allClasses() const
{
    return m_d->symbols(Symbol::Class);
}

QList<Symbol> Indexer::allMethods() const
{
    return m_d->symbols(Symbol::Method);
}

QList<Symbol> Indexer::allConstructors() const
{
    return m_d->symbols(Symbol::Constructor);
}

QList<Symbol> Indexer::allDestructors() const
{
    return m_d->symbols(Symbol::Destructor);
}

QList<Symbol> Indexer::functionsFromFile(const QString &fileName) const
{
    return m_d->symbols(fileName, Symbol::Function);
}

QList<Symbol> Indexer::classesFromFile(const QString &fileName) const
{
    return m_d->symbols(fileName, Symbol::Class);
}

QList<Symbol> Indexer::methodsFromFile(const QString &fileName) const
{
    return m_d->symbols(fileName, Symbol::Method);
}

QList<Symbol> Indexer::constructorsFromFile(const QString &fileName) const
{
    return m_d->symbols(fileName, Symbol::Constructor);
}

QList<Symbol> Indexer::destructorsFromFile(const QString &fileName) const
{
    return m_d->symbols(fileName, Symbol::Destructor);
}

QList<Symbol> Indexer::allFromFile(const QString &fileName) const
{
    return m_d->symbols(fileName, Symbol::Unknown);
}

void Indexer::match(ClangSymbolSearcher *searcher) const
{
    m_d->match(searcher);
}

void Indexer::runQuickIndexing(Unit::Ptr unit, const CppTools::ProjectPart::Ptr &part)
{
    m_d->runQuickIndexing(unit, part);
}

bool Indexer::isTracking(const QString &fileName) const
{
    return m_d->isTrackingFile(fileName, IndexerPrivate::ImplementationFile)
            || m_d->isTrackingFile(fileName, IndexerPrivate::HeaderFile);
}

#include "indexer.moc"
