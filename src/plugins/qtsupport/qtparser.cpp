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

#include "qtparser.h"

#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace QtSupport;
using ProjectExplorer::Task;

// opt. drive letter + filename: (2 brackets)
#define FILE_PATTERN "^(([A-Za-z]:)?[^:]+\\.[^:]+)"

QtParser::QtParser() :
    m_mocRegExp(QLatin1String(FILE_PATTERN"[:\\(](\\d+)\\)?:\\s([Ww]arning|[Ee]rror|[Nn]ote):\\s(.+)$")),
    m_translationRegExp(QLatin1String("^([Ww]arning|[Ee]rror):\\s+(.*) in '(.*)'$"))
{
    setObjectName(QLatin1String("QtParser"));
    m_mocRegExp.setMinimal(true);
    m_translationRegExp.setMinimal(true);
}

void QtParser::stdError(const QString &line)
{
    QString lne = rightTrimmed(line);
    if (m_mocRegExp.indexIn(lne) > -1) {
        bool ok;
        int lineno = m_mocRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Task::TaskType type = Task::Error;
        const QString level = m_mocRegExp.cap(4);
        if (level.compare(QLatin1String("Warning"), Qt::CaseInsensitive) == 0)
            type = Task::Warning;
        if (level.compare(QLatin1String("Note"), Qt::CaseInsensitive) == 0)
            type = Task::Unknown;
        Task task(type, m_mocRegExp.cap(5).trimmed() /* description */,
                  Utils::FileName::fromUserInput(m_mocRegExp.cap(1)) /* filename */,
                  lineno,
                  ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }
    if (m_translationRegExp.indexIn(lne) > -1) {
        Task::TaskType type = Task::Warning;
        if (m_translationRegExp.cap(1) == QLatin1String("Error"))
            type = Task::Error;
        Task task(type, m_translationRegExp.cap(2),
                  Utils::FileName::fromUserInput(m_translationRegExp.cap(3)) /* filename */,
                  -1,
                  ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }
    IOutputParser::stdError(line);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qtsupportplugin.h"
#   include <projectexplorer/projectexplorerconstants.h>
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
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("moc warning")
            << QString::fromLatin1("..\\untitled\\errorfile.h:0: Warning: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       Utils::FileName::fromUserInput(QLatin1String("..\\untitled\\errorfile.h")), 0,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("moc warning 2")
            << QString::fromLatin1("c:\\code\\test.h(96): Warning: Property declaration ) has no READ accessor function. The property will be invalid.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("Property declaration ) has no READ accessor function. The property will be invalid."),
                                                       Utils::FileName::fromUserInput(QLatin1String("c:\\code\\test.h")), 96,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Unknown,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       Utils::FileName::fromUserInput(QLatin1String("/home/qtwebkithelpviewer.h")), 0,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("ninja with moc")
            << QString::fromLatin1("E:/sandbox/creator/loaden/src/libs/utils/iwelcomepage.h(54): Error: Undefined interface")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Error,
                                                       QLatin1String("Undefined interface"),
                                                       Utils::FileName::fromUserInput(QLatin1String("E:/sandbox/creator/loaden/src/libs/utils/iwelcomepage.h")), 54,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("translation")
            << QString::fromLatin1("Warning: dropping duplicate messages in '/some/place/qtcreator_fr.qm'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("dropping duplicate messages"),
                                                       Utils::FileName::fromUserInput(QLatin1String("/some/place/qtcreator_fr.qm")), -1,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
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
