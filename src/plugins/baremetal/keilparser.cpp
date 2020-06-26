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

#include "keilparser.h"

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
    if (msgType == "Warning" || msgType == "WARNING") {
        return Task::TaskType::Warning;
    } else if (msgType == "Error" || msgType == "ERROR"
               || msgType == "Fatal error" || msgType == "FATAL ERROR") {
        return Task::TaskType::Error;
    }
    return Task::TaskType::Unknown;
}

// KeilParser

KeilParser::KeilParser()
{
    setObjectName("KeilParser");
}

Utils::Id KeilParser::id()
{
    return "BareMetal.OutputParser.Keil";
}

void KeilParser::newTask(const Task &task)
{
    flush();
    m_lastTask = task;
    m_lines = 1;
}

// ARM compiler specific parsers.

OutputLineParser::Result KeilParser::parseArmWarningOrErrorDetailsMessage(const QString &lne)
{
    const QRegularExpression re("^\"(.+)\", line (\\d+).*:\\s+(Warning|Error):(\\s+|.+)([#|L].+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return Status::NotHandled;
    enum CaptureIndex { FilePathIndex = 1, LineNumberIndex,
                        MessageTypeIndex, MessageNoteIndex, DescriptionIndex };
    const Utils::FilePath fileName = Utils::FilePath::fromUserInput(
                match.captured(FilePathIndex));
    const int lineno = match.captured(LineNumberIndex).toInt();
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const QString descr = match.captured(DescriptionIndex);
    newTask(CompileTask(type, descr, absoluteFilePath(fileName), lineno));
    LinkSpecs linkSpecs;
    addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, match,
                                   FilePathIndex);
    return {Status::InProgress, linkSpecs};
}

bool KeilParser::parseArmErrorOrFatalErorrMessage(const QString &lne)
{
    const QRegularExpression re("^(Error|Fatal error):\\s(.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return false;
    enum CaptureIndex { MessageTypeIndex = 1, DescriptionIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const QString descr = match.captured(DescriptionIndex);
    newTask(CompileTask(type, descr));
    return true;
}

// MCS51 compiler specific parsers.

OutputLineParser::Result KeilParser::parseMcs51WarningOrErrorDetailsMessage1(const QString &lne)
{
    const QRegularExpression re("^\\*{3} (WARNING|ERROR) (\\w+) IN LINE (\\d+) OF (.+\\.\\S+): (.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return Status::NotHandled;
    enum CaptureIndex { MessageTypeIndex = 1, MessageCodeIndex, LineNumberIndex,
                        FilePathIndex, MessageTextIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const int lineno = match.captured(LineNumberIndex).toInt();
    const Utils::FilePath fileName = Utils::FilePath::fromUserInput(
                match.captured(FilePathIndex));
    const QString descr = QString("%1: %2").arg(match.captured(MessageCodeIndex),
                                                match.captured(MessageTextIndex));
    newTask(CompileTask(type, descr, absoluteFilePath(fileName), lineno));
    LinkSpecs linkSpecs;
    addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, match,
                                   FilePathIndex);
    return {Status::InProgress, linkSpecs};
}

OutputLineParser::Result KeilParser::parseMcs51WarningOrErrorDetailsMessage2(const QString &lne)
{
    const QRegularExpression re("^\\*{3} (WARNING|ERROR) (#\\w+) IN (\\d+) \\((.+), LINE \\d+\\): (.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return Status::NotHandled;
    enum CaptureIndex { MessageTypeIndex = 1, MessageCodeIndex, LineNumberIndex,
                        FilePathIndex, MessageTextIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const int lineno = match.captured(LineNumberIndex).toInt();
    const Utils::FilePath fileName = Utils::FilePath::fromUserInput(
                match.captured(FilePathIndex));
    const QString descr = QString("%1: %2").arg(match.captured(MessageCodeIndex),
                                                match.captured(MessageTextIndex));
    newTask(CompileTask(type, descr, absoluteFilePath(fileName), lineno));
    LinkSpecs linkSpecs;
    addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, match,
                                   FilePathIndex);
    return {Status::InProgress, linkSpecs};
}

bool KeilParser::parseMcs51WarningOrFatalErrorMessage(const QString &lne)
{
    const QRegularExpression re("^\\*{3} (WARNING|FATAL ERROR) (.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return false;
    enum CaptureIndex { MessageTypeIndex = 1, MessageDescriptionIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const QString descr = match.captured(MessageDescriptionIndex);
    newTask(CompileTask(type, descr));
    return true;
}

bool KeilParser::parseMcs51FatalErrorMessage2(const QString &lne)
{
    const QRegularExpression re("^(A|C)51 FATAL[ |-]ERROR");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return false;
    const QString key = match.captured(1);
    QString descr;
    if (key == 'A')
        descr = "Assembler fatal error";
    else if (key == 'C')
        descr = "Compiler fatal error";
    newTask(CompileTask(Task::TaskType::Error, descr));
    return true;
}

static bool hasDetailsEntry(const QString &trimmedLine)
{
    const QRegularExpression re("^([0-9A-F]{4})");
    const QRegularExpressionMatch match = re.match(trimmedLine);
    return match.hasMatch();
}

static bool hasDetailsPointer(const QString &trimmedLine)
{
    if (!trimmedLine.startsWith("*** "))
        return false;
    if (!trimmedLine.endsWith('^'))
        return false;
    return trimmedLine.contains('_');
}

OutputLineParser::Result KeilParser::handleLine(const QString &line, OutputFormat type)
{
    QString lne = rightTrimmed(line);
    if (type == StdOutFormat) {
        // Check for MSC51 compiler specific patterns.
        Result res = parseMcs51WarningOrErrorDetailsMessage1(lne);
        if (res.status != Status::NotHandled)
            return res;
        res = parseMcs51WarningOrErrorDetailsMessage2(lne);
        if (res.status != Status::NotHandled)
            return res;
        if (parseMcs51WarningOrFatalErrorMessage(lne))
            return Status::InProgress;
        if (parseMcs51FatalErrorMessage2(lne))
            return Status::InProgress;

        if (m_lastTask.isNull()) {
            // Check for details, which are comes on a previous
            // lines, before the message.

            // This code handles the details in a form like:
            // 0000                  24         ljmp    usb_stub_isr ; (00) Setup data available.
            // *** _____________________________________^
            // 003C                  54         ljmp    usb_stub_isr ; (3C) EP8 in/out.
            // *** _____________________________________^
            if (hasDetailsEntry(lne) || hasDetailsPointer(lne)) {
                lne.replace(0, 4, "    ");
                m_snippets.push_back(lne);
                return Status::InProgress;
            }
        } else {
            // Check for details, which are comes on a next
            // lines, after the message.
            if (lne.startsWith(' ')) {
                m_snippets.push_back(lne);
                return Status::InProgress;
            }
        }
        flush();
        return Status::NotHandled;
    }

    // Check for ARM compiler specific patterns.
    const Result res = parseArmWarningOrErrorDetailsMessage(lne);
    if (res.status != Status::NotHandled)
        return res;
    if (parseArmErrorOrFatalErorrMessage(lne))
        return Status::InProgress;

    if (lne.startsWith(' ')) {
        m_snippets.push_back(lne);
        return Status::InProgress;
    }

    flush();
    return Status::NotHandled;
}

void KeilParser::flush()
{
    if (m_lastTask.isNull())
        return;

    m_lastTask.details = m_snippets;
    m_snippets.clear();
    m_lines += m_lastTask.details.count();
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

void BareMetalPlugin::testKeilOutputParsers_data()
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

    // ARM compiler specific patterns.

    QTest::newRow("ARM: No details warning")
            << QString::fromLatin1("\"c:\\foo\\main.c\", line 63: Warning: #1234: Some warning")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "#1234: Some warning",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("ARM: Details warning")
            << QString::fromLatin1("\"c:\\foo\\main.c\", line 63: Warning: #1234: Some warning\n"
                                   "      int f;\n"
                                   "          ^")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "#1234: Some warning\n"
                                       "      int f;\n"
                                       "          ^",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("ARM: No details error")
            << QString::fromLatin1("\"c:\\foo\\main.c\", line 63: Error: #1234: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "#1234: Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("ARM: No details error with column")
            << QString::fromLatin1("\"flash.sct\", line 51 (column 20): Error: L1234: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "L1234: Some error",
                                       FilePath::fromUserInput("flash.sct"),
                                       51))
            << QString();

    QTest::newRow("ARM: Details error")
            << QString::fromLatin1("\"c:\\foo\\main.c\", line 63: Error: #1234: Some error\n"
                                   "      int f;\n"
                                   "          ^")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "#1234: Some error\n"
                                       "      int f;\n"
                                       "          ^",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("ARM: At end of source")
            << QString::fromLatin1("\"c:\\foo\\main.c\", line 71: Error: At end of source:  #40: Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "#40: Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       71))
            << QString();

    QTest::newRow("ARM: Starts with error")
            << QString::fromLatin1("Error: L6226E: Some error.")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "L6226E: Some error."))
            << QString();

    // MCS51 compiler specific patterns.

    // Assembler messages.
    QTest::newRow("MCS51: Assembler simple warning")
            << QString::fromLatin1("*** WARNING #A9 IN 15 (c:\\foo\\dscr.a51, LINE 15): Some warning")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "#A9: Some warning",
                                       FilePath::fromUserInput("c:\\foo\\dscr.a51"),
                                       15))
            << QString();

    QTest::newRow("MCS51: Assembler simple error")
            << QString::fromLatin1("*** ERROR #A9 IN 15 (c:\\foo\\dscr.a51, LINE 15): Some error")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "#A9: Some error",
                                       FilePath::fromUserInput("c:\\foo\\dscr.a51"),
                                       15))
            << QString();

    QTest::newRow("MCS51: Assembler fatal error")
            << QString::fromLatin1("A51 FATAL ERROR -\n"
                                   "  Some detail 1\n"
                                   "  Some detail N")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Assembler fatal error\n"
                                       "  Some detail 1\n"
                                       "  Some detail N"))
            << QString();

    QTest::newRow("MCS51: Assembler details error")
            << QString::fromLatin1("01AF   Some detail\n"
                                   "*** ___^\n"
                                   "*** ERROR #A45 IN 28 (d:\\foo.a51, LINE 28): Some error")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "#A45: Some error\n"
                                       "       Some detail\n"
                                       "    ___^",
                                       FilePath::fromUserInput("d:\\foo.a51"),
                                       28))
            << QString();

    // Compiler messages.
    QTest::newRow("MCS51: Compiler simple warning")
            << QString::fromLatin1("*** WARNING C123 IN LINE 13 OF c:\\foo.c: Some warning")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "C123: Some warning",
                                       FilePath::fromUserInput("c:\\foo.c"),
                                       13))
            << QString();

    QTest::newRow("MCS51: Compiler extended warning")
            << QString::fromLatin1("*** WARNING C123 IN LINE 13 OF c:\\foo.c: Some warning : 'extended text'")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "C123: Some warning : 'extended text'",
                                       FilePath::fromUserInput("c:\\foo.c"),
                                       13))
            << QString();

    QTest::newRow("MCS51: Compiler simple error")
            << QString::fromLatin1("*** ERROR C123 IN LINE 13 OF c:\\foo.c: Some error")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "C123: Some error",
                                       FilePath::fromUserInput("c:\\foo.c"),
                                       13))
            << QString();

    QTest::newRow("MCS51: Compiler extended error")
            << QString::fromLatin1("*** ERROR C123 IN LINE 13 OF c:\\foo.c: Some error : 'extended text'")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "C123: Some error : 'extended text'",
                                       FilePath::fromUserInput("c:\\foo.c"),
                                       13))
            << QString();

    QTest::newRow("MCS51: Compiler fatal error")
            << QString::fromLatin1("C51 FATAL-ERROR -\n"
                                   "  Some detail 1\n"
                                   "  Some detail N")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Compiler fatal error\n"
                                       "  Some detail 1\n"
                                       "  Some detail N"))
            << QString();

    // Linker messages.
    QTest::newRow("MCS51: Linker warning")
            << QString::fromLatin1("*** WARNING L16: Some warning\n"
                                   "    Some detail 1")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "L16: Some warning\n"
                                       "    Some detail 1"))
            << QString();

    QTest::newRow("MCS51: Linker simple fatal error")
            << QString::fromLatin1("*** FATAL ERROR L456: Some error")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "L456: Some error"))
            << QString();

    QTest::newRow("MCS51: Linker extended fatal error")
            << QString::fromLatin1("*** FATAL ERROR L456: Some error\n"
                                   "    Some detail 1\n"
                                   "    Some detail N")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "L456: Some error\n"
                                       "    Some detail 1\n"
                                       "    Some detail N"))
            << QString();
}

void BareMetalPlugin::testKeilOutputParsers()
{
    OutputParserTester testbench;
    testbench.addLineParser(new KeilParser);
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
