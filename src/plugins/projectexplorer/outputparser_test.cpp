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
#include "projectexplorer.h"
#include "task.h"
#include "taskhub.h"

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
    connect(TaskHub::instance(), &TaskHub::taskAdded, this, [this](const Task &t) {
        m_receivedTasks.append(t);
    });
}

OutputParserTester::~OutputParserTester()
{
    TaskHub::instance()->disconnect(this);
}

void OutputParserTester::testParsing(const QString &lines,
                                     Channel inputChannel,
                                     Tasks tasks,
                                     const QString &childStdOutLines,
                                     const QString &childStdErrLines,
                                     const QString &outputLines)
{
    const auto terminator = new TestTerminator(this);
    if (!lineParsers().isEmpty())
        terminator->setRedirectionDetector(lineParsers().last());
    addLineParser(terminator);
    reset();

    if (inputChannel == STDOUT)
        appendMessage(lines + '\n', Utils::StdOutFormat);
    else
        appendMessage(lines + '\n', Utils::StdErrFormat);
    flush();

    // delete the parser(s) to test
    emit aboutToDeleteParser();
    setLineParsers({});

    QCOMPARE(m_receivedOutput, outputLines);
    QCOMPARE(m_receivedStdErrChildLine, childStdErrLines);
    QCOMPARE(m_receivedStdOutChildLine, childStdOutLines);
    QCOMPARE(m_receivedTasks.size(), tasks.size());
    if (m_receivedTasks.size() == tasks.size()) {
        for (int i = 0; i < tasks.size(); ++i) {
            QCOMPARE(m_receivedTasks.at(i).category, tasks.at(i).category);
            if (m_receivedTasks.at(i).description() != tasks.at(i).description()) {
                qDebug() << "---" << tasks.at(i).description();
                qDebug() << "+++" << m_receivedTasks.at(i).description();
            }
            QCOMPARE(m_receivedTasks.at(i).description(), tasks.at(i).description());
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

Utils::OutputLineParser::Result TestTerminator::handleLine(const QString &line, Utils::OutputFormat type)
{
    QTC_CHECK(line.endsWith('\n'));
    if (type == Utils::StdOutFormat)
        m_tester->m_receivedStdOutChildLine.append(line);
    else
        m_tester->m_receivedStdErrChildLine.append(line);
    return Status::Done;
}

void ProjectExplorerPlugin::testAnsiFilterOutputParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QString>("outputLines");

    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n");

    QString input = QString::fromLatin1("te") + QChar(27) + QString::fromLatin1("Nst");
    QTest::newRow("ANSI: ESC-N")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("^ignored") + QChar(27) + QLatin1String("\\st");
    QTest::newRow("ANSI: ESC-^ignoredESC-\\")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("]0;ignored") + QChar(7) + QLatin1String("st");
    QTest::newRow("ANSI: window title change")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[Ast");
    QTest::newRow("ANSI: cursor up")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[2Ast");
    QTest::newRow("ANSI: cursor up (with int parameter)")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[2;3Hst");
    QTest::newRow("ANSI: position cursor")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
    input = QString::fromLatin1("te") + QChar(27) + QLatin1String("[31;1mst");
    QTest::newRow("ANSI: bold red")
            << input << OutputParserTester::STDOUT
            << QString::fromLatin1("test\n") << QString();
}

void ProjectExplorerPlugin::testAnsiFilterOutputParser()
{
    OutputParserTester testbench;
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);

    testbench.testParsing(input, inputChannel,
                          Tasks(), childStdOutLines, childStdErrLines,
                          QString());
}

} // namespace ProjectExplorer

#endif
