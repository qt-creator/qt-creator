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

#include "locator.h"
#include "locatorconstants.h"
#include "locatorfiltersfilter.h"
#include "locatormanager.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "filesystemfilter.h"
#include "locatorsettingspage.h"
#include "externaltoolsfilter.h"

#include <coreplugin/coreplugin.h>
#include <coreplugin/statusbarwidget.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/editormanager_p.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/mapreduce.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QSettings>
#include <QtPlugin>
#include <QFuture>
#include <QAction>

#ifdef Q_OS_OSX
#include "spotlightlocatorfilter.h"
#endif

namespace Core {
namespace Internal {

static Locator *m_instance = nullptr;

Locator::Locator()
{
    m_instance = this;
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() { refresh(); });
}

Locator::~Locator()
{
    m_corePlugin->removeObject(m_openDocumentsFilter);
    m_corePlugin->removeObject(m_fileSystemFilter);
    m_corePlugin->removeObject(m_executeFilter);
    m_corePlugin->removeObject(m_settingsPage);
    m_corePlugin->removeObject(m_externalToolsFilter);
    delete m_openDocumentsFilter;
    delete m_fileSystemFilter;
    delete m_executeFilter;
    delete m_settingsPage;
    delete m_externalToolsFilter;
    qDeleteAll(m_customFilters);
}

Locator *Locator::instance()
{
    return m_instance;
}

void Locator::initialize(CorePlugin *corePlugin, const QStringList &, QString *)
{
    m_corePlugin = corePlugin;

    m_settingsPage = new LocatorSettingsPage(this);
    m_corePlugin->addObject(m_settingsPage);

    QAction *action = new QAction(Utils::Icons::ZOOM.icon(), tr("Locate..."), this);
    Command *cmd = ActionManager::registerAction(action, Constants::LOCATE);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+K")));
    connect(action, &QAction::triggered, this, [] {
        LocatorManager::show(QString());
    });

    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    mtools->addAction(cmd);

    auto locatorWidget = LocatorManager::createLocatorInputWidget(ICore::mainWindow());
    StatusBarWidget *view = new StatusBarWidget;
    view->setWidget(locatorWidget);
    view->setContext(Context("LocatorWidget"));
    view->setPosition(StatusBarWidget::First);
    m_corePlugin->addAutoReleasedObject(view);

    new LocatorManager(this);

    m_openDocumentsFilter = new OpenDocumentsFilter;
    m_corePlugin->addObject(m_openDocumentsFilter);

    m_fileSystemFilter = new FileSystemFilter();
    m_corePlugin->addObject(m_fileSystemFilter);

    m_executeFilter = new ExecuteFilter();
    m_corePlugin->addObject(m_executeFilter);

    m_externalToolsFilter = new ExternalToolsFilter;
    m_corePlugin->addObject(m_externalToolsFilter);

    m_corePlugin->addAutoReleasedObject(new LocatorFiltersFilter);
#ifdef Q_OS_OSX
    m_corePlugin->addAutoReleasedObject(new SpotlightLocatorFilter);
#endif

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &Locator::saveSettings);
}

void Locator::extensionsInitialized()
{
    m_filters = ExtensionSystem::PluginManager::getObjects<ILocatorFilter>();
    Utils::sort(m_filters, [](const ILocatorFilter *first, const ILocatorFilter *second) -> bool {
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

void Locator::loadSettings()
{
    SettingsDatabase *settings = ICore::settingsDatabase();
    settings->beginGroup(QLatin1String("QuickOpen"));
    m_refreshTimer.setInterval(settings->value(QLatin1String("RefreshInterval"), 60).toInt() * 60000);

    foreach (ILocatorFilter *filter, m_filters) {
        if (settings->contains(filter->id().toString())) {
            const QByteArray state = settings->value(filter->id().toString()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    settings->beginGroup(QLatin1String("CustomFilters"));
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = settings->childKeys();
    int count = 0;
    Id baseId(Constants::CUSTOM_FILTER_BASEID);
    foreach (const QString &key, keys) {
        ILocatorFilter *filter = new DirectoryFilter(baseId.withSuffix(++count));
        filter->restoreState(settings->value(key).toByteArray());
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    settings->endGroup();
    settings->endGroup();

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
    for (ILocatorFilter *filter : m_filters) {
        if (filter->shortcutString().isEmpty() || filter->isHidden())
            continue;
        Id filterId = filter->id();
        Id actionId = filter->actionId();
        QAction *action = 0;
        if (!actionCopy.contains(filterId)) {
            // register new action
            action = new QAction(filter->displayName(), this);
            Command *cmd = ActionManager::registerAction(action, actionId);
            cmd->setAttribute(Command::CA_UpdateText);
            connect(action, &QAction::triggered, this, [filter] {
                LocatorManager::showFilter(filter);
            });
        } else {
            action = actionCopy.take(filterId);
            action->setText(filter->displayName());
        }
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
    const QString placeholderText = tr("<html><body style=\"color:#909090; font-size:14px\">"
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
         .arg(m_fileSystemFilter->shortcutString());

    QString classes;
    // not nice, but anyhow
    ILocatorFilter *classesFilter = Utils::findOrDefault(m_filters,
                                                         Utils::equal(&ILocatorFilter::id,
                                                                      Id("Classes")));
    if (classesFilter)
        classes = tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a class definition</div>").arg(classesFilter->shortcutString());

    QString methods;
    // not nice, but anyhow
    ILocatorFilter *methodsFilter = Utils::findOrDefault(m_filters, Utils::equal(&ILocatorFilter::id,
                                                                                 Id("Methods")));
    if (methodsFilter)
        methods = tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a function definition</div>").arg(methodsFilter->shortcutString());

    EditorManagerPrivate::setPlaceholderText(placeholderText.arg(classes, methods));
}

void Locator::saveSettings()
{
    if (m_settingsInitialized) {
        SettingsDatabase *s = ICore::settingsDatabase();
        s->beginTransaction();
        s->beginGroup(QLatin1String("QuickOpen"));
        s->remove(QString());
        s->setValue(QLatin1String("RefreshInterval"), refreshInterval());
        foreach (ILocatorFilter *filter, m_filters) {
            if (!m_customFilters.contains(filter))
                s->setValue(filter->id().toString(), filter->saveState());
        }
        s->beginGroup(QLatin1String("CustomFilters"));
        int i = 0;
        foreach (ILocatorFilter *filter, m_customFilters) {
            s->setValue(QLatin1String("directory") + QString::number(i),
                        filter->saveState());
            ++i;
        }
        s->endGroup();
        s->endGroup();
        s->endTransaction();
    }
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

int Locator::refreshInterval()
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

void Locator::refresh(QList<ILocatorFilter *> filters)
{
    if (filters.isEmpty())
        filters = m_filters;
    QFuture<void> task = Utils::map(filters, &ILocatorFilter::refresh, Utils::MapReduceOption::Unordered);
    FutureProgress *progress =
        ProgressManager::addTask(task, tr("Updating Locator Caches"), Constants::TASK_INDEX);
    connect(progress, &FutureProgress::finished, this, &Locator::saveSettings);
}

} // namespace Internal
} // namespace Core
