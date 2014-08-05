#include "builtinindexingsupport.h"

#include "cppmodelmanager.h"
#include "cppsourceprocessor.h"
#include "searchsymbols.h"
#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"
#include "cppprojectfile.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/runextensions.h>

#include <QCoreApplication>

using namespace CppTools;
using namespace CppTools::Internal;

static const bool DumpFileNameWhileParsing = qgetenv("QTC_DUMP_FILENAME_WHILE_PARSING") == "1";

namespace {

class ParseParams
{
public:
    ParseParams()
        : dumpFileNameWhileParsing(DumpFileNameWhileParsing)
        , revision(0)
    {}

    int dumpFileNameWhileParsing;
    int revision;
    ProjectPart::HeaderPaths headerPaths;
    CppModelManagerInterface::WorkingCopy workingCopy;
    QStringList sourceFiles;
};

static void parse(QFutureInterface<void> &future, const ParseParams params)
{
    if (params.sourceFiles.isEmpty())
        return;

    QScopedPointer<CppSourceProcessor> sourceProcessor(CppModelManager::createSourceProcessor());
    sourceProcessor->setDumpFileNameWhileParsing(params.dumpFileNameWhileParsing);
    sourceProcessor->setRevision(params.revision);
    sourceProcessor->setHeaderPaths(params.headerPaths);
    sourceProcessor->setWorkingCopy(params.workingCopy);

    QStringList files = params.sourceFiles;
    QStringList sources;
    QStringList headers;

    foreach (const QString &file, files) {
        sourceProcessor->removeFromCache(file);
        if (ProjectFile::isSource(ProjectFile::classify(file)))
            sources.append(file);
        else
            headers.append(file);
    }

    const int sourceCount = sources.size();
    files = sources;
    files += headers;

    sourceProcessor->setTodo(files);

    future.setProgressRange(0, files.size());

    const QString conf = CppModelManagerInterface::configurationFileName();
    bool processingHeaders = false;

    CppModelManager *cmm = CppModelManager::instance();
    const ProjectPart::HeaderPaths fallbackHeaderPaths = cmm->headerPaths();
    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            break;

        const QString fileName = files.at(i);

        const bool isSourceFile = i < sourceCount;
        if (isSourceFile) {
            (void) sourceProcessor->run(conf);
        } else if (!processingHeaders) {
            (void) sourceProcessor->run(conf);

            processingHeaders = true;
        }

        QList<ProjectPart::Ptr> parts = cmm->projectPart(fileName);
        ProjectPart::HeaderPaths headerPaths = parts.isEmpty()
                ? fallbackHeaderPaths
                : parts.first()->headerPaths;
        sourceProcessor->setHeaderPaths(headerPaths);
        sourceProcessor->run(fileName);

        future.setProgressValue(files.size() - sourceProcessor->todo().size());

        if (isSourceFile)
            sourceProcessor->resetEnvironment();
    }

    future.setProgressValue(files.size());
    cmm->finishedRefreshingSourceFiles(files);
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
                            text = IndexItem::representDeclaration(info->symbolName(),
                                                                       info->symbolType());
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
    : m_revision(0)
{
    m_synchronizer.setCancelOnWait(true);
}

BuiltinIndexingSupport::~BuiltinIndexingSupport()
{}

QFuture<void> BuiltinIndexingSupport::refreshSourceFiles(const QStringList &sourceFiles,
    CppModelManagerInterface::ProgressNotificationMode mode)
{
    CppModelManager *mgr = CppModelManager::instance();

    ParseParams params;
    params.revision = ++m_revision;
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

    if (mode == CppModelManagerInterface::ForcedProgressNotification || sourceFiles.count() > 1) {
        Core::ProgressManager::addTask(result, QCoreApplication::translate("CppTools::Internal::BuiltinIndexingSupport", "Parsing C/C++ Files"),
                                                CppTools::Constants::TASK_INDEX);
    }

    return result;
}

SymbolSearcher *BuiltinIndexingSupport::createSymbolSearcher(SymbolSearcher::Parameters parameters, QSet<QString> fileNames)
{
    return new BuiltinSymbolSearcher(CppModelManager::instance()->snapshot(), parameters, fileNames);
}
