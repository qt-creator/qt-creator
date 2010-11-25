/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmakeparser.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/taskwindow.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>
#include <QDir>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::Task;

QMakeParser::QMakeParser()
{
    setObjectName(QLatin1String("QMakeParser"));

    m_error.setPattern("^(.+):(\\d+):\\s(.+)$");
    m_error.setMinimal(true);
}

void QMakeParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (lne.startsWith(QLatin1String("Project ERROR:"))) {
        const QString description = lne.mid(15);
        emit addTask(Task(Task::Error,
                          description,
                          QString() /* filename */,
                          -1 /* linenumber */,
                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        return;
    }
    if (lne.startsWith(QLatin1String("Project WARNING:"))) {
        const QString description = lne.mid(17);
        emit addTask(Task(Task::Warning,
                          description,
                          QString() /* filename */,
                          -1 /* linenumber */,
                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        return;
    }
    if (m_error.indexIn(lne) > -1) {
        QString fileName = QDir::fromNativeSeparators(m_error.cap(1));
        Task::TaskType type = Task::Error;
        if (fileName.startsWith("WARNING: ")) {
            type = Task::Warning;
            fileName = fileName.mid(9);
        } else if (fileName.startsWith("ERROR: ")) {
            fileName = fileName.mid(7);
        }
        emit addTask(Task(type,
                          m_error.cap(3) /* description */,
                          fileName,
                          m_error.cap(2).toInt() /* line */,
                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
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
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext")
            << QList<ProjectExplorer::Task>()
            << QString();

    QTest::newRow("qMake error")
            << QString::fromLatin1("Project ERROR: undefined file")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("undefined file"),
                        QString(), -1,
                        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString();

    QTest::newRow("qMake Parse Error")
            << QString::fromLatin1("e:\\project.pro:14: Parse Error ('sth odd')")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Parse Error ('sth odd')"),
                        QDir::fromNativeSeparators(QLatin1String("e:\\project.pro")),
                        14,
                        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString();

    QTest::newRow("qMake warning")
            << QString::fromLatin1("Project WARNING: bearer module might require ReadUserData capability")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("bearer module might require ReadUserData capability"),
                        QString(), -1,
                        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString();

    QTest::newRow("qMake warning with location")
            << QString::fromLatin1("WARNING: e:\\NokiaQtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl:1: Unescaped backslashes are deprecated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("Unescaped backslashes are deprecated."),
                        QLatin1String("e:/NokiaQtSDK/Simulator/Qt/msvc2008/lib/qtmaind.prl"), 1,
                        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM))
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
