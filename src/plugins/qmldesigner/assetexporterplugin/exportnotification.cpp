/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "exportnotification.h"
#include "assetexportpluginconstants.h"

#include "projectexplorer/taskhub.h"

#include <QLoggingCategory>

namespace  {
Q_LOGGING_CATEGORY(loggerDebug, "qtc.designer.assetExportPlugin.exportNotification", QtDebugMsg)
}

using namespace ProjectExplorer;
namespace  {
static void addTask(Task::TaskType type, const QString &desc)
{
    qCDebug(loggerDebug) << desc;
    Task task(type, desc, {}, -1, QmlDesigner::Constants::TASK_CATEGORY_ASSET_EXPORT);
    TaskHub::addTask(task);
}
}

namespace QmlDesigner {

void ExportNotification::addError(const QString &errMsg)
{
    addTask(Task::Error, errMsg);
}

void ExportNotification::addWarning(const QString &warningMsg)
{
    addTask(Task::Warning, warningMsg);
}

void ExportNotification::addInfo(const QString &infoMsg)
{
    addTask(Task::Unknown, infoMsg);
}
} // QmlDesigner
