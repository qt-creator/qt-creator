/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "todoplugin.h"
#include "constants.h"
#include "optionspage.h"
#include "keyword.h"
#include "todooutputpane.h"
#include "todoitemsprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

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
    m_settings.save(Core::ICore::settings());
}

bool TodoPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    m_settings.load(Core::ICore::settings());

    createOptionsPage();
    createItemsProvider();
    createTodoOutputPane();

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
    if (QFileInfo(item.file).exists()) {
        Core::IEditor *editor = Core::EditorManager::openEditor(item.file);
        editor->gotoLine(item.line);
    }
}

void TodoPlugin::createItemsProvider()
{
    m_todoItemsProvider = new TodoItemsProvider(m_settings);
    addAutoReleasedObject(m_todoItemsProvider);
}

void TodoPlugin::createTodoOutputPane()
{
    m_todoOutputPane = new TodoOutputPane(m_todoItemsProvider->todoItemsModel());
    addAutoReleasedObject(m_todoOutputPane);
    m_todoOutputPane->setScanningScope(m_settings.scanningScope);
    connect(m_todoOutputPane, SIGNAL(scanningScopeChanged(ScanningScope)), SLOT(scanningScopeChanged(ScanningScope)));
    connect(m_todoOutputPane, SIGNAL(todoItemClicked(TodoItem)), SLOT(todoItemClicked(TodoItem)));
}

void TodoPlugin::createOptionsPage()
{
    m_optionsPage = new OptionsPage(m_settings);
    addAutoReleasedObject(m_optionsPage);
    connect(m_optionsPage, SIGNAL(settingsChanged(Settings)), SLOT(settingsChanged(Settings)));
}

Q_EXPORT_PLUGIN2(Todo, TodoPlugin)

} // namespace Internal
} // namespace Todo
