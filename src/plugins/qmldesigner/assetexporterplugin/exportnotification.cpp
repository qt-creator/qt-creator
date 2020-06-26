/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Asset Importer module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/
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
