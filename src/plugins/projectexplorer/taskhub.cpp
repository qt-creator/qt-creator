/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "taskhub.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/ioutputpane.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

TaskHub *m_instance = 0;
QSet<Core::Id> TaskHub::m_registeredCategories;

class TaskMark : public TextEditor::BaseTextMark
{
public:
    TaskMark(unsigned int id, const QString &fileName, int lineNumber, bool visible)
        : BaseTextMark(fileName, lineNumber), m_id(id)
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
    BaseTextMark::updateLineNumber(lineNumber);
}

void TaskMark::updateFileName(const QString &fileName)
{
    TaskHub::updateTaskFileName(m_id, fileName);
    BaseTextMark::updateFileName(fileName);
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
}

TaskHub::~TaskHub()
{
    m_instance = 0;
}

void TaskHub::addCategory(Core::Id categoryId, const QString &displayName, bool visible)
{
    QTC_CHECK(!displayName.isEmpty());
    QTC_ASSERT(!m_registeredCategories.contains(categoryId), return);
    m_registeredCategories.insert(categoryId);
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
        TaskMark *mark = new TaskMark(task.taskId, task.file.toString(), task.line, !task.icon.isNull());
        mark->setIcon(task.icon);
        mark->setPriority(TextEditor::ITextMark::LowPriority);
        task.addMark(mark);
        emit m_instance->taskAdded(task);
        mark->init();
    } else {
        emit m_instance->taskAdded(task);
    }
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

void TaskHub::setCategoryVisibility(const Core::Id &categoryId, bool visible)
{
    QTC_ASSERT(m_registeredCategories.contains(categoryId), return);
    emit m_instance->categoryVisibilityChanged(categoryId, visible);
}

void TaskHub::requestPopup()
{
    emit m_instance->popupRequested(Core::IOutputPane::NoModeSwitch);
}

