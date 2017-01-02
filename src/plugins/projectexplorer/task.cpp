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

#include "task.h"

#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include "projectexplorerconstants.h"

namespace ProjectExplorer
{

static QIcon taskTypeIcon(Task::TaskType t)
{
    static QIcon icons[3] = { QIcon(),
                              Utils::Icons::CRITICAL.icon(),
                              Utils::Icons::WARNING.icon()};

    if (t < 0 || t > 2)
        t = Task::Unknown;

    return icons[t];
}

unsigned int Task::s_nextId = 1;

/*!
    \class  ProjectExplorer::Task
    \brief The Task class represents a build issue (warning or error).
    \sa ProjectExplorer::TaskHub
*/

Task::Task(TaskType type_, const QString &description_,
           const Utils::FileName &file_, int line_, Core::Id category_,
           const Utils::FileName &iconFile) :
    taskId(s_nextId), type(type_), description(description_),
    file(file_), line(line_), movedLine(line_), category(category_),
    icon(iconFile.isEmpty() ? taskTypeIcon(type_) : QIcon(iconFile.toString()))
{
    ++s_nextId;
}

Task Task::compilerMissingTask()
{
    return Task(Task::Error,
                QCoreApplication::translate("ProjectExplorer::Task",
                                            "Qt Creator needs a compiler set up to build. "
                                            "Configure a compiler in the kit options."),
                Utils::FileName(), -1,
                Constants::TASK_CATEGORY_BUILDSYSTEM);
}

Task Task::buildConfigurationMissingTask()
{
    return Task(Task::Error,
                QCoreApplication::translate("ProjectExplorer::Task",
                                            "Qt Creator needs a build configuration set up to build. "
                                            "Configure a build configuration in the project settings."),
                Utils::FileName(), -1,
                Constants::TASK_CATEGORY_BUILDSYSTEM);
}

void Task::setMark(TextEditor::TextMark *mark)
{
    QTC_ASSERT(mark, return);
    QTC_ASSERT(m_mark.isNull(), return);
    m_mark = QSharedPointer<TextEditor::TextMark>(mark);
}

bool Task::isNull() const
{
    return taskId == 0;
}

void Task::clear()
{
    taskId = 0;
    type = Task::Unknown;
    description.clear();
    file = Utils::FileName();
    line = -1;
    movedLine = -1;
    category = Core::Id();
    icon = QIcon();
    formats.clear();
    m_mark.clear();
}

//
// functions
//
bool operator==(const Task &t1, const Task &t2)
{
    return t1.taskId == t2.taskId;
}

bool operator<(const Task &a, const Task &b)
{
    if (a.type != b.type) {
        if (a.type == Task::Error)
            return true;
        if (b.type == Task::Error)
            return false;
        if (a.type == Task::Warning)
            return true;
        if (b.type == Task::Warning)
            return false;
        // Can't happen
        return true;
    } else {
        if (a.category < b.category)
            return true;
        if (b.category < a.category)
            return false;
        return a.taskId < b.taskId;
    }
}


uint qHash(const Task &task)
{
    return task.taskId;
}

} // namespace ProjectExplorer
