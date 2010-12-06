/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "ioutputparser.h"
#include "task.h"

#include <utils/qtcassert.h>

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
    QTC_ASSERT(parser, return);
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

bool IOutputParser::hasFatalErrors() const
{
    return false || (m_parser && m_parser->hasFatalErrors());
}

void IOutputParser::setWorkingDirectory(const QString &workingDirectory)
{
    if (m_parser)
        m_parser->setWorkingDirectory(workingDirectory);
}

}
