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
#include "optionsdialog.h"
#include "keyword.h"
#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectsettingswidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/projectpanelfactory.h>

#include <QFileInfo>
#include <QSettings>

namespace Todo {
namespace Internal {

class TodoPluginPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Todo::Internal::TodoPlugin)

public:
    TodoPluginPrivate();

    void settingsChanged(const Settings &settings);
    void scanningScopeChanged(ScanningScope scanningScope);
    void todoItemClicked(const TodoItem &item);
    void createItemsProvider();
    void createTodoOutputPane();

    Settings m_settings;
    TodoOutputPane *m_todoOutputPane = nullptr;
    TodoOptionsPage m_optionsPage{&m_settings, [this] { settingsChanged(m_settings); }};
    TodoItemsProvider *m_todoItemsProvider = nullptr;
};

TodoPluginPrivate::TodoPluginPrivate()
{
    m_settings.load(Core::ICore::settings());

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
}

void TodoPluginPrivate::settingsChanged(const Settings &settings)
{
    settings.save(Core::ICore::settings());
    m_settings = settings;

    m_todoItemsProvider->settingsChanged(m_settings);
    m_todoOutputPane->setScanningScope(m_settings.scanningScope);
}

void TodoPluginPrivate::scanningScopeChanged(ScanningScope scanningScope)
{
    Settings newSettings = m_settings;
    newSettings.scanningScope = scanningScope;
    settingsChanged(newSettings);
}

void TodoPluginPrivate::todoItemClicked(const TodoItem &item)
{
    if (item.file.exists())
        Core::EditorManager::openEditorAt(item.file.toString(), item.line);
}

void TodoPluginPrivate::createItemsProvider()
{
    m_todoItemsProvider = new TodoItemsProvider(m_settings, this);
}

void TodoPluginPrivate::createTodoOutputPane()
{
    m_todoOutputPane = new TodoOutputPane(m_todoItemsProvider->todoItemsModel(), &m_settings, this);
    m_todoOutputPane->setScanningScope(m_settings.scanningScope);
    connect(m_todoOutputPane, &TodoOutputPane::scanningScopeChanged,
            this, &TodoPluginPrivate::scanningScopeChanged);
    connect(m_todoOutputPane, &TodoOutputPane::todoItemClicked,
            this, &TodoPluginPrivate::todoItemClicked);
}

TodoPlugin::TodoPlugin()
{
    qRegisterMetaType<TodoItem>("TodoItem");
}

TodoPlugin::~TodoPlugin()
{
    delete d;
}

bool TodoPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args)
    Q_UNUSED(errMsg)

    d = new TodoPluginPrivate;

    return true;
}

} // namespace Internal
} // namespace Todo
