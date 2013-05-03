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

#include "ioutputparser.h"
#include "task.h"

/*!
    \class ProjectExplorer::IOutputParser

    \brief Interface for an output parser that emit issues (tasks).

    \sa ProjectExplorer::Task
*/

/*!
    \fn void ProjectExplorer::IOutputParser::appendOutputParser(IOutputParser *parser)

    \brief Append a subparser to this parser, of which IOutputParser will take ownership.
*/

/*!
    \fn IOutputParser *ProjectExplorer::IOutputParser::takeOutputParserChain()

    \brief Remove the appended outputparser chain from this parser, transferring
           ownership of the parser chain to the caller.
*/

/*!
    \fn IOutputParser *ProjectExplorer::IOutputParser::childParser() const

    \brief Return the head of this parsers output parser children, IOutputParser keeps ownership.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::stdOutput(const QString &line)

   \brief Called once for each line if standard output to parse.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::stdError(const QString &line)

   \brief Called once for each line if standard error to parse.
*/

/*!
   \fn bool ProjectExplorer::IOutputParser::hasFatalErrors() const

   \brief This is mainly a symbian specific quirk.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)

   \brief Should be emitted whenever some additional information should be added to the output.

   Note: This is additional information. There is no need to add each line!
*/

/*!
   \fn void ProjectExplorer::IOutputParser::addTask(const ProjectExplorer::Task &task)

   \brief Should be emitted for each task seen in the output.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)

   \brief Subparsers have their addOutput signal connected to this slot.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)

   \brief This method can be overwritten to change the string.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::taskAdded(const ProjectExplorer::Task &task)

   \brief Subparsers have their addTask signal connected to this slot.
   This method can be overwritten to change the task.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::doFlush()

   \brief This method is called whenever a parser is supposed to flush his state.
   Parsers may have state (e.g. because they need to aggregate several lines into one task). This
   method is called when this state needs to be flushed out to be visible.

   doFlush() is called by flush(). flush() is called on childparsers whenever a new task is added.
   It is also called once when all input has been parsed.
*/

namespace ProjectExplorer {

IOutputParser::IOutputParser() : m_parser(0)
{
}

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
    connect(parser, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
            this, SLOT(outputAdded(QString,ProjectExplorer::BuildStep::OutputFormat)), Qt::DirectConnection);
    connect(parser, SIGNAL(addTask(ProjectExplorer::Task)),
            this, SLOT(taskAdded(ProjectExplorer::Task)), Qt::DirectConnection);
}

IOutputParser *IOutputParser::takeOutputParserChain()
{
    IOutputParser *parser = m_parser;
    disconnect(parser, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
               this, SLOT(outputAdded(QString,ProjectExplorer::BuildStep::OutputFormat)));
    disconnect(parser, SIGNAL(addTask(ProjectExplorer::Task)),
               this, SLOT(taskAdded(ProjectExplorer::Task)));
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

void IOutputParser::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)
{
    emit addOutput(string, format);
}

void IOutputParser::taskAdded(const ProjectExplorer::Task &task)
{
    emit addTask(task);
}

void IOutputParser::doFlush()
{ }

bool IOutputParser::hasFatalErrors() const
{
    return false || (m_parser && m_parser->hasFatalErrors());
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

}
