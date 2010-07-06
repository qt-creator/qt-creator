/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef TASKHUB_H
#define TASKHUB_H

#include "task.h"
#include "projectexplorer_export.h"
#include <QtCore/QObject>
#include <QtGui/QIcon>

namespace ProjectExplorer {
class Task;

class PROJECTEXPLORER_EXPORT TaskHub : public QObject
{
    Q_OBJECT
public:
    TaskHub();
    virtual ~TaskHub();

    void addCategory(const QString &categoryId, const QString &displayName);
    void addTask(const Task &task);
    void clearTasks(const QString &categoryId = QString());
    void removeTask(const Task &task);

    // TODO now there are two places for icons
    QIcon taskTypeIcon(ProjectExplorer::Task::TaskType t) const;
signals:
    void categoryAdded(const QString &categoryId, const QString &displayName);
    void taskAdded(const ProjectExplorer::Task &task);
    void taskRemoved(const ProjectExplorer::Task &task);
    void tasksCleared(const QString &categoryId);
private:
    const QIcon m_errorIcon;
    const QIcon m_warningIcon;
};
} // namespace ProjectExplorer
#endif // TASKHUB_H
