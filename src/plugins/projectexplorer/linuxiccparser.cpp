/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "linuxiccparser.h"
#include "ldparser.h"
#include "projectexplorerconstants.h"

using namespace ProjectExplorer;

LinuxIccParser::LinuxIccParser()
    : m_expectFirstLine(true), m_indent(0), m_temporary(Task())
{
    setObjectName(QLatin1String("LinuxIccParser"));
    // main.cpp(53): error #308: function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible

    m_firstLine.setPattern(QLatin1String("^([^\\(\\)]+)"    // filename (cap 1)
                           "\\((\\d+)\\):"                  // line number including : (cap 2)
                           " ((error|warning)( #\\d+)?: )?" // optional type (cap 4) and optional error number // TODO really optional ?
                           "(.*)$"));                       // description (cap 6)
    //m_firstLine.setMinimal(true);

                                            // Note pattern also matches caret lines
    m_continuationLines.setPattern(QLatin1String("^\\s+"  // At least one whitespace
                                                 "(.*)$"));// description
    m_continuationLines.setMinimal(true);

    m_caretLine.setPattern(QLatin1String("^\\s*"          // Whitespaces
                                         "\\^"            // a caret
                                         "\\s*$"));       // and again whitespaces
    m_caretLine.setMinimal(true);

    appendOutputParser(new LdParser);
}

void LinuxIccParser::stdError(const QString &line)
{
    if (m_expectFirstLine  && m_firstLine.indexIn(line) != -1) {
        // Clear out old task
        m_temporary = ProjectExplorer::Task(Task::Unknown, m_firstLine.cap(6).trimmed(),
                                            Utils::FileName::fromUserInput(m_firstLine.cap(1)),
                                            m_firstLine.cap(2).toInt(),
                                            Core::Id(Constants::TASK_CATEGORY_COMPILE));
        QString category = m_firstLine.cap(4);
        if (category == QLatin1String("error"))
            m_temporary.type = Task::Error;
        else if (category == QLatin1String("warning"))
            m_temporary.type = Task::Warning;

        m_expectFirstLine = false;
    } else if (!m_expectFirstLine && m_caretLine.indexIn(line) != -1) {
        // Format the last line as code
        QTextLayout::FormatRange fr;
        fr.start = m_temporary.description.lastIndexOf(QLatin1Char('\n')) + 1;
        fr.length = m_temporary.description.length() - fr.start;
        fr.format.setFontItalic(true);
        m_temporary.formats.append(fr);

        QTextLayout::FormatRange fr2;
        fr2.start = fr.start + line.indexOf(QLatin1Char('^')) - m_indent;
        fr2.length = 1;
        fr2.format.setFontWeight(QFont::Bold);
        m_temporary.formats.append(fr2);
    } else if (!m_expectFirstLine && line.trimmed().isEmpty()) { // last Line
        m_expectFirstLine = true;
        emit addTask(m_temporary);
        m_temporary = Task();
    } else if (!m_expectFirstLine && m_continuationLines.indexIn(line) != -1) {
        m_temporary.description.append(QLatin1Char('\n'));
        m_indent = 0;
        while (m_indent < line.length() && line.at(m_indent).isSpace())
            m_indent++;
        m_temporary.description.append(m_continuationLines.cap(1).trimmed());
    } else {
        IOutputParser::stdError(line);
    }
}

void LinuxIccParser::doFlush()
{
    if (m_temporary.isNull())
        return;
    Task t = m_temporary;
    m_temporary.clear();
    emit addTask(t);
}

#ifdef WITH_TESTS
#   include <QTest>
#   include "projectexplorer.h"
#   include "metatypedeclarations.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testLinuxIccOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

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

    QTest::newRow("undeclared function")
            << QString::fromLatin1("main.cpp(13): error: identifier \"f\" is undefined\n"
                                   "      f(0);\n"
                                   "      ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("identifier \"f\" is undefined\nf(0);"),
                        Utils::FileName::fromUserInput(QLatin1String("main.cpp")), 13,
                        Core::Id(Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("private function")
            << QString::fromLatin1("main.cpp(53): error #308: function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible\n"
                                   "      b.privatefunc();\n"
                                   "        ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("function \"AClass::privatefunc\" (declared at line 4 of \"main.h\") is inaccessible\nb.privatefunc();"),
                        Utils::FileName::fromUserInput(QLatin1String("main.cpp")), 53,
                        Core::Id(Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("simple warning")
            << QString::fromLatin1("main.cpp(41): warning #187: use of \"=\" where \"==\" may have been intended\n"
                                   "      while (a = true)\n"
                                   "             ^\n"
                                   "\n")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("\n")
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("use of \"=\" where \"==\" may have been intended\nwhile (a = true)"),
                        Utils::FileName::fromUserInput(QLatin1String("main.cpp")), 41,
                        Core::Id(Constants::TASK_CATEGORY_COMPILE)))
            << QString();
}

void ProjectExplorerPlugin::testLinuxIccOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new LinuxIccParser);
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
