// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copytaskhandler.h"

#include "projectexplorertr.h"

#include <coreplugin/coreconstants.h>

#include <utils/stringutils.h>

#include <QAction>

namespace ProjectExplorer::Internal {

void CopyTaskHandler::handle(const Tasks &tasks)
{
    QStringList lines;
    for (const Task &task : tasks) {
        QString type;
        switch (task.type) {
        case Task::Error:
            //: Task is of type: error
            type = Tr::tr("error:") + QLatin1Char(' ');
            break;
        case Task::Warning:
            //: Task is of type: warning
            type = Tr::tr("warning:") + QLatin1Char(' ');
            break;
        default:
            break;
        }
        lines << task.file.toUserOutput() + ':' + QString::number(task.line)
                 + ": " + type + task.description();
    }
    Utils::setClipboardAndSelection(lines.join('\n'));
}

Utils::Id CopyTaskHandler::actionManagerId() const
{
    return Utils::Id(Core::Constants::COPY);
}

QAction *CopyTaskHandler::createAction(QObject *parent) const
{
    return new QAction(parent);
}

} // ProjectExplorer::Internal
