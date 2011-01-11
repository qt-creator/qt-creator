/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
