/****************************************************************************
**
** Copyright (C) 2015 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#include "customparser.h"
#include "task.h"
#include "projectexplorerconstants.h"
#include "buildmanager.h"

#include <utils/qtcassert.h>

#include <QString>

using namespace Utils;
using namespace ProjectExplorer;

CustomParserSettings::CustomParserSettings() :
    fileNameCap(1),
    lineNumberCap(2),
    messageCap(3)
{ }

bool CustomParserSettings::operator ==(const CustomParserSettings &other) const
{
    return errorPattern == other.errorPattern && fileNameCap == other.fileNameCap
            && lineNumberCap == other.lineNumberCap && messageCap == other.messageCap;
}

CustomParser::CustomParser(const CustomParserSettings &settings) :
    m_parserChannels(ParseBothChannels)
{
    setObjectName(QLatin1String("CustomParser"));

    setSettings(settings);
}

CustomParser::~CustomParser()
{
}

void CustomParser::setErrorPattern(const QString &errorPattern)
{
    m_errorRegExp.setPattern(errorPattern);
    m_errorRegExp.setMinimal(true);
    QTC_CHECK(m_errorRegExp.isValid());
}

QString CustomParser::errorPattern() const
{
    return m_errorRegExp.pattern();
}

int CustomParser::lineNumberCap() const
{
    return m_lineNumberCap;
}

void CustomParser::setLineNumberCap(int lineNumberCap)
{
    m_lineNumberCap = lineNumberCap;
}

int CustomParser::fileNameCap() const
{
    return m_fileNameCap;
}

void CustomParser::setFileNameCap(int fileNameCap)
{
    m_fileNameCap = fileNameCap;
}

int CustomParser::messageCap() const
{
    return m_messageCap;
}

void CustomParser::setMessageCap(int messageCap)
{
    m_messageCap = messageCap;
}

void CustomParser::stdError(const QString &line)
{
    if (m_parserChannels & ParseStdErrChannel)
        if (parseLine(line))
            return;

    IOutputParser::stdError(line);
}

void CustomParser::stdOutput(const QString &line)
{
    if (m_parserChannels & ParseStdOutChannel)
        if (parseLine(line))
            return;

    IOutputParser::stdOutput(line);
}

void CustomParser::setSettings(const CustomParserSettings &settings)
{
    setErrorPattern(settings.errorPattern);
    setFileNameCap(settings.fileNameCap);
    setLineNumberCap(settings.lineNumberCap);
    setMessageCap(settings.messageCap);
}

bool CustomParser::parseLine(const QString &rawLine)
{
    if (m_errorRegExp.isEmpty())
        return false;

    if (m_errorRegExp.indexIn(rawLine.trimmed()) == -1)
        return false;

    const FileName fileName = FileName::fromUserInput(m_errorRegExp.cap(m_fileNameCap));
    const int lineNumber = m_errorRegExp.cap(m_lineNumberCap).toInt();
    const QString message = m_errorRegExp.cap(m_messageCap);

    Task task = Task(Task::Error, message, fileName, lineNumber, Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    return true;
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "metatypedeclarations.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testCustomOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("fileNameCap");
    QTest::addColumn<int>("lineNumberCap");
    QTest::addColumn<int>("messageCap");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

    const Core::Id categoryCompile = Constants::TASK_CATEGORY_COMPILE;
    const QString simplePattern = QLatin1String("^([a-z]+\\.[a-z]+):(\\d+): error: ([^\\s].+)$");
    const FileName fileName = FileName::fromUserInput(QLatin1String("main.c"));

    QTest::newRow("empty pattern")
            << QString::fromLatin1("Sometext")
            << OutputParserTester::STDOUT
            << QString::fromLatin1("")
            << 1 << 2 << 3
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString();

    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext")
            << OutputParserTester::STDOUT
            << simplePattern
            << 1 << 2 << 3
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString();

    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext")
            << OutputParserTester::STDERR
            << simplePattern
            << 1 << 2 << 3
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString();

    const QString simpleError = QLatin1String("main.c:9: error: `sfasdf' undeclared (first use this function)");
    const QString message = QLatin1String("`sfasdf' undeclared (first use this function)");

    QTest::newRow("simple error")
            << simpleError
            << OutputParserTester::STDERR
            << simplePattern
            << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, 9, categoryCompile)
                )
            << QString();

    const QString simpleError2 = QLatin1String("Error: main.c:19: `sfasdf' undeclared (first use this function)");
    const QString simplePattern2 = QLatin1String("^Error: ([a-z]+\\.[a-z]+):(\\d+): ([^\\s].+)$");
    const int lineNumber2 = 19;

    QTest::newRow("another simple error on stderr")
            << simpleError2
            << OutputParserTester::STDERR
            << simplePattern2
            << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, lineNumber2, categoryCompile)
                )
            << QString();

    QTest::newRow("another simple error on stdout")
            << simpleError2
            << OutputParserTester::STDOUT
            << simplePattern2
            << 1 << 2 << 3
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error, message, fileName, lineNumber2, categoryCompile)
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
            << unitTestPattern
            << 1 << 2 << 3
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
    QFETCH(QString, pattern);
    QFETCH(int, fileNameCap);
    QFETCH(int, lineNumberCap);
    QFETCH(int, messageCap);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, outputLines);

    CustomParser *parser = new CustomParser;
    parser->setErrorPattern(pattern);
    parser->setFileNameCap(fileNameCap);
    parser->setLineNumberCap(lineNumberCap);
    parser->setMessageCap(messageCap);

    OutputParserTester testbench;
    testbench.appendOutputParser(parser);
    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}
#endif
