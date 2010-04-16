/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmltaskmanager.h"
#include "qmlprojectconstants.h"

#include <extensionsystem/pluginmanager.h>

#include <QDebug>

namespace QmlProjectManager {
namespace Internal {

QmlTaskManager::QmlTaskManager(QObject *parent) :
        QObject(parent),
        m_taskWindow(0)
{
}

QmlTaskManager *QmlTaskManager::instance()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    return pluginManager->getObject<QmlTaskManager>();
}

void QmlTaskManager::setTaskWindow(ProjectExplorer::TaskWindow *taskWindow)
{
    Q_ASSERT(taskWindow);
    m_taskWindow = taskWindow;

    m_taskWindow->addCategory(Constants::TASK_CATEGORY_QML, tr("QML"));
}

void QmlTaskManager::documentChangedOnDisk(QmlJS::Document::Ptr doc)
{
    const QString fileName = doc->fileName();
    removeTasksForFile(fileName);

    foreach (const QmlJS::DiagnosticMessage &msg, doc->diagnosticMessages()) {
        ProjectExplorer::Task::TaskType type
                = msg.isError() ? ProjectExplorer::Task::Error
                                : ProjectExplorer::Task::Warning;

        ProjectExplorer::Task task(type, msg.message, fileName, msg.loc.startLine,
                                   Constants::TASK_CATEGORY_QML);
        insertTask(fileName, task);
    }
}

void QmlTaskManager::documentsRemoved(const QStringList path)
{
    foreach (const QString &item, path)
        removeTasksForFile(item);
}

void QmlTaskManager::insertTask(const QString &fileName, const ProjectExplorer::Task &task)
{
    QList<ProjectExplorer::Task> tasks = m_docsWithTasks.value(fileName);
    tasks.append(task);
    m_docsWithTasks.insert(fileName, tasks);
    m_taskWindow->addTask(task);
}

void QmlTaskManager::removeTasksForFile(const QString &fileName)
{
    if (m_docsWithTasks.contains(fileName)) {
        const QList<ProjectExplorer::Task> tasks = m_docsWithTasks.value(fileName);
        foreach (const ProjectExplorer::Task &task, tasks)
            m_taskWindow->removeTask(task);
        m_docsWithTasks.remove(fileName);
    }
}

} // Internal
} // QmlProjectManager
