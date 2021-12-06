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

#include <utils/id.h>
#include <utils/fileutils.h>
#include <utils/porting.h>

#include <QIcon>
#include <QMetaType>
#include <QStringList>
#include <QTextLayout>

namespace TextEditor {
class TextMark;
}

namespace ProjectExplorer {

class TaskHub;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Task
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Task)

public:
    enum TaskType : char {
        Unknown,
        Error,
        Warning
    };

    enum Option : char {
        NoOptions   = 0,
        AddTextMark = 1 << 0,
        FlashWorthy = 1 << 1,
    };
    using Options = char;

    Task() = default;
    Task(TaskType type, const QString &description,
         const Utils::FilePath &file, int line, Utils::Id category,
         const QIcon &icon = QIcon(),
         Options options = AddTextMark | FlashWorthy);

    static Task compilerMissingTask();

    bool isNull() const;
    void clear();
    void setFile(const Utils::FilePath &file);
    QString description() const;
    QIcon icon() const;

    friend PROJECTEXPLORER_EXPORT bool operator==(const Task &t1, const Task &t2);
    friend PROJECTEXPLORER_EXPORT bool operator<(const Task &a, const Task &b);
    friend PROJECTEXPLORER_EXPORT Utils::QHashValueType qHash(const Task &task);

    unsigned int taskId = 0;
    TaskType type = Unknown;
    Options options = AddTextMark | FlashWorthy;
    QString summary;
    QStringList details;
    Utils::FilePath file;
    Utils::FilePaths fileCandidates;
    int line = -1;
    int movedLine = -1; // contains a line number if the line was moved in the editor
    int column = 0;
    Utils::Id category;

    // Having a container of QTextLayout::FormatRange in Task isn't that great
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
    mutable QIcon m_icon;
    static unsigned int s_nextId;

    friend class TaskHub;
};

class PROJECTEXPLORER_EXPORT CompileTask : public Task
{
public:
    CompileTask(TaskType type,
                 const QString &description,
                 const Utils::FilePath &file = {},
                 int line = -1,
                 int column = 0);
};

class PROJECTEXPLORER_EXPORT BuildSystemTask : public Task
{
public:
    BuildSystemTask(TaskType type,
                    const QString &description,
                    const Utils::FilePath &file = {},
                    int line = -1);
};

class PROJECTEXPLORER_EXPORT DeploymentTask : public Task
{
public:
    DeploymentTask(TaskType type, const QString &description);
};

using Tasks = QVector<Task>;

PROJECTEXPLORER_EXPORT QString toHtml(const Tasks &issues);
PROJECTEXPLORER_EXPORT bool containsType(const Tasks &issues, Task::TaskType);

} //namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Task)
