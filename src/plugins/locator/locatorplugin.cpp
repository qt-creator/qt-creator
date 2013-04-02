/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "locatorplugin.h"
#include "locatorconstants.h"
#include "locatorfiltersfilter.h"
#include "locatormanager.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "filesystemfilter.h"
#include "settingspage.h"

#include <coreplugin/statusbarwidget.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/QtConcurrentTools>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QtPlugin>
#include <QFuture>
#include <QAction>

/*!
    \namespace Locator
    The Locator namespace provides the hooks for Locator content.
*/
/*!
    \namespace Locator::Internal
    \internal
*/

using namespace Locator;
using namespace Locator::Internal;

namespace {
    static bool filterLessThan(const ILocatorFilter *first, const ILocatorFilter *second)
    {
        if (first->priority() < second->priority())
            return true;
        if (first->priority() > second->priority())
            return false;
        return first->id().alphabeticallyBefore(second->id());
    }
}

LocatorPlugin::LocatorPlugin()
    : m_settingsInitialized(false)
{
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
}

LocatorPlugin::~LocatorPlugin()
{
    removeObject(m_openDocumentsFilter);
    removeObject(m_fileSystemFilter);
    removeObject(m_executeFilter);
    removeObject(m_settingsPage);
    delete m_openDocumentsFilter;
    delete m_fileSystemFilter;
    delete m_executeFilter;
    delete m_settingsPage;
    qDeleteAll(m_customFilters);
}

bool LocatorPlugin::initialize(const QStringList &, QString *)
{
    m_settingsPage = new SettingsPage(this);
    addObject(m_settingsPage);

    m_locatorWidget = new LocatorWidget(this);
    m_locatorWidget->setEnabled(false);
    Core::StatusBarWidget *view = new Core::StatusBarWidget;
    view->setWidget(m_locatorWidget);
    view->setContext(Core::Context("LocatorWidget"));
    view->setPosition(Core::StatusBarWidget::First);
    addAutoReleasedObject(view);

    QAction *action = new QAction(m_locatorWidget->windowIcon(), m_locatorWidget->windowTitle(), this);
    Core::Command *cmd = Core::ActionManager::registerAction(action, "QtCreator.Locate",
                                                             Core::Context(Core::Constants::C_GLOBAL));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+K")));
    connect(action, SIGNAL(triggered()), this, SLOT(openLocator()));
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updatePlaceholderText()));
    updatePlaceholderText(cmd);

    Core::ActionContainer *mtools = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    mtools->addAction(cmd);

    addObject(new LocatorManager(m_locatorWidget));

    m_openDocumentsFilter = new OpenDocumentsFilter(Core::ICore::editorManager());
    addObject(m_openDocumentsFilter);

    m_fileSystemFilter = new FileSystemFilter(Core::ICore::editorManager(), m_locatorWidget);
    addObject(m_fileSystemFilter);

    m_executeFilter = new ExecuteFilter();
    addObject(m_executeFilter);

    addAutoReleasedObject(new LocatorFiltersFilter(this, m_locatorWidget));

    return true;
}

void LocatorPlugin::updatePlaceholderText(Core::Command *command)
{
    if (!command)
        command = qobject_cast<Core::Command *>(sender());
    QTC_ASSERT(command, return);
    if (command->keySequence().isEmpty())
        m_locatorWidget->setPlaceholderText(tr("Type to locate"));
    else
        m_locatorWidget->setPlaceholderText(tr("Type to locate (%1)").arg(
                                                command->keySequence().toString(QKeySequence::NativeText)));
}

void LocatorPlugin::openLocator()
{
    m_locatorWidget->show(QString());
}

void LocatorPlugin::extensionsInitialized()
{
    m_filters = ExtensionSystem::PluginManager::getObjects<ILocatorFilter>();
    qSort(m_filters.begin(), m_filters.end(), filterLessThan);
    setFilters(m_filters);
}

bool LocatorPlugin::delayedInitialize()
{
    loadSettings();
    return true;
}

void LocatorPlugin::loadSettings()
{
    QSettings *qs = Core::ICore::settings();

    // Backwards compatibility to old settings location
    if (qs->contains(QLatin1String("QuickOpen/FiltersFilter"))) {
        loadSettingsHelper(qs);
    } else {
        Core::SettingsDatabase *settings = Core::ICore::settingsDatabase();
        loadSettingsHelper(settings);
    }

    qs->remove(QLatin1String("QuickOpen"));

    m_locatorWidget->updateFilterList();
    m_locatorWidget->setEnabled(true);
    if (m_refreshTimer.interval() > 0)
        m_refreshTimer.start();
    m_settingsInitialized = true;
}

void LocatorPlugin::saveSettings()
{
    if (m_settingsInitialized) {
        Core::SettingsDatabase *s = Core::ICore::settingsDatabase();
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
    }
}

/*!
    \fn QList<ILocatorFilter*> LocatorPlugin::filters()

    Return all filters, including the ones created by the user.
*/
QList<ILocatorFilter*> LocatorPlugin::filters()
{
    return m_filters;
}

/*!
    \fn QList<ILocatorFilter*> LocatorPlugin::customFilters()

    This returns a subset of all the filters, that contains only the filters that
    have been created by the user at some point (maybe in a previous session).
 */
QList<ILocatorFilter*> LocatorPlugin::customFilters()
{
    return m_customFilters;
}

void LocatorPlugin::setFilters(QList<ILocatorFilter*> f)
{
    m_filters = f;
    m_locatorWidget->updateFilterList();
}

void LocatorPlugin::setCustomFilters(QList<ILocatorFilter *> filters)
{
    m_customFilters = filters;
}

int LocatorPlugin::refreshInterval()
{
    return m_refreshTimer.interval() / 60000;
}

void LocatorPlugin::setRefreshInterval(int interval)
{
    if (interval < 1) {
        m_refreshTimer.stop();
        m_refreshTimer.setInterval(0);
        return;
    }
    m_refreshTimer.setInterval(interval * 60000);
    m_refreshTimer.start();
}

void LocatorPlugin::refresh(QList<ILocatorFilter*> filters)
{
    if (filters.isEmpty())
        filters = m_filters;
    QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
    Core::FutureProgress *progress = Core::ICore::progressManager()
        ->addTask(task, tr("Indexing"), QLatin1String(Locator::Constants::TASK_INDEX));
    connect(progress, SIGNAL(finished()), this, SLOT(saveSettings()));
}

Q_EXPORT_PLUGIN(LocatorPlugin)
