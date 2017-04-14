/****************************************************************************
**
** Copyright (C) 2016 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#include "customparser.h"
#include "task.h"
#include "projectexplorerconstants.h"
#include "buildmanager.h"

#include <utils/qtcassert.h>

#include <QString>

using namespace Utils;
using namespace ProjectExplorer;

bool CustomParserExpression::operator ==(const CustomParserExpression &other) const
{
    return pattern() == other.pattern() && fileNameCap() == other.fileNameCap()
            && lineNumberCap() == other.lineNumberCap() && messageCap() == other.messageCap()
            && channel() == other.channel() && example() == other.example();
}

QString CustomParserExpression::pattern() const
{
    return m_regExp.pattern();
}

void ProjectExplorer::CustomParserExpression::setPattern(const QString &pattern)
{
    m_regExp.setPattern(pattern);
    QTC_CHECK(m_regExp.isValid());
}

CustomParserExpression::CustomParserChannel CustomParserExpression::channel() const
{
    return m_channel;
}

void CustomParserExpression::setChannel(CustomParserExpression::CustomParserChannel channel)
{
    QTC_ASSERT(channel > ParseNoChannel && channel <= ParseBothChannels,
               channel = ParseBothChannels);

    m_channel = channel;
}

QString CustomParserExpression::example() const
{
    return m_example;
}

void CustomParserExpression::setExample(const QString &example)
{
    m_example = example;
}

int CustomParserExpression::messageCap() const
{
    return m_messageCap;
}

void CustomParserExpression::setMessageCap(int messageCap)
{
    m_messageCap = messageCap;
}

int CustomParserExpression::lineNumberCap() const
{
    return m_lineNumberCap;
}

void CustomParserExpression::setLineNumberCap(int lineNumberCap)
{
    m_lineNumberCap = lineNumberCap;
}

int CustomParserExpression::fileNameCap() const
{
    return m_fileNameCap;
}

void CustomParserExpression::setFileNameCap(int fileNameCap)
{
    m_fileNameCap = fileNameCap;
}

bool CustomParserSettings::operator ==(const CustomParserSettings &other) const
{
    return error == other.error && warning == other.warning;
}

CustomParser::CustomParser(const CustomParserSettings &settings)
{
    setObjectName(QLatin1String("CustomParser"));

    setSettings(settings);
}

void CustomParser::stdError(const QString &line)
{
    if (parseLine(line, CustomParserExpression::ParseStdErrChannel))
        return;

    IOutputParser::stdError(line);
}

void CustomParser::stdOutput(const QString &line)
{
    if (parseLine(line, CustomParserExpression::ParseStdOutChannel))
        return;

    IOutputParser::stdOutput(line);
}

void CustomParser::setSettings(const CustomParserSettings &settings)
{
    m_error = settings.error;
    m_warning = settings.warning;
}

Core::Id CustomParser::id()
{
    return Core::Id("ProjectExplorer.OutputParser.Custom");
}

bool CustomParser::hasMatch(const QString &line, CustomParserExpression::CustomParserChannel channel,
                            const CustomParserExpression &expression, Task::TaskType taskType)
{
    if (!(channel & expression.channel()))
        return false;

    if (expression.pattern().isEmpty())
        return false;

    const QRegularExpressionMatch match = expression.match(line);
    if (!match.hasMatch())
        return false;

    const FileName fileName = FileName::fromUserInput(match.captured(expression.fileNameCap()));
    const int lineNumber = match.captured(expression.lineNumberCap()).toInt();
    const QString message = match.captured(expression.messageCap());

    const Task task = Task(taskType, message, fileName, lineNumber, Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    return true;
}

bool CustomParser::parseLine(const QString &rawLine, CustomParserExpression::CustomParserChannel channel)
{
    const QString line = rawLine.trimmed();

    if (hasMatch(line, channel, m_error, Task::Error))
        return true;

    return hasMatch(line, channel, m_warning, Task::Warning);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testCustomOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<CustomParserExpression::CustomParserChannel>("filterErrorChannel");
    QTest::addColumn<CustomParserExpression::CustomParserChannel>("filterWarningChannel");
    QTest::addColumn<QString>("errorPattern");
    QTest::addColumn<int>("errorFileNameCap");
    QTest::addColumn<int>("errorLineNumberCap");
    QTest::addColumn<int>("errorMessageCap");
    QTest::addColumn<QString>("warningPattern");
    QTest::addColumn<int>("warningFileNameCap");
    QTest::addColumn<int>("warningLineNumberCap");
    QTest::addColumn<int>("warningMessageCap");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

    const Core::Id categoryCompile = Constants::TASK_CATEGORY_COMPILE;
    const QString simplePattern = QLatin1String("^([a-z]+\\.[a-z]+):(\\d+): error: ([^\\s].+)$");
    const FileName fileName = FileName::fromUserInput(QLatin1String("main.c"));

    QTest::newRow("empty patterns")
            << QString::fromLatin1("Sometext")
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << QString() << 1 << 2 << 3
            << QString() << 1 << 2 << 3
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString();

    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext")
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << QString() << 1 << 2 << 3
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString();

    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext")
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << QString() << 1 << 2 << 3
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString();

    const QString simpleError = QLatin1String("main.c:9: error: `sfasdf' undeclared (first use this function)");
    const QString simpleErrorPassThrough = simpleError + QLatin1Char('\n');
    const QString message = QLatin1String("`sfasdf' undeclared (first use this function)");

    QTest::newRow("simple error")
            << simpleError
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << QString() << 0 << 0 << 0
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, 9, categoryCompile)
                )
            << QString();

    QTest::newRow("simple error on wrong channel")
            << simpleError
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseStdErrChannel << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << QString() << 0 << 0 << 0
            << simpleErrorPassThrough << QString()
            << QList<Task>()
            << QString();

    QTest::newRow("simple error on other wrong channel")
            << simpleError
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseStdOutChannel << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << QString() << 0 << 0 << 0
            << QString() << simpleErrorPassThrough
            << QList<Task>()
            << QString();

    const QString simpleError2 = QLatin1String("Error: Line 19 in main.c: `sfasdf' undeclared (first use this function)");
    const QString simplePattern2 = QLatin1String("^Error: Line (\\d+) in ([a-z]+\\.[a-z]+): ([^\\s].+)$");
    const int lineNumber2 = 19;

    QTest::newRow("another simple error on stderr")
            << simpleError2
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern2 << 2 << 1 << 3
            << QString() << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, lineNumber2, categoryCompile)
                )
            << QString();

    QTest::newRow("another simple error on stdout")
            << simpleError2
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern2 << 2 << 1 << 3
            << QString() << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, lineNumber2, categoryCompile)
                )
            << QString();

    const QString simpleWarningPattern = QLatin1String("^([a-z]+\\.[a-z]+):(\\d+): warning: ([^\\s].+)$");
    const QString simpleWarning = QLatin1String("main.c:1234: warning: `helloWorld' declared but not used");
    const QString warningMessage = QLatin1String("`helloWorld' declared but not used");

    QTest::newRow("simple warning")
            << simpleWarning
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << QString() << 1 << 2 << 3
            << simpleWarningPattern << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning, warningMessage, fileName, 1234, categoryCompile)
                )
            << QString();

    const QString simpleWarning2 = QLatin1String("Warning: `helloWorld' declared but not used (main.c:19)");
    const QString simpleWarningPassThrough2 = simpleWarning2 + QLatin1Char('\n');
    const QString simpleWarningPattern2 = QLatin1String("^Warning: (.*) \\(([a-z]+\\.[a-z]+):(\\d+)\\)$");

    QTest::newRow("another simple warning on stdout")
            << simpleWarning2
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseStdOutChannel
            << simplePattern2 << 1 << 2 << 3
            << simpleWarningPattern2 << 2 << 3 << 1
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning, warningMessage, fileName, lineNumber2, categoryCompile)
                )
            << QString();

    QTest::newRow("warning on wrong channel")
            << simpleWarning2
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseStdErrChannel
            << QString() << 1 << 2 << 3
            << simpleWarningPattern2 << 2 << 3 << 1
            << simpleWarningPassThrough2 << QString()
            << QList<Task>()
            << QString();

    QTest::newRow("warning on other wrong channel")
            << simpleWarning2
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseStdOutChannel
            << QString() << 1 << 2 << 3
            << simpleWarningPattern2 << 2 << 3 << 1
            << QString() << simpleWarningPassThrough2
            << QList<Task>()
            << QString();

    QTest::newRow("error and *warning*")
            << simpleWarning
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << simpleWarningPattern << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning, warningMessage, fileName, 1234, categoryCompile)
                )
            << QString();

    QTest::newRow("*error* when equal pattern")
            << simpleError
            << OutputParserTester::STDERR
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << simplePattern << 1 << 2 << 3
            << simplePattern << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, 9, categoryCompile)
                )
            << QString();

    const QString unitTestError = QLatin1String("../LedDriver/LedDriverTest.c:63: FAIL: Expected 0x0080 Was 0xffff");
    const FileName unitTestFileName = FileName::fromUserInput(QLatin1String("../LedDriver/LedDriverTest.c"));
    const QString unitTestMessage = QLatin1String("Expected 0x0080 Was 0xffff");
    const QString unitTestPattern = QLatin1String("^([^:]+):(\\d+): FAIL: ([^\\s].+)$");
    const int unitTestLineNumber = 63;

    QTest::newRow("unit test error")
            << unitTestError
            << OutputParserTester::STDOUT
            << CustomParserExpression::ParseBothChannels << CustomParserExpression::ParseBothChannels
            << unitTestPattern << 1 << 2 << 3
            << QString() << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, unitTestMessage, unitTestFileName, unitTestLineNumber, categoryCompile)
                )
            << QString();
}

void ProjectExplorerPlugin::testCustomOutputParsers()
{
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(CustomParserExpression::CustomParserChannel, filterErrorChannel);
    QFETCH(CustomParserExpression::CustomParserChannel, filterWarningChannel);
    QFETCH(QString, errorPattern);
    QFETCH(int,     errorFileNameCap);
    QFETCH(int,     errorLineNumberCap);
    QFETCH(int,     errorMessageCap);
    QFETCH(QString, warningPattern);
    QFETCH(int,     warningFileNameCap);
    QFETCH(int,     warningLineNumberCap);
    QFETCH(int,     warningMessageCap);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, outputLines);

    CustomParserSettings settings;
    settings.error.setPattern(errorPattern);
    settings.error.setFileNameCap(errorFileNameCap);
    settings.error.setLineNumberCap(errorLineNumberCap);
    settings.error.setMessageCap(errorMessageCap);
    settings.error.setChannel(filterErrorChannel);
    settings.warning.setPattern(warningPattern);
    settings.warning.setFileNameCap(warningFileNameCap);
    settings.warning.setLineNumberCap(warningLineNumberCap);
    settings.warning.setMessageCap(warningMessageCap);
    settings.warning.setChannel(filterWarningChannel);

    CustomParser *parser = new CustomParser;
    parser->setSettings(settings);

    OutputParserTester testbench;
    testbench.appendOutputParser(parser);
    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}
#endif
