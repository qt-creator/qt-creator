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

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {

QMakeParser::QMakeParser() : m_error(QLatin1String("^(.+?):(\\d+?):\\s(.+?)$"))
{
    setObjectName(QLatin1String("QMakeParser"));
}

OutputLineParser::Result QMakeParser::handleLine(const QString &line, OutputFormat type)
{
    if (type != Utils::StdErrFormat)
        return Status::NotHandled;
    QString lne = rightTrimmed(line);
    QRegularExpressionMatch match = m_error.match(lne);
    if (match.hasMatch()) {
        QString fileName = match.captured(1);
        Task::TaskType type = Task::Error;
        const QString description = match.captured(3);
        int fileNameOffset = match.capturedStart(1);
        if (fileName.startsWith(QLatin1String("WARNING: "))) {
            type = Task::Warning;
            fileName = fileName.mid(9);
            fileNameOffset += 9;
        } else if (fileName.startsWith(QLatin1String("ERROR: "))) {
            fileName = fileName.mid(7);
            fileNameOffset += 7;
        }
        if (description.startsWith(QLatin1String("note:"), Qt::CaseInsensitive))
            type = Task::Unknown;
        else if (description.startsWith(QLatin1String("warning:"), Qt::CaseInsensitive))
            type = Task::Warning;
        else if (description.startsWith(QLatin1String("error:"), Qt::CaseInsensitive))
            type = Task::Error;

        BuildSystemTask t(type, description, absoluteFilePath(FilePath::fromUserInput(fileName)),
                          match.captured(2).toInt() /* line */);
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, t.file, t.line, fileNameOffset,
                                       fileName.length());
        scheduleTask(t, 1);
        return {Status::Done, linkSpecs};
    }
    if (lne.startsWith(QLatin1String("Project ERROR: "))
            || lne.startsWith(QLatin1String("ERROR: "))) {
        const QString description = lne.mid(lne.indexOf(QLatin1Char(':')) + 2);
        scheduleTask(BuildSystemTask(Task::Error, description), 1);
        return Status::Done;
    }
    if (lne.startsWith(QLatin1String("Project WARNING: "))
            || lne.startsWith(QLatin1String("WARNING: "))) {
        const QString description = lne.mid(lne.indexOf(QLatin1Char(':')) + 2);
        scheduleTask(BuildSystemTask(Task::Warning, description), 1);
        return Status::Done;
    }
    return Status::NotHandled;
}

} // QmakeProjectManager

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qmakeprojectmanagerplugin.h"

#   include "projectexplorer/outputparser_test.h"

using namespace QmakeProjectManager::Internal;

namespace QmakeProjectManager {

void QmakeProjectManagerPlugin::testQmakeOutputParsers_data()
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

    QTest::newRow("qMake error")
            << QString::fromLatin1("Project ERROR: undefined file")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "undefined file"))
            << QString();

    QTest::newRow("qMake Parse Error")
            << QString::fromLatin1("e:\\project.pro:14: Parse Error ('sth odd')")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Parse Error ('sth odd')",
                                   FilePath::fromUserInput("e:\\project.pro"),
                                   14))
            << QString();

    QTest::newRow("qMake warning")
            << QString::fromLatin1("Project WARNING: bearer module might require ReadUserData capability")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "bearer module might require ReadUserData capability"))
            << QString();

    QTest::newRow("qMake warning 2")
            << QString::fromLatin1("WARNING: Failure to find: blackberrycreatepackagestepconfigwidget.cpp")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "Failure to find: blackberrycreatepackagestepconfigwidget.cpp"))
            << QString();

    QTest::newRow("qMake warning with location")
            << QString::fromLatin1("WARNING: e:\\QtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl:1: Unescaped backslashes are deprecated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "Unescaped backslashes are deprecated.",
                                   FilePath::fromUserInput("e:\\QtSDK\\Simulator\\Qt\\msvc2008\\lib\\qtmaind.prl"), 1))
            << QString();

    QTest::newRow("moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << BuildSystemTask(Task::Unknown,
                        "Note: No relevant classes found. No output generated.",
                        FilePath::fromUserInput("/home/qtwebkithelpviewer.h"), -1))
            << QString();
}

void QmakeProjectManagerPlugin::testQmakeOutputParsers()
{
    OutputParserTester testbench;
    testbench.addLineParser(new QMakeParser);
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

} // QmakeProjectManager

#endif
