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

#include "qmakeparser.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::Task;

QMakeParser::QMakeParser() : m_error(QLatin1String("^(.+):(\\d+):\\s(.+)$"))
{
    setObjectName(QLatin1String("QMakeParser"));
    m_error.setMinimal(true);
}

void QMakeParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (lne.startsWith(QLatin1String("Project ERROR:"))) {
        const QString description = lne.mid(15);
        emit addTask(Task(Task::Error,
                          description,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }
    if (lne.startsWith(QLatin1String("Project WARNING:"))) {
        const QString description = lne.mid(17);
        emit addTask(Task(Task::Warning,
                          description,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }
    if (m_error.indexIn(lne) > -1) {
        QString fileName = m_error.cap(1);
        Task::TaskType type = Task::Error;
        if (fileName.startsWith(QLatin1String("WARNING: "))) {
            type = Task::Warning;
            fileName = fileName.mid(9);
        } else if (fileName.startsWith(QLatin1String("ERROR: "))) {
            fileName = fileName.mid(7);
        }
        emit addTask(Task(type,
                          m_error.cap(3) /* description */,
                          Utils::FileName::fromUserInput(fileName),
                          m_error.cap(2).toInt() /* line */,
                          Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }
    IOutputParser::stdError(line);
}


// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qt4projectmanagerplugin.h"

#   include "projectexplorer/outputparser_test.h"

using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

void Qt4ProjectManagerPlugin::testQmakeOutputParsers_data()
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

    QTest::newRow("qMake warning with location")
            << QString::fromLatin1("WARNING: e:\\NokiaQtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl:1: Unescaped backslashes are deprecated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("Unescaped backslashes are deprecated."),
                        Utils::FileName::fromUserInput(QLatin1String("e:\\NokiaQtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl")), 1,
                        categoryBuildSystem))
            << QString();
}

void Qt4ProjectManagerPlugin::testQmakeOutputParsers()
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
