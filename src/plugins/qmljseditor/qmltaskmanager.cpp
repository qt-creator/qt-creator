/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmltaskmanager.h"
#include "qmljseditorconstants.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/taskhub.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtCore/QDebug>

namespace QmlJSEditor {
namespace Internal {

QmlTaskManager::QmlTaskManager(QObject *parent) :
        QObject(parent),
        m_taskHub(0)
{
    m_taskHub = ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::TaskHub>();
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
    m_taskHub->addTask(task);
}

void QmlTaskManager::removeTasksForFile(const QString &fileName)
{
    if (m_docsWithTasks.contains(fileName)) {
        const QList<ProjectExplorer::Task> tasks = m_docsWithTasks.value(fileName);
        foreach (const ProjectExplorer::Task &task, tasks)
            m_taskHub->removeTask(task);
        m_docsWithTasks.remove(fileName);
    }
}

} // Internal
} // QmlProjectManager
