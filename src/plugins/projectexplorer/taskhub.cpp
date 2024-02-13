// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskhub.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/ioutputpane.h>
#include <texteditor/textmark.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/threadutils.h>
#include <utils/utilsicons.h>

using namespace Utils;

namespace ProjectExplorer {

// Task mark categories
const char TASK_MARK_WARNING[] = "Task.Mark.Warning";
const char TASK_MARK_ERROR[] = "Task.Mark.Error";

static QVector<Id> s_registeredCategories;

static TextEditor::TextMarkCategory categoryForType(Task::TaskType type)
{
    switch (type) {
    case Task::Error:
        return {::ProjectExplorer::Tr::tr("Taskhub Error"), TASK_MARK_ERROR};
    case Task::Warning:
        return {::ProjectExplorer::Tr::tr("Taskhub Warning"), TASK_MARK_WARNING};
    default:
        return {};
    }
}

class TaskMark : public TextEditor::TextMark
{
public:
    TaskMark(const Task &task) :
        TextMark(task.file, task.line, categoryForType(task.type)),
        m_task(task)
    {
        setColor(task.type == Task::Error ? Theme::ProjectExplorer_TaskError_TextMarkColor
                                          : Theme::ProjectExplorer_TaskWarn_TextMarkColor);
        setDefaultToolTip(task.type == Task::Error ? Tr::tr("Error")
                                                   : Tr::tr("Warning"));
        setPriority(task.type == Task::Error ? TextEditor::TextMark::NormalPriority
                                             : TextEditor::TextMark::LowPriority);
        setToolTip(task.formattedDescription({Task::WithSummary | Task::WithLinks},
                                             task.category == Constants::TASK_CATEGORY_COMPILE
                                                 ? Tr::tr("Build Issue") : QString()));
        setIcon(task.icon());
        setVisible(!task.icon().isNull());
    }

    bool isClickable() const override;
    void clicked() override;

    void updateFilePath(const FilePath &fileName) override;
    void updateLineNumber(int lineNumber) override;
    void removedFromEditor() override;
private:
    const Task m_task;
};

void TaskMark::updateLineNumber(int lineNumber)
{
    TaskHub::updateTaskLineNumber(m_task, lineNumber);
    TextMark::updateLineNumber(lineNumber);
}

void TaskMark::updateFilePath(const FilePath &fileName)
{
    TaskHub::updateTaskFileName(m_task, fileName.toString());
    TextMark::updateFilePath(FilePath::fromString(fileName.toString()));
}

void TaskMark::removedFromEditor()
{
    TaskHub::updateTaskLineNumber(m_task, -1);
}

bool TaskMark::isClickable() const
{
    return true;
}

void TaskMark::clicked()
{
    TaskHub::taskMarkClicked(m_task);
}

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

void TaskHub::addTask(Task::TaskType type, const QString &description, Id category)
{
    addTask(Task(type, description, {}, -1, category));
}

void TaskHub::addTask(Task task)
{
    if (!isMainThread()) {
        QMetaObject::invokeMethod(qApp, [task = std::move(task)] {
            TaskHub::addTask(task);
        });

        return;
    }

    QTC_ASSERT(s_registeredCategories.contains(task.category), return);
    QTC_ASSERT(!task.description().isEmpty(), return);
    QTC_ASSERT(!task.isNull(), return);
    QTC_ASSERT(!task.m_mark, return);

    if (task.file.isEmpty() || task.line <= 0)
        task.line = -1;
    task.movedLine = task.line;

    if ((task.options & Task::AddTextMark) && task.line != -1 && task.type != Task::Unknown)
        task.setMark(new TaskMark(task));
    emit taskHub().taskAdded(task);
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

void TaskHub::updateTaskFileName(const Task &task, const QString &fileName)
{
    emit taskHub().taskFileNameUpdated(task, fileName);
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
