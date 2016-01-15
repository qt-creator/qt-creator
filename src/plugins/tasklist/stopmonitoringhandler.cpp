/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "stopmonitoringhandler.h"

#include "tasklistconstants.h"
#include "tasklistplugin.h"

#include <projectexplorer/task.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QCoreApplication>

using namespace TaskList;
using namespace TaskList::Internal;

// --------------------------------------------------------------------------
// StopMonitoringHandler
// --------------------------------------------------------------------------

bool StopMonitoringHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return task.category == Constants::TASKLISTTASK_ID;
}

void StopMonitoringHandler::handle(const ProjectExplorer::Task &task)
{
    QTC_ASSERT(canHandle(task), return);
    Q_UNUSED(task);
    TaskListPlugin::stopMonitoring();
}

QAction *StopMonitoringHandler::createAction(QObject *parent) const
{
    const QString text =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop Monitoring");
    const QString toolTip =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop monitoring task files.");
    auto stopMonitoringAction = new QAction(text, parent);
    stopMonitoringAction->setToolTip(toolTip);
    return stopMonitoringAction;
}
