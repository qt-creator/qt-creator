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

#include "builtinindexingsupport.h"

#include "builtineditordocumentparser.h"
#include "cppchecksymbols.h"
#include "cppmodelmanager.h"
#include "cppprojectfile.h"
#include "cppsourceprocessor.h"
#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"
#include "searchsymbols.h"

#include <coreplugin/icore.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cplusplus/LookupContext.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>

using namespace CppTools;
using namespace CppTools::Internal;

static const bool FindErrorsIndexing = qgetenv("QTC_FIND_ERRORS_INDEXING") == "1";

namespace {

class ParseParams
{
public:
    ProjectPart::HeaderPaths headerPaths;
    WorkingCopy workingCopy;
    QSet<QString> sourceFiles;
};

class WriteTaskFileForDiagnostics
{
    Q_DISABLE_COPY(WriteTaskFileForDiagnostics)

public:
    WriteTaskFileForDiagnostics()
        : m_processedDiagnostics(0)
    {
        const QString fileName = QDir::tempPath()
                + QLatin1String("/qtc_findErrorsIndexing.diagnostics.")
                + QDateTime::currentDateTime().toString(QLatin1String("yyMMdd_HHmm"))
                + QLatin1String(".tasks");

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

    int processedDiagnostics() const { return m_processedDiagnostics; }

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
    int m_processedDiagnostics;
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

void indexFindErrors(QFutureInterface<void> &future, const ParseParams params)
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
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled())
            break;

        const QString file = files.at(i);
        qDebug("FindErrorsIndexing: \"%s\"", qPrintable(file));

        // Parse the file as precisely as possible
        BuiltinEditorDocumentParser parser(file);
        parser.setReleaseSourceAndAST(false);
        parser.update(BuiltinEditorDocumentParser::InMemoryInfo(false));
        CPlusPlus::Document::Ptr document = parser.document();
        QTC_ASSERT(document, return);

        // Write diagnostic messages
        taskFileWriter.process(document);

        // Look up symbols
        CPlusPlus::LookupContext context(document, parser.snapshot());
        CheckSymbols::go(document, context, QList<CheckSymbols::Result>()).waitForFinished();

        document->releaseSourceAndAST();

        future.setProgressValue(files.size() - (files.size() - (i + 1)));
    }

    const QTime format = QTime(0, 0, 0, 0).addMSecs(timer.elapsed() + 500);
    const QString time = format.toString(QLatin1String("hh:mm:ss"));
    qDebug("FindErrorsIndexing: Finished after %s.", qPrintable(time));
}

void index(QFutureInterface<void> &future, const ParseParams params)
{
    QScopedPointer<CppSourceProcessor> sourceProcessor(CppModelManager::createSourceProcessor());
    sourceProcessor->setHeaderPaths(params.headerPaths);
    sourceProcessor->setWorkingCopy(params.workingCopy);

    QStringList sources;
    QStringList headers;
    classifyFiles(params.sourceFiles, &headers, &sources);

    foreach (const QString &file, params.sourceFiles)
        sourceProcessor->removeFromCache(file);

    const int sourceCount = sources.size();
    QStringList files = sources + headers;

    sourceProcessor->setTodo(files.toSet());

    const QString conf = CppModelManager::configurationFileName();
    bool processingHeaders = false;

    CppModelManager *cmm = CppModelManager::instance();
    const ProjectPart::HeaderPaths fallbackHeaderPaths = cmm->headerPaths();
    const CPlusPlus::LanguageFeatures defaultFeatures =
            CPlusPlus::LanguageFeatures::defaultFeatures();
    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
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

        ProjectPart::HeaderPaths headerPaths = parts.isEmpty()
                ? fallbackHeaderPaths
                : parts.first()->headerPaths;
        sourceProcessor->setHeaderPaths(headerPaths);
        sourceProcessor->run(fileName);

        future.setProgressValue(files.size() - sourceProcessor->todo().size());

        if (isSourceFile)
            sourceProcessor->resetEnvironment();
    }
}

void parse(QFutureInterface<void> &future, const ParseParams params)
{
    const QSet<QString> &files = params.sourceFiles;
    if (files.isEmpty())
        return;

    future.setProgressRange(0, files.size());

    if (FindErrorsIndexing)
        indexFindErrors(future, params);
    else
        index(future, params);

    future.setProgressValue(files.size());
    CppModelManager::instance()->finishedRefreshingSourceFiles(files);
}

class BuiltinSymbolSearcher: public SymbolSearcher
{
public:
    BuiltinSymbolSearcher(const CPlusPlus::Snapshot &snapshot,
                          Parameters parameters, QSet<QString> fileNames)
        : m_snapshot(snapshot)
        , m_parameters(parameters)
        , m_fileNames(fileNames)
    {}

    ~BuiltinSymbolSearcher()
    {}

    void runSearch(QFutureInterface<Core::SearchResultItem> &future)
    {
        future.setProgressRange(0, m_snapshot.size());
        future.setProgressValue(0);
        int progress = 0;

        SearchSymbols search(CppToolsPlugin::stringTable());
        search.setSymbolsToSearchFor(m_parameters.types);
        CPlusPlus::Snapshot::const_iterator it = m_snapshot.begin();

        QString findString = (m_parameters.flags & Core::FindRegularExpression
                              ? m_parameters.text : QRegExp::escape(m_parameters.text));
        if (m_parameters.flags & Core::FindWholeWords)
            findString = QString::fromLatin1("\\b%1\\b").arg(findString);
        QRegExp matcher(findString, (m_parameters.flags & Core::FindCaseSensitively
                                     ? Qt::CaseSensitive : Qt::CaseInsensitive));
        while (it != m_snapshot.end()) {
            if (future.isPaused())
                future.waitForResume();
            if (future.isCanceled())
                break;
            if (m_fileNames.isEmpty() || m_fileNames.contains(it.value()->fileName())) {
                QVector<Core::SearchResultItem> resultItems;
                auto filter = [&](const IndexItem::Ptr &info) -> IndexItem::VisitorResult {
                    if (matcher.indexIn(info->symbolName()) != -1) {
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
                        item.path = scope.split(QLatin1String("::"), QString::SkipEmptyParts);
                        item.text = text;
                        item.textMarkPos = -1;
                        item.textMarkLength = 0;
                        item.icon = info->icon();
                        item.lineNumber = -1;
                        item.userData = qVariantFromValue(info);
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

BuiltinIndexingSupport::~BuiltinIndexingSupport()
{}

QFuture<void> BuiltinIndexingSupport::refreshSourceFiles(const QSet<QString> &sourceFiles,
    CppModelManager::ProgressNotificationMode mode)
{
    CppModelManager *mgr = CppModelManager::instance();

    ParseParams params;
    params.headerPaths = mgr->headerPaths();
    params.workingCopy = mgr->workingCopy();
    params.sourceFiles = sourceFiles;

    QFuture<void> result = QtConcurrent::run(&parse, params);

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

SymbolSearcher *BuiltinIndexingSupport::createSymbolSearcher(SymbolSearcher::Parameters parameters, QSet<QString> fileNames)
{
    return new BuiltinSymbolSearcher(CppModelManager::instance()->snapshot(), parameters, fileNames);
}

bool BuiltinIndexingSupport::isFindErrorsIndexingActive()
{
    return FindErrorsIndexing;
}
