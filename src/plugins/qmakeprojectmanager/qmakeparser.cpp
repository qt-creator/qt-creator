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

#include "qmakeparser.h"

#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>

using namespace QmakeProjectManager;
using ProjectExplorer::Task;

QMakeParser::QMakeParser() : m_error(QLatin1String("^(.+):(\\d+):\\s(.+)$"))
{
    setObjectName(QLatin1String("QMakeParser"));
    m_error.setMinimal(true);
}

void QMakeParser::stdError(const QString &line)
{
    QString lne = rightTrimmed(line);
    if (m_error.indexIn(lne) > -1) {
        QString fileName = m_error.cap(1);
        Task::TaskType type = Task::Error;
        const QString description = m_error.cap(3);
        if (fileName.startsWith(QLatin1String("WARNING: "))) {
            type = Task::Warning;
            fileName = fileName.mid(9);
        } else if (fileName.startsWith(QLatin1String("ERROR: "))) {
            fileName = fileName.mid(7);
        }
        if (description.startsWith(QLatin1String("note:"), Qt::CaseInsensitive))
            type = Task::Unknown;
        else if (description.startsWith(QLatin1String("warning:"), Qt::CaseInsensitive))
            type = Task::Warning;
        else if (description.startsWith(QLatin1String("error:"), Qt::CaseInsensitive))
            type = Task::Error;
        Task task = Task(type,
                         description,
                         Utils::FileName::fromUserInput(fileName),
                         m_error.cap(2).toInt() /* line */,
                         Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        emit addTask(task, 1);
        return;
    }
    if (lne.startsWith(QLatin1String("Project ERROR: "))
            || lne.startsWith(QLatin1String("ERROR: "))) {
        const QString description = lne.mid(lne.indexOf(QLatin1Char(':')) + 2);
        Task task = Task(Task::Error,
                         description,
                         Utils::FileName() /* filename */,
                         -1 /* linenumber */,
                         Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        emit addTask(task, 1);
        return;
    }
    if (lne.startsWith(QLatin1String("Project WARNING: "))
            || lne.startsWith(QLatin1String("WARNING: "))) {
        const QString description = lne.mid(lne.indexOf(QLatin1Char(':')) + 2);
        Task task = Task(Task::Warning,
                         description,
                         Utils::FileName() /* filename */,
                         -1 /* linenumber */,
                         Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        emit addTask(task, 1);
        return;
    }
    IOutputParser::stdError(line);
}


// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qmakeprojectmanagerplugin.h"

#   include "projectexplorer/outputparser_test.h"

using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;

void QmakeProjectManagerPlugin::testQmakeOutputParsers_data()
{
    const Core::Id categoryBuildSystem = Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
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

    QTest::newRow("qMake error")
            << QString::fromLatin1("Project ERROR: undefined file")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("undefined file"),
                        Utils::FileName(), -1,
                        categoryBuildSystem))
            << QString();

    QTest::newRow("qMake Parse Error")
            << QString::fromLatin1("e:\\project.pro:14: Parse Error ('sth odd')")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Parse Error ('sth odd')"),
                        Utils::FileName::fromUserInput(QLatin1String("e:\\project.pro")),
                        14,
                        categoryBuildSystem))
            << QString();

    QTest::newRow("qMake warning")
            << QString::fromLatin1("Project WARNING: bearer module might require ReadUserData capability")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("bearer module might require ReadUserData capability"),
                        Utils::FileName(), -1,
                        categoryBuildSystem))
            << QString();

    QTest::newRow("qMake warning 2")
            << QString::fromLatin1("WARNING: Failure to find: blackberrycreatepackagestepconfigwidget.cpp")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("Failure to find: blackberrycreatepackagestepconfigwidget.cpp"),
                        Utils::FileName(), -1,
                        categoryBuildSystem))
            << QString();

    QTest::newRow("qMake warning with location")
            << QString::fromLatin1("WARNING: e:\\QtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl:1: Unescaped backslashes are deprecated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("Unescaped backslashes are deprecated."),
                        Utils::FileName::fromUserInput(QLatin1String("e:\\QtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl")), 1,
                        categoryBuildSystem))
            << QString();
    QTest::newRow("moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("Note: No relevant classes found. No output generated."),
                        Utils::FileName::fromUserInput(QLatin1String("/home/qtwebkithelpviewer.h")), 0,
                        categoryBuildSystem)
                )
            << QString();}

void QmakeProjectManagerPlugin::testQmakeOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new QMakeParser);
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
