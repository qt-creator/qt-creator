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

#pragma once

#include "settings.h"

#include <extensionsystem/iplugin.h>

namespace Todo {
namespace Internal {

class TodoOutputPane;
class OptionsPage;
class TodoItemsProvider;
class TodoItem;

class TodoPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Todo.json")

public:
    TodoPlugin();
    ~TodoPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList &arguments, QString *errorString);

private:
    void settingsChanged(const Settings &settings);
    void scanningScopeChanged(ScanningScope scanningScope);
    void todoItemClicked(const TodoItem &item);
    void createItemsProvider();
    void createTodoOutputPane();
    void createOptionsPage();

    Settings m_settings;
    TodoOutputPane *m_todoOutputPane;
    OptionsPage *m_optionsPage;
    TodoItemsProvider *m_todoItemsProvider;
};

} // namespace Internal
} // namespace Todo
