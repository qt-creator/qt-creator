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

class PROJECTEXPLORER_EXPORT TaskHub final : public QObject
{
    Q_OBJECT

public:
    template<typename T, typename ... Args> static void addTask(Args ... args)
    {
        addTask(T(args...));
    }

    static void addTask(Task::TaskType type, const QString &description,
                        Utils::Id category);

    static void addTask(Task task);
    static void clearTasks(Utils::Id categoryId = Utils::Id());
    static void removeTask(const Task &task);
    static void clearAndRemoveTask(Task &task);

    static void addCategory(const TaskCategory &category);
    static void updateTaskFilePath(const Task &task, const Utils::FilePath &filePath);
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
    void taskFilePathUpdated(const Task &task, const Utils::FilePath &filePath);
    void taskLineNumberUpdated(const Task &task, int line);
    void categoryVisibilityChanged(Utils::Id categoryId, bool visible);
    void popupRequested(int);
    void showTask(const Task &task);
    void openTask(const Task &task);

private:
    friend PROJECTEXPLORER_EXPORT TaskHub &taskHub();

    TaskHub();
    ~TaskHub() final;
};

PROJECTEXPLORER_EXPORT TaskHub &taskHub();

} // namespace ProjectExplorer
