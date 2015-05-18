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

#include "todoitemsprovider.h"
#include "constants.h"
#include "cpptodoitemsscanner.h"
#include "qmljstodoitemsscanner.h"
#include "todoitemsmodel.h"
#include "todoitemsscanner.h"

#include <projectexplorer/projectexplorer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/session.h>

#include <QTimer>

using namespace ProjectExplorer;

namespace Todo {
namespace Internal {

TodoItemsProvider::TodoItemsProvider(Settings settings, QObject *parent) :
    QObject(parent),
    m_settings(settings)
{
    setupItemsModel();
    setupStartupProjectBinding();
    setupCurrentEditorBinding();
    setupUpdateListTimer();
    createScanners();
}

TodoItemsModel *TodoItemsProvider::todoItemsModel()
{
    return m_itemsModel;
}

void TodoItemsProvider::settingsChanged(const Settings &newSettings)
{
    if (newSettings.keywords != m_settings.keywords) {
        foreach (TodoItemsScanner *scanner, m_scanners)
            scanner->setParams(newSettings.keywords);
    }

    m_settings = newSettings;

    updateList();
}

void TodoItemsProvider::projectSettingsChanged(Project *project)
{
    Q_UNUSED(project);
    updateList();
}

void TodoItemsProvider::updateList()
{
    m_itemsList.clear();

    // Show only items of the current file if any
    if (m_settings.scanningScope == ScanningScopeCurrentFile) {
        if (m_currentEditor)
            m_itemsList = m_itemsHash.value(m_currentEditor->document()->filePath().toString());
    // Show only items of the startup project if any
    } else if (m_startupProject) {
        setItemsListWithinStartupProject();
    }

    m_itemsModel->todoItemsListUpdated();
}

void TodoItemsProvider::createScanners()
{
    qRegisterMetaType<QList<TodoItem> >("QList<TodoItem>");

    if (CppTools::CppModelManager::instance())
        m_scanners << new CppTodoItemsScanner(m_settings.keywords, this);

    if (QmlJS::ModelManagerInterface::instance())
        m_scanners << new QmlJsTodoItemsScanner(m_settings.keywords, this);

    foreach (TodoItemsScanner *scanner, m_scanners) {
        connect(scanner, SIGNAL(itemsFetched(QString,QList<TodoItem>)), this,
            SLOT(itemsFetched(QString,QList<TodoItem>)), Qt::QueuedConnection);
    }
}

void TodoItemsProvider::setItemsListWithinStartupProject()
{
    QHashIterator<QString, QList<TodoItem> > it(m_itemsHash);
    QSet<QString> fileNames = QSet<QString>::fromList(m_startupProject->files(Project::ExcludeGeneratedFiles));

    QVariantMap settings = m_startupProject->namedSettings(QLatin1String(Constants::SETTINGS_NAME_KEY)).toMap();

    while (it.hasNext()) {
        it.next();
        QString fileName = it.key();
        if (fileNames.contains(fileName)) {
            bool skip = false;
            for (const QVariant &pattern : settings[QLatin1String(Constants::EXCLUDES_LIST_KEY)].toList()) {
                QRegExp re(pattern.toString());
                if (re.indexIn(fileName) != -1) {
                    skip = true;
                    break;
                }
            }
            if (!skip)
                m_itemsList << it.value();
        }
    }
}

void TodoItemsProvider::itemsFetched(const QString &fileName, const QList<TodoItem> &items)
{
    // Replace old items with new ones
    m_itemsHash.insert(fileName, items);

    m_shouldUpdateList = true;
}

void TodoItemsProvider::startupProjectChanged(Project *project)
{
    m_startupProject = project;
    updateList();
}

void TodoItemsProvider::projectsFilesChanged()
{
    updateList();
}

void TodoItemsProvider::currentEditorChanged(Core::IEditor *editor)
{
    m_currentEditor = editor;
    if (m_settings.scanningScope == ScanningScopeCurrentFile)
        updateList();
}

void TodoItemsProvider::updateListTimeoutElapsed()
{
    if (m_shouldUpdateList) {
        m_shouldUpdateList = false;
        updateList();
    }
}

void TodoItemsProvider::setupStartupProjectBinding()
{
    m_startupProject = SessionManager::startupProject();
    connect(SessionManager::instance(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        SLOT(startupProjectChanged(ProjectExplorer::Project*)));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(fileListChanged()), SLOT(projectsFilesChanged()));
}

void TodoItemsProvider::setupCurrentEditorBinding()
{
    m_currentEditor = Core::EditorManager::currentEditor();
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
        SLOT(currentEditorChanged(Core::IEditor*)));
}

void TodoItemsProvider::setupUpdateListTimer()
{
    m_shouldUpdateList = false;
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TodoItemsProvider::updateListTimeoutElapsed);
    timer->start(Constants::OUTPUT_PANE_UPDATE_INTERVAL);
}

void TodoItemsProvider::setupItemsModel()
{
    m_itemsModel = new TodoItemsModel(this);
    m_itemsModel->setTodoItemsList(&m_itemsList);
}

}
}
