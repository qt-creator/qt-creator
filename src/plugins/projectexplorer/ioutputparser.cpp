// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ioutputparser.h"

#include "task.h"
#include "taskhub.h"

#include <coreplugin/outputwindow.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/algorithm.h>
#include <utils/ansiescapecodehandler.h>

#include <QPlainTextEdit>


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

void OutputTaskParser::setDetailsFormat(Task &task, const LinkSpecs &linkSpecs)
{
    if (task.details.isEmpty())
        return;

    Utils::FormattedText monospacedText(task.details.join('\n'));
    monospacedText.format.setFont(TextEditor::TextEditorSettings::fontSettings().font());
    monospacedText.format.setFontStyleHint(QFont::Monospace);
    const QList<Utils::FormattedText> linkifiedText =
            Utils::OutputFormatter::linkifiedText({monospacedText}, linkSpecs);
    task.formats.clear();
    int offset = task.summary.length() + 1;
    for (const Utils::FormattedText &ft : linkifiedText) {
        task.formats << QTextLayout::FormatRange{offset, int(ft.text.length()), ft.format};
        offset += ft.text.length();
    }
}

void OutputTaskParser::runPostPrintActions(QPlainTextEdit *edit)
{
    int offset = 0;
    if (const auto ow = qobject_cast<Core::OutputWindow *>(edit)) {
        Utils::reverseForeach(taskInfo(), [ow, &offset](const TaskInfo &ti) {
            ow->registerPositionOf(ti.task.taskId, ti.linkedLines, ti.skippedLines, offset);
                offset += ti.linkedLines;
        });
    }

    for (const TaskInfo &t : std::as_const(d->scheduledTasks))
        TaskHub::addTask(t.task);
    d->scheduledTasks.clear();
}

} // namespace ProjectExplorer
