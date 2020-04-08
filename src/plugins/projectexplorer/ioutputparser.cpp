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

#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>

/*!
    \class ProjectExplorer::IOutputParser

    \brief The IOutputParser class provides an interface for an output parser
    that emits issues (tasks).

    \sa ProjectExplorer::Task
*/

/*!
    \fn void ProjectExplorer::IOutputParser::appendOutputParser(IOutputParser *parser)

    Appends a subparser to this parser, of which IOutputParser will take
    ownership.
*/

/*!
    \fn IOutputParser *ProjectExplorer::IOutputParser::childParser() const

    Returns the head of this parser's output parser children. IOutputParser
    keeps ownership.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::handleLine(const QString &line, Utils::OutputFormat type)

   Called once for each line of standard output or standard error to parse.
*/

/*!
   \fn bool ProjectExplorer::IOutputParser::hasFatalErrors() const

   This is mainly a Symbian specific quirk.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)

   Should be emitted whenever some additional information should be added to the
   output.

   \note This is additional information. There is no need to add each line.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::addTask(const ProjectExplorer::Task &task)

   Should be emitted for each task seen in the output.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::taskAdded(const ProjectExplorer::Task &task)

   Subparsers have their addTask signal connected to this slot.
   This function can be overwritten to change the task.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::doFlush()

   Instructs a parser to flush its state.
   Parsers may have state (for example, because they need to aggregate several
   lines into one task). This
   function is called when this state needs to be flushed out to be visible.

   doFlush() is called by flush(). flush() is called on child parsers
   whenever a new task is added.
   It is also called once when all input has been parsed.
*/

namespace ProjectExplorer {

class IOutputParser::OutputChannelState
{
public:
    using LineHandler = void (IOutputParser::*)(const QString &line);

    OutputChannelState(IOutputParser *parser, Utils::OutputFormat type)
        : parser(parser), type(type) {}

    void handleData(const QString &newData)
    {
        pendingData += newData;
        pendingData = Utils::SynchronousProcess::normalizeNewlines(pendingData);
        while (true) {
            const int eolPos = pendingData.indexOf('\n');
            if (eolPos == -1)
                break;
            const QString line = pendingData.left(eolPos + 1);
            pendingData.remove(0, eolPos + 1);
            parser->handleLine(parser->filteredLine(line), type);
        }
    }

    void flush()
    {
        if (!pendingData.isEmpty()) {
            parser->handleLine(parser->filteredLine(pendingData), type);
            pendingData.clear();
        }
    }

    IOutputParser * const parser;
    const Utils::OutputFormat type;
    QString pendingData;
};

class IOutputParser::IOutputParserPrivate
{
public:
    IOutputParserPrivate(IOutputParser *parser)
        : stdoutState(parser, Utils::StdOutFormat),
          stderrState(parser, Utils::StdErrFormat)
    {}

    IOutputParser *childParser = nullptr;
    QList<Filter> filters;
    Utils::FilePaths searchDirs;
    OutputChannelState stdoutState;
    OutputChannelState stderrState;
    bool skipFileExistsCheck = false;
};

IOutputParser::IOutputParser() : d(new IOutputParserPrivate(this))
{
}

IOutputParser::~IOutputParser()
{
    delete d->childParser;
    delete d;
}

void IOutputParser::handleStdout(const QString &data)
{
    d->stdoutState.handleData(data);
}

void IOutputParser::handleStderr(const QString &data)
{
    d->stderrState.handleData(data);
}

void IOutputParser::appendOutputParser(IOutputParser *parser)
{
    if (!parser)
        return;
    if (d->childParser) {
        d->childParser->appendOutputParser(parser);
        return;
    }

    d->childParser = parser;
    connect(parser, &IOutputParser::addTask, this, &IOutputParser::addTask);
}

IOutputParser *IOutputParser::childParser() const
{
    return d->childParser;
}

void IOutputParser::setChildParser(IOutputParser *parser)
{
    if (d->childParser != parser)
        delete d->childParser;
    d->childParser = parser;
    if (parser)
        connect(parser, &IOutputParser::addTask, this, &IOutputParser::addTask);
}

void IOutputParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    if (d->childParser)
        d->childParser->handleLine(line, type);
}

void IOutputParser::skipFileExistsCheck()
{
    d->skipFileExistsCheck = true;
}

void IOutputParser::doFlush() { }

QString IOutputParser::filteredLine(const QString &line) const
{
    QString l = line;
    for (const IOutputParser::Filter &f : qAsConst(d->filters))
        l = f(l);
    return l;
}

bool IOutputParser::hasFatalErrors() const
{
    return d->childParser && d->childParser->hasFatalErrors();
}

void IOutputParser::flush()
{
    flushTasks();
    d->stdoutState.flush();
    d->stderrState.flush();
    flushTasks();
}

void IOutputParser::flushTasks()
{
    doFlush();
    if (d->childParser)
        d->childParser->flushTasks();
}

QString IOutputParser::rightTrimmed(const QString &in)
{
    int pos = in.length();
    for (; pos > 0; --pos) {
        if (!in.at(pos - 1).isSpace())
            break;
    }
    return in.mid(0, pos);
}

void IOutputParser::addFilter(const Filter &filter)
{
    d->filters << filter;
}

void IOutputParser::addSearchDir(const Utils::FilePath &dir)
{
    d->searchDirs << dir;
    if (d->childParser)
        d->childParser->addSearchDir(dir);
}

void IOutputParser::dropSearchDir(const Utils::FilePath &dir)
{
    const int idx = d->searchDirs.lastIndexOf(dir);
    QTC_ASSERT(idx != -1, return);
    d->searchDirs.removeAt(idx);
    if (d->childParser)
        d->childParser->dropSearchDir(dir);
}

const Utils::FilePaths IOutputParser::searchDirectories() const
{
    return d->searchDirs;
}

Utils::FilePath IOutputParser::absoluteFilePath(const Utils::FilePath &filePath)
{
    if (filePath.isEmpty() || filePath.toFileInfo().isAbsolute())
        return filePath;
    Utils::FilePaths candidates;
    for (const Utils::FilePath &dir : searchDirectories()) {
        const Utils::FilePath candidate = dir.pathAppended(filePath.toString());
        if (candidate.exists() || d->skipFileExistsCheck)
            candidates << candidate;
    }
    if (candidates.count() == 1)
        return Utils::FilePath::fromString(QDir::cleanPath(candidates.first().toString()));
    return filePath;
}

} // namespace ProjectExplorer
