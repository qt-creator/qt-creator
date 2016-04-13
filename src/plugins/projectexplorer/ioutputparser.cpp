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
    \fn IOutputParser *ProjectExplorer::IOutputParser::takeOutputParserChain()

    Removes the appended outputparser chain from this parser, transferring
           ownership of the parser chain to the caller.
*/

/*!
    \fn IOutputParser *ProjectExplorer::IOutputParser::childParser() const

    Returns the head of this parser's output parser children. IOutputParser
    keeps ownership.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::stdOutput(const QString &line)

   Called once for each line if standard output to parse.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::stdError(const QString &line)

   Called once for each line if standard error to parse.
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
   \fn void ProjectExplorer::IOutputParser::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)

   Subparsers have their addOutput signal connected to this slot.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)

   This function can be overwritten to change the string.
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

IOutputParser::~IOutputParser()
{
    delete m_parser;
}

void IOutputParser::appendOutputParser(IOutputParser *parser)
{
    if (!parser)
        return;
    if (m_parser) {
        m_parser->appendOutputParser(parser);
        return;
    }

    m_parser = parser;
    connect(parser, &IOutputParser::addOutput,
            this, &IOutputParser::outputAdded, Qt::DirectConnection);
    connect(parser, &IOutputParser::addTask,
            this, &IOutputParser::taskAdded, Qt::DirectConnection);
}

IOutputParser *IOutputParser::takeOutputParserChain()
{
    IOutputParser *parser = m_parser;
    disconnect(parser, &IOutputParser::addOutput, this, &IOutputParser::outputAdded);
    disconnect(parser, &IOutputParser::addTask, this, &IOutputParser::taskAdded);
    m_parser = 0;
    return parser;
}

IOutputParser *IOutputParser::childParser() const
{
    return m_parser;
}

void IOutputParser::setChildParser(IOutputParser *parser)
{
    if (m_parser != parser)
        delete m_parser;
    m_parser = parser;
    if (parser) {
        connect(parser, &IOutputParser::addOutput,
                this, &IOutputParser::outputAdded, Qt::DirectConnection);
        connect(parser, &IOutputParser::addTask,
                this, &IOutputParser::taskAdded, Qt::DirectConnection);
    }
}

void IOutputParser::stdOutput(const QString &line)
{
    if (m_parser)
        m_parser->stdOutput(line);
}

void IOutputParser::stdError(const QString &line)
{
    if (m_parser)
        m_parser->stdError(line);
}

void IOutputParser::outputAdded(const QString &string, BuildStep::OutputFormat format)
{
    emit addOutput(string, format);
}

void IOutputParser::taskAdded(const Task &task, int linkedOutputLines, int skipLines)
{
    emit addTask(task, linkedOutputLines, skipLines);
}

void IOutputParser::doFlush()
{ }

bool IOutputParser::hasFatalErrors() const
{
    return m_parser && m_parser->hasFatalErrors();
}

void IOutputParser::setWorkingDirectory(const QString &workingDirectory)
{
    if (m_parser)
        m_parser->setWorkingDirectory(workingDirectory);
}

void IOutputParser::flush()
{
    doFlush();
    if (m_parser)
        m_parser->flush();
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

} // namespace ProjectExplorer
