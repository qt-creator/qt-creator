/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "sdccparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// Helpers:

static Task::TaskType taskType(const QString &msgType)
{
    if (msgType == "warning" || msgType == "Warning") {
        return Task::TaskType::Warning;
    } else if (msgType == "error" || msgType == "Error"
               || msgType == "syntax error") {
        return Task::TaskType::Error;
    }
    return Task::TaskType::Unknown;
}

// SdccParser

SdccParser::SdccParser()
{
    setObjectName("SdccParser");
}

Utils::Id SdccParser::id()
{
    return "BareMetal.OutputParser.Sdcc";
}

void SdccParser::newTask(const Task &task)
{
    flush();
    m_lastTask = task;
    m_lines = 1;
}

void SdccParser::amendDescription(const QString &desc)
{
    m_lastTask.details.append(desc);
    ++m_lines;
}

OutputLineParser::Result SdccParser::handleLine(const QString &line, OutputFormat type)
{
    if (type == StdOutFormat)
        return Status::NotHandled;

    const QString lne = rightTrimmed(line);

    QRegularExpression re;
    QRegularExpressionMatch match;

    re.setPattern("^(.+\\.\\S+):(\\d+): (warning|error) (\\d+): (.+)$");
    match = re.match(lne);
    if (match.hasMatch()) {
        enum CaptureIndex { FilePathIndex = 1, LineNumberIndex,
                            MessageTypeIndex, MessageCodeIndex, MessageTextIndex };
        const Utils::FilePath fileName = Utils::FilePath::fromUserInput(
                    match.captured(FilePathIndex));
        const int lineno = match.captured(LineNumberIndex).toInt();
        const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
        const QString descr = match.captured(MessageTextIndex);
        newTask(CompileTask(type, descr, absoluteFilePath(fileName), lineno));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, match,
                                       FilePathIndex);
        return {Status::InProgress, linkSpecs};
    }

    re.setPattern("^(.+\\.\\S+):(\\d+): (Error|error|syntax error): (.+)$");
    match = re.match(lne);
    if (match.hasMatch()) {
        enum CaptureIndex { FilePathIndex = 1, LineNumberIndex,
                            MessageTypeIndex, MessageTextIndex };
        const Utils::FilePath fileName = Utils::FilePath::fromUserInput(
                    match.captured(FilePathIndex));
        const int lineno = match.captured(LineNumberIndex).toInt();
        const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
        const QString descr = match.captured(MessageTextIndex);
        newTask(CompileTask(type, descr, absoluteFilePath(fileName), lineno));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, match,
                                       FilePathIndex);
        return {Status::InProgress, linkSpecs};
    }

    re.setPattern("^at (\\d+): (warning|error) \\d+: (.+)$");
    match = re.match(lne);
    if (match.hasMatch()) {
        enum CaptureIndex { MessageCodeIndex = 1, MessageTypeIndex, MessageTextIndex };
        const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
        const QString descr = match.captured(MessageTextIndex);
        newTask(CompileTask(type, descr));
        return Status::InProgress;
    }

    re.setPattern("^\\?ASlink-(Warning|Error)-(.+)$");
    match = re.match(lne);
    if (match.hasMatch()) {
        enum CaptureIndex { MessageTypeIndex = 1, MessageTextIndex };
        const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
        const QString descr = match.captured(MessageTextIndex);
        newTask(CompileTask(type, descr));
        return Status::InProgress;
    }

    if (!m_lastTask.isNull()) {
        amendDescription(lne);
        return Status::InProgress;
    }

    flush();
    return Status::NotHandled;
}

void SdccParser::flush()
{
    if (m_lastTask.isNull())
        return;

    setDetailsFormat(m_lastTask);
    Task t = m_lastTask;
    m_lastTask.clear();
    scheduleTask(t, m_lines, 1);
    m_lines = 0;
}

} // namespace Internal
} // namespace BareMetal

// Unit tests:

#ifdef WITH_TESTS
#include "baremetalplugin.h"
#include <projectexplorer/outputparser_test.h>
#include <QTest>

namespace BareMetal {
namespace Internal {

void BareMetalPlugin::testSdccOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<Tasks >("tasks");
    QTest::addColumn<QString>("outputLines");

    QTest::newRow("pass-through stdout")
            << "Sometext" << OutputParserTester::STDOUT
            << "Sometext\n" << QString()
            << Tasks()
            << QString();
    QTest::newRow("pass-through stderr")
            << "Sometext" << OutputParserTester::STDERR
            << QString() << "Sometext\n"
            << Tasks()
            << QString();

    // Compiler messages.

    QTest::newRow("Assembler error")
            << QString::fromLatin1("c:\\foo\\main.c:63: Error: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler single line warning")
            << QString::fromLatin1("c:\\foo\\main.c:63: warning 123: Some warning")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "Some warning",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler multi line warning")
            << QString::fromLatin1("c:\\foo\\main.c:63: warning 123: Some warning\n"
                                   "details #1\n"
                                   "  details #2")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "Some warning\n"
                                       "details #1\n"
                                       "  details #2",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler simple single line error")
            << QString::fromLatin1("c:\\foo\\main.c:63: error: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler single line error")
            << QString::fromLatin1("c:\\foo\\main.c:63: error 123: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler multi line error")
            << QString::fromLatin1("c:\\foo\\main.c:63: error 123: Some error\n"
                                   "details #1\n"
                                   "  details #2")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Some error\n"
                                       "details #1\n"
                                       "  details #2",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler syntax error")
            << QString::fromLatin1("c:\\foo\\main.c:63: syntax error: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Compiler bad option error")
            << QString::fromLatin1("at 1: error 123: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Some error"))
            << QString();

    QTest::newRow("Compiler bad option warning")
            << QString::fromLatin1("at 1: warning 123: Some warning")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "Some warning"))
            << QString();

    QTest::newRow("Linker warning")
            << QString::fromLatin1("?ASlink-Warning-Couldn't find library 'foo.lib'")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "Couldn't find library 'foo.lib'"))
            << QString();

    QTest::newRow("Linker error")
            << QString::fromLatin1("?ASlink-Error-<cannot open> : \"foo.rel\"")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "<cannot open> : \"foo.rel\""))
            << QString();
}

void BareMetalPlugin::testSdccOutputParsers()
{
    OutputParserTester testbench;
    testbench.addLineParser(new SdccParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}

} // namespace Internal
} // namespace BareMetal

#endif // WITH_TESTS
