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

#ifndef PROJECTEXPLORER_TASK_H
#define PROJECTEXPLORER_TASK_H

#include "projectexplorer_export.h"

#include <QtCore/QMetaType>

#include <QtGui/QTextLayout>

namespace ProjectExplorer {

// Documentation inside.
class PROJECTEXPLORER_EXPORT Task
{
public:
    enum TaskType {
        Unknown,
        Error,
        Warning
    };

    Task();
    Task(TaskType type_, const QString &description_,
         const QString &file_, int line_, const QString &category_);

    bool isNull() const;

    unsigned int taskId;
    TaskType type;
    QString description;
    QString file;
    int line;
    QString category;
    // Having a QList<QTextLayout::FormatRange> in Task isn't that great
    // It would be cleaner to split up the text into
    // the logical hunks and then assemble them again
    // (That is different consumers of tasks could show them in
    // different ways!)
    // But then again, the wording of the text most likely
    // doesn't work if you split it up, nor are our parsers
    // anywhere near being that good
    QList<QTextLayout::FormatRange> formats;
private:
    static unsigned int s_nextId;
};

bool PROJECTEXPLORER_EXPORT operator==(const Task &t1, const Task &t2);
uint PROJECTEXPLORER_EXPORT qHash(const Task &task);

bool PROJECTEXPLORER_EXPORT operator<(const Task &a, const Task &b);

} //namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Task)
Q_DECLARE_METATYPE(QList<ProjectExplorer::Task>)

#endif // PROJECTEXPLORER_TASK_H
