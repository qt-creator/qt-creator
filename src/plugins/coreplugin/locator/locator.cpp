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

#include "locator.h"
#include "locatorconstants.h"
#include "locatorfiltersfilter.h"
#include "locatormanager.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "filesystemfilter.h"
#include "locatorsettingspage.h"

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
#include <utils/QtConcurrentTools>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QtPlugin>
#include <QFuture>
#include <QAction>

#ifdef Q_OS_OSX
#include "spotlightlocatorfilter.h"
#endif

namespace Core {
namespace Internal {

Locator::Locator()
    : m_settingsInitialized(false)
{
    m_corePlugin = 0;
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
}

Locator::~Locator()
{
    m_corePlugin->removeObject(m_openDocumentsFilter);
    m_corePlugin->removeObject(m_fileSystemFilter);
    m_corePlugin->removeObject(m_executeFilter);
    m_corePlugin->removeObject(m_settingsPage);
    delete m_openDocumentsFilter;
    delete m_fileSystemFilter;
    delete m_executeFilter;
    delete m_settingsPage;
    qDeleteAll(m_customFilters);
}

void Locator::initialize(CorePlugin *corePlugin, const QStringList &, QString *)
{
    m_corePlugin = corePlugin;

    m_settingsPage = new LocatorSettingsPage(this);
    m_corePlugin->addObject(m_settingsPage);

    m_locatorWidget = new LocatorWidget(this);
    m_locatorWidget->setEnabled(false);
    StatusBarWidget *view = new StatusBarWidget;
    view->setWidget(m_locatorWidget);
    view->setContext(Context("LocatorWidget"));
    view->setPosition(StatusBarWidget::First);
    m_corePlugin->addAutoReleasedObject(view);

    QAction *action = new QAction(m_locatorWidget->windowIcon(), m_locatorWidget->windowTitle(), this);
    Command *cmd = ActionManager::registerAction(action, Constants::LOCATE);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+K")));
    connect(action, &QAction::triggered, this, &Locator::openLocator);
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updatePlaceholderText()));
    updatePlaceholderText(cmd);

    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    mtools->addAction(cmd);

    m_corePlugin->addObject(new LocatorManager(m_locatorWidget));

    m_openDocumentsFilter = new OpenDocumentsFilter;
    m_corePlugin->addObject(m_openDocumentsFilter);

    m_fileSystemFilter = new FileSystemFilter(m_locatorWidget);
    m_corePlugin->addObject(m_fileSystemFilter);

    m_executeFilter = new ExecuteFilter();
    m_corePlugin->addObject(m_executeFilter);

    m_corePlugin->addAutoReleasedObject(new LocatorFiltersFilter(this, m_locatorWidget));
#ifdef Q_OS_OSX
    m_corePlugin->addAutoReleasedObject(new SpotlightLocatorFilter);
#endif

    connect(ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
}

void Locator::updatePlaceholderText(Command *command)
{
    if (!command)
        command = qobject_cast<Command *>(sender());
    QTC_ASSERT(command, return);
    if (command->keySequence().isEmpty())
        m_locatorWidget->setPlaceholderText(tr("Type to locate"));
    else
        m_locatorWidget->setPlaceholderText(tr("Type to locate (%1)").arg(
                                                command->keySequence().toString(QKeySequence::NativeText)));
}

void Locator::openLocator()
{
    m_locatorWidget->show(QString());
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
    QSettings *qs = ICore::settings();

    // Backwards compatibility to old settings location
    if (qs->contains(QLatin1String("QuickOpen/FiltersFilter"))) {
        loadSettingsHelper(qs);
    } else {
        SettingsDatabase *settings = ICore::settingsDatabase();
        loadSettingsHelper(settings);
    }

    qs->remove(QLatin1String("QuickOpen"));

    m_locatorWidget->updateFilterList();
    m_locatorWidget->setEnabled(true);
    if (m_refreshTimer.interval() > 0)
        m_refreshTimer.start();
    m_settingsInitialized = true;
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
    ILocatorFilter *classesFilter = Utils::findOrDefault(m_filters, [](const ILocatorFilter *filter) {
        return filter->id() == Id("Classes"); // not nice, but anyhow
    });
    if (classesFilter)
        classes = tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a class definition</div>").arg(classesFilter->shortcutString());

    QString methods;
    ILocatorFilter *methodsFilter = Utils::findOrDefault(m_filters, [](const ILocatorFilter *filter) {
        return filter->id() == Id("Methods"); // not nice, but anyhow
    });
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
    return m_filters;
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
    updateEditorManagerPlaceholderText(); // possibly some shortcut changed
    m_locatorWidget->updateFilterList();
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
    QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
    FutureProgress *progress =
        ProgressManager::addTask(task, tr("Updating Locator Caches"), Constants::TASK_INDEX);
    connect(progress, &FutureProgress::finished, this, &Locator::saveSettings);
}

} // namespace Internal
} // namespace Core
