// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectsettingswidget.h"
#include "todotr.h"

#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

namespace Todo::Internal {

class TodoPluginPrivate : public QObject
{
public:
    TodoPluginPrivate();

    void settingsChanged();
};

TodoPluginPrivate::TodoPluginPrivate()
{
    todoSettings().load(Core::ICore::settings());

    setupTodoItemsProvider(this);
    setupTodoOutputPane(this);

    setupTodoSettingsPage([this] { settingsChanged(); });

    setupTodoSettingsProjectPanel();

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, [] { todoSettings().save(Core::ICore::settings()); });
}

void TodoPluginPrivate::settingsChanged()
{
    todoSettings().save(Core::ICore::settings());

    todoItemsProvider().settingsChanged();
    todoOutputPane().setScanningScope(todoSettings().scanningScope);
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
