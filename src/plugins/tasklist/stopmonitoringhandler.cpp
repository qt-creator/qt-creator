/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    TaskList::TaskListPlugin::instance()->stopMonitoring();
}

QAction *StopMonitoringHandler::createAction(QObject *parent) const
{
    const QString text =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop Monitoring");
    const QString toolTip =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop monitoring task files.");
    QAction *stopMonitoringAction = new QAction(text, parent);
    stopMonitoringAction->setToolTip(toolTip);
    return stopMonitoringAction;
}
