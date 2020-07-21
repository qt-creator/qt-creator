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

#include "builtinindexingsupport.h"

#include "builtineditordocumentparser.h"
#include "cppchecksymbols.h"
#include "cppmodelmanager.h"
#include "cppprojectfile.h"
#include "cppsourceprocessor.h"
#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"
#include "cpptoolsreuse.h"
#include "searchsymbols.h"

#include <coreplugin/icore.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cplusplus/LookupContext.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QRegularExpression>

using namespace CppTools;
using namespace CppTools::Internal;

static const bool FindErrorsIndexing = qgetenv("QTC_FIND_ERRORS_INDEXING") == "1";
static Q_LOGGING_CATEGORY(indexerLog, "qtc.cpptools.indexer", QtWarningMsg)

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
        const QString fileName = Utils::TemporaryDirectory::masterDirectoryPath()
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
        const QString fileName = document->fileName();

        foreach (const Document::DiagnosticMessage &message, document->diagnosticMessages()) {
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

void classifyFiles(const QSet<QString> &files, QStringList *headers, QStringList *sources)
{
    foreach (const QString &file, files) {
        if (ProjectFile::isSource(ProjectFile::classify(file)))
            sources->append(file);
        else
            headers->append(file);
    }
}

void indexFindErrors(QFutureInterface<void> &indexingFuture,
                     const QFutureInterface<void> &superFuture,
                     const ParseParams params)
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
        if (indexingFuture.isCanceled() || superFuture.isCanceled())
            break;

        const QString file = files.at(i);
        qDebug("FindErrorsIndexing: \"%s\"", qPrintable(file));

        // Parse the file as precisely as possible
        BuiltinEditorDocumentParser parser(file);
        parser.setReleaseSourceAndAST(false);
        parser.update({CppModelManager::instance()->workingCopy(), nullptr, Language::Cxx, false});
        CPlusPlus::Document::Ptr document = parser.document();
        QTC_ASSERT(document, return);

        // Write diagnostic messages
        taskFileWriter.process(document);

        // Look up symbols
        CPlusPlus::LookupContext context(document, parser.snapshot());
        CheckSymbols::go(document, context, QList<CheckSymbols::Result>()).waitForFinished();

        document->releaseSourceAndAST();

        indexingFuture.setProgressValue(files.size() - (files.size() - (i + 1)));
    }

    const QString elapsedTime = Utils::formatElapsedTime(timer.elapsed());
    qDebug("FindErrorsIndexing: %s", qPrintable(elapsedTime));
}

void index(QFutureInterface<void> &indexingFuture,
           const QFutureInterface<void> &superFuture,
           const ParseParams params)
{
    QScopedPointer<CppSourceProcessor> sourceProcessor(CppModelManager::createSourceProcessor());
    sourceProcessor->setFileSizeLimitInMb(params.indexerFileSizeLimitInMb);
    sourceProcessor->setHeaderPaths(params.headerPaths);
    sourceProcessor->setWorkingCopy(params.workingCopy);

    QStringList sources;
    QStringList headers;
    classifyFiles(params.sourceFiles, &headers, &sources);

    foreach (const QString &file, params.sourceFiles)
        sourceProcessor->removeFromCache(file);

    const int sourceCount = sources.size();
    QStringList files = sources + headers;

    sourceProcessor->setTodo(Utils::toSet(files));

    const QString conf = CppModelManager::configurationFileName();
    bool processingHeaders = false;

    CppModelManager *cmm = CppModelManager::instance();
    const ProjectExplorer::HeaderPaths fallbackHeaderPaths = cmm->headerPaths();
    const CPlusPlus::LanguageFeatures defaultFeatures =
            CPlusPlus::LanguageFeatures::defaultFeatures();

    qCDebug(indexerLog) << "About to index" << files.size() << "files.";
    for (int i = 0; i < files.size(); ++i) {
        if (indexingFuture.isCanceled() || superFuture.isCanceled())
            break;

        const QString fileName = files.at(i);
        const QList<ProjectPart::Ptr> parts = cmm->projectPart(fileName);
        const CPlusPlus::LanguageFeatures languageFeatures = parts.isEmpty()
                ? defaultFeatures
                : parts.first()->languageFeatures;
        sourceProcessor->setLanguageFeatures(languageFeatures);

        const bool isSourceFile = i < sourceCount;
        if (isSourceFile) {
            (void) sourceProcessor->run(conf);
        } else if (!processingHeaders) {
            (void) sourceProcessor->run(conf);

            processingHeaders = true;
        }

        qCDebug(indexerLog) << "  Indexing" << i + 1 << "of" << files.size() << ":" << fileName;
        ProjectExplorer::HeaderPaths headerPaths = parts.isEmpty()
                ? fallbackHeaderPaths
                : parts.first()->headerPaths;
        sourceProcessor->setHeaderPaths(headerPaths);
        sourceProcessor->run(fileName);

        indexingFuture.setProgressValue(files.size() - sourceProcessor->todo().size());

        if (isSourceFile)
            sourceProcessor->resetEnvironment();
    }
    qCDebug(indexerLog) << "Indexing finished.";
}

void parse(QFutureInterface<void> &indexingFuture,
           const QFutureInterface<void> &superFuture,
           const ParseParams params)
{
    const QSet<QString> &files = params.sourceFiles;
    if (files.isEmpty())
        return;

    indexingFuture.setProgressRange(0, files.size());

    if (FindErrorsIndexing)
        indexFindErrors(indexingFuture, superFuture, params);
    else
        index(indexingFuture, superFuture, params);

    indexingFuture.setProgressValue(files.size());
    CppModelManager::instance()->finishedRefreshingSourceFiles(files);
}

class BuiltinSymbolSearcher: public SymbolSearcher
{
public:
    BuiltinSymbolSearcher(const CPlusPlus::Snapshot &snapshot,
                          const Parameters &parameters, const QSet<QString> &fileNames)
        : m_snapshot(snapshot)
        , m_parameters(parameters)
        , m_fileNames(fileNames)
    {}

    ~BuiltinSymbolSearcher() override = default;

    void runSearch(QFutureInterface<Core::SearchResultItem> &future) override
    {
        future.setProgressRange(0, m_snapshot.size());
        future.setProgressValue(0);
        int progress = 0;

        SearchSymbols search;
        search.setSymbolsToSearchFor(m_parameters.types);
        CPlusPlus::Snapshot::const_iterator it = m_snapshot.begin();

        QString findString = (m_parameters.flags & Core::FindRegularExpression
                              ? m_parameters.text : QRegularExpression::escape(m_parameters.text));
        if (m_parameters.flags & Core::FindWholeWords)
            findString = QString::fromLatin1("\\b%1\\b").arg(findString);
        QRegularExpression matcher(findString, (m_parameters.flags & Core::FindCaseSensitively
                                                ? QRegularExpression::NoPatternOption
                                                : QRegularExpression::CaseInsensitiveOption));
        matcher.optimize();
        while (it != m_snapshot.end()) {
            if (future.isPaused())
                future.waitForResume();
            if (future.isCanceled())
                break;
            if (m_fileNames.isEmpty() || m_fileNames.contains(it.value()->fileName())) {
                QVector<Core::SearchResultItem> resultItems;
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

                        Core::SearchResultItem item;
                        item.path = scope.split(QLatin1String("::"), Qt::SkipEmptyParts);
                        item.text = text;
                        item.icon = info->icon();
                        item.userData = QVariant::fromValue(info);
                        resultItems << item;
                    }

                    return IndexItem::Recurse;
                };
                search(it.value())->visitAllChildren(filter);
                if (!resultItems.isEmpty())
                    future.reportResults(resultItems);
            }
            ++it;
            ++progress;
            future.setProgressValue(progress);
        }
        if (future.isPaused())
            future.waitForResume();
    }

private:
    const CPlusPlus::Snapshot m_snapshot;
    const Parameters m_parameters;
    const QSet<QString> m_fileNames;
};

} // anonymous namespace

BuiltinIndexingSupport::BuiltinIndexingSupport()
{
    m_synchronizer.setCancelOnWait(true);
}

BuiltinIndexingSupport::~BuiltinIndexingSupport() = default;

QFuture<void> BuiltinIndexingSupport::refreshSourceFiles(
        const QFutureInterface<void> &superFuture,
        const QSet<QString> &sourceFiles,
        CppModelManager::ProgressNotificationMode mode)
{
    CppModelManager *mgr = CppModelManager::instance();

    ParseParams params;
    params.indexerFileSizeLimitInMb = indexerFileSizeLimitInMb();
    params.headerPaths = mgr->headerPaths();
    params.workingCopy = mgr->workingCopy();
    params.sourceFiles = sourceFiles;

    QFuture<void> result = Utils::runAsync(mgr->sharedThreadPool(), parse, superFuture, params);

    if (m_synchronizer.futures().size() > 10) {
        QList<QFuture<void> > futures = m_synchronizer.futures();

        m_synchronizer.clearFutures();

        foreach (const QFuture<void> &future, futures) {
            if (!(future.isFinished() || future.isCanceled()))
                m_synchronizer.addFuture(future);
        }
    }

    m_synchronizer.addFuture(result);

    if (mode == CppModelManager::ForcedProgressNotification || sourceFiles.count() > 1) {
        Core::ProgressManager::addTask(result, QCoreApplication::translate("CppTools::Internal::BuiltinIndexingSupport", "Parsing C/C++ Files"),
                                                CppTools::Constants::TASK_INDEX);
    }

    return result;
}

SymbolSearcher *BuiltinIndexingSupport::createSymbolSearcher(
        const SymbolSearcher::Parameters &parameters, const QSet<QString> &fileNames)
{
    return new BuiltinSymbolSearcher(CppModelManager::instance()->snapshot(), parameters, fileNames);
}

bool BuiltinIndexingSupport::isFindErrorsIndexingActive()
{
    return FindErrorsIndexing;
}
