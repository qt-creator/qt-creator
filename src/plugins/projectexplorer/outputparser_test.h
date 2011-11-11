/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef OUTPUTPARSER_TESTER_H
#define OUTPUTPARSER_TESTER_H

#if defined(WITH_TESTS)

#include "projectexplorer_export.h"
#include "metatypedeclarations.h"
#include "ioutputparser.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT OutputParserTester : public IOutputParser
{
    Q_OBJECT

public:
    enum Channel {
        STDOUT,
        STDERR
    };

    OutputParserTester();
    ~OutputParserTester();

    // test methods:
    void testParsing(const QString &lines, Channel inputChannel,
                     QList<Task> tasks,
                     const QString &childStdOutLines,
                     const QString &childStdErrLines,
                     const QString &outputLines);
    void testTaskMangling(const Task input,
                          const Task output);
    void testOutputMangling(const QString &input,
                            const QString &output);

    void setDebugEnabled(bool);

    // IOutputParser interface:
    void stdOutput(const QString &line);
    void stdError(const QString &line);

    void appendOutputParser(IOutputParser *parser);

signals:
    void aboutToDeleteParser();

private slots:
    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded(const ProjectExplorer::Task &task);

private:
    void reset();

    bool m_debug;

    QString m_receivedStdErrChildLine;
    QString m_receivedStdOutChildLine;
    QList<Task> m_receivedTasks;
    QString m_receivedOutput;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::OutputParserTester::Channel)

#endif

#endif // OUTPUTPARSER_TESTER_H
