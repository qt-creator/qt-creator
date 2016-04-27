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

using namespace CMakeProjectManager;
using namespace Internal;
using namespace ProjectExplorer;

const char COMMON_ERROR_PATTERN[] = "^CMake Error at (.*):([0-9]*) \\((.*)\\):";
const char NEXT_SUBERROR_PATTERN[] = "^CMake Error in (.*):";
const char LOCATION_LINE_PATTERN[] = ":(\\d+):(?:(\\d+))?$";

CMakeParser::CMakeParser()
{
    m_commonError.setPattern(QLatin1String(COMMON_ERROR_PATTERN));
    m_commonError.setMinimal(true);
    QTC_CHECK(m_commonError.isValid());

    m_nextSubError.setPattern(QLatin1String(NEXT_SUBERROR_PATTERN));
    m_nextSubError.setMinimal(true);
    QTC_CHECK(m_nextSubError.isValid());

    m_locationLine.setPattern(QLatin1String(LOCATION_LINE_PATTERN));
    QTC_CHECK(m_locationLine.isValid());
}

void CMakeParser::stdError(const QString &line)
{
    QString trimmedLine = rightTrimmed(line);

    switch (m_expectTripleLineErrorData) {
    case NONE:
        if (trimmedLine.isEmpty() && !m_lastTask.isNull()) {
            if (m_skippedFirstEmptyLine)
                doFlush();
            else
                m_skippedFirstEmptyLine = true;
            return;
        }
        if (m_skippedFirstEmptyLine)
            m_skippedFirstEmptyLine = false;

        if (m_commonError.indexIn(trimmedLine) != -1) {
            m_lastTask = Task(Task::Error, QString(), Utils::FileName::fromUserInput(m_commonError.cap(1)),
                              m_commonError.cap(2).toInt(), Constants::TASK_CATEGORY_BUILDSYSTEM);
            m_lines = 1;
            return;
        } else if (m_nextSubError.indexIn(trimmedLine) != -1) {
            m_lastTask = Task(Task::Error, QString(), Utils::FileName::fromUserInput(m_nextSubError.cap(1)), -1,
                              Constants::TASK_CATEGORY_BUILDSYSTEM);
            m_lines = 1;
            return;
        } else if (trimmedLine.startsWith(QLatin1String("  ")) && !m_lastTask.isNull()) {
            if (!m_lastTask.description.isEmpty())
                m_lastTask.description.append(QLatin1Char(' '));
            m_lastTask.description.append(trimmedLine.trimmed());
            ++m_lines;
            return;
        } else if (trimmedLine.endsWith(QLatin1String("in cmake code at"))) {
            m_expectTripleLineErrorData = LINE_LOCATION;
            doFlush();
            m_lastTask = Task(trimmedLine.contains(QLatin1String("Error")) ? Task::Error : Task::Warning,
                              QString(), Utils::FileName(), -1, Constants::TASK_CATEGORY_BUILDSYSTEM);
            return;
        } else if (trimmedLine.startsWith("CMake Error: ")) {
            m_lastTask = Task(Task::Error, trimmedLine.mid(13),
                              Utils::FileName(), -1, Constants::TASK_CATEGORY_BUILDSYSTEM);
            m_lines = 1;
            return;
        }
        IOutputParser::stdError(line);
        return;
    case LINE_LOCATION:
        {
            QRegularExpressionMatch m = m_locationLine.match(trimmedLine);
            QTC_CHECK(m.hasMatch());
            m_lastTask.file = Utils::FileName::fromUserInput(trimmedLine.mid(0, m.capturedStart()));
            m_lastTask.line = m.captured(1).toInt();
            m_expectTripleLineErrorData = LINE_DESCRIPTION;
        }
        return;
    case LINE_DESCRIPTION:
        m_lastTask.description = trimmedLine;
        if (trimmedLine.endsWith(QLatin1Char('\"')))
            m_expectTripleLineErrorData = LINE_DESCRIPTION2;
        else {
            m_expectTripleLineErrorData = NONE;
            doFlush();
        }
        return;
    case LINE_DESCRIPTION2:
        m_lastTask.description.append(QLatin1Char('\n'));
        m_lastTask.description.append(trimmedLine);
        m_expectTripleLineErrorData = NONE;
        doFlush();
        return;
    }
}

void CMakeParser::doFlush()
{
    if (m_lastTask.isNull())
        return;
    Task t = m_lastTask;
    m_lastTask.clear();
    emit addTask(t, m_lines, 1);
    m_lines = 0;
}

#ifdef WITH_TESTS
#include "cmakeprojectplugin.h"

#include <projectexplorer/outputparser_test.h>

#include <QTest>

void CMakeProjectPlugin::testCMakeParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

    const Core::Id categoryBuild = Constants::TASK_CATEGORY_BUILDSYSTEM;

    // negative tests
    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<ProjectExplorer::Task>()
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
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Cannot find source file: unknownFile.qml Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp .hxx .in .txx"),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/app/CMakeLists.txt")), 70,
                        categoryBuild)
            << Task(Task::Error,
                    QLatin1String("Cannot find source file: CMakeLists.txt2 Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp .hxx .in .txx"),
                    Utils::FileName::fromUserInput(QLatin1String("src/1/app/CMakeLists.txt")), -1,
                    categoryBuild))
            << QString();

    QTest::newRow("add subdirectory")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (add_subdirectory):\n"
                                   "  add_subdirectory given source \"app1\" which is not an existing directory.\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("add_subdirectory given source \"app1\" which is not an existing directory."),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/CMakeLists.txt")), 8,
                        categoryBuild))
            << QString();

    QTest::newRow("unknown command")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (i_am_wrong_command):\n"
                                   "  Unknown CMake command \"i_am_wrong_command\".\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Unknown CMake command \"i_am_wrong_command\"."),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/CMakeLists.txt")), 8,
                        categoryBuild))
            << QString();

    QTest::newRow("incorrect arguments")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (message):\n"
                                   "  message called with incorrect number of arguments\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("message called with incorrect number of arguments"),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/CMakeLists.txt")), 8,
                        categoryBuild))
            << QString();

    QTest::newRow("cmake error")
            << QString::fromLatin1("CMake Error: Error in cmake code at\n"
                                   "/test/path/CMakeLists.txt:9:\n"
                                   "Parse error.  Expected \"(\", got newline with text \"\n"
                                   "\".")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Parse error.  Expected \"(\", got newline with text \"\n\"."),
                        Utils::FileName::fromUserInput(QLatin1String("/test/path/CMakeLists.txt")), 9,
                        categoryBuild))
            << QString();

    QTest::newRow("cmake error2")
            << QString::fromLatin1("CMake Error: Error required internal CMake variable not set, cmake may be not be built correctly.\n"
                                   "Missing variable is:\n"
                                   "CMAKE_MAKE_PROGRAM\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Missing variable is:\nCMAKE_MAKE_PROGRAM\n")
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Error required internal CMake variable not set, cmake may be not be built correctly."),
                        Utils::FileName(), -1, categoryBuild))
            << QString();

    QTest::newRow("cmake warning")
            << QString::fromLatin1("Syntax Warning in cmake code at\n"
                                   "/test/path/CMakeLists.txt:9:15\n"
                                   "Argument not separated from preceding token by whitespace.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("Argument not separated from preceding token by whitespace."),
                        Utils::FileName::fromUserInput(QLatin1String("/test/path/CMakeLists.txt")), 9,
                        categoryBuild))
            << QString();
}

void CMakeProjectPlugin::testCMakeParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new CMakeParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}

#endif
