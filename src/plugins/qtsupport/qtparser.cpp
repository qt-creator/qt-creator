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

#include <QFileInfo>

#ifdef WITH_TESTS
#include "qtsupportplugin.h"
#include <projectexplorer/outputparser_test.h>
#include <QTest>
#endif

using namespace ProjectExplorer;

namespace QtSupport {

// opt. drive letter + filename: (2 brackets)
#define FILE_PATTERN "^(([A-Za-z]:)?[^:]+\\.[^:]+)"

QtParser::QtParser() :
    m_mocRegExp(QLatin1String(FILE_PATTERN"[:\\(](\\d+?)\\)?:\\s([Ww]arning|[Ee]rror|[Nn]ote):\\s(.+?)$")),
    m_translationRegExp(QLatin1String("^([Ww]arning|[Ee]rror):\\s+(.*?) in '(.*?)'$"))
{
    setObjectName(QLatin1String("QtParser"));
}

Utils::OutputLineParser::Result QtParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    if (type != Utils::StdErrFormat)
        return Status::NotHandled;

    QString lne = rightTrimmed(line);
    QRegularExpressionMatch match = m_mocRegExp.match(lne);
    if (match.hasMatch()) {
        bool ok;
        int lineno = match.captured(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Task::TaskType type = Task::Error;
        const QString level = match.captured(4);
        if (level.compare(QLatin1String("Warning"), Qt::CaseInsensitive) == 0)
            type = Task::Warning;
        if (level.compare(QLatin1String("Note"), Qt::CaseInsensitive) == 0)
            type = Task::Unknown;
        LinkSpecs linkSpecs;
        const Utils::FilePath file
                = absoluteFilePath(Utils::FilePath::fromUserInput(match.captured(1)));
        addLinkSpecForAbsoluteFilePath(linkSpecs, file, lineno, match, 1);
        CompileTask task(type, match.captured(5).trimmed() /* description */, file, lineno);
        scheduleTask(task, 1);
        return {Status::Done, linkSpecs};
    }
    match = m_translationRegExp.match(line);
    if (match.hasMatch()) {
        Task::TaskType type = Task::Warning;
        if (match.captured(1) == QLatin1String("Error"))
            type = Task::Error;
        LinkSpecs linkSpecs;
        const Utils::FilePath file
                = absoluteFilePath(Utils::FilePath::fromUserInput(match.captured(3)));
        addLinkSpecForAbsoluteFilePath(linkSpecs, file, 0, match, 3);
        CompileTask task(type, match.captured(2), file);
        scheduleTask(task, 1);
        return {Status::Done, linkSpecs};
    }
    return Status::NotHandled;
}

// Unit tests:

#ifdef WITH_TESTS
namespace Internal {

void QtSupportPlugin::testQtOutputParser_data()
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
            << Tasks()
            << QString();
    QTest::newRow("qdoc warning")
            << QString::fromLatin1("/home/user/dev/qt5/qtscript/src/script/api/qscriptengine.cpp:295: warning: Can't create link to 'Object Trees & Ownership'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                                       QLatin1String("Can't create link to 'Object Trees & Ownership'"),
                                                       Utils::FilePath::fromUserInput(QLatin1String("/home/user/dev/qt5/qtscript/src/script/api/qscriptengine.cpp")), 295))
            << QString();
    QTest::newRow("moc warning")
            << QString::fromLatin1("..\\untitled\\errorfile.h:0: Warning: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       Utils::FilePath::fromUserInput(QLatin1String("..\\untitled\\errorfile.h")), -1))
            << QString();
    QTest::newRow("moc warning 2")
            << QString::fromLatin1("c:\\code\\test.h(96): Warning: Property declaration ) has no READ accessor function. The property will be invalid.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                                       QLatin1String("Property declaration ) has no READ accessor function. The property will be invalid."),
                                                       Utils::FilePath::fromUserInput(QLatin1String("c:\\code\\test.h")), 96))
            << QString();
    QTest::newRow("moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Unknown,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       Utils::FilePath::fromUserInput(QLatin1String("/home/qtwebkithelpviewer.h")), -1))
            << QString();
    QTest::newRow("ninja with moc")
            << QString::fromLatin1("E:/sandbox/creator/loaden/src/libs/utils/iwelcomepage.h(54): Error: Undefined interface")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Error,
                                                       QLatin1String("Undefined interface"),
                                                       Utils::FilePath::fromUserInput(QLatin1String("E:/sandbox/creator/loaden/src/libs/utils/iwelcomepage.h")), 54))
            << QString();
    QTest::newRow("translation")
            << QString::fromLatin1("Warning: dropping duplicate messages in '/some/place/qtcreator_fr.qm'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                                       QLatin1String("dropping duplicate messages"),
                                                       Utils::FilePath::fromUserInput(QLatin1String("/some/place/qtcreator_fr.qm")), -1))
            << QString();
}

void QtSupportPlugin::testQtOutputParser()
{
    OutputParserTester testbench;
    testbench.addLineParser(new QtParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines, outputLines);
}

} // namespace Internal
#endif

} // namespace QtSupport
