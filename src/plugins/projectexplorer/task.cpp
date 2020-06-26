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

#include "fileinsessionfinder.h"
#include "projectexplorerconstants.h"

#include <app/app_version.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QTextStream>

using namespace Utils;

namespace ProjectExplorer {

static QIcon taskTypeIcon(Task::TaskType t)
{
    static QIcon icons[3] = {QIcon(),
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

Task::Task(TaskType type_, const QString &description,
           const Utils::FilePath &file_, int line_, Utils::Id category_,
           const QIcon &icon, Options options) :
    taskId(s_nextId), type(type_), options(options), summary(description),
    line(line_), movedLine(line_), category(category_),
    icon(icon.isNull() ? taskTypeIcon(type_) : icon)
{
    ++s_nextId;
    setFile(file_);
    QStringList desc = description.split('\n');
    if (desc.length() > 1) {
        summary = desc.first();
        details = desc.mid(1);
    }
}

Task Task::compilerMissingTask()
{
    return BuildSystemTask(Task::Error,
                           tr("%1 needs a compiler set up to build. "
                              "Configure a compiler in the kit options.")
                           .arg(Core::Constants::IDE_DISPLAY_NAME));
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
    summary.clear();
    details.clear();
    file = Utils::FilePath();
    line = -1;
    movedLine = -1;
    category = Utils::Id();
    icon = QIcon();
    formats.clear();
    m_mark.clear();
}

void Task::setFile(const Utils::FilePath &file_)
{
    file = file_;
    if (!file.isEmpty() && !file.toFileInfo().isAbsolute()) {
        Utils::FilePaths possiblePaths = findFileInSession(file);
        if (possiblePaths.length() == 1)
            file = possiblePaths.first();
        else
            fileCandidates = possiblePaths;
    }
}

QString Task::description() const
{
    QString desc = summary;
    if (!details.isEmpty())
        desc.append('\n').append(details.join('\n'));
    return desc;
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

QString toHtml(const Tasks &issues)
{
    QString result;
    QTextStream str(&result);

    for (const Task &t : issues) {
        str << "<b>";
        switch (t.type) {
        case Task::Error:
            str << QCoreApplication::translate("ProjectExplorer::Kit", "Error:") << " ";
            break;
        case Task::Warning:
            str << QCoreApplication::translate("ProjectExplorer::Kit", "Warning:") << " ";
            break;
        case Task::Unknown:
        default:
            break;
        }
        str << "</b>" << t.description() << "<br>";
    }
    return result;
}

bool containsType(const Tasks &issues, Task::TaskType type)
{
    return Utils::contains(issues, [type](const Task &t) { return t.type == type; });
}

// CompilerTask

CompileTask::CompileTask(TaskType type, const QString &desc, const FilePath &file, int line)
    : Task(type, desc, file, line, ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)
{}

// BuildSystemTask

BuildSystemTask::BuildSystemTask(Task::TaskType type, const QString &desc, const FilePath &file, int line)
    : Task(type, desc, file, line, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)
{}

// DeploymentTask

DeploymentTask::DeploymentTask(Task::TaskType type, const QString &desc)
    : Task(type, desc, {}, -1, ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT)
{}

} // namespace ProjectExplorer
