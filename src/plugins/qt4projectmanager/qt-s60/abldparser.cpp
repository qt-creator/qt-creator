/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "abldparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace Qt4ProjectManager;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

AbldParser::AbldParser() :
    m_currentLine(-1),
    m_waitingForStdErrContinuation(false),
    m_waitingForStdOutContinuation(false)
{
    setObjectName(QLatin1String("AbldParser"));
    m_perlIssue.setPattern(QLatin1String("^(WARNING|ERROR):\\s([^\\(\\)]+[^\\d])\\((\\d+)\\) : (.+)$"));
    m_perlIssue.setMinimal(true);
}

void AbldParser::stdOutput(const QString &line)
{
    m_waitingForStdErrContinuation = false;

    QString lne = line.trimmed();
    // possible ABLD.bat errors:
    if (lne.startsWith(QLatin1String("Is Perl, version "))) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }
    if (lne.startsWith(QLatin1String("FATAL ERROR:")) ||
        lne.startsWith(QLatin1String("Error :"))) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        m_waitingForStdOutContinuation = false;
        return;
    }

    if (m_perlIssue.indexIn(lne) > -1) {
        m_waitingForStdOutContinuation = true;
        m_currentFile = m_perlIssue.cap(2);
        m_currentLine = m_perlIssue.cap(3).toInt();

        Task task(Task::Unknown,
                  m_perlIssue.cap(4) /* description */,
                  Utils::FileName::fromUserInput(m_currentFile), m_currentLine,
                  Core::Id(TASK_CATEGORY_BUILDSYSTEM));

        if (m_perlIssue.cap(1) == QLatin1String("WARNING"))
            task.type = Task::Warning;
        else if (m_perlIssue.cap(1) == QLatin1String("ERROR"))
            task.type = Task::Error;

        emit addTask(task);
        return;
    }

    if (lne.startsWith(QLatin1String("SIS creation failed!"))) {
        m_waitingForStdOutContinuation = false;
        emit addTask(Task(Task::Error,
                          line, Utils::FileName(), -1,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }

    if (lne.isEmpty()) {
        m_waitingForStdOutContinuation = false;
        return;
    }

    if (m_waitingForStdOutContinuation) {
        emit addTask(Task(Task::Unknown,
                          lne /* description */,
                          Utils::FileName::fromUserInput(m_currentFile), m_currentLine,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        m_waitingForStdOutContinuation = true;
        return;
    }
    IOutputParser::stdOutput(line);
}

void AbldParser::stdError(const QString &line)
{
    m_waitingForStdOutContinuation = false;

    QString lne = line.trimmed();

    // possible abld.pl errors:
    if (lne.startsWith(QLatin1String("ABLD ERROR:")) ||
        lne.startsWith(QLatin1String("This project does not support ")) ||
        lne.startsWith(QLatin1String("Platform "))) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }

    if (lne.startsWith(QLatin1String("Died at "))) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        m_waitingForStdErrContinuation = false;
        return;
    }

    if (lne.startsWith(QLatin1String("MMPFILE \""))) {
        m_currentFile = lne.mid(9, lne.size() - 10);
        m_waitingForStdErrContinuation = false;
        return;
    }
    if (lne.isEmpty()) {
        m_waitingForStdErrContinuation = false;
        return;
    }
    if (lne.startsWith(QLatin1String("WARNING: "))) {
        QString description = lne.mid(9);
        emit addTask(Task(Task::Warning, description,
                          Utils::FileName::fromUserInput(m_currentFile),
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        m_waitingForStdErrContinuation = true;
        return;
    }
    if (lne.startsWith(QLatin1String("ERROR: "))) {
        QString description = lne.mid(7);
        emit addTask(Task(Task::Error, description,
                          Utils::FileName::fromUserInput(m_currentFile),
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        m_waitingForStdErrContinuation = true;
        return;
    }
    if (m_waitingForStdErrContinuation)
    {
        emit addTask(Task(Task::Unknown,
                          lne /* description */,
                          Utils::FileName::fromUserInput(m_currentFile),
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_BUILDSYSTEM)));
        m_waitingForStdErrContinuation = true;
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

void Qt4ProjectManagerPlugin::testAbldOutputParsers_data()
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
}

void Qt4ProjectManagerPlugin::testAbldOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new AbldParser);
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
