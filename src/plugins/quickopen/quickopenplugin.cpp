/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "quickopenplugin.h"
#include "quickopenconstants.h"
#include "quickopenfiltersfilter.h"
#include "quickopenmanager.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "filesystemfilter.h"
#include "settingspage.h"

#include <QtCore/QSettings>
#include <QtCore/QtPlugin>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>

#include <coreplugin/baseview.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <qtconcurrent/QtConcurrentTools>

/*!
    \namespace QuickOpen
    The QuickOpen namespace provides the hooks for Locator content.
*/
/*!
    \namespace QuickOpen::Internal
    \internal
*/

using namespace QuickOpen;
using namespace QuickOpen::Internal;

namespace {
    static bool filterLessThan(const ILocatorFilter *first, const ILocatorFilter *second)
    {
        return first->priority() < second->priority();
    }
}

QuickOpenPlugin::QuickOpenPlugin()
{
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
}

QuickOpenPlugin::~QuickOpenPlugin()
{
    removeObject(m_openDocumentsFilter);
    removeObject(m_fileSystemFilter);
    removeObject(m_settingsPage);
    delete m_openDocumentsFilter;
    delete m_fileSystemFilter;
    delete m_settingsPage;
    qDeleteAll(m_customFilters);
}

bool QuickOpenPlugin::initialize(const QStringList &, QString *)
{
    Core::ICore *core = Core::ICore::instance();
    m_settingsPage = new SettingsPage(this);
    addObject(m_settingsPage);

    m_locatorWidget = new LocatorWidget(this);
    m_locatorWidget->setEnabled(false);
    Core::BaseView *view = new Core::BaseView;
    view->setUniqueViewName("QuickOpen");
    view->setWidget(m_locatorWidget);
    view->setContext(QList<int>() << core->uniqueIDManager()
        ->uniqueIdentifier(QLatin1String("LocatorWidget")));
    view->setDefaultPosition(Core::IView::First);
    addAutoReleasedObject(view);

    const QString actionId = QLatin1String("QtCreator.Locate");
    QAction *action = new QAction(m_locatorWidget->windowIcon(), m_locatorWidget->windowTitle(), this);
    Core::Command *cmd = core->actionManager()->registerAction(action, actionId, QList<int>() << Core::Constants::C_GLOBAL_ID);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+K"));
    connect(action, SIGNAL(triggered()), this, SLOT(openQuickOpen()));

    Core::ActionContainer *mtools = core->actionManager()->actionContainer(Core::Constants::M_TOOLS);
    mtools->addAction(cmd);

    addObject(new QuickOpenManager(m_locatorWidget));

    m_openDocumentsFilter = new OpenDocumentsFilter(core->editorManager());
    addObject(m_openDocumentsFilter);

    m_fileSystemFilter = new FileSystemFilter(core->editorManager(), m_locatorWidget);
    addObject(m_fileSystemFilter);

    addAutoReleasedObject(new QuickOpenFiltersFilter(this, m_locatorWidget));

    connect(core, SIGNAL(coreOpened()), this, SLOT(startSettingsLoad()));
    return true;
}

void QuickOpenPlugin::openQuickOpen()
{
    m_locatorWidget->show("");
}

void QuickOpenPlugin::extensionsInitialized()
{
    m_filters = ExtensionSystem::PluginManager::instance()->getObjects<ILocatorFilter>();
    qSort(m_filters.begin(), m_filters.end(), filterLessThan);
}

void QuickOpenPlugin::startSettingsLoad()
{
    m_loadWatcher.setFuture(QtConcurrent::run(this, &QuickOpenPlugin::loadSettings));
    connect(&m_loadWatcher, SIGNAL(finished()), this, SLOT(settingsLoaded()));
}

void QuickOpenPlugin::loadSettings()
{
    Core::ICore *core = Core::ICore::instance();
    QSettings *qs = core->settings();

    // Backwards compatibility to old settings location
    if (qs->contains("QuickOpen/FiltersFilter")) {
        loadSettingsHelper(qs);
    } else {
        Core::SettingsDatabase *settings = core->settingsDatabase();
        loadSettingsHelper(settings);
    }

    qs->remove("QuickOpen");
}

void QuickOpenPlugin::settingsLoaded()
{
    m_locatorWidget->updateFilterList();
    m_locatorWidget->setEnabled(true);
    if (m_refreshTimer.interval() > 0)
        m_refreshTimer.start();
}

void QuickOpenPlugin::saveSettings()
{
    Core::ICore *core = Core::ICore::instance();
    if (core && core->settingsDatabase()) {
        Core::SettingsDatabase *s = core->settingsDatabase();
        s->beginGroup("QuickOpen");
        s->remove("");
        s->setValue("RefreshInterval", refreshInterval());
        foreach (ILocatorFilter *filter, m_filters) {
            if (!m_customFilters.contains(filter))
                s->setValue(filter->name(), filter->saveState());
        }
        s->beginGroup("CustomFilters");
        int i = 0;
        foreach (ILocatorFilter *filter, m_customFilters) {
            s->setValue(QString("directory%1").arg(i), filter->saveState());
            ++i;
        }
        s->endGroup();
        s->endGroup();
    }
}

/*!
    \fn QList<ILocatorFilter*> QuickOpenPlugin::filter()

    Return all filters, including the ones created by the user.
*/
QList<ILocatorFilter*> QuickOpenPlugin::filters()
{
    return m_filters;
}

/*!
    \fn QList<ILocatorFilter*> QuickOpenPlugin::customFilter()

    This returns a subset of all the filters, that contains only the filters that
    have been created by the user at some point (maybe in a previous session).
 */
QList<ILocatorFilter*> QuickOpenPlugin::customFilters()
{
    return m_customFilters;
}

void QuickOpenPlugin::setFilters(QList<ILocatorFilter*> f)
{
    m_filters = f;
    m_locatorWidget->updateFilterList();
}

void QuickOpenPlugin::setCustomFilters(QList<ILocatorFilter *> filters)
{
    m_customFilters = filters;
}

int QuickOpenPlugin::refreshInterval()
{
    return m_refreshTimer.interval() / 60000;
}

void QuickOpenPlugin::setRefreshInterval(int interval)
{
    if (interval < 1) {
        m_refreshTimer.stop();
        m_refreshTimer.setInterval(0);
        return;
    }
    m_refreshTimer.setInterval(interval * 60000);
    m_refreshTimer.start();
}

void QuickOpenPlugin::refresh(QList<ILocatorFilter*> filters)
{
    if (filters.isEmpty())
        filters = m_filters;
    QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
    Core::FutureProgress *progress = Core::ICore::instance()
            ->progressManager()->addTask(task, tr("Indexing"),
                                         QuickOpen::Constants::TASK_INDEX,
                                         Core::ProgressManager::CloseOnSuccess);
    connect(progress, SIGNAL(finished()), this, SLOT(saveSettings()));
}

Q_EXPORT_PLUGIN(QuickOpenPlugin)
