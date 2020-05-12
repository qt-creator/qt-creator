/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qttestparser.h"

#include "qtoutputformatter.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#ifdef WITH_TESTS
#include "qtsupportplugin.h"
#include <projectexplorer/outputparser_test.h>
#include <QTest>
#endif // WITH_TESTS

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {
namespace Internal {

OutputLineParser::Result QtTestParser::handleLine(const QString &line, OutputFormat type)
{
    if (type != StdOutFormat)
        return Status::NotHandled;

    const QString theLine = rightTrimmed(line);
    static const QRegularExpression triggerPattern("^(?:XPASS|FAIL!)  : .+$");
    QTC_CHECK(triggerPattern.isValid());
    if (triggerPattern.match(theLine).hasMatch()) {
        emitCurrentTask();
        m_currentTask = Task(Task::Error, theLine, FilePath(), -1,
                             Constants::TASK_CATEGORY_AUTOTEST);
        return Status::InProgress;
    }
    if (m_currentTask.isNull())
        return Status::NotHandled;
    static const QRegularExpression locationPattern(HostOsInfo::isWindowsHost()
        ? QString(QT_TEST_FAIL_WIN_REGEXP)
        : QString(QT_TEST_FAIL_UNIX_REGEXP));
    QTC_CHECK(locationPattern.isValid());
    const QRegularExpressionMatch match = locationPattern.match(theLine);
    if (match.hasMatch()) {
        LinkSpecs linkSpecs;
        m_currentTask.file = absoluteFilePath(FilePath::fromString(
                    QDir::fromNativeSeparators(match.captured("file"))));
        m_currentTask.line = match.captured("line").toInt();
        addLinkSpecForAbsoluteFilePath(linkSpecs, m_currentTask.file, m_currentTask.line, match,
                                       "file");
        emitCurrentTask();
        return {Status::Done, linkSpecs};
    }
    m_currentTask.details.append(theLine);
    return Status::InProgress;
}

void QtTestParser::emitCurrentTask()
{
    if (!m_currentTask.isNull()) {
        scheduleTask(m_currentTask, 1);
        m_currentTask.clear();
    }
}

#ifdef WITH_TESTS
void QtSupportPlugin::testQtTestOutputParser()
{
    OutputParserTester testbench;
    testbench.addLineParser(new QtTestParser);
    const QString input = "random output\n"
            "PASS   : MyTest::someTest()\n"
            "XPASS  : MyTest::someTest()\n"
#ifdef Q_OS_WIN
            "C:\\dev\\tests\\tst_mytest.cpp(154) : failure location\n"
#else
            "   Loc: [/home/me/tests/tst_mytest.cpp(154)]\n"
#endif
            "FAIL!  : MyTest::someOtherTest(init) Compared values are not the same\n"
            "   Actual   (exceptionCaught): 0\n"
            "   Expected (true)           : 1\n"
#ifdef Q_OS_WIN
            "C:\\dev\\tests\\tst_mytest.cpp(220) : failure location\n"
#else
            "   Loc: [/home/me/tests/tst_mytest.cpp(220)]\n"
#endif
            "XPASS: irrelevant\n"
            "PASS   : MyTest::anotherTest()";
    const QString expectedChildOutput =
            "random output\n"
            "PASS   : MyTest::someTest()\n"
            "XPASS: irrelevant\n"
            "PASS   : MyTest::anotherTest()\n";
    const FilePath theFile = FilePath::fromString(HostOsInfo::isWindowsHost()
        ? QString("C:/dev/tests/tst_mytest.cpp") : QString("/home/me/tests/tst_mytest.cpp"));
    const Tasks expectedTasks{
        Task(Task::Error, "XPASS  : MyTest::someTest()", theFile, 154,
             Constants::TASK_CATEGORY_AUTOTEST),
        Task(Task::Error, "FAIL!  : MyTest::someOtherTest(init) "
                          "Compared values are not the same\n"
                          "   Actual   (exceptionCaught): 0\n"
                          "   Expected (true)           : 1",
             theFile, 220, Constants::TASK_CATEGORY_AUTOTEST)};
    testbench.testParsing(input, OutputParserTester::STDOUT, expectedTasks, expectedChildOutput,
                          QString(), QString());
}
#endif // WITH_TESTS

} // namespace Internal
} // namespace QtSupport
