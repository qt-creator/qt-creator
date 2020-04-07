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

#include "outputparser_test.h"
#include "task.h"

#if defined(WITH_TESTS)

#include <QtTest>

namespace ProjectExplorer {

static inline QByteArray msgFileComparisonFail(const Utils::FilePath &f1, const Utils::FilePath &f2)
{
    const QString result = '"' + f1.toUserOutput() + "\" != \"" + f2.toUserOutput() + '"';
    return result.toLocal8Bit();
}

// test functions:
OutputParserTester::OutputParserTester()
{
    connect(this, &IOutputParser::addTask, this, [this](const Task &t) {
        m_receivedTasks.append(t);
    });
}

void OutputParserTester::testParsing(const QString &lines,
                                     Channel inputChannel,
                                     Tasks tasks,
                                     const QString &childStdOutLines,
                                     const QString &childStdErrLines,
                                     const QString &outputLines)
{
    if (!m_terminator) {
        m_terminator = new TestTerminator(this);
        appendOutputParser(m_terminator);
    }
    reset();
    Q_ASSERT(childParser());

    if (inputChannel == STDOUT)
        handleStdout(lines + '\n');
    else
        handleStderr(lines + '\n');
    flush();

    // delete the parser(s) to test
    emit aboutToDeleteParser();
    setChildParser(nullptr);

    QCOMPARE(m_receivedOutput, outputLines);
    QCOMPARE(m_receivedStdErrChildLine, childStdErrLines);
    QCOMPARE(m_receivedStdOutChildLine, childStdOutLines);
    QCOMPARE(m_receivedTasks.size(), tasks.size());
    if (m_receivedTasks.size() == tasks.size()) {
        for (int i = 0; i < tasks.size(); ++i) {
            QCOMPARE(m_receivedTasks.at(i).category, tasks.at(i).category);
            QCOMPARE(m_receivedTasks.at(i).description, tasks.at(i).description);
            QVERIFY2(m_receivedTasks.at(i).file == tasks.at(i).file,
                     msgFileComparisonFail(m_receivedTasks.at(i).file, tasks.at(i).file));
            QCOMPARE(m_receivedTasks.at(i).line, tasks.at(i).line);
            QCOMPARE(static_cast<int>(m_receivedTasks.at(i).type), static_cast<int>(tasks.at(i).type));
        }
    }
}

void OutputParserTester::setDebugEnabled(bool debug)
{
    m_debug = debug;
}

void OutputParserTester::reset()
{
    m_receivedStdErrChildLine.clear();
    m_receivedStdOutChildLine.clear();
    m_receivedTasks.clear();
    m_receivedOutput.clear();
}

TestTerminator::TestTerminator(OutputParserTester *t) :
    m_tester(t)
{ }

void TestTerminator::stdOutput(const QString &line)
{
    QVERIFY(line.endsWith('\n'));
    m_tester->m_receivedStdOutChildLine.append(line);
}

void TestTerminator::stdError(const QString &line)
{
    QVERIFY(line.endsWith('\n'));
    m_tester->m_receivedStdErrChildLine.append(line);
}

} // namespace ProjectExplorer

#endif
