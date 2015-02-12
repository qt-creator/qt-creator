/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
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

#ifndef TODOPLUGIN_H
#define TODOPLUGIN_H

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

private slots:
    void settingsChanged(const Settings &settings);
    void scanningScopeChanged(ScanningScope scanningScope);
    void todoItemClicked(const TodoItem &item);

private:
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

#endif // TODOPLUGIN_H

