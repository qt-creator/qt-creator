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

#if defined(WITH_TESTS)

#include "projectexplorer_export.h"

#include "ioutputparser.h"
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

    void appendOutputParser(IOutputParser *parser) override;

signals:
    void aboutToDeleteParser();

private:
    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format) override;
    void taskAdded(const ProjectExplorer::Task &task, int linkedLines, int skipLines) override;

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

    void stdOutput(const QString &line) override;
    void stdError(const QString &line) override;

private:
    OutputParserTester *m_tester;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::OutputParserTester::Channel)

#endif
