/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "task.h"

#include <QObject>
#include <QIcon>

namespace ProjectExplorer {

class ProjectExplorerPlugin;

class PROJECTEXPLORER_EXPORT TaskHub : public QObject
{
    Q_OBJECT
public:
    static TaskHub *instance();

    // Convenience overload
    static void addTask(Task::TaskType type, const QString &description,
                        Utils::Id category);

public slots:
    static void addTask(ProjectExplorer::Task task);
    static void clearTasks(Utils::Id categoryId = Utils::Id());
    static void removeTask(const ProjectExplorer::Task &task);

public:
    static void addCategory(Utils::Id categoryId, const QString &displayName, bool visible = true,
                            int priority = 0);
    static void updateTaskFileName(const Task &task, const QString &fileName);
    static void updateTaskLineNumber(const Task &task, int line);
    static void taskMarkClicked(const Task &task);
    static void showTaskInEditor(const Task &task);
    static void setCategoryVisibility(Utils::Id categoryId, bool visible);

    static void requestPopup();

signals:
    void categoryAdded(Utils::Id categoryId, const QString &displayName, bool visible,
                       int priority);
    void taskAdded(const ProjectExplorer::Task &task);
    void taskRemoved(const ProjectExplorer::Task &task);
    void tasksCleared(Utils::Id categoryId);
    void taskFileNameUpdated(const Task &task, const QString &fileName);
    void taskLineNumberUpdated(const Task &task, int line);
    void categoryVisibilityChanged(Utils::Id categoryId, bool visible);
    void popupRequested(int);
    void showTask(const Task &task);
    void openTask(const Task &task);

private:
    TaskHub();
    ~TaskHub() override;

    static QVector<Utils::Id> m_registeredCategories;

    friend class ProjectExplorerPluginPrivate;
};

} // namespace ProjectExplorer
