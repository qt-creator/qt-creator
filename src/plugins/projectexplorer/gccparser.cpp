/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gccparser.h"
#include "projectexplorerconstants.h"
#include "taskwindow.h"

using namespace ProjectExplorer;

GccParser::GccParser()
{
    m_regExp.setPattern("^([^\\(\\)]+[^\\d]):(\\d+):(\\d+:)*(\\s(warning|error):)?\\s(.+)$");
    m_regExp.setMinimal(true);

    m_regExpIncluded.setPattern("^.*from\\s([^:]+):(\\d+)(,|:)$");
    m_regExpIncluded.setMinimal(true);

    m_regExpLinker.setPattern("^(\\S*)\\(\\S+\\):\\s(.+)$");
    m_regExpLinker.setMinimal(true);
}

void GccParser::stdError(const QString &line)
{
    QString lne = line.trimmed();
    if (m_regExpLinker.indexIn(lne) > -1) {
        QString description = m_regExpLinker.cap(2);
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      description,
                                      m_regExpLinker.cap(1) /* filename */,
                                      -1 /* linenumber */,
                                      Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (m_regExp.indexIn(lne) > -1) {
        TaskWindow::Task task(TaskWindow::Unknown,
                              m_regExp.cap(6) /* description */,
                              m_regExp.cap(1) /* filename */,
                              m_regExp.cap(2).toInt() /* line number */,
                              Constants::TASK_CATEGORY_COMPILE);
        if (m_regExp.cap(5) == "warning")
            task.type = TaskWindow::Warning;
        else if (m_regExp.cap(5) == "error")
            task.type = TaskWindow::Error;

        emit addTask(task);
        return;
    } else if (m_regExpIncluded.indexIn(lne) > -1) {
        emit addTask(TaskWindow::Task(TaskWindow::Unknown,
                                      lne /* description */,
                                      m_regExpIncluded.cap(1) /* filename */,
                                      m_regExpIncluded.cap(2).toInt() /* linenumber */,
                                      Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (lne.startsWith(QLatin1String("collect2:")) ||
               lne.startsWith(QLatin1String("ERROR:")) ||
               lne == QLatin1String("* cpp failed")) {
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      lne /* description */,
                                      QString() /* filename */,
                                      -1 /* linenumber */,
                                      Constants::TASK_CATEGORY_COMPILE));
        return;
    }
    IOutputParser::stdError(line);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "metatypedeclarations.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testGccOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::TaskWindow::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext") << QString()
            << QList<ProjectExplorer::TaskWindow::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext")
            << QList<ProjectExplorer::TaskWindow::Task>()
            << QString();

    QTest::newRow("GCCE error")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "/temp/test/untitled8/main.cpp:9: error: `sfasdf' undeclared (first use this function)\n"
                                   "/temp/test/untitled8/main.cpp:9: error: (Each undeclared identifier is reported only once for each function it appears in.)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>()
                << TaskWindow::Task(TaskWindow::Unknown,
                                    QLatin1String("In function `int main(int, char**)':"),
                                    QLatin1String("/temp/test/untitled8/main.cpp"), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                << TaskWindow::Task(TaskWindow::Error,
                                    QLatin1String("`sfasdf' undeclared (first use this function)"),
                                    QLatin1String("/temp/test/untitled8/main.cpp"), 9,
                                    Constants::TASK_CATEGORY_COMPILE)
                << TaskWindow::Task(TaskWindow::Error,
                                    QLatin1String("(Each undeclared identifier is reported only once for each function it appears in.)"),
                                    QLatin1String("/temp/test/untitled8/main.cpp"), 9,
                                    Constants::TASK_CATEGORY_COMPILE)
                )
            << QString();
    QTest::newRow("GCCE warning")
            << QString::fromLatin1("/src/corelib/global/qglobal.h:1635: warning: inline function `QDebug qDebug()' used but never defined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>()
                << TaskWindow::Task(TaskWindow::Warning,
                                    QLatin1String("inline function `QDebug qDebug()' used but never defined"),
                                    QLatin1String("/src/corelib/global/qglobal.h"), 1635,
                                    Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("warning")
            << QString::fromLatin1("main.cpp:7:2: warning: Some warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>() << TaskWindow::Task(TaskWindow::Warning,
                                                              QLatin1String("Some warning"),
                                                              QLatin1String("main.cpp"), 7,
                                                              Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("GCCE #error")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:7: #error Symbian error")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>() << TaskWindow::Task(TaskWindow::Error,
                                                              QLatin1String("#error Symbian error"),
                                                              QLatin1String("C:\\temp\\test\\untitled8\\main.cpp"), 7,
                                                              Constants::TASK_CATEGORY_COMPILE))
            << QString();
    // Symbian reports #warning(s) twice (using different syntax).
    QTest::newRow("GCCE #warning1")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:8: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>() << TaskWindow::Task(TaskWindow::Warning,
                                                              QLatin1String("#warning Symbian warning"),
                                                              QLatin1String("C:\\temp\\test\\untitled8\\main.cpp"), 8,
                                                              Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("GCCE #warning2")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp:8:2: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>() << TaskWindow::Task(TaskWindow::Warning,
                                                              QLatin1String("#warning Symbian warning"),
                                                              QLatin1String("/temp/test/untitled8/main.cpp"), 8,
                                                              Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("Undefined reference (debug)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>()
                << TaskWindow::Task(TaskWindow::Unknown,
                                    QLatin1String("In function `main':"),
                                    QLatin1String("main.o"), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                << TaskWindow::Task(TaskWindow::Error,
                                    QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                                    QLatin1String("C:\\temp\\test\\untitled8/main.cpp"), 8,
                                    Constants::TASK_CATEGORY_COMPILE)
                << TaskWindow::Task(TaskWindow::Error,
                                    QLatin1String("collect2: ld returned 1 exit status"),
                                    QString(), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                )
            << QString();
    QTest::newRow("Undefined reference (release)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::TaskWindow::Task>()
                << TaskWindow::Task(TaskWindow::Unknown,
                                    QLatin1String("In function `main':"),
                                    QLatin1String("main.o"), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                << TaskWindow::Task(TaskWindow::Error,
                                    QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                                    QLatin1String("C:\\temp\\test\\untitled8/main.cpp"), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                << TaskWindow::Task(TaskWindow::Error,
                                    QLatin1String("collect2: ld returned 1 exit status"),
                                    QString(), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                )
            << QString();
}

void ProjectExplorerPlugin::testGccOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new GccParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<TaskWindow::Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}
#endif
