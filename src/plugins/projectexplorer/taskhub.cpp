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
#include <texteditor/textmark.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QThread>

using namespace Utils;

namespace ProjectExplorer {

// Task mark categories
const char TASK_MARK_WARNING[] = "Task.Mark.Warning";
const char TASK_MARK_ERROR[] = "Task.Mark.Error";

static TaskHub *m_instance = nullptr;
QVector<Utils::Id> TaskHub::m_registeredCategories;

static Utils::Id categoryForType(Task::TaskType type)
{
    switch (type) {
    case Task::Error:
        return TASK_MARK_ERROR;
    case Task::Warning:
        return TASK_MARK_WARNING;
    default:
        return Utils::Id();
    }
}

class TaskMark : public TextEditor::TextMark
{
public:
    TaskMark(const Task &task) :
        TextMark(task.file, task.line, categoryForType(task.type)),
        m_task(task)
    {
        setColor(task.type == Task::Error ? Utils::Theme::ProjectExplorer_TaskError_TextMarkColor
                                          : Utils::Theme::ProjectExplorer_TaskWarn_TextMarkColor);
        setDefaultToolTip(task.type == Task::Error ? QApplication::translate("TaskHub", "Error")
                                                   : QApplication::translate("TaskHub", "Warning"));
        setPriority(task.type == Task::Error ? TextEditor::TextMark::NormalPriority
                                             : TextEditor::TextMark::LowPriority);
        if (task.category == Constants::TASK_CATEGORY_COMPILE) {
            setToolTip("<html><body><b>" + QApplication::translate("TaskHub", "Build Issue")
                       + "</b><br/><code style=\"white-space:pre;font-family:monospace\">"
                       + task.description().toHtmlEscaped() + "</code></body></html>");
        } else {
            setToolTip(task.description());
        }
        setIcon(task.icon());
        setVisible(!task.icon().isNull());
    }

    bool isClickable() const override;
    void clicked() override;

    void updateFileName(const FilePath &fileName) override;
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

void TaskMark::updateFileName(const FilePath &fileName)
{
    TaskHub::updateTaskFileName(m_task, fileName.toString());
    TextMark::updateFileName(FilePath::fromString(fileName.toString()));
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
    m_instance = this;
    qRegisterMetaType<ProjectExplorer::Task>("ProjectExplorer::Task");
    qRegisterMetaType<Tasks >("Tasks");
}

TaskHub::~TaskHub()
{
    m_instance = nullptr;
}

void TaskHub::addCategory(Utils::Id categoryId, const QString &displayName, bool visible,
                          int priority)
{
    QTC_CHECK(!displayName.isEmpty());
    QTC_ASSERT(!m_registeredCategories.contains(categoryId), return);
    m_registeredCategories.push_back(categoryId);
    emit m_instance->categoryAdded(categoryId, displayName, visible, priority);
}

TaskHub *TaskHub::instance()
{
    return m_instance;
}

void TaskHub::addTask(Task::TaskType type, const QString &description, Utils::Id category)
{
    addTask(Task(type, description, {}, -1, category));
}

void TaskHub::addTask(Task task)
{
    QTC_ASSERT(m_registeredCategories.contains(task.category), return);
    QTC_ASSERT(!task.description().isEmpty(), return);
    QTC_ASSERT(!task.isNull(), return);
    QTC_ASSERT(task.m_mark.isNull(), return);
    QTC_ASSERT(QThread::currentThread() == qApp->thread(), return);

    if (task.file.isEmpty() || task.line <= 0)
        task.line = -1;
    task.movedLine = task.line;

    if ((task.options & Task::AddTextMark) && task.line != -1 && task.type != Task::Unknown)
        task.setMark(new TaskMark(task));
    emit m_instance->taskAdded(task);
}

void TaskHub::clearTasks(Utils::Id categoryId)
{
    QTC_ASSERT(!categoryId.isValid() || m_registeredCategories.contains(categoryId), return);
    emit m_instance->tasksCleared(categoryId);
}

void TaskHub::removeTask(const Task &task)
{
    emit m_instance->taskRemoved(task);
}

void TaskHub::updateTaskFileName(const Task &task, const QString &fileName)
{
    emit m_instance->taskFileNameUpdated(task, fileName);
}

void TaskHub::updateTaskLineNumber(const Task &task, int line)
{
    emit m_instance->taskLineNumberUpdated(task, line);
}

void TaskHub::taskMarkClicked(const Task &task)
{
    emit m_instance->showTask(task);
}

void TaskHub::showTaskInEditor(const Task &task)
{
    emit m_instance->openTask(task);
}

void TaskHub::setCategoryVisibility(Utils::Id categoryId, bool visible)
{
    QTC_ASSERT(m_registeredCategories.contains(categoryId), return);
    emit m_instance->categoryVisibilityChanged(categoryId, visible);
}

void TaskHub::requestPopup()
{
    emit m_instance->popupRequested(Core::IOutputPane::NoModeSwitch);
}

} // namespace ProjectExplorer
