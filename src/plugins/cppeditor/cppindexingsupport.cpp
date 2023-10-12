// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppindexingsupport.h"

#include "builtineditordocumentparser.h"
#include "cppchecksymbols.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppsourceprocessor.h"
#include "searchsymbols.h"

#include <coreplugin/progressmanager/progressmanager.h>

#include <cplusplus/LookupContext.h>

#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/searchresultitem.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QLoggingCategory>
#include <QRegularExpression>

using namespace Utils;

namespace CppEditor {

static Q_LOGGING_CATEGORY(indexerLog, "qtc.cppeditor.indexer", QtWarningMsg)

SymbolSearcher::SymbolSearcher(const SymbolSearcher::Parameters &parameters,
                               const QSet<QString> &fileNames)
    : m_snapshot(CppModelManager::snapshot())
    , m_parameters(parameters)
    , m_fileNames(fileNames)
{}

namespace {

class ParseParams
{
public:
    ProjectExplorer::HeaderPaths headerPaths;
    WorkingCopy workingCopy;
    QSet<QString> sourceFiles;
    int indexerFileSizeLimitInMb = -1;
};

class WriteTaskFileForDiagnostics
{
    Q_DISABLE_COPY(WriteTaskFileForDiagnostics)

public:
    WriteTaskFileForDiagnostics()
    {
        const QString fileName = TemporaryDirectory::masterDirectoryPath()
                                 + "/qtc_findErrorsIndexing.diagnostics."
                                 + QDateTime::currentDateTime().toString("yyMMdd_HHmm") + ".tasks";

        m_file.setFileName(fileName);
        Q_ASSERT(m_file.open(QIODevice::WriteOnly | QIODevice::Text));
        m_out.setDevice(&m_file);

        qDebug("FindErrorsIndexing: Task file for diagnostics is \"%s\".",
               qPrintable(m_file.fileName()));
    }

    ~WriteTaskFileForDiagnostics()
    {
        qDebug("FindErrorsIndexing: %d diagnostic messages written to \"%s\".",
               m_processedDiagnostics, qPrintable(m_file.fileName()));
    }

    void process(const CPlusPlus::Document::Ptr document)
    {
        using namespace CPlusPlus;
        const QString fileName = document->filePath().toString();

        const QList<Document::DiagnosticMessage> messages = document->diagnosticMessages();
        for (const Document::DiagnosticMessage &message : messages) {
            ++m_processedDiagnostics;

            QString type;
            switch (message.level()) {
            case Document::DiagnosticMessage::Warning:
                type = QLatin1String("warn"); break;
            case Document::DiagnosticMessage::Error:
            case Document::DiagnosticMessage::Fatal:
                type = QLatin1String("err"); break;
            default:
                break;
            }

            // format: file\tline\ttype\tdescription
            m_out << fileName << "\t"
                  << message.line() << "\t"
                  << type << "\t"
                  << message.text() << "\n";
        }
    }

private:
    QFile m_file;
    QTextStream m_out;
    int m_processedDiagnostics = 0;
};

static void classifyFiles(const QSet<QString> &files, QStringList *headers, QStringList *sources)
{
    for (const QString &file : files) {
        if (ProjectFile::isSource(ProjectFile::classify(file)))
            sources->append(file);
        else
            headers->append(file);
    }
}

static void indexFindErrors(QPromise<void> &promise, const ParseParams params)
{
    QStringList sources, headers;
    classifyFiles(params.sourceFiles, &headers, &sources);
    sources.sort();
    headers.sort();
    QStringList files = sources + headers;

    WriteTaskFileForDiagnostics taskFileWriter;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0, end = files.size(); i < end ; ++i) {
        if (promise.isCanceled())
            break;

        const QString file = files.at(i);
        qDebug("FindErrorsIndexing: \"%s\"", qPrintable(file));

        // Parse the file as precisely as possible
        BuiltinEditorDocumentParser parser(FilePath::fromString(file));
        parser.setReleaseSourceAndAST(false);
        parser.update({CppModelManager::workingCopy(), nullptr, Language::Cxx, false});
        CPlusPlus::Document::Ptr document = parser.document();
        QTC_ASSERT(document, return);

        // Write diagnostic messages
        taskFileWriter.process(document);

        // Look up symbols
        CPlusPlus::LookupContext context(document, parser.snapshot());
        CheckSymbols::go(document, context, QList<CheckSymbols::Result>()).waitForFinished();

        document->releaseSourceAndAST();

        promise.setProgressValue(i + 1);
    }

    const QString elapsedTime = Utils::formatElapsedTime(timer.elapsed());
    qDebug("FindErrorsIndexing: %s", qPrintable(elapsedTime));
}

static void index(QPromise<void> &promise, const ParseParams params)
{
    QScopedPointer<Internal::CppSourceProcessor> sourceProcessor(CppModelManager::createSourceProcessor());
    sourceProcessor->setFileSizeLimitInMb(params.indexerFileSizeLimitInMb);
    sourceProcessor->setHeaderPaths(params.headerPaths);
    sourceProcessor->setWorkingCopy(params.workingCopy);

    QStringList sources;
    QStringList headers;
    classifyFiles(params.sourceFiles, &headers, &sources);

    for (const QString &file : std::as_const(params.sourceFiles))
        sourceProcessor->removeFromCache(FilePath::fromString(file));

    const int sourceCount = sources.size();
    QStringList files = sources + headers;

    sourceProcessor->setTodo(Utils::toSet(files));

    const FilePath &conf = CppModelManager::configurationFileName();
    bool processingHeaders = false;

    const ProjectExplorer::HeaderPaths fallbackHeaderPaths = CppModelManager::headerPaths();
    const CPlusPlus::LanguageFeatures defaultFeatures =
        CPlusPlus::LanguageFeatures::defaultFeatures();

    qCDebug(indexerLog) << "About to index" << files.size() << "files.";
    for (int i = 0; i < files.size(); ++i) {
        if (promise.isCanceled())
            break;

        const QString fileName = files.at(i);
        const QList<ProjectPart::ConstPtr> parts = CppModelManager::projectPart(fileName);
        const CPlusPlus::LanguageFeatures languageFeatures = parts.isEmpty()
                                                                 ? defaultFeatures
                                                                 : parts.first()->languageFeatures;
        sourceProcessor->setLanguageFeatures(languageFeatures);

        const bool isSourceFile = i < sourceCount;
        if (isSourceFile) {
            sourceProcessor->run(conf);
        } else if (!processingHeaders) {
            sourceProcessor->run(conf);

            processingHeaders = true;
        }

        qCDebug(indexerLog) << "  Indexing" << i + 1 << "of" << files.size() << ":" << fileName;
        ProjectExplorer::HeaderPaths headerPaths = parts.isEmpty()
                                                       ? fallbackHeaderPaths
                                                       : parts.first()->headerPaths;
        sourceProcessor->setHeaderPaths(headerPaths);
        sourceProcessor->run(FilePath::fromString(fileName));

        promise.setProgressValue(files.size() - sourceProcessor->todo().size());

        if (isSourceFile)
            sourceProcessor->resetEnvironment();
    }
    qCDebug(indexerLog) << "Indexing finished.";
}

static void parse(QPromise<void> &promise, const ParseParams &params)
{
    const QSet<QString> &files = params.sourceFiles;
    if (files.isEmpty())
        return;

    promise.setProgressRange(0, files.size());

    if (CppIndexingSupport::isFindErrorsIndexingActive())
        indexFindErrors(promise, params);
    else
        index(promise, params);

    promise.setProgressValue(files.size());
    CppModelManager::finishedRefreshingSourceFiles(files);
}

} // anonymous namespace

void SymbolSearcher::runSearch(QPromise<SearchResultItem> &promise)
{
    promise.setProgressRange(0, m_snapshot.size());
    promise.setProgressValue(0);
    int progress = 0;

    SearchSymbols search;
    search.setSymbolsToSearchFor(m_parameters.types);
    CPlusPlus::Snapshot::const_iterator it = m_snapshot.begin();

    QString findString = (m_parameters.flags & FindRegularExpression
                              ? m_parameters.text : QRegularExpression::escape(m_parameters.text));
    if (m_parameters.flags & FindWholeWords)
        findString = QString::fromLatin1("\\b%1\\b").arg(findString);
    QRegularExpression matcher(findString, (m_parameters.flags & FindCaseSensitively
                                                ? QRegularExpression::NoPatternOption
                                                : QRegularExpression::CaseInsensitiveOption));
    matcher.optimize();
    while (it != m_snapshot.end()) {
        promise.suspendIfRequested();
        if (promise.isCanceled())
            break;
        if (m_fileNames.isEmpty() || m_fileNames.contains(it.value()->filePath().path())) {
            SearchResultItems resultItems;
            auto filter = [&](const IndexItem::Ptr &info) -> IndexItem::VisitorResult {
                if (matcher.match(info->symbolName()).hasMatch()) {
                    QString text = info->symbolName();
                    QString scope = info->symbolScope();
                    if (info->type() == IndexItem::Function) {
                        QString name;
                        info->unqualifiedNameAndScope(info->symbolName(), &name, &scope);
                        text = name + info->symbolType();
                    } else if (info->type() == IndexItem::Declaration){
                        text = info->representDeclaration();
                    }

                    SearchResultItem item;
                    item.setPath(scope.split(QLatin1String("::"), Qt::SkipEmptyParts));
                    item.setLineText(text);
                    item.setIcon(info->icon());
                    item.setUserData(QVariant::fromValue(info));
                    resultItems << item;
                }

                return IndexItem::Recurse;
            };
            search(it.value())->visitAllChildren(filter);
            for (const SearchResultItem &item : std::as_const(resultItems))
                promise.addResult(item);
        }
        ++it;
        ++progress;
        promise.setProgressValue(progress);
    }
    promise.suspendIfRequested();
}

bool CppIndexingSupport::isFindErrorsIndexingActive()
{
    return Utils::qtcEnvironmentVariable("QTC_FIND_ERRORS_INDEXING") == "1";
}

QFuture<void> CppIndexingSupport::refreshSourceFiles(const QSet<QString> &sourceFiles,
                                                     CppModelManager::ProgressNotificationMode mode)
{
    ParseParams params;
    params.indexerFileSizeLimitInMb = indexerFileSizeLimitInMb();
    params.headerPaths = CppModelManager::headerPaths();
    params.workingCopy = CppModelManager::workingCopy();
    params.sourceFiles = sourceFiles;

    QFuture<void> result = Utils::asyncRun(CppModelManager::sharedThreadPool(), parse, params);
    m_synchronizer.addFuture(result);

    if (mode == CppModelManager::ForcedProgressNotification || sourceFiles.count() > 1) {
        Core::ProgressManager::addTask(result, Tr::tr("Parsing C/C++ Files"),
                                       CppEditor::Constants::TASK_INDEX);
    }

    return result;
}

} // namespace CppEditor
