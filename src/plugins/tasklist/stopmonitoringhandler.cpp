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
