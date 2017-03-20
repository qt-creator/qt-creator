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

#include "taskhub.h"
#include "projectexplorerconstants.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/ioutputpane.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

namespace ProjectExplorer {

// Task mark categories
const char TASK_MARK_WARNING[] = "Task.Mark.Warning";
const char TASK_MARK_ERROR[] = "Task.Mark.Error";

static TaskHub *m_instance = nullptr;
QVector<Core::Id> TaskHub::m_registeredCategories;

static Core::Id categoryForType(Task::TaskType type)
{
    switch (type) {
    case Task::Error:
        return TASK_MARK_ERROR;
    case Task::Warning:
        return TASK_MARK_WARNING;
    default:
        return Core::Id();
    }
}

class TaskMark : public TextEditor::TextMark
{
public:
    TaskMark(unsigned int id, const QString &fileName, int lineNumber,
             Task::TaskType type, bool visible) :
        TextMark(fileName, lineNumber, categoryForType(type)),
        m_id(id)
    {
        setVisible(visible);
    }

    bool isClickable() const;
    void clicked();

    void updateFileName(const QString &fileName);
    void updateLineNumber(int lineNumber);
    void removedFromEditor();
private:
    unsigned int m_id;
};

void TaskMark::updateLineNumber(int lineNumber)
{
    TaskHub::updateTaskLineNumber(m_id, lineNumber);
    TextMark::updateLineNumber(lineNumber);
}

void TaskMark::updateFileName(const QString &fileName)
{
    TaskHub::updateTaskFileName(m_id, fileName);
    TextMark::updateFileName(fileName);
}

void TaskMark::removedFromEditor()
{
    TaskHub::updateTaskLineNumber(m_id, -1);
}

bool TaskMark::isClickable() const
{
    return true;
}

void TaskMark::clicked()
{
    TaskHub::taskMarkClicked(m_id);
}

TaskHub::TaskHub()
{
    m_instance = this;
    qRegisterMetaType<ProjectExplorer::Task>("ProjectExplorer::Task");
    qRegisterMetaType<QList<ProjectExplorer::Task> >("QList<ProjectExplorer::Task>");
    TaskMark::setCategoryColor(TASK_MARK_ERROR,
                               Utils::Theme::ProjectExplorer_TaskError_TextMarkColor);
    TaskMark::setCategoryColor(TASK_MARK_WARNING,
                               Utils::Theme::ProjectExplorer_TaskWarn_TextMarkColor);
    TaskMark::setDefaultToolTip(TASK_MARK_ERROR, tr("Error"));
    TaskMark::setDefaultToolTip(TASK_MARK_WARNING, tr("Warning"));
}

TaskHub::~TaskHub()
{
    m_instance = nullptr;
}

void TaskHub::addCategory(Core::Id categoryId, const QString &displayName, bool visible)
{
    QTC_CHECK(!displayName.isEmpty());
    QTC_ASSERT(!m_registeredCategories.contains(categoryId), return);
    m_registeredCategories.push_back(categoryId);
    emit m_instance->categoryAdded(categoryId, displayName, visible);
}

TaskHub *TaskHub::instance()
{
    return m_instance;
}

void TaskHub::addTask(Task::TaskType type, const QString &description, Core::Id category, const Utils::FileName &file, int line)
{
    addTask(Task(type, description, file, line, category));
}

void TaskHub::addTask(Task task)
{
    QTC_ASSERT(m_registeredCategories.contains(task.category), return);
    QTC_ASSERT(!task.description.isEmpty(), return);
    QTC_ASSERT(!task.isNull(), return);
    QTC_ASSERT(task.m_mark.isNull(), return);

    if (task.file.isEmpty() || task.line <= 0)
        task.line = -1;
    task.movedLine = task.line;

    if (task.line != -1) {
        auto mark = new TaskMark(task.taskId, task.file.toString(), task.line, task.type, !task.icon.isNull());
        mark->setIcon(task.icon);
        mark->setPriority(TextEditor::TextMark::LowPriority);
        mark->setToolTip(task.description);
        task.setMark(mark);
    }
    emit m_instance->taskAdded(task);
}

void TaskHub::clearTasks(Core::Id categoryId)
{
    QTC_ASSERT(!categoryId.isValid() || m_registeredCategories.contains(categoryId), return);
    emit m_instance->tasksCleared(categoryId);
}

void TaskHub::removeTask(const Task &task)
{
    emit m_instance->taskRemoved(task);
}

void TaskHub::updateTaskFileName(unsigned int id, const QString &fileName)
{
    emit m_instance->taskFileNameUpdated(id, fileName);
}

void TaskHub::updateTaskLineNumber(unsigned int id, int line)
{
    emit m_instance->taskLineNumberUpdated(id, line);
}

void TaskHub::taskMarkClicked(unsigned int id)
{
    emit m_instance->showTask(id);
}

void TaskHub::showTaskInEditor(unsigned int id)
{
    emit m_instance->openTask(id);
}

void TaskHub::setCategoryVisibility(Core::Id categoryId, bool visible)
{
    QTC_ASSERT(m_registeredCategories.contains(categoryId), return);
    emit m_instance->categoryVisibilityChanged(categoryId, visible);
}

void TaskHub::requestPopup()
{
    emit m_instance->popupRequested(Core::IOutputPane::NoModeSwitch);
}

} // namespace ProjectExplorer
