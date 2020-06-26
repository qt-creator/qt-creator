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

#include "iarewparser.h"

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
    if (msgType == "Warning")
        return Task::TaskType::Warning;
    else if (msgType == "Error" || msgType == "Fatal error")
        return Task::TaskType::Error;
    return Task::TaskType::Unknown;
}

// IarParser

IarParser::IarParser()
{
    setObjectName("IarParser");
}

Utils::Id IarParser::id()
{
    return "BareMetal.OutputParser.Iar";
}

void IarParser::newTask(const Task &task)
{
    flush();
    m_lastTask = task;
    m_lines = 1;
}

void IarParser::amendFilePath()
{
    if (m_filePathParts.isEmpty())
        return;
    QString filePath;
    while (!m_filePathParts.isEmpty())
        filePath.append(m_filePathParts.takeFirst().trimmed());
    m_lastTask.setFile(Utils::FilePath::fromUserInput(filePath));
    m_expectFilePath = false;
}

bool IarParser::parseErrorOrFatalErrorDetailsMessage1(const QString &lne)
{
    const QRegularExpression re("^(Error|Fatal error)\\[(.+)\\]:\\s(.+)\\s\\[(.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return false;
    enum CaptureIndex { MessageTypeIndex = 1, MessageCodeIndex,
                        DescriptionIndex, FilepathBeginIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const QString descr = QString("[%1]: %2").arg(match.captured(MessageCodeIndex),
                                                  match.captured(DescriptionIndex));
    // This task has a file path, but this patch are split on
    // some lines, which will be received later.
    newTask(CompileTask(type, descr));
    // Prepare first part of a file path.
    QString firstPart = match.captured(FilepathBeginIndex);
    firstPart.remove("referenced from ");
    m_filePathParts.push_back(firstPart);
    m_expectFilePath = true;
    m_expectSnippet = false;
    return true;
}

bool IarParser::parseErrorOrFatalErrorDetailsMessage2(const QString &lne)
{
    const QRegularExpression re("^.*(Error|Fatal error)\\[(.+)\\]:\\s(.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return false;
    enum CaptureIndex { MessageTypeIndex = 1, MessageCodeIndex,
                        DescriptionIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const QString descr = QString("[%1]: %2").arg(match.captured(MessageCodeIndex),
                                                  match.captured(DescriptionIndex));
    // This task has not a file path. The description details
    // will be received later on the next lines.
    newTask(CompileTask(type, descr));
    m_expectSnippet = true;
    m_expectFilePath = false;
    m_expectDescription = false;
    return true;
}

OutputLineParser::Result IarParser::parseWarningOrErrorOrFatalErrorDetailsMessage1(const QString &lne)
{
    const QRegularExpression re("^\"(.+)\",(\\d+)?\\s+(Warning|Error|Fatal error)\\[(.+)\\].+$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return Status::NotHandled;
    enum CaptureIndex { FilePathIndex = 1, LineNumberIndex,
                        MessageTypeIndex, MessageCodeIndex };
    const Utils::FilePath fileName = Utils::FilePath::fromUserInput(
                match.captured(FilePathIndex));
    const int lineno = match.captured(LineNumberIndex).toInt();
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    // A full description will be received later on next lines.
    newTask(CompileTask(type, {}, absoluteFilePath(fileName), lineno));
    const QString firstPart = QString("[%1]: ").arg(match.captured(MessageCodeIndex));
    m_descriptionParts.append(firstPart);
    m_expectDescription = true;
    m_expectSnippet = false;
    m_expectFilePath = false;
    LinkSpecs linkSpecs;
    addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, match,
                                   FilePathIndex);
    return {Status::InProgress, linkSpecs};
}

bool IarParser::parseErrorInCommandLineMessage(const QString &lne)
{
    if (!lne.startsWith("Error in command line"))
        return false;
    newTask(CompileTask(Task::TaskType::Error, lne.trimmed()));
    return true;
}

bool IarParser::parseErrorMessage1(const QString &lne)
{
    const QRegularExpression re("^(Error)\\[(.+)\\]:\\s(.+)$");
    const QRegularExpressionMatch match = re.match(lne);
    if (!match.hasMatch())
        return false;
    enum CaptureIndex { MessageTypeIndex = 1, MessageCodeIndex, DescriptionIndex };
    const Task::TaskType type = taskType(match.captured(MessageTypeIndex));
    const QString descr = QString("[%1]: %2").arg(match.captured(MessageCodeIndex),
                                                  match.captured(DescriptionIndex));
    // This task has not a file path and line number (as it is a linker message)
    newTask(CompileTask(type, descr));
    return true;
}

OutputLineParser::Result IarParser::handleLine(const QString &line, OutputFormat type)
{
    const QString lne = rightTrimmed(line);
    if (type == StdOutFormat) {
        // The call sequence has the meaning!
        const bool leastOneParsed = parseErrorInCommandLineMessage(lne)
                || parseErrorMessage1(lne);
        if (!leastOneParsed) {
            flush();
            return Status::NotHandled;
        }
        return Status::InProgress;
    }

    if (parseErrorOrFatalErrorDetailsMessage1(lne))
        return Status::InProgress;
    if (parseErrorOrFatalErrorDetailsMessage2(lne))
        return Status::InProgress;
    const Result res = parseWarningOrErrorOrFatalErrorDetailsMessage1(lne);
    if (res.status != Status::NotHandled)
        return res;

    if (m_expectFilePath) {
        if (lne.endsWith(']')) {
            const QString lastPart = lne.left(lne.size() - 1);
            m_filePathParts.push_back(lastPart);
            flush();
            return Status::Done;
        } else {
            m_filePathParts.push_back(lne);
            return Status::InProgress;
        }
    }
    if (m_expectSnippet && lne.startsWith(' ')) {
        if (!lne.endsWith("Fatal error detected, aborting.")) {
            m_snippets.push_back(lne);
            return Status::InProgress;
        }
    } else if (m_expectDescription) {
        if (!lne.startsWith("            ")) {
            m_descriptionParts.push_back(lne.trimmed());
            return Status::InProgress;
        }
    }

    if (!m_lastTask.isNull()) {
        flush();
        return Status::Done;
    }

    return Status::NotHandled;
}

void IarParser::flush()
{
    if (m_lastTask.isNull())
        return;

    while (!m_descriptionParts.isEmpty())
        m_lastTask.summary.append(m_descriptionParts.takeFirst());
    m_lastTask.details = m_snippets;
    m_snippets.clear();
    m_lines += m_lastTask.details.count();
    setDetailsFormat(m_lastTask);
    amendFilePath();

    m_expectSnippet = true;
    m_expectFilePath = false;
    m_expectDescription = false;

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

void BareMetalPlugin::testIarOutputParsers_data()
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

    // For std out.
    QTest::newRow("Error in command line")
            << QString::fromLatin1("Error in command line: Some error")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Error in command line: Some error"))
            << QString();

    QTest::newRow("Linker error")
            << QString::fromLatin1("Error[e46]: Some error")
            << OutputParserTester::STDOUT
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[e46]: Some error"))
            << QString();

    // For std error.
    QTest::newRow("No details warning")
            << QString::fromLatin1("\"c:\\foo\\main.c\",63 Warning[Pe223]:\n"
                                   "          Some warning \"foo\" bar")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "[Pe223]: Some warning \"foo\" bar",
                                       Utils::FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Details warning")
            << QString::fromLatin1("      some_detail;\n"
                                   "      ^\n"
                                   "\"c:\\foo\\main.c\",63 Warning[Pe223]:\n"
                                   "          Some warning")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "[Pe223]: Some warning\n"
                                       "      some_detail;\n"
                                       "      ^",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("No details split-description warning")
            << QString::fromLatin1("\"c:\\foo\\main.c\",63 Warning[Pe223]:\n"
                                   "          Some warning\n"
                                   "          , split")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "[Pe223]: Some warning, split",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("No details error")
            << QString::fromLatin1("\"c:\\foo\\main.c\",63 Error[Pe223]:\n"
                                   "          Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[Pe223]: Some error",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("Details error")
            << QString::fromLatin1("      some_detail;\n"
                                   "      ^\n"
                                   "\"c:\\foo\\main.c\",63 Error[Pe223]:\n"
                                   "          Some error")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[Pe223]: Some error\n"
                                       "      some_detail;\n"
                                       "      ^",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("No details split-description error")
            << QString::fromLatin1("\"c:\\foo\\main.c\",63 Error[Pe223]:\n"
                                   "          Some error\n"
                                   "          , split")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[Pe223]: Some error, split",
                                       FilePath::fromUserInput("c:\\foo\\main.c"),
                                       63))
            << QString();

    QTest::newRow("No definition for")
            << QString::fromLatin1("Error[Li005]: Some error \"foo\" [referenced from c:\\fo\n"
                                   "         o\\bar\\mai\n"
                                   "         n.c.o\n"
                                   "]")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[Li005]: Some error \"foo\"",
                                       FilePath::fromUserInput("c:\\foo\\bar\\main.c.o")))
            << QString();

    QTest::newRow("More than one source file specified")
            << QString::fromLatin1("Fatal error[Su011]: Some error:\n"
                                   "                      c:\\foo.c\n"
                                   "            c:\\bar.c\n"
                                   "Fatal error detected, aborting.")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[Su011]: Some error:\n"
                                       "                      c:\\foo.c\n"
                                       "            c:\\bar.c"))
            << QString();

    QTest::newRow("At end of source")
            << QString::fromLatin1("At end of source  Error[Pe040]: Some error \";\"")
            << OutputParserTester::STDERR
            << QString()
            << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "[Pe040]: Some error \";\""))
            << QString();
}

void BareMetalPlugin::testIarOutputParsers()
{
    OutputParserTester testbench;
    testbench.addLineParser(new IarParser);
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
