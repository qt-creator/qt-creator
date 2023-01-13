// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "removetaskhandler.h"

#include "projectexplorertr.h"
#include "task.h"
#include "taskhub.h"

#include <QAction>

namespace ProjectExplorer {
namespace Internal {

void RemoveTaskHandler::handle(const Tasks &tasks)
{
    for (const Task &task : tasks)
        TaskHub::removeTask(task);
}

QAction *RemoveTaskHandler::createAction(QObject *parent) const
{
    QAction *removeAction = new QAction(Tr::tr("Remove", "Name of the action triggering the removetaskhandler"), parent);
    removeAction->setToolTip(Tr::tr("Remove task from the task list."));
    removeAction->setShortcuts({QKeySequence::Delete, QKeySequence::Backspace});
    removeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    return removeAction;
}

} // namespace Internal
} // namespace ProjectExplorer
