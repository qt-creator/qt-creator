/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QDebug>

namespace QmlProjectManager {
namespace Internal {

QmlTaskManager::QmlTaskManager(QObject *parent) :
        QObject(parent),
        m_taskWindow(0)
{
}

void QmlTaskManager::setTaskWindow(ProjectExplorer::TaskWindow *taskWindow)
{
    Q_ASSERT(taskWindow);
    m_taskWindow = taskWindow;

    m_taskWindow->addCategory(Constants::TASK_CATEGORY_QML, "Qml");
}

void QmlTaskManager::documentUpdated(QmlEditor::QmlDocument::Ptr doc)
{
    m_taskWindow->clearTasks(Constants::TASK_CATEGORY_QML);

    foreach (const QmlJS::DiagnosticMessage &msg, doc->diagnosticMessages()) {
        ProjectExplorer::TaskWindow::TaskType type
                = msg.isError() ? ProjectExplorer::TaskWindow::Error
                                : ProjectExplorer::TaskWindow::Warning;

        ProjectExplorer::TaskWindow::Task task(type, msg.message, doc->fileName(), msg.loc.startLine,
                                                Constants::TASK_CATEGORY_QML);
        m_taskWindow->addTask(task);
    }
}

} // Internal
} // QmlEditor
