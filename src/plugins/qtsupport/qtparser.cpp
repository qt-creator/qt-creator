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

#include "qtparser.h"

#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

using namespace QtSupport;
using ProjectExplorer::Task;

// opt. drive letter + filename: (2 brackets)
#define FILE_PATTERN "^(([A-Za-z]:)?[^:]+\\.[^:]+)"

QtParser::QtParser() :
    m_mocRegExp(QLatin1String(FILE_PATTERN"[:\\(](\\d+)\\)?:\\s([Ww]arning|[Ee]rror):\\s(.+)$"))
{
    setObjectName(QLatin1String("QtParser"));
    m_mocRegExp.setMinimal(true);
}

void QtParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (m_mocRegExp.indexIn(lne) > -1) {
        bool ok;
        int lineno = m_mocRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Task task(Task::Error,
                  m_mocRegExp.cap(5).trimmed(),
                  Utils::FileName::fromUserInput(m_mocRegExp.cap(1)) /* filename */,
                  lineno,
                  Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE));
        if (m_mocRegExp.cap(4).compare(QLatin1String("Warning"), Qt::CaseInsensitive) == 0)
            task.type = Task::Warning;
        emit addTask(task);
        return;
    }
    IOutputParser::stdError(line);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qtsupportplugin.h"
#   include <projectexplorer/projectexplorerconstants.h>
#   include <projectexplorer/metatypedeclarations.h>
#   include <projectexplorer/outputparser_test.h>

using namespace ProjectExplorer;
using namespace QtSupport::Internal;

void QtSupportPlugin::testQtOutputParser_data()
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
    QTest::newRow("pass-through gcc infos")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here")
            << OutputParserTester::STDERR
            << QString()
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("qdoc warning")
            << QString::fromLatin1("/home/user/dev/qt5/qtscript/src/script/api/qscriptengine.cpp:295: warning: Can't create link to 'Object Trees & Ownership'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("Can't create link to 'Object Trees & Ownership'"),
                                                       Utils::FileName::fromUserInput(QLatin1String("/home/user/dev/qt5/qtscript/src/script/api/qscriptengine.cpp")), 295,
                                                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("moc warning")
            << QString::fromLatin1("..\\untitled\\errorfile.h:0: Warning: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       Utils::FileName::fromUserInput(QLatin1String("..\\untitled\\errorfile.h")), 0,
                                                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("moc warning 2")
            << QString::fromLatin1("c:\\code\\test.h(96): Warning: Property declaration ) has no READ accessor function. The property will be invalid.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("Property declaration ) has no READ accessor function. The property will be invalid."),
                                                       Utils::FileName::fromUserInput(QLatin1String("c:\\code\\test.h")), 96,
                                                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("ninja with moc")
            << QString::fromLatin1("E:/sandbox/creator/loaden/src/libs/utils/iwelcomepage.h(54): Error: Undefined interface")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Error,
                                                       QLatin1String("Undefined interface"),
                                                       Utils::FileName::fromUserInput(QLatin1String("E:/sandbox/creator/loaden/src/libs/utils/iwelcomepage.h")), 54,
                                                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
}

void QtSupportPlugin::testQtOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new QtParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines, outputLines);
}
#endif
