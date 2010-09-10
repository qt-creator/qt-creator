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

#include "stopmonitoringhandler.h"

#include "tasklistconstants.h"
#include "tasklistplugin.h"

#include <projectexplorer/task.h>

#include <QtGui/QAction>
#include <QtCore/QCoreApplication>

using namespace TaskList;
using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// StopMonitoringHandler
// --------------------------------------------------------------------------

StopMonitoringHandler::StopMonitoringHandler() :
    ProjectExplorer::ITaskHandler(QLatin1String("TaskList.StopMonitoringHandler"))
{ }

StopMonitoringHandler::~StopMonitoringHandler()
{ }

bool StopMonitoringHandler::canHandle(const ProjectExplorer::Task &task)
{
    return task.category == QLatin1String(Constants::TASKLISTTASK_ID);
}

void StopMonitoringHandler::handle(const ProjectExplorer::Task &task)
{
    Q_ASSERT(canHandle(task));
    TaskList::TaskListPlugin::instance()->stopMonitoring();
}

QAction *StopMonitoringHandler::createAction(QObject *parent)
{
    const QString text =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop monitoring");
    const QString toolTip =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop monitoring task files.");
    QAction *stopMonitoringAction = new QAction(text, parent);
    stopMonitoringAction->setToolTip(toolTip);
    return stopMonitoringAction;
}
