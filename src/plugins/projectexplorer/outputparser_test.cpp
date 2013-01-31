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

#include "outputparser_test.h"
#include "task.h"

#if defined(WITH_TESTS)

#include <QtTest>

using namespace ProjectExplorer;

OutputParserTester::OutputParserTester() :
        m_debug(false)
{
}

OutputParserTester::~OutputParserTester()
{
    if (childParser())
        childParser()->takeOutputParserChain();
}

// test methods:
void OutputParserTester::testParsing(const QString &lines,
                                     Channel inputChannel,
                                     QList<Task> tasks,
                                     const QString &childStdOutLines,
                                     const QString &childStdErrLines,
                                     const QString &outputLines)
{
    reset();
    Q_ASSERT(childParser());

    QStringList inputLines = lines.split(QLatin1Char('\n'));
    foreach (const QString &input, inputLines) {
        if (inputChannel == STDOUT)
            childParser()->stdOutput(input + QLatin1Char('\n'));
        else
            childParser()->stdError(input + QLatin1Char('\n'));
    }
     // first disconnect ourselves from the end of the parser chain again
    IOutputParser * parser = this;
    while ( (parser = parser->childParser()) ) {
        if (parser->childParser() == this) {
            childParser()->takeOutputParserChain();
            break;
        }
    }
    parser = 0;
    emit aboutToDeleteParser();

    // then delete the parser(s) to test
    setChildParser(0);

    QCOMPARE(m_receivedOutput, outputLines);
    QCOMPARE(m_receivedStdErrChildLine, childStdErrLines);
    QCOMPARE(m_receivedStdOutChildLine, childStdOutLines);
    QCOMPARE(m_receivedTasks.size(), tasks.size());
    if (m_receivedTasks.size() == tasks.size()) {
        for (int i = 0; i < tasks.size(); ++i) {
            QCOMPARE(m_receivedTasks.at(i).category, tasks.at(i).category);
            QCOMPARE(m_receivedTasks.at(i).description, tasks.at(i).description);
            QCOMPARE(m_receivedTasks.at(i).file, tasks.at(i).file);
            QCOMPARE(m_receivedTasks.at(i).line, tasks.at(i).line);
            QCOMPARE(static_cast<int>(m_receivedTasks.at(i).type), static_cast<int>(tasks.at(i).type));
        }
    }
}

void OutputParserTester::testTaskMangling(const Task input,
                                          const Task output)
{
    reset();
    childParser()->taskAdded(input);

    QVERIFY(m_receivedOutput.isNull());
    QVERIFY(m_receivedStdErrChildLine.isNull());
    QVERIFY(m_receivedStdOutChildLine.isNull());
    QVERIFY(m_receivedTasks.size() == 1);
    if (m_receivedTasks.size() == 1) {
        QCOMPARE(m_receivedTasks.at(0).category, output.category);
        QCOMPARE(m_receivedTasks.at(0).description, output.description);
        QCOMPARE(m_receivedTasks.at(0).file, output.file);
        QCOMPARE(m_receivedTasks.at(0).line, output.line);
        QCOMPARE(m_receivedTasks.at(0).type, output.type);
    }
}

void OutputParserTester::testOutputMangling(const QString &input,
                                            const QString &output)
{
    reset();

    childParser()->outputAdded(input, BuildStep::NormalOutput);

    QCOMPARE(m_receivedOutput, output);
    QVERIFY(m_receivedStdErrChildLine.isNull());
    QVERIFY(m_receivedStdOutChildLine.isNull());
    QVERIFY(m_receivedTasks.isEmpty());
}

void OutputParserTester::setDebugEnabled(bool debug)
{
    m_debug = debug;
}

void OutputParserTester::stdOutput(const QString &line)
{
    QVERIFY(line.endsWith(QLatin1Char('\n')));
    m_receivedStdOutChildLine.append(line);
}

void OutputParserTester::stdError(const QString &line)
{
    QVERIFY(line.endsWith(QLatin1Char('\n')));
    m_receivedStdErrChildLine.append(line);
}

void OutputParserTester::appendOutputParser(IOutputParser *parser)
{
    Q_ASSERT(!childParser());

    IOutputParser::appendOutputParser(parser);
    parser->appendOutputParser(this);
}

void OutputParserTester::outputAdded(const QString &line, ProjectExplorer::BuildStep::OutputFormat format)
{
    Q_UNUSED(format);
    if (!m_receivedOutput.isEmpty())
        m_receivedOutput.append(QLatin1Char('\n'));
    m_receivedOutput.append(line);
}

void OutputParserTester::taskAdded(const ProjectExplorer::Task &task)
{
    m_receivedTasks.append(task);
}

void OutputParserTester::reset()
{
    m_receivedStdErrChildLine.clear();
    m_receivedStdOutChildLine.clear();
    m_receivedTasks.clear();
    m_receivedOutput.clear();
}

#endif
