/****************************************************************************
**
** Copyright (C) 2016 Axonian LLC.
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

#include "cmakeparser.h"

#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

const char COMMON_ERROR_PATTERN[] = "^CMake Error at (.*?):([0-9]*?)( \\((.*?)\\))?:";
const char NEXT_SUBERROR_PATTERN[] = "^CMake Error in (.*?):";
const char LOCATION_LINE_PATTERN[] = ":(\\d+?):(?:(\\d+?))?$";

CMakeParser::CMakeParser()
{
    m_commonError.setPattern(QLatin1String(COMMON_ERROR_PATTERN));
    QTC_CHECK(m_commonError.isValid());

    m_nextSubError.setPattern(QLatin1String(NEXT_SUBERROR_PATTERN));
    QTC_CHECK(m_nextSubError.isValid());

    m_locationLine.setPattern(QLatin1String(LOCATION_LINE_PATTERN));
    QTC_CHECK(m_locationLine.isValid());
}

void CMakeParser::setSourceDirectory(const QString &sourceDir)
{
    if (m_sourceDirectory)
        emit searchDirExpired(FilePath::fromString(m_sourceDirectory.value().path()));
    m_sourceDirectory = QDir(sourceDir);
    emit addSearchDir(FilePath::fromString(sourceDir));
}

OutputLineParser::Result CMakeParser::handleLine(const QString &line, OutputFormat type)
{
    if (type != StdErrFormat)
        return Status::NotHandled;

    QRegularExpressionMatch match;
    QString trimmedLine = rightTrimmed(line);
    switch (m_expectTripleLineErrorData) {
    case NONE:
        if (trimmedLine.isEmpty() && !m_lastTask.isNull()) {
            if (m_skippedFirstEmptyLine) {
                flush();
                return Status::InProgress;
            }
            m_skippedFirstEmptyLine = true;
            return Status::InProgress;
        }
        if (m_skippedFirstEmptyLine)
            m_skippedFirstEmptyLine = false;

        match = m_commonError.match(trimmedLine);
        if (match.hasMatch()) {
            QString path = m_sourceDirectory ? m_sourceDirectory->absoluteFilePath(
                               QDir::fromNativeSeparators(match.captured(1)))
                                             : QDir::fromNativeSeparators(match.captured(1));
            m_lastTask = BuildSystemTask(Task::Error,
                                         QString(),
                                         absoluteFilePath(FilePath::fromUserInput(path)),
                                         match.captured(2).toInt());
            m_lines = 1;
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line,
                                           match, 1);
            return {Status::InProgress, linkSpecs};
        }
        match = m_nextSubError.match(trimmedLine);
        if (match.hasMatch()) {
            m_lastTask = BuildSystemTask(Task::Error, QString(),
                                         absoluteFilePath(FilePath::fromUserInput(match.captured(1))));
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line,
                                           match, 1);
            m_lines = 1;
            return {Status::InProgress, linkSpecs};
        } else if (trimmedLine.startsWith(QLatin1String("  ")) && !m_lastTask.isNull()) {
            if (!m_lastTask.summary.isEmpty())
                m_lastTask.summary.append(' ');
            m_lastTask.summary.append(trimmedLine.trimmed());
            ++m_lines;
            return Status::InProgress;
        } else if (trimmedLine.endsWith(QLatin1String("in cmake code at"))) {
            m_expectTripleLineErrorData = LINE_LOCATION;
            flush();
            const Task::TaskType type =
                    trimmedLine.contains(QLatin1String("Error")) ? Task::Error : Task::Warning;
            m_lastTask = BuildSystemTask(type, QString());
            return Status::InProgress;
        } else if (trimmedLine.startsWith("CMake Error: ")) {
            m_lastTask = BuildSystemTask(Task::Error, trimmedLine.mid(13));
            m_lines = 1;
            return Status::InProgress;
        } else if (trimmedLine.startsWith("-- ") || trimmedLine.startsWith(" * ")) {
            // Do not pass on lines starting with "-- " or "* ". Those are typical CMake output
            return Status::InProgress;
        }
        return Status::NotHandled;
    case LINE_LOCATION:
        {
            match = m_locationLine.match(trimmedLine);
            QTC_CHECK(match.hasMatch());
            m_lastTask.file = absoluteFilePath(FilePath::fromUserInput(
                                                   trimmedLine.mid(0, match.capturedStart())));
            m_lastTask.line = match.captured(1).toInt();
            m_expectTripleLineErrorData = LINE_DESCRIPTION;
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(linkSpecs, m_lastTask.file, m_lastTask.line, 0,
                                           match.capturedStart());
            return {Status::InProgress, linkSpecs};
        }
    case LINE_DESCRIPTION:
        m_lastTask.summary = trimmedLine;
        if (trimmedLine.endsWith(QLatin1Char('\"')))
            m_expectTripleLineErrorData = LINE_DESCRIPTION2;
        else {
            m_expectTripleLineErrorData = NONE;
            flush();
            return Status::Done;
        }
        return Status::InProgress;
    case LINE_DESCRIPTION2:
        m_lastTask.details.append(trimmedLine);
        m_expectTripleLineErrorData = NONE;
        flush();
        return Status::Done;
    }
    return Status::NotHandled;
}

void CMakeParser::flush()
{
    if (m_lastTask.isNull())
        return;
    Task t = m_lastTask;
    m_lastTask.clear();
    scheduleTask(t, m_lines, 1);
    m_lines = 0;
}

} // CMakeProjectManager

#ifdef WITH_TESTS
#include "cmakeprojectplugin.h"

#include <projectexplorer/outputparser_test.h>

#include <QTest>

namespace CMakeProjectManager {

void Internal::CMakeProjectPlugin::testCMakeParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<Tasks>("tasks");
    QTest::addColumn<QString>("outputLines");

    // negative tests
    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << Tasks()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << Tasks()
            << QString();

    // positive tests
    QTest::newRow("add custom target")
            << QString::fromLatin1("CMake Error at src/1/app/CMakeLists.txt:70 (add_custom_target):\n"
                                   "  Cannot find source file:\n\n"
                                   "    unknownFile.qml\n\n"
                                   "  Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   "  .hxx .in .txx\n\n\n"
                                   "CMake Error in src/1/app/CMakeLists.txt:\n"
                                   "  Cannot find source file:\n\n"
                                   "    CMakeLists.txt2\n\n"
                                   "  Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   "  .hxx .in .txx\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Cannot find source file: unknownFile.qml Tried extensions "
                                   ".c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp .hxx .in .txx",
                                    FilePath::fromUserInput("src/1/app/CMakeLists.txt"), 70)
                << BuildSystemTask(Task::Error,
                                   "Cannot find source file: CMakeLists.txt2 Tried extensions "
                                   ".c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp .hxx .in .txx",
                                   FilePath::fromUserInput("src/1/app/CMakeLists.txt"), -1))
            << QString();

    QTest::newRow("add subdirectory")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (add_subdirectory):\n"
                                   "  add_subdirectory given source \"app1\" which is not an existing directory.\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "add_subdirectory given source \"app1\" which is not an existing directory.",
                                   FilePath::fromUserInput("src/1/CMakeLists.txt"), 8))
            << QString();

    QTest::newRow("unknown command")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (i_am_wrong_command):\n"
                                   "  Unknown CMake command \"i_am_wrong_command\".\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Unknown CMake command \"i_am_wrong_command\".",
                                   FilePath::fromUserInput("src/1/CMakeLists.txt"), 8))
            << QString();

    QTest::newRow("incorrect arguments")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (message):\n"
                                   "  message called with incorrect number of arguments\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "message called with incorrect number of arguments",
                                   FilePath::fromUserInput("src/1/CMakeLists.txt"), 8))
            << QString();

    QTest::newRow("cmake error")
            << QString::fromLatin1("CMake Error: Error in cmake code at\n"
                                   "/test/path/CMakeLists.txt:9:\n"
                                   "Parse error.  Expected \"(\", got newline with text \"\n"
                                   "\".")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Parse error.  Expected \"(\", got newline with text \"\n\".",
                                   FilePath::fromUserInput("/test/path/CMakeLists.txt"), 9))
            << QString();

    QTest::newRow("cmake error2")
            << QString::fromLatin1("CMake Error: Error required internal CMake variable not set, cmake may be not be built correctly.\n"
                                   "Missing variable is:\n"
                                   "CMAKE_MAKE_PROGRAM\n")
            << OutputParserTester::STDERR
            << QString() << QString("Missing variable is:\nCMAKE_MAKE_PROGRAM\n")
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Error required internal CMake variable not set, "
                                   "cmake may be not be built correctly."))
            << QString();

    QTest::newRow("cmake error at")
            << QString::fromLatin1("CMake Error at CMakeLists.txt:4:\n"
                                   "  Parse error.  Expected \"(\", got newline with text \"\n"
                                   "\n"
                                   "  \".\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Parse error.  Expected \"(\", got newline with text \" \".",
                                   FilePath::fromUserInput("CMakeLists.txt"), 4))
            << QString();

    QTest::newRow("cmake warning")
            << QString::fromLatin1("Syntax Warning in cmake code at\n"
                                   "/test/path/CMakeLists.txt:9:15\n"
                                   "Argument not separated from preceding token by whitespace.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "Argument not separated from preceding token by whitespace.",
                                   FilePath::fromUserInput("/test/path/CMakeLists.txt"), 9))
            << QString();
    QTest::newRow("eat normal CMake output")
        << QString::fromLatin1("-- Qt5 install prefix: /usr/lib\n"
                               " * Plugin componentsplugin, with CONDITION TARGET QmlDesigner")
        << OutputParserTester::STDERR << QString() << QString() << (Tasks()) << QString();
}

void Internal::CMakeProjectPlugin::testCMakeParser()
{
    OutputParserTester testbench;
    testbench.addLineParser(new CMakeParser);
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

} // CMakeProjectManager

#endif
