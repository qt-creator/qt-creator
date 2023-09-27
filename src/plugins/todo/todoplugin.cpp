// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todoplugin.h"

#include "optionsdialog.h"
#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectsettingswidget.h"
#include "todotr.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/projectpanelfactory.h>
#include <utils/link.h>

namespace Todo {
namespace Internal {

class TodoPluginPrivate : public QObject
{
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
    panelFactory->setDisplayName(Tr::tr("To-Do"));
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
        Core::EditorManager::openEditorAt(Utils::Link(item.file, item.line));
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

void TodoPlugin::initialize()
{
    d = new TodoPluginPrivate;
}

} // namespace Internal
} // namespace Todo
