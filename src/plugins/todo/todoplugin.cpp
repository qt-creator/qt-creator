// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectsettingswidget.h"

#include <extensionsystem/iplugin.h>

namespace Todo::Internal {

class TodoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Todo.json")

public:
    TodoPlugin()
    {
        qRegisterMetaType<TodoItem>("TodoItem");
    }

    void initialize() final
    {
        todoSettings().load();

        setupTodoItemsProvider(this);
        setupTodoOutputPane(this);

        setupTodoSettingsPage();

        setupTodoSettingsProjectPanel();
    }
};

} // Todo::Internal

#include "todoplugin.moc"
