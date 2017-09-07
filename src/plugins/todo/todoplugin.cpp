/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include "todoplugin.h"
#include "constants.h"
#include "optionspage.h"
#include "keyword.h"
#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectsettingswidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/projectpanelfactory.h>

#include <QtPlugin>
#include <QFileInfo>
#include <QSettings>

namespace Todo {
namespace Internal {

TodoPlugin::TodoPlugin() :
    m_todoOutputPane(0),
    m_optionsPage(0),
    m_todoItemsProvider(0)
{
    qRegisterMetaType<TodoItem>("TodoItem");
}

TodoPlugin::~TodoPlugin()
{
}

bool TodoPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    m_settings.load(Core::ICore::settings());

    createOptionsPage();
    createItemsProvider();
    createTodoOutputPane();

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(100);
    panelFactory->setDisplayName(TodoProjectSettingsWidget::tr("To-Do"));
    panelFactory->setCreateWidgetFunction([this](ProjectExplorer::Project *project) {
        auto widget = new TodoProjectSettingsWidget(project);
        connect(widget, &TodoProjectSettingsWidget::projectSettingsChanged,
                m_todoItemsProvider, [this, project] { m_todoItemsProvider->projectSettingsChanged(project); });
        return widget;
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, [this] { m_settings.save(Core::ICore::settings()); });

    return true;
}

void TodoPlugin::extensionsInitialized()
{
}

void TodoPlugin::settingsChanged(const Settings &settings)
{
    settings.save(Core::ICore::settings());
    m_settings = settings;

    m_todoItemsProvider->settingsChanged(m_settings);
    m_todoOutputPane->setScanningScope(m_settings.scanningScope);
    m_optionsPage->setSettings(m_settings);
}

void TodoPlugin::scanningScopeChanged(ScanningScope scanningScope)
{
    Settings newSettings = m_settings;
    newSettings.scanningScope = scanningScope;
    settingsChanged(newSettings);
}

void TodoPlugin::todoItemClicked(const TodoItem &item)
{
    if (item.file.exists())
        Core::EditorManager::openEditorAt(item.file.toString(), item.line);
}

void TodoPlugin::createItemsProvider()
{
    m_todoItemsProvider = new TodoItemsProvider(m_settings);
    addAutoReleasedObject(m_todoItemsProvider);
}

void TodoPlugin::createTodoOutputPane()
{
    m_todoOutputPane = new TodoOutputPane(m_todoItemsProvider->todoItemsModel(), &m_settings);
    addAutoReleasedObject(m_todoOutputPane);
    m_todoOutputPane->setScanningScope(m_settings.scanningScope);
    connect(m_todoOutputPane, &TodoOutputPane::scanningScopeChanged,
            this, &TodoPlugin::scanningScopeChanged);
    connect(m_todoOutputPane, &TodoOutputPane::todoItemClicked,
            this, &TodoPlugin::todoItemClicked);
}

void TodoPlugin::createOptionsPage()
{
    m_optionsPage = new OptionsPage(m_settings);
    addAutoReleasedObject(m_optionsPage);
    connect(m_optionsPage, &OptionsPage::settingsChanged,
            this, &TodoPlugin::settingsChanged);
}

} // namespace Internal
} // namespace Todo
