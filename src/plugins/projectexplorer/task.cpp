/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "task.h"

namespace ProjectExplorer
{

unsigned int Task::s_nextId = 1;

Task::Task() : taskId(0), type(Unknown), line(-1)
{ }

Task::Task(TaskType type_, const QString &description_,
           const QString &file_, int line_, const QString &category_) :
    taskId(s_nextId), type(type_), description(description_), file(file_),
    line(line_), category(category_)
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

uint qHash(const Task &task)
{
    return task.taskId;
}

} // namespace ProjectExplorer
