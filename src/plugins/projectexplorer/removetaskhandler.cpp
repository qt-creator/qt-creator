/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2013 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "removetaskhandler.h"

#include "projectexplorer.h"
#include "task.h"
#include "taskhub.h"

#include <coreplugin/coreconstants.h>

#include <QDir>
#include <QAction>
#include <QApplication>
#include <QClipboard>

using namespace ProjectExplorer::Internal;

void RemoveTaskHandler::handle(const ProjectExplorer::Task &task)
{
    ProjectExplorerPlugin::instance()->taskHub()->removeTask(task);
}

QAction *RemoveTaskHandler::createAction(QObject *parent) const
{
    QAction *removeAction = new QAction(tr("Remove", "Name of the action triggering the removetaskhandler"), parent);
    removeAction->setToolTip(tr("Remove task from the task list"));
    removeAction->setShortcut(QKeySequence(QKeySequence::Delete));
    removeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    return removeAction;
}
