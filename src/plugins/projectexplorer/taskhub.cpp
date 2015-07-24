/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "taskhub.h"
#include "projectexplorerconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/ioutputpane.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

using namespace ProjectExplorer;

TaskHub *m_instance = 0;
QVector<Core::Id> TaskHub::m_registeredCategories;

static Core::Id categoryForType(Task::TaskType type)
{
    switch (type) {
    case Task::Error:
        return Constants::TASK_MARK_ERROR;
    case Task::Warning:
        return Constants::TASK_MARK_WARNING;
    default:
        return Core::Id();
    }
}

class TaskMark : public TextEditor::TextMark
{
public:
    TaskMark(unsigned int id, const QString &fileName, int lineNumber, Task::TaskType type, bool visible)
        : TextMark(fileName, lineNumber, categoryForType(type))
        , m_id(id)
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
    : m_errorIcon(QLatin1String(Core::Constants::ICON_ERROR)),
      m_warningIcon(QLatin1String(Core::Constants::ICON_WARNING))
{
    m_instance = this;
    qRegisterMetaType<ProjectExplorer::Task>("ProjectExplorer::Task");
    qRegisterMetaType<QList<ProjectExplorer::Task> >("QList<ProjectExplorer::Task>");
    TaskMark::setCategoryColor(Constants::TASK_MARK_ERROR,
                               Utils::Theme::ProjectExplorer_TaskError_TextMarkColor);
    TaskMark::setCategoryColor(Constants::TASK_MARK_WARNING,
                               Utils::Theme::ProjectExplorer_TaskWarn_TextMarkColor);
}

TaskHub::~TaskHub()
{
    m_instance = 0;
}

void TaskHub::addCategory(Core::Id categoryId, const QString &displayName, bool visible)
{
    QTC_CHECK(!displayName.isEmpty());
    QTC_ASSERT(!m_registeredCategories.contains(categoryId), return);
    m_registeredCategories.push_back(categoryId);
    emit m_instance->categoryAdded(categoryId, displayName, visible);
}

QObject *TaskHub::instance()
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

    if (task.file.isEmpty())
        task.line = -1;

    if (task.line <= 0)
        task.line = -1;
    task.movedLine = task.line;

    if (task.line != -1 && !task.file.isEmpty()) {
        TaskMark *mark = new TaskMark(task.taskId, task.file.toString(), task.line,
                                      task.type, !task.icon.isNull());
        mark->setIcon(task.icon);
        mark->setPriority(TextEditor::TextMark::LowPriority);
        task.addMark(mark);
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

