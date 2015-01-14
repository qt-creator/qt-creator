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

#ifndef PROJECTEXPLORER_TASK_H
#define PROJECTEXPLORER_TASK_H

#include "projectexplorer_export.h"

#include <coreplugin/id.h>
#include <texteditor/textmark.h>
#include <utils/fileutils.h>

#include <QIcon>
#include <QMetaType>
#include <QTextLayout>

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
    Task(TaskType type, const QString &description,
         const Utils::FileName &file, int line, Core::Id category,
         const Utils::FileName &iconName = Utils::FileName());

    static Task compilerMissingTask();
    static Task buildConfigurationMissingTask();

    bool isNull() const;
    void clear();

    unsigned int taskId;
    TaskType type;
    QString description;
    Utils::FileName file;
    int line;
    int movedLine; // contains a line number if the line was moved in the editor
    Core::Id category;
    QIcon icon;
    void addMark(TextEditor::TextMark *mark);

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
    QSharedPointer<TextEditor::TextMark> m_mark;
    static unsigned int s_nextId;
};

bool PROJECTEXPLORER_EXPORT operator==(const Task &t1, const Task &t2);
uint PROJECTEXPLORER_EXPORT qHash(const Task &task);

bool PROJECTEXPLORER_EXPORT operator<(const Task &a, const Task &b);

} //namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Task)
Q_DECLARE_METATYPE(QList<ProjectExplorer::Task>)

#endif // PROJECTEXPLORER_TASK_H
