// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionsdialog.h"
#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectsettingswidget.h"
#include "todotr.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <extensionsystem/iplugin.h>

#include <utils/link.h>

namespace Todo::Internal {

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

    setupTodoSettingsProjectPanel(m_todoItemsProvider);

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

class TodoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Todo.json")

public:
    TodoPlugin()
    {
        qRegisterMetaType<TodoItem>("TodoItem");
    }

    ~TodoPlugin() final
    {
        delete d;
    }

    void initialize() final
    {
        d = new TodoPluginPrivate;
    }

private:
    TodoPluginPrivate *d = nullptr;
};

} // Todo::Internal

#include "todoplugin.moc"
