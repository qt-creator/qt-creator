// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
