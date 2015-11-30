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

#include "outputparser_test.h"
#include "task.h"

#if defined(WITH_TESTS)

#include <QtTest>

namespace ProjectExplorer {

static inline QByteArray msgFileComparisonFail(const Utils::FileName &f1, const Utils::FileName &f2)
{
    const QString result = QLatin1Char('"') + f1.toUserOutput()
        + QLatin1String("\" != \"") + f2.toUserOutput() + QLatin1Char('"');
    return result.toLocal8Bit();
}

OutputParserTester::OutputParserTester() :
    m_debug(false)
{ }

// test functions:
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
    childParser()->flush();

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
            QVERIFY2(m_receivedTasks.at(i).file == tasks.at(i).file,
                     msgFileComparisonFail(m_receivedTasks.at(i).file, tasks.at(i).file));
            QCOMPARE(m_receivedTasks.at(i).line, tasks.at(i).line);
            QCOMPARE(static_cast<int>(m_receivedTasks.at(i).type), static_cast<int>(tasks.at(i).type));
        }
    }
}

void OutputParserTester::testTaskMangling(const Task &input,
                                          const Task &output)
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
        QVERIFY2(m_receivedTasks.at(0).file == output.file,
                 msgFileComparisonFail(m_receivedTasks.at(0).file, output.file));
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

void OutputParserTester::appendOutputParser(IOutputParser *parser)
{
    Q_ASSERT(!childParser());
    parser->appendOutputParser(new TestTerminator(this));
    IOutputParser::appendOutputParser(parser);
}

void OutputParserTester::outputAdded(const QString &line, BuildStep::OutputFormat format)
{
    Q_UNUSED(format);
    if (!m_receivedOutput.isEmpty())
        m_receivedOutput.append(QLatin1Char('\n'));
    m_receivedOutput.append(line);
}

void OutputParserTester::taskAdded(const Task &task, int linkedLines, int skipLines)
{
    Q_UNUSED(linkedLines);
    Q_UNUSED(skipLines);
    m_receivedTasks.append(task);
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
    QVERIFY(line.endsWith(QLatin1Char('\n')));
    m_tester->m_receivedStdOutChildLine.append(line);
}

void TestTerminator::stdError(const QString &line)
{
    QVERIFY(line.endsWith(QLatin1Char('\n')));
    m_tester->m_receivedStdErrChildLine.append(line);
}

} // namespace ProjectExplorer

#endif
