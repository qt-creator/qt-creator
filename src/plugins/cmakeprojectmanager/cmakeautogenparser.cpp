// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeautogenparser.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

const char COMMON_ERROR_PATTERN[] = "^(AutoMoc|AUTOMOC|AutoUic).*error.*$";
const char COMMON_WARNING_PATTERN[] = "^(AutoMoc|AUTOMOC|AutoUic).*warning.*$";
const char COMMON_SEPARATOR_PATTERN[] = "^[-]+$";

CMakeAutogenParser::CMakeAutogenParser()
{
    m_commonError.setPattern(COMMON_ERROR_PATTERN);
    QTC_CHECK(m_commonError.isValid());

    m_commonWarning.setPattern(COMMON_WARNING_PATTERN);
    QTC_CHECK(m_commonWarning.isValid());

    m_separatorLine.setPattern(COMMON_SEPARATOR_PATTERN);
    QTC_CHECK(m_separatorLine.isValid());
}

OutputLineParser::Result CMakeAutogenParser::handleLine(const QString &line, OutputFormat /*type*/)
{
    QRegularExpressionMatch match;
    QString trimmedLine = rightTrimmed(line);
    switch (m_expectedState) {
    case NONE: {
        match = m_commonError.match(trimmedLine);
        if (match.hasMatch()) {
            m_lastTask = BuildSystemTask(Task::Error, match.captured());
            m_lines = 1;

            m_expectedState = LINE_SEPARATOR;
            return Status::InProgress;
        }
        match = m_commonWarning.match(trimmedLine);
        if (match.hasMatch()) {
            m_lastTask = BuildSystemTask(Task::Warning, match.captured());
            m_lines = 1;

            m_expectedState = LINE_SEPARATOR;
            return Status::InProgress;
        }
        return Status::NotHandled;
    }
    case LINE_SEPARATOR: {
        match = m_separatorLine.match(trimmedLine);
        m_expectedState = LINE_DESCRIPTION;
        if (!match.hasMatch())
            m_lastTask.details.append(trimmedLine);

        return Status::InProgress;
    }
    case LINE_DESCRIPTION: {
        if (trimmedLine.isEmpty() && !m_lastTask.isNull()) {
            m_expectedState = NONE;

            flush();
            return Status::Done;
        }
        m_lastTask.details.append(trimmedLine);

        return Status::InProgress;
    }
    }

    return Status::NotHandled;
}

void CMakeAutogenParser::flush()
{
    if (m_lastTask.isNull())
        return;

    Task t = m_lastTask;
    m_lastTask.clear();

    if (t.summary.isEmpty() && !t.details.isEmpty())
        t.summary = t.details.takeFirst();
    m_lines += t.details.count();

    scheduleTask(t, m_lines, 1);
    m_lines = 0;
}

} // namespace CMakeProjectManager

#ifdef WITH_TESTS

#include <projectexplorer/outputparser_test.h>

#include <QTest>

namespace CMakeProjectManager::Internal {

class CMakeAutogenParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCMakeAutogenParser_data();
    void testCMakeAutogenParser();
};

void CMakeAutogenParserTest::testCMakeAutogenParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<Tasks>("tasks");
    QTest::addColumn<QString>("outputLines");

    // negative tests
    QTest::newRow("pass-through stdout")
        << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
        << QString::fromLatin1("Sometext\n") << QString() << Tasks() << QString();
    QTest::newRow("pass-through stderr")
        << QString::fromLatin1("Sometext") << OutputParserTester::STDERR << QString()
        << QString::fromLatin1("Sometext\n") << Tasks() << QString();

    // positive tests
    QTest::newRow("AutoMoc error") << R"(AutoMoc error
-------------
"SRC:/main.cpp"
contains a "Q_OBJECT" macro, but does not include "main.moc"!
Consider to
  - add #include "main.moc"
  - enable SKIP_AUTOMOC for this file)"
                                   << OutputParserTester::STDERR << QString() << QString()
                                   << (Tasks() << BuildSystemTask(
                                           Task::Error,
                                           R"(AutoMoc error
"SRC:/main.cpp"
contains a "Q_OBJECT" macro, but does not include "main.moc"!
Consider to
  - add #include "main.moc"
  - enable SKIP_AUTOMOC for this file)"))
                                   << QString();

    QTest::newRow("AutoMoc subprocess error") << R"(AutoMoc subprocess error
------------------------
The moc process failed to compile
  "BIN:/src/quickcontrols/basic/impl/qtquickcontrols2basicstyleimplplugin_QtQuickControls2BasicStyleImplPlugin.cpp"
into
  "BIN:/src/quickcontrols/basic/impl/qtquickcontrols2basicstyleimplplugin_autogen/include/qtquickcontrols2basicstyleimplplugin_QtQuickControls2BasicStyleImplPlugin.moc"
included by
  "BIN:/src/quickcontrols/basic/impl/qtquickcontrols2basicstyleimplplugin_QtQuickControls2BasicStyleImplPlugin.cpp"
Process failed with return value 1)" << OutputParserTester::STDERR
                                              << QString() << QString()
                                              << (Tasks() << BuildSystemTask(
                                                      Task::Error,
                                                      R"(AutoMoc subprocess error
The moc process failed to compile
  "BIN:/src/quickcontrols/basic/impl/qtquickcontrols2basicstyleimplplugin_QtQuickControls2BasicStyleImplPlugin.cpp"
into
  "BIN:/src/quickcontrols/basic/impl/qtquickcontrols2basicstyleimplplugin_autogen/include/qtquickcontrols2basicstyleimplplugin_QtQuickControls2BasicStyleImplPlugin.moc"
included by
  "BIN:/src/quickcontrols/basic/impl/qtquickcontrols2basicstyleimplplugin_QtQuickControls2BasicStyleImplPlugin.cpp"
Process failed with return value 1)")) << QString();

    QTest::newRow("AUTOMOC: warning:") << R"(AUTOMOC: warning:
/home/alex/src/CMake/tests/solid.orig/solid/solid/device.cpp: The file
includes the moc file "device_p.moc" instead of "moc_device_p.cpp". Running
moc on "/home/alex/src/CMake/tests/solid.orig/solid/solid/device_p.h" !
Include "moc_device_p.cpp" for compatibility with strict mode (see
CMAKE_AUTOMOC_RELAXED_MODE).)" << OutputParserTester::STDERR
                                       << QString() << QString()
                                       << (Tasks() << BuildSystemTask(
                                               Task::Warning,
                                               R"(AUTOMOC: warning:
/home/alex/src/CMake/tests/solid.orig/solid/solid/device.cpp: The file
includes the moc file "device_p.moc" instead of "moc_device_p.cpp". Running
moc on "/home/alex/src/CMake/tests/solid.orig/solid/solid/device_p.h" !
Include "moc_device_p.cpp" for compatibility with strict mode (see
CMAKE_AUTOMOC_RELAXED_MODE).)")) << QString();

    QTest::newRow("AutoMoc warning") << R"(AutoMoc warning
---------------
"SRC:/src/main.cpp"
includes the moc file "main.moc", but does not contain a Q_OBJECT, Q_GADGET, Q_NAMESPACE, Q_NAMESPACE_EXPORT, Q_GADGET_EXPORT, Q_ENUM_NS, K_PLUGIN_FACTORY, K_PLUGIN_CLASS, K_PLUGIN_FACTORY_WITH_JSON or K_PLUGIN_CLASS_WITH_JSON macro.)"
                                     << OutputParserTester::STDERR << QString() << QString()
                                     << (Tasks() << BuildSystemTask(
                                             Task::Warning,
                                             R"(AutoMoc warning
"SRC:/src/main.cpp"
includes the moc file "main.moc", but does not contain a Q_OBJECT, Q_GADGET, Q_NAMESPACE, Q_NAMESPACE_EXPORT, Q_GADGET_EXPORT, Q_ENUM_NS, K_PLUGIN_FACTORY, K_PLUGIN_CLASS, K_PLUGIN_FACTORY_WITH_JSON or K_PLUGIN_CLASS_WITH_JSON macro.)"))
                                     << QString();

    QTest::newRow("AutoUic error") << R"(AutoUic error
-------------
"SRC:/monitor/ui/LiveBoard.h"
includes the uic file "ui_global.h",
but the user interface file "global.ui"
could not be found in the following directories
  "SRC:/monitor/ui")" << OutputParserTester::STDERR
                                   << QString() << QString()
                                   << (Tasks() << BuildSystemTask(
                                           Task::Error,
                                           R"(AutoUic error
"SRC:/monitor/ui/LiveBoard.h"
includes the uic file "ui_global.h",
but the user interface file "global.ui"
could not be found in the following directories
  "SRC:/monitor/ui")")) << QString();
}

void CMakeAutogenParserTest::testCMakeAutogenParser()
{
    OutputParserTester testbench;
    testbench.addLineParser(new CMakeAutogenParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench
        .testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines, outputLines);
}

QObject *createCMakeAutogenParserTest()
{
    return new CMakeAutogenParserTest;
}

} // namespace CMakeProjectManager::Internal

#endif

#include "cmakeautogenparser.moc"
