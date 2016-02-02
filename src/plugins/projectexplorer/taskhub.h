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
                        Core::Id category,
                        const Utils::FileName &file = Utils::FileName(),
                        int line = -1);

public slots:
    static void addTask(ProjectExplorer::Task task);
    static void clearTasks(Core::Id categoryId = Core::Id());
    static void removeTask(const ProjectExplorer::Task &task);

public:
    static void addCategory(Core::Id categoryId, const QString &displayName, bool visible = true);
    static void updateTaskFileName(unsigned int id, const QString &fileName);
    static void updateTaskLineNumber(unsigned int id, int line);
    static void taskMarkClicked(unsigned int id);
    static void showTaskInEditor(unsigned int id);
    static void setCategoryVisibility(Core::Id categoryId, bool visible);

    static void requestPopup();

signals:
    void categoryAdded(Core::Id categoryId, const QString &displayName, bool visible);
    void taskAdded(const ProjectExplorer::Task &task);
    void taskRemoved(const ProjectExplorer::Task &task);
    void tasksCleared(Core::Id categoryId);
    void taskFileNameUpdated(unsigned int id, const QString &fileName);
    void taskLineNumberUpdated(unsigned int id, int line);
    void categoryVisibilityChanged(Core::Id categoryId, bool visible);
    void popupRequested(int);
    void showTask(unsigned int id);
    void openTask(unsigned int id);

private:
    TaskHub();
    ~TaskHub() override;

    static QVector<Core::Id> m_registeredCategories;

    friend class ProjectExplorerPlugin;
};

} // namespace ProjectExplorer
