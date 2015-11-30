/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OUTPUTPARSER_TESTER_H
#define OUTPUTPARSER_TESTER_H

#if defined(WITH_TESTS)

#include "projectexplorer_export.h"

#include "ioutputparser.h"
#include "metatypedeclarations.h"
#include "task.h"

namespace ProjectExplorer {

class TestTerminator;

class PROJECTEXPLORER_EXPORT OutputParserTester : public IOutputParser
{
    Q_OBJECT

public:
    enum Channel {
        STDOUT,
        STDERR
    };

    OutputParserTester();

    // test functions:
    void testParsing(const QString &lines, Channel inputChannel,
                     QList<Task> tasks,
                     const QString &childStdOutLines,
                     const QString &childStdErrLines,
                     const QString &outputLines);
    void testTaskMangling(const Task &input,
                          const Task &output);
    void testOutputMangling(const QString &input,
                            const QString &output);

    void setDebugEnabled(bool);

    void appendOutputParser(IOutputParser *parser);

signals:
    void aboutToDeleteParser();

private slots:
    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded(const ProjectExplorer::Task &task, int linkedLines, int skipLines);

private:
    void reset();

    bool m_debug;

    QString m_receivedStdErrChildLine;
    QString m_receivedStdOutChildLine;
    QList<Task> m_receivedTasks;
    QString m_receivedOutput;

    friend class TestTerminator;
};

class TestTerminator : public IOutputParser
{
    Q_OBJECT

public:
    TestTerminator(OutputParserTester *t);

    void stdOutput(const QString &line);
    void stdError(const QString &line);

private:
    OutputParserTester *m_tester;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::OutputParserTester::Channel)

#endif

#endif // OUTPUTPARSER_TESTER_H
