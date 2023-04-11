// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ilocatorfilter.h"

#include "../coreplugin.h"
#include "../coreplugintr.h"
#include "../editormanager/editormanager.h"

#include <utils/asynctask.h>
#include <utils/fuzzymatcher.h>
#include <utils/tasktree.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>

#include <unordered_set>

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

class OutputDataProvider
{
    enum class State {
        Awaiting,
        NewData,
        Canceled
    };

    struct WorkingData {
        WorkingData() = default;
        WorkingData(const LocatorMatcherTask::OutputData &entries, std::atomic<State> &state) {
            mergeWith(entries, state);
        }
        LocatorMatcherTask::OutputData mergeWith(const LocatorMatcherTask::OutputData &entries,
                                                 std::atomic<State> &state) {
            LocatorMatcherTask::OutputData results;
            results.reserve(entries.size());
            for (const LocatorFilterEntry &entry : entries) {
                if (state == State::Canceled)
                    return LocatorMatcherTask::OutputData();
                const auto &link = entry.linkForEditor;
                if (!link || m_cache.emplace(*link).second)
                    results.append(entry);
            }
            if (state == State::Canceled)
                return LocatorMatcherTask::OutputData();

            m_data += results;
            return results;
        }
        LocatorMatcherTask::OutputData entries() const { return m_data; }
    private:
        LocatorMatcherTask::OutputData m_data;
        std::unordered_set<Utils::Link> m_cache;
    };

public:
    OutputDataProvider(int filterCount)
        : m_filterCount(filterCount)
        , m_outputData(filterCount, {})
    {}

    void addOutputData(int index, const LocatorMatcherTask::OutputData &outputData)
    {
        QTC_ASSERT(index >= 0, return);

        QMutexLocker locker(&m_mutex);
        QTC_ASSERT(index < m_filterCount, return);
        QTC_ASSERT(m_state != State::Canceled, return);
        QTC_ASSERT(!m_outputData.at(index).has_value(), return);

        m_outputData[index] = outputData;
        m_state = State::NewData;
        m_waitCondition.wakeOne();
    }

    void cancel()
    {
        QMutexLocker locker(&m_mutex);
        m_state = State::Canceled;
        m_waitCondition.wakeOne();
    }

    // Called from separate thread (OutputFilter's thread)
    void run(QPromise<LocatorMatcherTask::OutputData> &promise)
    {
        QList<std::optional<LocatorMatcherTask::OutputData>> data;
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
                    const LocatorMatcherTask::OutputData dataForIndex = mergeToCurrent
                        ? *outputData : LocatorMatcherTask::OutputData();
                    workingList[currentIndex] = std::make_optional(WorkingData(dataForIndex,
                                                                               m_state));
                    if (m_state == State::Canceled)
                        return;
                    const LocatorMatcherTask::OutputData newData = mergeToCurrent
                        ? workingList[currentIndex]->entries()
                        : workingList[mergeToIndex]->mergeWith(*outputData, m_state);
                    if (m_state == State::Canceled)
                        return;
                    if (!hasGap && !newData.isEmpty())
                        promise.addResult(newData);
                } else if (currentIndex != mergeToIndex) {
                    const LocatorMatcherTask::OutputData newData
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
    bool waitForData(QList<std::optional<LocatorMatcherTask::OutputData>> *data)
    {
        QMutexLocker locker(&m_mutex);
        if (m_state == State::Canceled)
            return false;
        if (m_state == State::NewData) {
            m_state = State::Awaiting;
            *data = m_outputData;
            return true;
        }
        m_waitCondition.wait(&m_mutex);
        QTC_ASSERT(m_state != State::Awaiting, return false);
        if (m_state == State::Canceled)
            return false;
        m_state = State::Awaiting;
        *data = m_outputData;
        return true;
    }

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    const int m_filterCount = 0;
    std::atomic<State> m_state = State::Awaiting;
    QList<std::optional<LocatorMatcherTask::OutputData>> m_outputData;
};

class OutputFilter : public QObject
{
    Q_OBJECT

public:
    ~OutputFilter();
    void setFilterCount(int count);
    // When last index is added it ends automatically (asynchronously)
    void addOutputData(int index, const LocatorMatcherTask::OutputData &outputData);
    void start();

    bool isRunning() const { return m_watcher.get(); }

signals:
    void serialOutputDataReady(const LocatorMatcherTask::OutputData &serialOutputData);
    void done();

private:
    int m_filterCount = 0;
    std::unique_ptr<QFutureWatcher<LocatorMatcherTask::OutputData>> m_watcher;
    std::shared_ptr<OutputDataProvider> m_dataProvider;
};

OutputFilter::~OutputFilter()
{
    if (!isRunning())
        return;

    m_dataProvider->cancel();
    Internal::CorePlugin::futureSynchronizer()->addFuture(m_watcher->future());
}

void OutputFilter::setFilterCount(int count)
{
    QTC_ASSERT(!isRunning(), return);
    QTC_ASSERT(count >= 0, return);

    m_filterCount = count;
}

void OutputFilter::addOutputData(int index, const LocatorMatcherTask::OutputData &outputData)
{
    QTC_ASSERT(isRunning(), return);

    m_dataProvider->addOutputData(index, outputData);
}

void OutputFilter::start()
{
    QTC_ASSERT(!m_watcher, return);
    QTC_ASSERT(!isRunning(), return);
    if (m_filterCount == 0) {
        emit done();
        return;
    }

    m_dataProvider.reset(new OutputDataProvider(m_filterCount));
    m_watcher.reset(new QFutureWatcher<LocatorMatcherTask::OutputData>);
    connect(m_watcher.get(), &QFutureWatcherBase::resultReadyAt, this, [this](int index) {
        emit serialOutputDataReady(m_watcher->resultAt(index));
    });
    connect(m_watcher.get(), &QFutureWatcherBase::finished, this, [this] {
        emit done();
        m_watcher.release()->deleteLater();
        m_dataProvider.reset();
    });

    // TODO: When filterCount == 1, deliver results directly and finish?
    auto filter = [](QPromise<LocatorMatcherTask::OutputData> &promise,
                     const std::shared_ptr<OutputDataProvider> &dataProvider) {
        dataProvider->run(promise);
    };
    m_watcher->setFuture(Utils::asyncRun(filter, m_dataProvider));
}

class OutputFilterAdapter : public Tasking::TaskAdapter<OutputFilter>
{
public:
    OutputFilterAdapter() {
        connect(task(), &OutputFilter::done, this, [this] { emit done(true); });
    }
    void start() final { task()->start(); }
};

} // namespace Core

QTC_DECLARE_CUSTOM_TASK(Filter, Core::OutputFilterAdapter);

namespace Core {

class LocatorMatcherPrivate
{
public:
    QList<LocatorMatcherTask> m_tasks;
    LocatorMatcherTask::Storage m_storage;
    int m_parallelLimit = 0;
    std::unique_ptr<TaskTree> m_taskTree;
};

LocatorMatcher::LocatorMatcher()
    : d(new LocatorMatcherPrivate) {}

LocatorMatcher::~LocatorMatcher() = default;

void LocatorMatcher::setTasks(const QList<LocatorMatcherTask> &tasks)
{
    d->m_tasks = tasks;
}

void LocatorMatcher::setInputData(const LocatorMatcherTask::InputData &inputData)
{
    d->m_storage.input = inputData;
}

void LocatorMatcher::setParallelLimit(int limit)
{
    d->m_parallelLimit = limit;
}

void LocatorMatcher::start()
{
    QTC_ASSERT(!isRunning(), return);
    d->m_storage.output = {};
    d->m_taskTree.reset(new TaskTree);

    using namespace Tasking;

    struct FilterStorage
    {
        OutputFilter *m_filter = nullptr;
    };
    TreeStorage<FilterStorage> filterStorage;

    const auto onFilterSetup = [this, filterCount = d->m_tasks.size(), filterStorage](OutputFilter &filter) {
        filterStorage->m_filter = &filter;
        filter.setFilterCount(filterCount);
        connect(&filter, &OutputFilter::serialOutputDataReady,
                this, [this](const LocatorMatcherTask::OutputData &serialOutputData) {
            d->m_storage.output += serialOutputData;
            emit serialOutputDataReady(serialOutputData);
        });
    };
    const auto onFilterDone = [filterStorage](const OutputFilter &filter) {
        Q_UNUSED(filter)
        filterStorage->m_filter = nullptr;
    };

    QList<TaskItem> parallelTasks { ParallelLimit(d->m_parallelLimit) };

    const auto onGroupSetup = [this](const TreeStorage<LocatorMatcherTask::Storage> &storage) {
        return [this, storage] { storage->input = d->m_storage.input; };
    };
    const auto onGroupDone = [filterStorage]
        (const TreeStorage<LocatorMatcherTask::Storage> &storage, int index) {
        return [filterStorage, storage, index] {
            OutputFilter *outputFilter = filterStorage->m_filter;
            QTC_ASSERT(outputFilter, return);
            outputFilter->addOutputData(index, storage->output);
        };
    };

    int index = 0;
    for (const LocatorMatcherTask &task : std::as_const(d->m_tasks)) {
        const auto storage = task.storage;
        const Group group {
            optional,
            Storage(storage),
            OnGroupSetup(onGroupSetup(storage)),
            OnGroupDone(onGroupDone(storage, index)),
            OnGroupError(onGroupDone(storage, index)),
            task.task
        };
        parallelTasks << group;
        ++index;
    }

    const Group root {
        parallel,
        Storage(filterStorage),
        Filter(onFilterSetup, onFilterDone, onFilterDone),
        Group {
            parallelTasks
        }
    };

    d->m_taskTree->setupRoot(root);

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

LocatorMatcherTask::OutputData LocatorMatcher::outputData() const
{
    return d->m_storage.output;
}

LocatorMatcherTask::OutputData LocatorMatcher::runBlocking(const QList<LocatorMatcherTask> &tasks,
                               const LocatorMatcherTask::InputData &input, int parallelLimit)
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

QList<LocatorMatcherTask> LocatorMatcher::matchers(MatcherType type)
{
    const QList<LocatorMatcherTaskCreator> creators = s_matcherCreators.value(type);
    QList<LocatorMatcherTask> result;
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
    Performs actions that need to be done in the main thread before actually
    running the search for \a entry.

    Called on the main thread before matchesFor() is called in a separate
    thread.

    The default implementation does nothing.

    \sa matchesFor()
*/
void ILocatorFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
}


/*!
    Sets the refresh recipe for refreshing cached data.
*/
void ILocatorFilter::setRefreshRecipe(const std::optional<Utils::Tasking::TaskItem> &recipe)
{
    m_refreshRecipe = recipe;
}

/*!
    Returns the refresh recipe for refreshing cached data. By default, the locator filter has
    no recipe set, so that it won't be refreshed.
*/
std::optional<Utils::Tasking::TaskItem> ILocatorFilter::refreshRecipe() const
{
    return m_refreshRecipe;
}

/*!
    Called with the entry specified by \a selection when the user activates it
    in the result list.
    Implementations can return a new search term \a newText, which has \a selectionLength characters
    starting from \a selectionStart preselected, and the cursor set to the end of the selection.

    The default implementation tries to open an editor for \a selections's linkForEditor,
    if it exists.
*/
void ILocatorFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                            int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    EditorManager::openEditor(selection);
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
    \fn QList<Core::LocatorFilterEntry> Core::ILocatorFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)

    Returns the list of results of this filter for the search term \a entry.
    This is run in a separate thread, but is guaranteed to only run in a single
    thread at any given time. Quickly running preparations can be done in the
    GUI thread in prepareSearch().

    Implementations should do a case sensitive or case insensitive search
    depending on caseSensitivity(). If \a future is \c canceled, the search
    should be aborted.

    \sa prepareSearch()
    \sa caseSensitivity()
*/

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

} // Core

#include "ilocatorfilter.moc"
