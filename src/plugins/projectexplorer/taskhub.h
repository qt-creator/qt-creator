// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "task.h"

#include <QObject>
#include <QIcon>

namespace ProjectExplorer {

class ProjectExplorerPlugin;

class PROJECTEXPLORER_EXPORT TaskCategory
{
public:
    Utils::Id id;
    QString displayName;
    QString description;
    bool visible = true;
    int priority = 0;
};

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
    static void addCategory(const TaskCategory &category);
    static void updateTaskFileName(const Task &task, const QString &fileName);
    static void updateTaskLineNumber(const Task &task, int line);
    static void taskMarkClicked(const Task &task);
    static void showTaskInEditor(const Task &task);
    static void setCategoryVisibility(Utils::Id categoryId, bool visible);

    static void requestPopup();

signals:
    void categoryAdded(const TaskCategory &category);
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
