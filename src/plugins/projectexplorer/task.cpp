/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "task.h"

#include <QtCore/QDir>

namespace ProjectExplorer
{

unsigned int Task::s_nextId = 1;

/*!
    \class  ProjectExplorer::Task
    \brief Build issue (warning or error).
    \sa ProjectExplorer::TaskHub
*/

Task::Task() : taskId(0), type(Unknown), line(-1)
{ }

Task::Task(TaskType type_, const QString &description_,
           const QString &file_, int line_, const QString &category_) :
    taskId(s_nextId), type(type_), description(description_), file(QDir::fromNativeSeparators(file_)), line(line_), category(category_)
{
    ++s_nextId;
}

bool Task::isNull() const
{ return taskId == 0; }

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
        if (a.type == ProjectExplorer::Task::Error)
            return true;
        if (b.type == ProjectExplorer::Task::Error)
            return false;
        if (a.type == ProjectExplorer::Task::Warning)
            return true;
        if (b.type == ProjectExplorer::Task::Warning)
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
