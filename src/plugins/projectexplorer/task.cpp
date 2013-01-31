/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "task.h"

#include <QDir>

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
           const Utils::FileName &file_, int line_, const Core::Id &category_) :
    taskId(s_nextId), type(type_), description(description_),
    file(file_), line(line_), movedLine(line_), category(category_)
{
    ++s_nextId;
}

void Task::addMark(TextEditor::BaseTextMark *mark)
{
    m_mark = QSharedPointer<TextEditor::BaseTextMark>(mark);
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
        if (a.category.uniqueIdentifier() < b.category.uniqueIdentifier())
            return true;
        if (b.category.uniqueIdentifier() < a.category.uniqueIdentifier())
            return false;
        return a.taskId < b.taskId;
    }
}


uint qHash(const Task &task)
{
    return task.taskId;
}

} // namespace ProjectExplorer
