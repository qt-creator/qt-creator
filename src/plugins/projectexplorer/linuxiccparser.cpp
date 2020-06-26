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

#include "linuxiccparser.h"
#include "ldparser.h"
#include "lldparser.h"
#include "projectexplorerconstants.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

LinuxIccParser::LinuxIccParser() :
    m_temporary(Task())
{
    setObjectName(QLatin1String("LinuxIccParser"));
    // main.cpp(53): error #308: function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible

    m_firstLine.setPattern(QLatin1String("^([^\\(\\)]+?)"    // filename (cap 1)
                           "\\((\\d+?)\\):"                  // line number including : (cap 2)
                           " ((error|warning)( #\\d+?)?: )?" // optional type (cap 4) and optional error number // TODO really optional ?
                           "(.*?)$"));                       // description (cap 6)
    QTC_CHECK(m_firstLine.isValid());

                                            // Note pattern also matches caret lines
    m_continuationLines.setPattern(QLatin1String("^\\s+"  // At least one whitespace
                                                 "(.*)$"));// description
    QTC_CHECK(m_continuationLines.isValid());

    m_caretLine.setPattern(QLatin1String("^\\s*?"          // Whitespaces
                                         "\\^"            // a caret
                                         "\\s*?$"));       // and again whitespaces
    QTC_CHECK(m_caretLine.isValid());

    // ".pch/Qt5Core.pchi.cpp": creating precompiled header file ".pch/Qt5Core.pchi"
    // "animation/qabstractanimation.cpp": using precompiled header file ".pch/Qt5Core.pchi"
    m_pchInfoLine.setPattern(QLatin1String("^\".*?\": (creating|using) precompiled header file \".*?\"\n$"));
    QTC_CHECK(m_pchInfoLine.isValid());
}

OutputLineParser::Result LinuxIccParser::handleLine(const QString &line, OutputFormat type)
{
    if (type != Utils::StdErrFormat)
        return Status::NotHandled;

    if (line.indexOf(m_pchInfoLine) != -1)
        return Status::Done; // totally ignore this line

    if (m_expectFirstLine) {
        const QRegularExpressionMatch match = m_firstLine.match(line);
        if (match.hasMatch()) {
            // Clear out old task
            Task::TaskType type = Task::Unknown;
            QString category = match.captured(4);
            if (category == QLatin1String("error"))
                type = Task::Error;
            else if (category == QLatin1String("warning"))
                type = Task::Warning;
            const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(1)));
            const int lineNo = match.captured(2).toInt();
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, match, 1);
            m_temporary = CompileTask(type, match.captured(6).trimmed(), filePath, lineNo);

            m_lines = 1;
            m_expectFirstLine = false;
            return Status::InProgress;
        }
    }
    if (!m_expectFirstLine && line.indexOf(m_caretLine) != -1) {
        // FIXME: m_temporary.details.append(line);
        return Status::InProgress;
    }
    if (!m_expectFirstLine && line.trimmed().isEmpty()) { // last Line
        m_expectFirstLine = true;
        scheduleTask(m_temporary, m_lines);
        m_temporary = Task();
        return Status::Done;
    }
    const QRegularExpressionMatch match = m_continuationLines.match(line);
    if (!m_expectFirstLine && match.hasMatch()) {
        m_temporary.details.append(match.captured(1).trimmed());
        ++m_lines;
        return Status::InProgress;
    }
    QTC_CHECK(m_temporary.isNull());
    return Status::NotHandled;
}

Utils::Id LinuxIccParser::id()
{
    return Utils::Id("ProjectExplorer.OutputParser.Icc");
}

QList<OutputLineParser *> LinuxIccParser::iccParserSuite()
{
    return {new LinuxIccParser, new Internal::LldParser, new LdParser};
}

void LinuxIccParser::flush()
{
    if (m_temporary.isNull())
        return;

    setDetailsFormat(m_temporary);
    Task t = m_temporary;
    m_temporary.clear();
    scheduleTask(t, m_lines, 1);
}

#ifdef WITH_TESTS
#   include <QTest>
#   include "projectexplorer.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testLinuxIccOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<Tasks >("tasks");
    QTest::addColumn<QString>("outputLines");

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

    QTest::newRow("pch creation")
            << QString::fromLatin1("\".pch/Qt5Core.pchi.cpp\": creating precompiled header file \".pch/Qt5Core.pchi\"")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks()
            << QString();

    QTest::newRow("undeclared function")
            << QString::fromLatin1("main.cpp(13): error: identifier \"f\" is undefined\n"
                                   "      f(0);\n"
                                   "      ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (Tasks()
                << CompileTask(Task::Error,
                               "identifier \"f\" is undefined\nf(0);",
                               FilePath::fromUserInput(QLatin1String("main.cpp")), 13))
            << QString();

    // same, with PCH remark
    QTest::newRow("pch use+undeclared function")
            << QString::fromLatin1("\"main.cpp\": using precompiled header file \".pch/Qt5Core.pchi\"\n"
                                   "main.cpp(13): error: identifier \"f\" is undefined\n"
                                   "      f(0);\n"
                                   "      ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (Tasks()
                << CompileTask(Task::Error,
                               "identifier \"f\" is undefined\nf(0);",
                               FilePath::fromUserInput("main.cpp"), 13))
            << QString();


    QTest::newRow("private function")
            << QString::fromLatin1("main.cpp(53): error #308: function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible\n"
                                   "      b.privatefunc();\n"
                                   "        ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (Tasks()
                << CompileTask(Task::Error,
                               "function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible\nb.privatefunc();",
                               FilePath::fromUserInput("main.cpp"), 53))
            << QString();

    QTest::newRow("simple warning")
            << QString::fromLatin1("main.cpp(41): warning #187: use of \"=\" where \"==\" may have been intended\n"
                                   "      while (a = true)\n"
                                   "             ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (Tasks()
                << CompileTask(Task::Warning,
                               "use of \"=\" where \"==\" may have been intended\nwhile (a = true)",
                               FilePath::fromUserInput("main.cpp"), 41))
            << QString();

    QTest::newRow("moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Unknown,
                               "Note: No relevant classes found. No output generated.",
                               FilePath::fromUserInput("/home/qtwebkithelpviewer.h"), -1))
            << QString();
}

void ProjectExplorerPlugin::testLinuxIccOutputParsers()
{
    OutputParserTester testbench;
    testbench.setLineParsers(LinuxIccParser::iccParserSuite());
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

#endif
