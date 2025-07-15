// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskhub.h"

#include <coreplugin/ioutputpane.h>

#include <utils/qtcassert.h>
#include <utils/threadutils.h>

#include <QGuiApplication>

using namespace Utils;

namespace ProjectExplorer {

static QList<Id> s_registeredCategories;

TaskHub::TaskHub()
{
    qRegisterMetaType<ProjectExplorer::Task>("ProjectExplorer::Task");
    qRegisterMetaType<Tasks >("Tasks");
}

TaskHub::~TaskHub() = default;

void TaskHub::addCategory(const TaskCategory &category)
{
    QTC_CHECK(!category.displayName.isEmpty());
    QTC_ASSERT(!s_registeredCategories.contains(category.id), return);
    s_registeredCategories.push_back(category.id);
    emit taskHub().categoryAdded(category);
}

void TaskHub::addTask(Task::TaskType type, const QString &description, Utils::Id category)
{
    addTask<Task>(type, description, Utils::FilePath(), -1, category);
}

void TaskHub::addTask(Task task)
{
    if (!isMainThread()) {
        QMetaObject::invokeMethod(qApp, [task = std::move(task)] {
            TaskHub::addTask(task);
        });

        return;
    }

    bool popUp = false;
    if (task.type() == Task::DisruptingError) {
        popUp = true;
        task.setType(Task::Error);
    }

    QTC_ASSERT(s_registeredCategories.contains(task.category()), return);
    QTC_ASSERT(!task.description().isEmpty(), return);
    QTC_ASSERT(!task.isNull(), return);
    QTC_ASSERT(!task.hasMark(), return);

    if (task.file().isEmpty() || task.line() <= 0)
        task.setLine(-1);
    emit taskHub().taskAdded(task);
    if (popUp)
        requestPopup();
}

void TaskHub::clearTasks(Id categoryId)
{
    QTC_ASSERT(!categoryId.isValid() || s_registeredCategories.contains(categoryId), return);
    emit taskHub().tasksCleared(categoryId);
}

void TaskHub::removeTask(const Task &task)
{
    emit taskHub().taskRemoved(task);
}

void TaskHub::updateTaskFilePath(const Task &task, const FilePath &filePath)
{
    emit taskHub().taskFilePathUpdated(task, filePath);
}

void TaskHub::updateTaskLineNumber(const Task &task, int line)
{
    emit taskHub().taskLineNumberUpdated(task, line);
}

void TaskHub::taskMarkClicked(const Task &task)
{
    emit taskHub().showTask(task);
}

void TaskHub::showTaskInEditor(const Task &task)
{
    emit taskHub().openTask(task);
}

void TaskHub::setCategoryVisibility(Id categoryId, bool visible)
{
    QTC_ASSERT(s_registeredCategories.contains(categoryId), return);
    emit taskHub().categoryVisibilityChanged(categoryId, visible);
}

void TaskHub::requestPopup()
{
    emit taskHub().popupRequested(Core::IOutputPane::NoModeSwitch);
}

TaskHub &taskHub()
{
    static TaskHub theTaskHub;
    return theTaskHub;
}

} // namespace ProjectExplorer
