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

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/id.h>
#include <texteditor/textmark.h>
#include <utils/fileutils.h>

#include <QIcon>
#include <QMetaType>
#include <QTextLayout>

namespace ProjectExplorer {

class TaskHub;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Task
{
public:
    enum TaskType {
        Unknown,
        Error,
        Warning
    };

    Task() = default;
    Task(TaskType type, const QString &description,
         const Utils::FileName &file, int line, Core::Id category,
         const Utils::FileName &iconName = Utils::FileName());

    static Task compilerMissingTask();
    static Task buildConfigurationMissingTask();

    bool isNull() const;
    void clear();

    unsigned int taskId = 0;
    TaskType type = Unknown;
    QString description;
    Utils::FileName file;
    int line = -1;
    int movedLine = -1; // contains a line number if the line was moved in the editor
    Core::Id category;
    QIcon icon;

    // Having a QList<QTextLayout::FormatRange> in Task isn't that great
    // It would be cleaner to split up the text into
    // the logical hunks and then assemble them again
    // (That is different consumers of tasks could show them in
    // different ways!)
    // But then again, the wording of the text most likely
    // doesn't work if you split it up, nor are our parsers
    // anywhere near being that good
    QVector<QTextLayout::FormatRange> formats;

private:
    void setMark(TextEditor::TextMark *mark);

    QSharedPointer<TextEditor::TextMark> m_mark;
    static unsigned int s_nextId;

    friend class TaskHub;
};

bool PROJECTEXPLORER_EXPORT operator==(const Task &t1, const Task &t2);
uint PROJECTEXPLORER_EXPORT qHash(const Task &task);

bool PROJECTEXPLORER_EXPORT operator<(const Task &a, const Task &b);

} //namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Task)
