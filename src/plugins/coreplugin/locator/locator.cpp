// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locator.h"

#include "directoryfilter.h"
#include "executefilter.h"
#include "externaltoolsfilter.h"
#include "filesystemfilter.h"
#include "javascriptfilter.h"
#include "locatorconstants.h"
#include "locatorfiltersfilter.h"
#include "locatormanager.h"
#include "locatorsettingspage.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "spotlightlocatorfilter.h"
#include "urllocatorfilter.h"
#include "../actionmanager/actioncontainer.h"
#include "../actionmanager/actionmanager.h"
#include "../actionsfilter.h"
#include "../coreplugintr.h"
#include "../editormanager/editormanager_p.h"
#include "../icore.h"
#include "../progressmanager/taskprogress.h"
#include "../settingsdatabase.h"
#include "../statusbarmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QMainWindow>

using namespace Utils;

namespace Core {
namespace Internal {

static Locator *m_instance = nullptr;

const char kDirectoryFilterPrefix[] = "directory";
const char kUrlFilterPrefix[] = "url";
const char kUseCenteredPopup[] = "UseCenteredPopupForShortcut";

class LocatorData
{
public:
    LocatorData();

    LocatorManager m_locatorManager;
    LocatorSettingsPage m_locatorSettingsPage;

    JavaScriptFilter m_javaScriptFilter;
    OpenDocumentsFilter m_openDocumentsFilter;
    FileSystemFilter m_fileSystemFilter;
    ExecuteFilter m_executeFilter;
    ExternalToolsFilter m_externalToolsFilter;
    LocatorFiltersFilter m_locatorsFiltersFilter;
    ActionsFilter m_actionsFilter;
    UrlLocatorFilter m_urlFilter{Tr::tr("Web Search"), "RemoteHelpFilter"};
    UrlLocatorFilter m_bugFilter{Tr::tr("Qt Project Bugs"), "QtProjectBugs"};
    SpotlightLocatorFilter m_spotlightLocatorFilter;
};

LocatorData::LocatorData()
{
    m_urlFilter.setDescription(Tr::tr("Triggers a web search with the selected search engine."));
    m_urlFilter.setDefaultShortcutString("r");
    m_urlFilter.addDefaultUrl("https://www.bing.com/search?q=%1");
    m_urlFilter.addDefaultUrl("https://www.google.com/search?q=%1");
    m_urlFilter.addDefaultUrl("https://search.yahoo.com/search?p=%1");
    m_urlFilter.addDefaultUrl("https://stackoverflow.com/search?q=%1");
    m_urlFilter.addDefaultUrl(
        "http://en.cppreference.com/mwiki/index.php?title=Special%3ASearch&search=%1");
    m_urlFilter.addDefaultUrl("https://en.wikipedia.org/w/index.php?search=%1");

    m_bugFilter.setDescription(Tr::tr("Triggers a search in the Qt bug tracker."));
    m_bugFilter.setDefaultShortcutString("bug");
    m_bugFilter.addDefaultUrl("https://bugreports.qt.io/secure/QuickSearch.jspa?searchString=%1");
}

Locator::Locator()
{
    m_instance = this;
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this] { refresh(filters()); });
}

Locator::~Locator()
{
    delete m_locatorData;
    qDeleteAll(m_customFilters);
}

Locator *Locator::instance()
{
    return m_instance;
}

void Locator::initialize()
{
    m_locatorData = new LocatorData;

    QAction *action = new QAction(Utils::Icons::ZOOM.icon(), Tr::tr("Locate..."), this);
    Command *cmd = ActionManager::registerAction(action, Constants::LOCATE);
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+K")));
    connect(action, &QAction::triggered, this, [] {
        LocatorManager::show(QString());
    });

    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    mtools->addAction(cmd);

    auto locatorWidget = LocatorManager::createLocatorInputWidget(ICore::mainWindow());
    locatorWidget->setObjectName("LocatorInput"); // used for UI introduction
    StatusBarManager::addStatusBarWidget(locatorWidget, StatusBarManager::First,
                                         Context("LocatorWidget"));
    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &Locator::saveSettings);
}

void Locator::extensionsInitialized()
{
    m_filters = Utils::sorted(ILocatorFilter::allLocatorFilters(),
                [](const ILocatorFilter *first, const ILocatorFilter *second) -> bool {
        if (first->priority() != second->priority())
            return first->priority() < second->priority();
        return first->id().alphabeticallyBefore(second->id());
    });
    setFilters(m_filters);

    Command *openCommand = ActionManager::command(Constants::OPEN);
    Command *locateCommand = ActionManager::command(Constants::LOCATE);
    connect(openCommand, &Command::keySequenceChanged,
            this, &Locator::updateEditorManagerPlaceholderText);
    connect(locateCommand, &Command::keySequenceChanged,
            this, &Locator::updateEditorManagerPlaceholderText);
    updateEditorManagerPlaceholderText();
}

bool Locator::delayedInitialize()
{
    loadSettings();
    return true;
}

void Locator::aboutToShutdown()
{
    m_refreshTimer.stop();
    m_taskTree.reset();
}

void Locator::loadSettings()
{
    namespace DB = SettingsDatabase;
    // check if we have to read old settings
    // TOOD remove a few versions after 4.15
    const QString settingsGroup = DB::contains("Locator") ? QString("Locator")
                                                                : QString("QuickOpen");
    const Settings def;
    DB::beginGroup(settingsGroup);
    m_refreshTimer.setInterval(DB::value("RefreshInterval", 60).toInt() * 60000);
    m_settings.useCenteredPopup = DB::value(kUseCenteredPopup, def.useCenteredPopup).toBool();

    for (ILocatorFilter *filter : std::as_const(m_filters)) {
        if (DB::contains(filter->id().toString())) {
            const QByteArray state = DB::value(filter->id().toString()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    DB::beginGroup("CustomFilters");
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = DB::childKeys();
    int count = 0;
    const Id directoryBaseId(Constants::CUSTOM_DIRECTORY_FILTER_BASEID);
    const Id urlBaseId(Constants::CUSTOM_URL_FILTER_BASEID);
    for (const QString &key : keys) {
        ++count;
        ILocatorFilter *filter;
        if (key.startsWith(kDirectoryFilterPrefix)) {
            filter = new DirectoryFilter(directoryBaseId.withSuffix(count));
        } else {
            auto urlFilter = new UrlLocatorFilter(urlBaseId.withSuffix(count));
            urlFilter->setIsCustomFilter(true);
            filter = urlFilter;
        }
        filter->restoreState(DB::value(key).toByteArray());
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    DB::endGroup();
    DB::endGroup();

    if (m_refreshTimer.interval() > 0)
        m_refreshTimer.start();
    m_settingsInitialized = true;
    setFilters(m_filters + customFilters);
}

void Locator::updateFilterActions()
{
    QMap<Id, QAction *> actionCopy = m_filterActionMap;
    m_filterActionMap.clear();
    // register new actions, update existent
    for (ILocatorFilter *filter : std::as_const(m_filters)) {
        if (filter->shortcutString().isEmpty() || filter->isHidden())
            continue;
        Id filterId = filter->id();
        Id actionId = filter->actionId();
        QAction *action = nullptr;
        if (!actionCopy.contains(filterId)) {
            // register new action
            action = new QAction(filter->displayName(), this);
            Command *cmd = ActionManager::registerAction(action, actionId);
            cmd->setAttribute(Command::CA_UpdateText);
            cmd->setDefaultKeySequence(filter->defaultKeySequence());
            connect(action, &QAction::triggered, this, [filter] {
                LocatorManager::showFilter(filter);
            });
        } else {
            action = actionCopy.take(filterId);
            action->setText(filter->displayName());
        }
        action->setToolTip(filter->description());
        m_filterActionMap.insert(filterId, action);
    }

    // unregister actions that are deleted now
    const auto end = actionCopy.end();
    for (auto it = actionCopy.begin(); it != end; ++it) {
        ActionManager::unregisterAction(it.value(), it.key().withPrefix("Locator."));
        delete it.value();
    }
}

void Locator::updateEditorManagerPlaceholderText()
{
    Command *openCommand = ActionManager::command(Constants::OPEN);
    Command *locateCommand = ActionManager::command(Constants::LOCATE);
    const QString placeholderText = Tr::tr("<html><body style=\"color:#909090; font-size:14px\">"
          "<div align='center'>"
          "<div style=\"font-size:20px\">Open a document</div>"
          "<table><tr><td>"
          "<hr/>"
          "<div style=\"margin-top: 5px\">&bull; File > Open File or Project (%1)</div>"
          "<div style=\"margin-top: 5px\">&bull; File > Recent Files</div>"
          "<div style=\"margin-top: 5px\">&bull; Tools > Locate (%2) and</div>"
          "<div style=\"margin-left: 1em\">- type to open file from any open project</div>"
          "%4"
          "%5"
          "<div style=\"margin-left: 1em\">- type <code>%3&lt;space&gt;&lt;filename&gt;</code> to open file from file system</div>"
          "<div style=\"margin-left: 1em\">- select one of the other filters for jumping to a location</div>"
          "<div style=\"margin-top: 5px\">&bull; Drag and drop files here</div>"
          "</td></tr></table>"
          "</div>"
          "</body></html>")
         .arg(openCommand->keySequence().toString(QKeySequence::NativeText))
         .arg(locateCommand->keySequence().toString(QKeySequence::NativeText))
         .arg(m_locatorData->m_fileSystemFilter.shortcutString());

    QString classes;
    // not nice, but anyhow
    ILocatorFilter *classesFilter = Utils::findOrDefault(m_filters,
                                                         Utils::equal(&ILocatorFilter::id,
                                                                      Id("Classes")));
    if (classesFilter)
        classes = Tr::tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a class definition</div>").arg(classesFilter->shortcutString());

    QString methods;
    // not nice, but anyhow
    ILocatorFilter *methodsFilter = Utils::findOrDefault(m_filters, Utils::equal(&ILocatorFilter::id,
                                                                                 Id("Methods")));
    if (methodsFilter)
        methods = Tr::tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a function definition</div>").arg(methodsFilter->shortcutString());

    EditorManagerPrivate::setPlaceholderText(placeholderText.arg(classes, methods));
}

void Locator::saveSettings() const
{
    if (!m_settingsInitialized)
        return;

    const Settings def;
    namespace DB = SettingsDatabase;
    DB::beginTransaction();
    DB::beginGroup("Locator");
    DB::remove(QString());
    DB::setValue("RefreshInterval", refreshInterval());
    DB::setValueWithDefault(kUseCenteredPopup, m_settings.useCenteredPopup, def.useCenteredPopup);
    for (ILocatorFilter *filter : m_filters) {
        if (!m_customFilters.contains(filter) && filter->id().isValid()) {
            const QByteArray state = filter->saveState();
            DB::setValueWithDefault(filter->id().toString(), state);
        }
    }
    DB::beginGroup("CustomFilters");
    int i = 0;
    for (ILocatorFilter *filter : m_customFilters) {
        const char *prefix = filter->id().name().startsWith(
                                 Constants::CUSTOM_DIRECTORY_FILTER_BASEID)
                                 ? kDirectoryFilterPrefix
                                 : kUrlFilterPrefix;
        const QByteArray state = filter->saveState();
        DB::setValueWithDefault(prefix + QString::number(i), state);
        ++i;
    }
    DB::endGroup();
    DB::endGroup();
    DB::endTransaction();
}

/*!
    Return all filters, including the ones created by the user.
*/
QList<ILocatorFilter *> Locator::filters()
{
    return m_instance->m_filters;
}

/*!
    This returns a subset of all the filters, that contains only the filters that
    have been created by the user at some point (maybe in a previous session).
 */
QList<ILocatorFilter *> Locator::customFilters()
{
    return m_customFilters;
}

void Locator::setFilters(QList<ILocatorFilter *> f)
{
    m_filters = f;
    updateFilterActions();
    updateEditorManagerPlaceholderText(); // possibly some shortcut changed
    emit filtersChanged();
}

void Locator::setCustomFilters(QList<ILocatorFilter *> filters)
{
    m_customFilters = filters;
}

int Locator::refreshInterval() const
{
    return m_refreshTimer.interval() / 60000;
}

void Locator::setRefreshInterval(int interval)
{
    if (interval < 1) {
        m_refreshTimer.stop();
        m_refreshTimer.setInterval(0);
        return;
    }
    m_refreshTimer.setInterval(interval * 60000);
    m_refreshTimer.start();
}

bool Locator::useCenteredPopupForShortcut()
{
    return m_instance->m_settings.useCenteredPopup;
}

void Locator::setUseCenteredPopupForShortcut(bool center)
{
    m_instance->m_settings.useCenteredPopup = center;
}

void Locator::refresh(const QList<ILocatorFilter *> &filters)
{
    if (ExtensionSystem::PluginManager::isShuttingDown())
        return;

    m_taskTree.reset(); // Superfluous, just for clarity. The next reset() below is enough.
    m_refreshingFilters = Utils::filteredUnique(m_refreshingFilters + filters);

    using namespace Tasking;
    QList<GroupItem> tasks{parallel};
    for (ILocatorFilter *filter : std::as_const(m_refreshingFilters)) {
        const auto task = filter->refreshRecipe();
        if (!task.has_value())
            continue;

        const Group group {
            finishAllAndDone,
            *task,
            onGroupDone([this, filter] { m_refreshingFilters.removeOne(filter); })
        };
        tasks.append(group);
    }

    m_taskTree.reset(new TaskTree{tasks});
    connect(m_taskTree.get(), &TaskTree::done, this, [this] {
        saveSettings();
        m_taskTree.release()->deleteLater();
    });
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, [this] {
        m_taskTree.release()->deleteLater();
    });
    auto progress = new TaskProgress(m_taskTree.get());
    progress->setDisplayName(Tr::tr("Updating Locator Caches"));
    m_taskTree->start();
}

void Locator::showFilter(ILocatorFilter *filter, LocatorWidget *widget)
{
    QTC_ASSERT(filter, return );
    QTC_ASSERT(widget, return );
    std::optional<QString> searchText = filter->defaultSearchText();
    if (!searchText) {
        searchText = widget->currentText().trimmed();
        // add shortcut string at front or replace existing shortcut string
        if (!searchText->isEmpty()) {
            const QList<ILocatorFilter *> allFilters = Locator::filters();
            for (ILocatorFilter *otherfilter : allFilters) {
                if (searchText->startsWith(otherfilter->shortcutString() + ' ')) {
                    searchText = searchText->mid(otherfilter->shortcutString().length() + 1);
                    break;
                }
            }
        }
    }
    widget->showText(filter->shortcutString() + ' ' + *searchText,
                     filter->shortcutString().length() + 1,
                     searchText->length());
}

} // namespace Internal
} // namespace Core
