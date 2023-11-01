// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ilocatorfilter.h"

#include "../coreplugintr.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/fuzzymatcher.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QWaitCondition>

#include <unordered_set>

using namespace Tasking;
using namespace Utils;

/*!
    \class Core::ILocatorFilter
    \inheaderfile coreplugin/locator/ilocatorfilter.h
    \inmodule QtCreator

    \brief The ILocatorFilter class adds a locator filter.

    The filter is added to \uicontrol Tools > \uicontrol Locate.
*/

/*!
    \class Core::LocatorFilterEntry
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::LocatorFilterEntry::HighlightInfo
    \inmodule QtCreator
    \internal
*/

namespace Core {

// ResultsDeduplicator squashes upcoming results from various filters and removes
// duplicated entries. It also reports intermediate results (to be displayed in LocatorWidget).
//
// Assuming the results from filters come in this order (numbers are indices to filter results):
// 2, 4, 3, 1 - the various strategies are possible. The current strategy looks like this:
// - When 2nd result came - the result is stored
// - When 4th result came - the result is stored
// - When 3rd result came - it's being squased to the 2nd result and afterwards the 4th result
//                          result is being squashed into the common result list.
// - When 1st result came - the stored common list is squashed into the 1st result
//                          and intermediate results are reported (for 1-4 results).
//                          If the filterCount is 4, the deduplicator finishes now.
//                          If the filterCount is greater than 4, it waits for the remaining
//                          results.
//
// TODO: The other possible startegy would be to just store the newly reported data
//       and do the actual deduplication only when new results are reachable from 1st index
//       (i.e. skip the intermediate deduplication).
class ResultsDeduplicator
{
    enum class State {
        Awaiting, // Waiting in a separate thread for new data, or fetched the last new data and
                  // is currently deduplicating.
                  // This happens when all previous data were squashed in the separate thread but
                  // still some data needs to come (reportOutput wasn't called for all filters,
                  // yet). The expected number of calls to reportOutput equals m_filterCount.
        NewData,  // The new data came and the separate thread is being awaken in order to squash
                  // it. After the separate thread is awaken it transitions to Awaiting state.
        Canceled  // The Deduplicator task has been canceled.
    };

    // A separate item for keeping squashed entries. Call mergeWith() to squash a consecutive
    // results into this results.
    struct WorkingData {
        WorkingData() = default;
        WorkingData(const LocatorFilterEntries &entries, std::atomic<State> &state) {
            mergeWith(entries, state);
        }
        LocatorFilterEntries mergeWith(const LocatorFilterEntries &entries,
                                       std::atomic<State> &state) {
            LocatorFilterEntries results;
            results.reserve(entries.size());
            for (const LocatorFilterEntry &entry : entries) {
                if (state == State::Canceled)
                    return {};
                const auto &link = entry.linkForEditor;
                if (!link || m_cache.emplace(*link).second)
                    results.append(entry);
            }
            if (state == State::Canceled)
                return {};

            m_data += results;
            return results;
        }
        LocatorFilterEntries entries() const { return m_data; }
    private:
        LocatorFilterEntries m_data;
        std::unordered_set<Link> m_cache;
    };

public:
    // filterCount is the expected numbers of running filters. The separate thread executing run()
    // will stop after reportOutput was called filterCount times, for all different indices
    // in range [0, filterCount).
    ResultsDeduplicator(int filterCount)
        : m_filterCount(filterCount)
        , m_outputData(filterCount, {})
    {}

    void reportOutput(int index, const LocatorFilterEntries &outputData)
    // Called directly by running filters. The calls may come from main thread in case of
    // e.g. Sync task or directly from other threads when AsyncTask was used.
    {
        QTC_ASSERT(index >= 0, return);

        QMutexLocker locker(&m_mutex);
        // It may happen that the task tree was canceled, while tasks are still running in other
        // threads and are about to be canceled. In this case we just ignore the call.
        if (m_state == State::Canceled)
            return;
        QTC_ASSERT(index < m_filterCount, return);
        QTC_ASSERT(!m_outputData.at(index).has_value(), return);

        m_outputData[index] = outputData;
        m_state = State::NewData;
        m_waitCondition.wakeOne();
    }

    // Called when the LocatorMatcher was canceled. It wakes the separate thread in order to
    // finish it, soon.
    void cancel()
    {
        QMutexLocker locker(&m_mutex);
        m_state = State::Canceled;
        m_waitCondition.wakeOne();
    }

    // Called from the separate thread (ResultsCollector's thread)
    void run(QPromise<LocatorFilterEntries> &promise)
    {
        QList<std::optional<LocatorFilterEntries>> data;
        QList<std::optional<WorkingData>> workingList(m_filterCount, {});
        while (waitForData(&data)) {
            // Emit new results only when new data is reachable from the beginning (i.e. no gaps)
            int currentIndex = 0;
            int mergeToIndex = 0;
            bool hasGap = false;
            while (currentIndex < m_filterCount) {
                if (m_state == State::Canceled)
                    return;
                const auto &outputData = data.at(currentIndex);
                if (!outputData.has_value()) {
                    ++currentIndex;
                    mergeToIndex = currentIndex;
                    hasGap = true;
                    continue;
                }
                const auto &workingData = workingList.at(currentIndex);
                if (!workingData.has_value()) {
                    const bool mergeToCurrent = currentIndex == mergeToIndex;
                    const LocatorFilterEntries dataForIndex = mergeToCurrent ? *outputData
                        : LocatorFilterEntries();
                    workingList[currentIndex] = std::make_optional(WorkingData(dataForIndex,
                                                                               m_state));
                    if (m_state == State::Canceled)
                        return;
                    const LocatorFilterEntries newData = mergeToCurrent
                        ? workingList[currentIndex]->entries()
                        : workingList[mergeToIndex]->mergeWith(*outputData, m_state);
                    if (m_state == State::Canceled)
                        return;
                    if (!hasGap && !newData.isEmpty())
                        promise.addResult(newData);
                } else if (currentIndex != mergeToIndex) {
                    const LocatorFilterEntries newData
                        = workingList[mergeToIndex]->mergeWith(workingData->entries(), m_state);
                    workingList[currentIndex] = std::make_optional<WorkingData>({});
                    if (m_state == State::Canceled)
                        return;
                    if (!hasGap && !newData.isEmpty())
                        promise.addResult(newData);
                }
                ++currentIndex;
            }
            // All data arrived (no gap), so finish here
            if (!hasGap)
                return;
        }
    }

private:
    // Called from the separate thread, exclusively by run(). Checks if the new data already
    // came before sleeping with wait condition. If so, it doesn't sleep with wait condition,
    // but returns the data collected in meantime. Otherwise, it calls wait() on wait condition.
    bool waitForData(QList<std::optional<LocatorFilterEntries>> *data)
    {
        QMutexLocker locker(&m_mutex);
        if (m_state == State::Canceled)
            return false;
        if (m_state == State::NewData) {
            m_state = State::Awaiting; // Mark the state as awaiting to detect new calls to
                                       // setOutputData while the separate thread deduplicates the
                                       // new data.
            *data = m_outputData;
            return true;
        }
        m_waitCondition.wait(&m_mutex);
        QTC_ASSERT(m_state != State::Awaiting, return false);
        if (m_state == State::Canceled)
            return false;
        m_state = State::Awaiting; // Mark the state as awaiting to detect new calls to
                                   // setOutputData while the separate thread deduplicates the
                                   // new data.
        *data = m_outputData;
        return true;
    }

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    const int m_filterCount = 0;
    std::atomic<State> m_state = State::Awaiting;
    QList<std::optional<LocatorFilterEntries>> m_outputData;
};

// This instance of this object is created by LocatorMatcher tree.
// It starts a separate thread which collects and deduplicates the results reported
// by LocatorStorage instances. The ResultsCollector is started as a first task in
// LocatorMatcher and runs in parallel to all the filters started by LocatorMatcher.
// When all the results are reported (the expected number of reports is set with setFilterCount()),
// the ResultsCollector finishes. The intermediate results are reported with
// serialOutputDataReady() signal.
// The object of ResultsCollector is registered in Tasking namespace under the
// ResultsCollectorTask name.
class ResultsCollector : public QObject
{
    Q_OBJECT

public:
    ~ResultsCollector();
    void setFilterCount(int count);
    void start();

    bool isRunning() const { return m_watcher.get(); }

    std::shared_ptr<ResultsDeduplicator> deduplicator() const { return m_deduplicator; }

signals:
    void serialOutputDataReady(const LocatorFilterEntries &serialOutputData);
    void done();

private:
    int m_filterCount = 0;
    std::unique_ptr<QFutureWatcher<LocatorFilterEntries>> m_watcher;
    std::shared_ptr<ResultsDeduplicator> m_deduplicator;
};

ResultsCollector::~ResultsCollector()
{
    if (!isRunning())
        return;

    m_deduplicator->cancel();
    if (ExtensionSystem::PluginManager::futureSynchronizer()) {
        ExtensionSystem::PluginManager::futureSynchronizer()->addFuture(m_watcher->future());
        return;
    }
    m_watcher->future().waitForFinished();
}

void ResultsCollector::setFilterCount(int count)
{
    QTC_ASSERT(!isRunning(), return);
    QTC_ASSERT(count >= 0, return);

    m_filterCount = count;
}

void ResultsCollector::start()
{
    QTC_ASSERT(!m_watcher, return);
    QTC_ASSERT(!isRunning(), return);
    if (m_filterCount == 0) {
        emit done();
        return;
    }

    m_deduplicator.reset(new ResultsDeduplicator(m_filterCount));
    m_watcher.reset(new QFutureWatcher<LocatorFilterEntries>);
    connect(m_watcher.get(), &QFutureWatcherBase::resultReadyAt, this, [this](int index) {
        emit serialOutputDataReady(m_watcher->resultAt(index));
    });
    connect(m_watcher.get(), &QFutureWatcherBase::finished, this, [this] {
        emit done();
        m_watcher.release()->deleteLater();
        m_deduplicator.reset();
    });

    // TODO: When filterCount == 1, deliver results directly and finish?
    auto deduplicate = [](QPromise<LocatorFilterEntries> &promise,
                          const std::shared_ptr<ResultsDeduplicator> &deduplicator) {
        deduplicator->run(promise);
    };
    m_watcher->setFuture(Utils::asyncRun(deduplicate, m_deduplicator));
}

class ResultsCollectorTaskAdapter : public TaskAdapter<ResultsCollector>
{
public:
    ResultsCollectorTaskAdapter() {
        connect(task(), &ResultsCollector::done, this, [this] { emit done(true); });
    }
    void start() final { task()->start(); }
};

using ResultsCollectorTask = CustomTask<ResultsCollectorTaskAdapter>;

class LocatorStoragePrivate
{
public:
    LocatorStoragePrivate(const QString &input, int index,
                          const std::shared_ptr<ResultsDeduplicator> &deduplicator)
        : m_input(input)
        , m_index(index)
        , m_deduplicator(deduplicator)
    {}

    QString input() const { return m_input; }

    void reportOutput(const LocatorFilterEntries &outputData)
    {
        QMutexLocker locker(&m_mutex);
        QTC_ASSERT(m_deduplicator, return);
        reportOutputImpl(outputData);
    }

    void finalize()
    {
        QMutexLocker locker(&m_mutex);
        if (m_deduplicator)
            reportOutputImpl({});
    }

private:
    // Call me with mutex locked
    void reportOutputImpl(const LocatorFilterEntries &outputData)
    {
        QTC_ASSERT(m_index >= 0, return);
        m_deduplicator->reportOutput(m_index, outputData);
        // Deliver results only once for all copies of the storage, drop ref afterwards
        m_deduplicator.reset();
    }

    const QString m_input;
    const int m_index = -1;
    std::shared_ptr<ResultsDeduplicator> m_deduplicator;
    QMutex m_mutex = {};
};

QString LocatorStorage::input() const
{
    QTC_ASSERT(d, return {});
    return d->input();
}

void LocatorStorage::reportOutput(const LocatorFilterEntries &outputData) const
{
    QTC_ASSERT(d, return);
    d->reportOutput(outputData);
}

void LocatorStorage::finalize() const
{
    QTC_ASSERT(d, return);
    d->finalize();
}

class LocatorMatcherPrivate
{
public:
    LocatorMatcherTasks m_tasks;
    QString m_input;
    LocatorFilterEntries m_output;
    int m_parallelLimit = 0;
    std::unique_ptr<TaskTree> m_taskTree;
};

LocatorMatcher::LocatorMatcher()
    : d(new LocatorMatcherPrivate) {}

LocatorMatcher::~LocatorMatcher() = default;

void LocatorMatcher::setTasks(const LocatorMatcherTasks &tasks)
{
    d->m_tasks = tasks;
}

void LocatorMatcher::setInputData(const QString &inputData)
{
    d->m_input = inputData;
}

void LocatorMatcher::setParallelLimit(int limit)
{
    d->m_parallelLimit = limit;
}

void LocatorMatcher::start()
{
    QTC_ASSERT(!isRunning(), return);
    d->m_output = {};
    d->m_taskTree.reset(new TaskTree);

    struct CollectorStorage
    {
        ResultsCollector *m_collector = nullptr;
    };
    TreeStorage<CollectorStorage> collectorStorage;

    const int filterCount = d->m_tasks.size();
    const auto onCollectorSetup = [this, filterCount, collectorStorage](ResultsCollector &collector) {
        collectorStorage->m_collector = &collector;
        collector.setFilterCount(filterCount);
        connect(&collector, &ResultsCollector::serialOutputDataReady,
                this, [this](const LocatorFilterEntries &serialOutputData) {
            d->m_output += serialOutputData;
            emit serialOutputDataReady(serialOutputData);
        });
    };
    const auto onCollectorDone = [collectorStorage](const ResultsCollector &collector) {
        Q_UNUSED(collector)
        collectorStorage->m_collector = nullptr;
    };

    QList<GroupItem> parallelTasks {parallelLimit(d->m_parallelLimit)};

    const auto onSetup = [this, collectorStorage](const TreeStorage<LocatorStorage> &storage,
                                                    int index) {
        return [this, collectorStorage, storage, index] {
            ResultsCollector *collector = collectorStorage->m_collector;
            QTC_ASSERT(collector, return);
            *storage = std::make_shared<LocatorStoragePrivate>(d->m_input, index,
                                                               collector->deduplicator());
        };
    };

    const auto onDone = [](const TreeStorage<LocatorStorage> &storage) {
        return [storage] { storage->finalize(); };
    };

    int index = 0;
    for (const LocatorMatcherTask &task : std::as_const(d->m_tasks)) {
        const auto storage = task.storage;
        const Group group {
            finishAllAndDone,
            Storage(storage),
            onGroupSetup(onSetup(storage, index)),
            onGroupDone(onDone(storage)),
            onGroupError(onDone(storage)),
            task.task
        };
        parallelTasks << group;
        ++index;
    }

    const Group root {
        parallel,
        Storage(collectorStorage),
        ResultsCollectorTask(onCollectorSetup, onCollectorDone, onCollectorDone),
        Group {
            parallelTasks
        }
    };

    d->m_taskTree->setRecipe(root);

    const auto onFinish = [this](bool success) {
        return [this, success] {
            emit done(success);
            d->m_taskTree.release()->deleteLater();
        };
    };
    connect(d->m_taskTree.get(), &TaskTree::done, this, onFinish(true));
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, onFinish(false));
    d->m_taskTree->start();
}

void LocatorMatcher::stop()
{
    if (!isRunning())
        return;

    d->m_taskTree->stop();
    d->m_taskTree.reset();
}

bool LocatorMatcher::isRunning() const
{
    return d->m_taskTree.get() && d->m_taskTree->isRunning();
}

LocatorFilterEntries LocatorMatcher::outputData() const
{
    return d->m_output;
}

LocatorFilterEntries LocatorMatcher::runBlocking(const LocatorMatcherTasks &tasks,
                                                 const QString &input, int parallelLimit)
{
    LocatorMatcher tree;
    tree.setTasks(tasks);
    tree.setInputData(input);
    tree.setParallelLimit(parallelLimit);

    QEventLoop loop;
    connect(&tree, &LocatorMatcher::done, &loop, [&loop] { loop.quit(); });
    tree.start();
    if (tree.isRunning())
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    return tree.outputData();
}

static QHash<MatcherType, QList<LocatorMatcherTaskCreator>> s_matcherCreators = {};

void LocatorMatcher::addMatcherCreator(MatcherType type, const LocatorMatcherTaskCreator &creator)
{
    QTC_ASSERT(creator, return);
    s_matcherCreators[type].append(creator);
}

LocatorMatcherTasks LocatorMatcher::matchers(MatcherType type)
{
    const QList<LocatorMatcherTaskCreator> creators = s_matcherCreators.value(type);
    LocatorMatcherTasks result;
    for (const LocatorMatcherTaskCreator &creator : creators)
        result << creator();
    return result;
}

static QList<ILocatorFilter *> g_locatorFilters;

/*!
    Constructs a locator filter with \a parent. Call from subclasses.
*/
ILocatorFilter::ILocatorFilter(QObject *parent)
    : QObject(parent)
{
    g_locatorFilters.append(this);
}

ILocatorFilter::~ILocatorFilter()
{
    g_locatorFilters.removeOne(this);
}

/*!
    Returns the list of all locator filters.
*/
const QList<ILocatorFilter *> ILocatorFilter::allLocatorFilters()
{
    return g_locatorFilters;
}

/*!
    Specifies a shortcut string that can be used to explicitly choose this
    filter in the locator input field by preceding the search term with the
    shortcut string and a whitespace.

    The default value is an empty string.

    \sa setShortcutString()
*/
QString ILocatorFilter::shortcutString() const
{
    return m_shortcut;
}

/*!
    \internal
    Sets the refresh recipe for refreshing cached data.
*/
void ILocatorFilter::setRefreshRecipe(const std::optional<GroupItem> &recipe)
{
    m_refreshRecipe = recipe;
}

/*!
    Returns the refresh recipe for refreshing cached data. By default, the locator filter has
    no recipe set, so that it won't be refreshed.
*/
std::optional<GroupItem> ILocatorFilter::refreshRecipe() const
{
    return m_refreshRecipe;
}

/*!
    Sets the default \a shortcut string that can be used to explicitly choose
    this filter in the locator input field. Call for example from the
    constructor of subclasses.

    \sa shortcutString()
*/
void ILocatorFilter::setDefaultShortcutString(const QString &shortcut)
{
    m_defaultShortcut = shortcut;
    m_shortcut = shortcut;
}

/*!
    Sets the current shortcut string of the filter to \a shortcut. Use
    setDefaultShortcutString() if you want to set the default shortcut string
    instead.

    \sa setDefaultShortcutString()
*/
void ILocatorFilter::setShortcutString(const QString &shortcut)
{
    m_shortcut = shortcut;
}

QKeySequence ILocatorFilter::defaultKeySequence() const
{
    return m_defaultKeySequence;
}

void ILocatorFilter::setDefaultKeySequence(const QKeySequence &sequence)
{
    m_defaultKeySequence = sequence;
}

std::optional<QString> ILocatorFilter::defaultSearchText() const
{
    return m_defaultSearchText;
}

void ILocatorFilter::setDefaultSearchText(const QString &defaultSearchText)
{
    m_defaultSearchText = defaultSearchText;
}

const char kShortcutStringKey[] = "shortcut";
const char kIncludedByDefaultKey[] = "includeByDefault";

/*!
    Returns data that can be used to restore the settings for this filter
    (for example at startup).
    By default, adds the base settings (shortcut string, included by default)
    and calls saveState() with a JSON object where subclasses should write
    their custom settings.

    \sa restoreState()
*/
QByteArray ILocatorFilter::saveState() const
{
    QJsonObject obj;
    if (shortcutString() != m_defaultShortcut)
        obj.insert(kShortcutStringKey, shortcutString());
    if (isIncludedByDefault() != m_defaultIncludedByDefault)
        obj.insert(kIncludedByDefaultKey, isIncludedByDefault());
    saveState(obj);
    if (obj.isEmpty())
        return {};
    QJsonDocument doc;
    doc.setObject(obj);
    return doc.toJson(QJsonDocument::Compact);
}

/*!
    Restores the \a state of the filter from data previously created by
    saveState().

    \sa saveState()
*/
void ILocatorFilter::restoreState(const QByteArray &state)
{
    QJsonDocument doc = QJsonDocument::fromJson(state);
    if (state.isEmpty() || doc.isObject()) {
        const QJsonObject obj = doc.object();
        setShortcutString(obj.value(kShortcutStringKey).toString(m_defaultShortcut));
        setIncludedByDefault(obj.value(kIncludedByDefaultKey).toBool(m_defaultIncludedByDefault));
        restoreState(obj);
    } else {
        // TODO read old settings, remove some time after Qt Creator 4.15
        m_shortcut = m_defaultShortcut;
        m_includedByDefault = m_defaultIncludedByDefault;

        // TODO this reads legacy settings from Qt Creator < 4.15
        QDataStream in(state);
        in >> m_shortcut;
        in >> m_includedByDefault;
    }
}

/*!
    Opens a dialog for the \a parent widget that allows the user to configure
    various aspects of the filter. Called when the user requests to configure
    the filter.

    Set \a needsRefresh to \c true, if a refresh should be done after
    closing the dialog. Return \c false if the user canceled the dialog.

    The default implementation allows changing the shortcut and whether the
    filter is included by default.

    \sa refreshRecipe()
*/
bool ILocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    return openConfigDialog(parent, nullptr);
}

/*!
    Returns whether a case sensitive or case insensitive search should be
    performed for the search term \a str.
*/
Qt::CaseSensitivity ILocatorFilter::caseSensitivity(const QString &str)
{
    return str == str.toLower() ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

/*!
    Creates the search term \a text as a regular expression with case
    sensitivity set to \a caseSensitivity. Pass true to \a multiWord if the pattern is
    expected to contain spaces.
*/
QRegularExpression ILocatorFilter::createRegExp(const QString &text,
                                                Qt::CaseSensitivity caseSensitivity,
                                                bool multiWord)
{
    return FuzzyMatcher::createRegExp(text, caseSensitivity, multiWord);
}

/*!
    Returns information for highlighting the results of matching the regular
    expression, specified by \a match, for the data of the type \a dataType.
*/
LocatorFilterEntry::HighlightInfo ILocatorFilter::highlightInfo(
        const QRegularExpressionMatch &match, LocatorFilterEntry::HighlightInfo::DataType dataType)
{
    const FuzzyMatcher::HighlightingPositions positions =
            FuzzyMatcher::highlightingPositions(match);

    return LocatorFilterEntry::HighlightInfo(positions.starts, positions.lengths, dataType);
}

/*!
    Specifies a title for configuration dialogs.
*/
QString ILocatorFilter::msgConfigureDialogTitle()
{
    return Tr::tr("Filter Configuration");
}

/*!
    Specifies a label for the prefix input field in configuration dialogs.
*/
QString ILocatorFilter::msgPrefixLabel()
{
    return Tr::tr("Prefix:");
}

/*!
    Specifies a tooltip for the  prefix input field in configuration dialogs.
*/
QString ILocatorFilter::msgPrefixToolTip()
{
    return Tr::tr("Type the prefix followed by a space and search term to restrict search to the filter.");
}

/*!
    Specifies a label for the include by default input field in configuration
    dialogs.
*/
QString ILocatorFilter::msgIncludeByDefault()
{
    return Tr::tr("Include by default");
}

/*!
    Specifies a tooltip for the include by default input field in configuration
    dialogs.
*/
QString ILocatorFilter::msgIncludeByDefaultToolTip()
{
    return Tr::tr("Include the filter when not using a prefix for searches.");
}

/*!
    Returns whether a configuration dialog is available for this filter.

    The default is \c true.

    \sa setConfigurable()
*/
bool ILocatorFilter::isConfigurable() const
{
    return m_isConfigurable;
}

/*!
    Returns whether using the shortcut string is required to use this filter.
    The default is \c false.

    \sa shortcutString()
    \sa setIncludedByDefault()
*/
bool ILocatorFilter::isIncludedByDefault() const
{
    return m_includedByDefault;
}

/*!
    Sets the default setting for whether using the shortcut string is required
    to use this filter to \a includedByDefault.

    Call for example from the constructor of subclasses.

    \sa isIncludedByDefault()
*/
void ILocatorFilter::setDefaultIncludedByDefault(bool includedByDefault)
{
    m_defaultIncludedByDefault = includedByDefault;
    m_includedByDefault = includedByDefault;
}

/*!
    Sets whether using the shortcut string is required to use this filter to
    \a includedByDefault. Use setDefaultIncludedByDefault() if you want to
    set the default value instead.

    \sa setDefaultIncludedByDefault()
*/
void ILocatorFilter::setIncludedByDefault(bool includedByDefault)
{
    m_includedByDefault = includedByDefault;
}

/*!
    Returns whether the filter should be hidden in the
    \uicontrol {Locator filters} filter, menus, and locator settings.

    The default is \c false.

    \sa setHidden()
*/
bool ILocatorFilter::isHidden() const
{
    return m_hidden;
}

/*!
    Sets the filter in the \uicontrol {Locator filters} filter, menus, and
    locator settings to \a hidden. Call in the constructor of subclasses.
*/
void ILocatorFilter::setHidden(bool hidden)
{
    m_hidden = hidden;
}

/*!
    Returns whether the filter is currently available. Disabled filters are
    neither visible in menus nor included in searches, even when the search is
    prefixed with their shortcut string.

    The default is \c true.

    \sa setEnabled()
*/
bool ILocatorFilter::isEnabled() const
{
    return m_enabled;
}

/*!
    Returns the filter's unique ID.

    \sa setId()
*/
Id ILocatorFilter::id() const
{
    return m_id;
}

/*!
    Returns the filter's action ID.
*/
Id ILocatorFilter::actionId() const
{
    return m_id.withPrefix("Locator.");
}

/*!
    Returns the filter's translated display name.

    \sa setDisplayName()
*/
QString ILocatorFilter::displayName() const
{
    return m_displayName;
}

/*!
    Returns the priority that is used for ordering the results when multiple
    filters are used.

    The default is ILocatorFilter::Medium.

    \sa setPriority()
*/
ILocatorFilter::Priority ILocatorFilter::priority() const
{
    return m_priority;
}

/*!
    Sets whether the filter is currently available to \a enabled.

    \sa isEnabled()
*/
void ILocatorFilter::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

/*!
    Sets the filter's unique \a id.
    Subclasses must set the ID in their constructor.

    \sa id()
*/
void ILocatorFilter::setId(Id id)
{
    m_id = id;
}

/*!
    Sets the \a priority of results of this filter in the result list.

    \sa priority()
*/
void ILocatorFilter::setPriority(Priority priority)
{
    m_priority = priority;
}

/*!
    Sets the translated display name of this filter to \a
    displayString.

    Subclasses must set the display name in their constructor.

    \sa displayName()
*/
void ILocatorFilter::setDisplayName(const QString &displayString)
{
    m_displayName = displayString;
}

/*!
    Returns a longer, human-readable description of what the filter does.

    \sa setDescription()
*/
QString ILocatorFilter::description() const
{
    return m_description;
}

/*!
    Sets the longer, human-readable \a description of what the filter does.

    \sa description()
*/
void ILocatorFilter::setDescription(const QString &description)
{
    m_description = description;
}

/*!
    Sets whether the filter provides a configuration dialog to \a configurable.
    Most filters should at least provide the default dialog.

    \sa isConfigurable()
*/
void ILocatorFilter::setConfigurable(bool configurable)
{
    m_isConfigurable = configurable;
}

/*!
    Shows the standard configuration dialog with options for the prefix string
    and for isIncludedByDefault(). The \a additionalWidget is added at the top.
    Ownership of \a additionalWidget stays with the caller, but its parent is
    reset to \c nullptr.

    Returns \c false if the user canceled the dialog.
*/
bool ILocatorFilter::openConfigDialog(QWidget *parent, QWidget *additionalWidget)
{
    QDialog dialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    dialog.setWindowTitle(msgConfigureDialogTitle());

    auto vlayout = new QVBoxLayout(&dialog);
    auto hlayout = new QHBoxLayout;
    QLineEdit *shortcutEdit = new QLineEdit(shortcutString());
    QCheckBox *includeByDefault = new QCheckBox(msgIncludeByDefault());
    includeByDefault->setToolTip(msgIncludeByDefaultToolTip());
    includeByDefault->setChecked(isIncludedByDefault());

    auto prefixLabel = new QLabel(msgPrefixLabel());
    prefixLabel->setToolTip(msgPrefixToolTip());
    hlayout->addWidget(prefixLabel);
    hlayout->addWidget(shortcutEdit);
    hlayout->addWidget(includeByDefault);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (additionalWidget)
        vlayout->addWidget(additionalWidget);
    vlayout->addLayout(hlayout);
    vlayout->addStretch();
    vlayout->addWidget(buttonBox);

    bool accepted = false;
    if (dialog.exec() == QDialog::Accepted) {
        setShortcutString(shortcutEdit->text().trimmed());
        setIncludedByDefault(includeByDefault->isChecked());
        accepted = true;
    }
    if (additionalWidget) {
        additionalWidget->setVisible(false);
        additionalWidget->setParent(nullptr);
    }
    return accepted;
}

/*!
    Saves the filter settings and state to the JSON \a object.

    The default implementation does nothing.

    Implementations should write key-value pairs to the \a object for their
    custom settings that changed from the default. Default values should
    never be saved.
*/
void ILocatorFilter::saveState(QJsonObject &object) const
{
    Q_UNUSED(object)
}

/*!
    Reads the filter settings and state from the JSON \a object

    The default implementation does nothing.

    Implementations should read their custom settings from the \a object,
    resetting any missing setting to its default value.
*/
void ILocatorFilter::restoreState(const QJsonObject &object)
{
    Q_UNUSED(object)
}

/*!
    Returns if \a state must be restored via pre-4.15 settings reading.
*/
bool ILocatorFilter::isOldSetting(const QByteArray &state)
{
    if (state.isEmpty())
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(state);
    return !doc.isObject();
}

/*!
    \enum Core::ILocatorFilter::Priority

    This enum value holds the priority that is used for ordering the results
    when multiple filters are used.

    \value  Highest
            The results for this filter are placed above the results for filters
            that have other priorities.
    \value  High
    \value  Medium
            The default value.
    \value  Low
            The results for this filter are placed below the results for filters
            that have other priorities.
*/

/*!
    \enum Core::ILocatorFilter::MatchLevel

    This enum value holds the level for ordering the results based on how well
    they match the search criteria.

    \value Best
           The result is the best match for the regular expression.
    \value Better
    \value Good
    \value Normal
    \value Count
           The result has the highest number of matches for the regular
           expression.
*/

std::atomic_int s_executeId = 0;


class LocatorFileCachePrivate
{
public:
    bool isValid() const { return bool(m_generator); }
    void invalidate();
    bool ensureValidated();
    void bumpExecutionId() { m_executionId = s_executeId.fetch_add(1) + 1; }
    void update(const LocatorFileCachePrivate &newCache);
    void setGeneratorProvider(const LocatorFileCache::GeneratorProvider &provider)
    { m_provider = provider; }
    void setGenerator(const LocatorFileCache::FilePathsGenerator &generator);
    LocatorFilterEntries generate(const QFuture<void> &future, const QString &input);

    // Is persistent, does not reset on invalidate
    LocatorFileCache::GeneratorProvider m_provider;
    LocatorFileCache::FilePathsGenerator m_generator;
    int m_executionId = 0;

    std::optional<FilePaths> m_filePaths;

    QString m_lastInput;
    std::optional<FilePaths> m_cache;
};

// Clears all but provider
void LocatorFileCachePrivate::invalidate()
{
    LocatorFileCachePrivate that;
    that.m_provider = m_provider;
    *this = that;
}

/*!
    \internal

    Returns true if the cache is valid. Otherwise, tries to validate the cache and returns whether
    the validation succeeded.

    When the cache is valid, it does nothing and returns true.
    Otherwise, when the GeneratorProvider is not set, it does nothing and returns false.
    Otherwise, the GeneratorProvider is used for recreating the FilePathsGenerator.
    If the recreated FilePathsGenerator is not empty, it return true.
    Otherwise, it returns false;
*/
bool LocatorFileCachePrivate::ensureValidated()
{
    if (isValid())
        return true;

    if (!m_provider)
        return false;

    invalidate();
    m_generator = m_provider();
    return isValid();
}

void LocatorFileCachePrivate::update(const LocatorFileCachePrivate &newCache)
{
    if (m_executionId != newCache.m_executionId)
        return; // The mismatching executionId was detected, ignoring the update...
    auto provider = m_provider;
    *this = newCache;
    m_provider = provider;
}

void LocatorFileCachePrivate::setGenerator(const LocatorFileCache::FilePathsGenerator &generator)
{
    invalidate();
    m_generator = generator;
}

static bool containsPathSeparator(const QString &candidate)
{
    return candidate.contains('/') || candidate.contains('*');
};

/*!
    \internal

    Uses the generator to update the cache if needed and returns entries for the input.
    Uses the cached data when no need for re-generation. Updates the cache accordingly.
*/
LocatorFilterEntries LocatorFileCachePrivate::generate(const QFuture<void> &future,
                                                       const QString &input)
{
    QTC_ASSERT(isValid(), return {});

    // If search string contains spaces, treat them as wildcard '*' and search in full path
    const QString wildcardInput = QDir::fromNativeSeparators(input).replace(' ', '*');
    const Link inputLink = Link::fromString(wildcardInput, true);
    const QString newInput = inputLink.targetFilePath.toString();
    const QRegularExpression regExp = ILocatorFilter::createRegExp(newInput);
    if (!regExp.isValid())
        return {}; // Don't clear the cache - still remember the cache for the last valid input.

    if (future.isCanceled())
        return {};

    const bool hasPathSeparator = containsPathSeparator(newInput);
    const bool containsLastInput = !m_lastInput.isEmpty() && newInput.contains(m_lastInput);
    const bool pathSeparatorAdded = !containsPathSeparator(m_lastInput) && hasPathSeparator;
    const bool searchInCache = m_filePaths && m_cache && containsLastInput && !pathSeparatorAdded;

    std::optional<FilePaths> newPaths = m_filePaths;
    if (!searchInCache && !newPaths) {
        newPaths = m_generator(future);
        if (future.isCanceled()) // Ensure we got not canceled results from generator.
            return {};
    }

    const FilePaths &sourcePaths = searchInCache ? *m_cache : *newPaths;
    LocatorFileCache::MatchedEntries entries = {};
    const FilePaths newCache = LocatorFileCache::processFilePaths(
        future, sourcePaths, hasPathSeparator, regExp, inputLink, &entries);
    for (auto &entry : entries) {
        if (future.isCanceled())
            return {};

        if (entry.size() < 1000)
            Utils::sort(entry, LocatorFilterEntry::compareLexigraphically);
    }

    if (future.isCanceled())
        return {};

    // Update all the cache data in one go
    m_filePaths = newPaths;
    m_lastInput = newInput;
    m_cache = newCache;

    return std::accumulate(std::begin(entries), std::end(entries), LocatorFilterEntries());
}

/*!
    \class Core::LocatorFileCache
    \inmodule QtCreator

    \brief The LocatorFileCache class encapsulates all the responsibilities needed for
           implementing a cache for file filters.

    LocatorFileCache serves as a replacement for the old BaseFileFilter interface.
*/

/*!
    Constructs an invalid cache.

    The cache is considered to be in an invalid state after a call to invalidate(),
    of after a call to setFilePathsGenerator() when passed functions was empty.

    It it possible to setup the automatic validator for the cache through the
    setGeneratorProvider().

    \sa invalidate, setGeneratorProvider, setFilePathsGenerator, setFilePaths
*/

LocatorFileCache::LocatorFileCache()
    : d(new LocatorFileCachePrivate) {}

/*!
    Invalidates the cache.

    In order to validate it, use either setFilePathsGenerator() or setFilePaths().
    The cache may be automatically validated if the GeneratorProvider was set
    through the setGeneratorProvider().

    \note This function invalidates the cache permanently, clearing all the cached data,
          and removing the stored generator. The stored generator provider is preserved.
*/
void LocatorFileCache::invalidate()
{
    d->invalidate();
}

/*!
    Sets the file path generator provider.

    The \a provider serves for an automatic validation of the invalid cache by recreating
    the FilePathsGenerator. The automatic validation happens when the LocatorMatcherTask returned
    by matcher() is being started, and the cache is not valid at that moment. In this case
    the stored \a provider is being called.

    The passed \a provider function is always called from the main thread. If needed, it is
    called prior to starting an asynchronous task that collects the locator filter results.

    When this function is called, the cache isn't invalidated.
    Whenever cache's invalidation happens, e.g. when invalidate(), setFilePathsGenerator() or
    setFilePaths() is called, the stored GeneratorProvider is being preserved.
    In order to clear the stored GeneratorProvider, call this method with an empty
    function {}.
*/
void LocatorFileCache::setGeneratorProvider(const GeneratorProvider &provider)
{
    d->setGeneratorProvider(provider);
}

std::optional<FilePaths> LocatorFileCache::filePaths() const
{
    return d->m_filePaths;
}

/*!
    Sets the file path generator.

    The \a generator serves for returning the full input list of file paths when the
    associated LocatorMatherTask is being run in a separate thread. When the computation of the
    full list of file paths takes a considerable amount of time, this computation may
    be potentially moved to the separate thread, provided that all the dependent data may be safely
    passed to the \a generator function when this function is being set in the main thread.

    The passed \a generator is always called exclusively from the non-main thread when running
    LocatorMatcherTask returned by matcher(). It is called when the cached data is
    empty or when it needs to be regenerated due to a new search which can't reuse
    the cache from the previous search.

    Generating a new file path list may be a time consuming task. In order to finish the task early
    when being canceled, the \e future argument of the FilePathsGenerator may be used.
    The FilePathsGenerator returns the full list of file paths used for file filter's processing.

    Whenever it is possible to postpone the creation of a file path list so that it may be done
    safely later from the non-main thread, based on some other reentrant/thread-safe data,
    this method should be used. The other dependent data should be passed by lambda capture.
    The body of the passed \a generator should take extra care for ensuring that the passed
    other data via lambda captures are reentrant and the lambda body is thread safe.
    See the example usage of the generator inside CppIncludesFilter implementation.

    Otherwise, when postponing the creation of file paths list isn't safe, use setFilePaths()
    with ready made list, prepared in main thread.

    \note This function invalidates the cache, clearing all the cached data,
          and if the passed generator is non-empty, the cache is set to a valid state.
          The stored generator provider is preserved.

    \sa setGeneratorProvider, setFilePaths
*/
void LocatorFileCache::setFilePathsGenerator(const FilePathsGenerator &generator)
{
    d->setGenerator(generator);
}

/*!
    Wraps the passed \a filePaths into a trivial FilePathsGenerator and sets it
    as a cache's generator.

    \note This function invalidates the cache temporarily, clearing all the cached data,
          and sets it to a valid state with the new generator for the passed \a filePaths.
          The stored generator provider is preserved.

    \sa setGeneratorProvider
*/
void LocatorFileCache::setFilePaths(const FilePaths &filePaths)
{
    setFilePathsGenerator(filePathsGenerator(filePaths));
    d->m_filePaths = filePaths;
}

/*!
    Adapts the \a filePaths list into a LocatorFileCacheGenerator.
    Useful when implementing GeneratorProvider in case a creation of file paths
    can't be invoked from the non-main thread.
*/
LocatorFileCache::FilePathsGenerator LocatorFileCache::filePathsGenerator(
    const FilePaths &filePaths)
{
    return [filePaths](const QFuture<void> &) { return filePaths; };
}

static ILocatorFilter::MatchLevel matchLevelFor(const QRegularExpressionMatch &match,
                                                const QString &matchText)
{
    const int consecutivePos = match.capturedStart(1);
    if (consecutivePos == 0)
        return ILocatorFilter::MatchLevel::Best;
    if (consecutivePos > 0) {
        const QChar prevChar = matchText.at(consecutivePos - 1);
        if (prevChar == '_' || prevChar == '.')
            return ILocatorFilter::MatchLevel::Better;
    }
    if (match.capturedStart() == 0)
        return ILocatorFilter::MatchLevel::Good;
    return ILocatorFilter::MatchLevel::Normal;
}

/*!
    Helper used internally and by SpotlightLocatorFilter.

    To be called from non-main thread. The cancellation is controlled by the passed \a future.
    This function periodically checks for the cancellation state of the \a future and returns
    early when cancellation was detected.
    Creates lists of matching LocatorFilterEntries categorized by MatcherType. These lists
    are returned through the \a entries argument.

    Returns a list of all matching files.

    This function checks for each file in \a filePaths if it matches the passed \a regExp.
    If so, a new entry is created using \a hasPathSeparator and \a inputLink and
    it's being added into the \a entries argument and the results list.
*/
FilePaths LocatorFileCache::processFilePaths(const QFuture<void> &future,
                                             const FilePaths &filePaths,
                                             bool hasPathSeparator,
                                             const QRegularExpression &regExp,
                                             const Link &inputLink,
                                             LocatorFileCache::MatchedEntries *entries)
{
    FilePaths cache;
    for (const FilePath &path : filePaths) {
        if (future.isCanceled())
            return {};

        const QString matchText = hasPathSeparator ? path.toString() : path.fileName();
        const QRegularExpressionMatch match = regExp.match(matchText);

        if (match.hasMatch()) {
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = path.fileName();
            filterEntry.filePath = path;
            filterEntry.extraInfo = path.shortNativePath();
            filterEntry.linkForEditor = Link(path, inputLink.targetLine, inputLink.targetColumn);
            filterEntry.highlightInfo = hasPathSeparator
                ? ILocatorFilter::highlightInfo(regExp.match(filterEntry.extraInfo),
                                                LocatorFilterEntry::HighlightInfo::ExtraInfo)
                : ILocatorFilter::highlightInfo(match);
            const ILocatorFilter::MatchLevel matchLevel = matchLevelFor(match, matchText);
            (*entries)[int(matchLevel)].append(filterEntry);
            cache << path;
        }
    }
    return cache;
}

static void filter(QPromise<LocatorFileCachePrivate> &promise, const LocatorStorage &storage,
                   const LocatorFileCachePrivate &cache)
{
    QTC_ASSERT(cache.isValid(), return);
    auto newCache = cache;
    const LocatorFilterEntries output = newCache.generate(QFuture<void>(promise.future()),
                                                          storage.input());
    if (promise.isCanceled())
        return;
    storage.reportOutput(output);
    promise.addResult(newCache);
}

/*!
    Returns the locator matcher task for the cache. The task, when successfully finished,
    updates this LocatorFileCache instance if needed.

    This method is to be used directly by the FilePaths filters. The FilePaths filter should
    keep an instance of a LocatorFileCache internally. Ensure the LocatorFileCache instance
    outlives the running matcher, otherwise the cache won't be updated after the task finished.

    When returned LocatorMatcherTask is being run it checks if this cache is valid.
    When the cache is invalid, it uses GeneratorProvider to update the
    cache's FilePathsGenerator and validates the cache. If that failed, the task
    is not started. When the cache is valid, the running task will reuse cached data for
    calculating the LocatorMatcherTask's results.

    After a successful run of the task, this cache is updated according to the last search.
    When this cache started a new search in meantime, the cache was invalidated or even deleted,
    the update of the cache after a successful run of the task is ignored.
*/
LocatorMatcherTask LocatorFileCache::matcher() const
{
    TreeStorage<LocatorStorage> storage;
    std::weak_ptr<LocatorFileCachePrivate> weak = d;

    const auto onSetup = [storage, weak](Async<LocatorFileCachePrivate> &async) {
        auto that = weak.lock();
        if (!that) // LocatorMatcher is running after *this LocatorFileCache was destructed.
            return SetupResult::StopWithDone;

        if (!that->ensureValidated())
            return SetupResult::StopWithDone; // The cache is invalid and
                                             // no provider is set or it returned empty generator
        that->bumpExecutionId();

        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(&filter, *storage, *that);
        return SetupResult::Continue;
    };
    const auto onDone = [weak](const Async<LocatorFileCachePrivate> &async) {
        auto that = weak.lock();
        if (!that)
            return; // LocatorMatcherTask finished after *this LocatorFileCache was destructed.

        if (!that->isValid())
            return; // The cache has been invalidated in meantime.

        if (that->m_executionId == 0)
            return; // The cache has been invalidated and not started.

        if (!async.isResultAvailable())
            return; // The async task didn't report updated cache.

        that->update(async.result());
    };

    return {AsyncTask<LocatorFileCachePrivate>(onSetup, onDone), storage};
}

} // Core

#include "ilocatorfilter.moc"
