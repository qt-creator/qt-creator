/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "copytaskhandler.h"

#include "task.h"

#include <coreplugin/coreconstants.h>

#include <QDir>
#include <QAction>
#include <QApplication>
#include <QClipboard>

using namespace ProjectExplorer::Internal;

void CopyTaskHandler::handle(const ProjectExplorer::Task &task)
{
    QString type;
    switch (task.type) {
    case Task::Error:
        //: Task is of type: error
        type = tr("error: ");
        break;
    case Task::Warning:
        //: Task is of type: warning
        type = tr("warning: ");
        break;
    default:
        break;
    }

    QApplication::clipboard()->setText(task.file.toUserOutput() + QLatin1Char(':') +
                                       QString::number(task.line) + QLatin1String(": ")
                                       + type + task.description);
}

QAction *CopyTaskHandler::createAction(QObject *parent) const
{
    QAction *copyAction = new QAction(tr("&Copy", "Name of the action triggering the copytaskhandler"), parent);
    copyAction->setToolTip(tr("Copy task to clipboard"));
    return copyAction;
}
