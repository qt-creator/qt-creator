// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxiccparser.h"
#include "ldparser.h"
#include "lldparser.h"
#include "projectexplorerconstants.h"

#include <utils/qtcassert.h>

using namespace Utils;

namespace ProjectExplorer {

LinuxIccParser::LinuxIccParser()
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
    m_pchInfoLine.setPattern(QLatin1String("^\".*?\": (creating|using) precompiled header file \".*?\"$"));
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
            Task::TaskType type = Task::Unknown;
            QString category = match.captured(4);
            if (category == QLatin1String("error"))
                type = Task::Error;
            else if (category == QLatin1String("warning"))
                type = Task::Warning;
            const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(1)));
            const int lineNo = match.captured(2).toInt();
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, -1, match, 1);
            createOrAmendTask(type, match.captured(6).trimmed(), line, false, filePath, lineNo);
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
        flush();
        return Status::Done;
    }
    const QRegularExpressionMatch match = m_continuationLines.match(line);
    if (!m_expectFirstLine && match.hasMatch()) {
        createOrAmendTask(Task::Unknown, {}, line, true);
        return Status::InProgress;
    }
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

} // ProjectExplorer

#ifdef WITH_TESTS
#   include <QTest>
#   include "projectexplorer_test.h"
#   include "outputparser_test.h"

namespace ProjectExplorer::Internal {

void ProjectExplorerTest::testLinuxIccOutputParsers_data()
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
                           "identifier \"f\" is undefined\n"
                           "main.cpp(13): error: identifier \"f\" is undefined\n"
                           "      f(0);",
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
                           "identifier \"f\" is undefined\n"
                           "main.cpp(13): error: identifier \"f\" is undefined\n"
                           "      f(0);",
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
                           "function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible\n"
                           "main.cpp(53): error #308: function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible\n"
                           "      b.privatefunc();",
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
                           "use of \"=\" where \"==\" may have been intended\n"
                           "main.cpp(41): warning #187: use of \"=\" where \"==\" may have been intended\n"
                           "      while (a = true)",
                           FilePath::fromUserInput("main.cpp"), 41))
            << QString();
}

void ProjectExplorerTest::testLinuxIccOutputParsers()
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

} // ProjectExplorer::Internal

#endif // WITH_TESTS
