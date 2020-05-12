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

#include "ioutputparser.h"

#include "task.h"
#include "taskhub.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>


/*!
    \class ProjectExplorer::OutputTaskParser

    \brief The OutputTaskParser class provides an interface for an output parser
    that emits issues (tasks).

    \sa ProjectExplorer::Task
*/

/*!
   \fn ProjectExplorer::OutputTaskParser::Status ProjectExplorer::OutputTaskParser::handleLine(const QString &line, Utils::OutputFormat type)

   Called once for each line of standard output or standard error to parse.
*/

/*!
   \fn bool ProjectExplorer::OutputTaskParser::hasFatalErrors() const

   This is mainly a Symbian specific quirk.
*/

/*!
   \fn void ProjectExplorer::OutputTaskParser::addTask(const ProjectExplorer::Task &task)

   Should be emitted for each task seen in the output.
*/

/*!
   \fn void ProjectExplorer::OutputTaskParser::flush()

   Instructs a parser to flush its state.
   Parsers may have state (for example, because they need to aggregate several
   lines into one task). This
   function is called when this state needs to be flushed out to be visible.
*/

namespace ProjectExplorer {

class OutputTaskParser::Private
{
public:
    QList<TaskInfo> scheduledTasks;
};

OutputTaskParser::OutputTaskParser() : d(new Private) { }

OutputTaskParser::~OutputTaskParser() { delete d; }

const QList<OutputTaskParser::TaskInfo> OutputTaskParser::taskInfo() const
{
    return d->scheduledTasks;
}

void OutputTaskParser::scheduleTask(const Task &task, int outputLines, int skippedLines)
{
    TaskInfo ts(task, outputLines, skippedLines);
    if (ts.task.type == Task::Error && demoteErrorsToWarnings())
        ts.task.type = Task::Warning;
    d->scheduledTasks << ts;
    QTC_CHECK(d->scheduledTasks.size() <= 2);
}

void OutputTaskParser::setMonospacedDetailsFormat(Task &task)
{
    if (task.details.isEmpty())
        return;
    QTextLayout::FormatRange fr;
    fr.start = task.summary.length() + 1;
    fr.length = task.details.join('\n').length();
    fr.format.setFont(TextEditor::TextEditorSettings::fontSettings().font());
    fr.format.setFontStyleHint(QFont::Monospace);
    task.formats = {fr};
}

void OutputTaskParser::runPostPrintActions()
{
    for (const TaskInfo &t : qAsConst(d->scheduledTasks))
        TaskHub::addTask(t.task);
    d->scheduledTasks.clear();
}

} // namespace ProjectExplorer
