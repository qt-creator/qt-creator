#include "builtinindexingsupport.h"
#include "cppmodelmanager.h"
#include "searchsymbols.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/runextensions.h>

#include <QCoreApplication>

using namespace CppTools;
using namespace CppTools::Internal;

namespace {

static void parse(QFutureInterface<void> &future,
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

        preproc->m_snapshot.remove(file);

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

    const QString conf = CPlusPlus::CppModelManagerInterface::configurationFileName();
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

    void runSearch(QFutureInterface<Find::SearchResultItem> &future)
    {
        future.setProgressRange(0, m_snapshot.size());
        future.setProgressValue(0);
        int progress = 0;

        SearchSymbols search;
        search.setSymbolsToSearchFor(m_parameters.types);
        search.setSeparateScope(true);
        CPlusPlus::Snapshot::const_iterator it = m_snapshot.begin();

        QString findString = (m_parameters.flags & Find::FindRegularExpression
                              ? m_parameters.text : QRegExp::escape(m_parameters.text));
        if (m_parameters.flags & Find::FindWholeWords)
            findString = QString::fromLatin1("\\b%1\\b").arg(findString);
        QRegExp matcher(findString, (m_parameters.flags & Find::FindCaseSensitively
                                     ? Qt::CaseSensitive : Qt::CaseInsensitive));
        while (it != m_snapshot.end()) {
            if (future.isPaused())
                future.waitForResume();
            if (future.isCanceled())
                break;
            if (m_fileNames.isEmpty() || m_fileNames.contains(it.value()->fileName())) {
                QVector<Find::SearchResultItem> resultItems;
                QList<ModelItemInfo> modelInfos = search(it.value());
                foreach (const ModelItemInfo &info, modelInfos) {
                    int index = matcher.indexIn(info.symbolName);
                    if (index != -1) {
                        QStringList path = info.fullyQualifiedName.mid(0,
                            info.fullyQualifiedName.size() - 1);
                        Find::SearchResultItem item;
                        item.path = path;
                        item.text = info.symbolName;
                        item.textMarkPos = -1;
                        item.textMarkLength = 0;
                        item.icon = info.icon;
                        item.lineNumber = -1;
                        item.userData = qVariantFromValue(info);
                        resultItems << item;
                    }
                }
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
    m_dumpFileNameWhileParsing = !qgetenv("QTCREATOR_DUMP_FILENAME_WHILE_PARSING").isNull();
}

BuiltinIndexingSupport::~BuiltinIndexingSupport()
{}

QFuture<void> BuiltinIndexingSupport::refreshSourceFiles(const QStringList &sourceFiles)
{
    CppModelManager *mgr = CppModelManager::instance();
    const WorkingCopy workingCopy = mgr->workingCopy();

    CppPreprocessor *preproc = new CppPreprocessor(mgr, m_dumpFileNameWhileParsing);
    preproc->setRevision(++m_revision);
    preproc->setProjectFiles(mgr->projectFiles());
    preproc->setIncludePaths(mgr->includePaths());
    preproc->setFrameworkPaths(mgr->frameworkPaths());
    preproc->setWorkingCopy(workingCopy);

    QFuture<void> result = QtConcurrent::run(&parse, preproc, sourceFiles);

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
        Core::ICore::progressManager()->addTask(result,
                                                QCoreApplication::translate("CppTools::Internal::BuiltinIndexingSupport", "Parsing"),
                                                QLatin1String(CppTools::Constants::TASK_INDEX));
    }

    return result;
}

SymbolSearcher *BuiltinIndexingSupport::createSymbolSearcher(SymbolSearcher::Parameters parameters, QSet<QString> fileNames)
{
    return new BuiltinSymbolSearcher(CppModelManager::instance()->snapshot(), parameters, fileNames);
}
