// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "showineditortaskhandler.h"

#include "task.h"
#include "projectexplorertr.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QAction>
#include <QFileInfo>

namespace ProjectExplorer {
namespace Internal {

bool ShowInEditorTaskHandler::canHandle(const Task &task) const
{
    if (task.file.isEmpty())
        return false;
    QFileInfo fi(task.file.toFileInfo());
    return fi.exists() && fi.isFile() && fi.isReadable();
}

void ShowInEditorTaskHandler::handle(const Task &task)
{
    const int column = task.column ? task.column - 1 : 0;
    Core::EditorManager::openEditorAt({task.file, task.movedLine, column},
                                      {},
                                      Core::EditorManager::SwitchSplitIfAlreadyVisible);
}

QAction *ShowInEditorTaskHandler::createAction(QObject *parent) const
{
    QAction *showAction = new QAction(Tr::tr("Show in Editor"), parent);
    showAction->setToolTip(Tr::tr("Show task location in an editor."));
    showAction->setShortcut(QKeySequence(Qt::Key_Return));
    showAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    return showAction;
}

} // namespace Internal
} // namespace ProjectExplorer
