// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#define FILE_PATTERN R"((?<file>(?:[A-Za-z]:)?[^:\(]+\.[^:\(]+))"

QtParser::QtParser() :
    m_mocRegExp("^" FILE_PATTERN R"([:\(](?<line>\d+)?(?::(?<column>\d+))?\)?:\s(?<level>[Ww]arning|[Ee]rror|[Nn]ote):\s(?<description>.+?)$)"),
    m_uicRegExp("^" FILE_PATTERN R"(: Warning:\s(?<msg>.+?)$)"),
    m_translationRegExp(R"(^(?<level>[Ww]arning|[Ee]rror):\s+(?<description>.*?) in '(?<file>.*?)'$)"),
    m_qmlToolsRegExp(R"(^(?<level>Warning|Error):\s*)" FILE_PATTERN R"([:\(](?<line>\d+)?(?::(?<column>\d+))?\)?:\s(?<description>.+?)$)")
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
        int lineno = match.captured("line").toInt(&ok);
        if (!ok)
            lineno = -1;
        Task::TaskType type = Task::Error;
        const QString level = match.captured("level");
        if (level.compare(QLatin1String("Warning"), Qt::CaseInsensitive) == 0)
            type = Task::Warning;
        if (level.compare(QLatin1String("Note"), Qt::CaseInsensitive) == 0)
            type = Task::Unknown;
        LinkSpecs linkSpecs;
        const Utils::FilePath file
                = absoluteFilePath(Utils::FilePath::fromUserInput(match.captured("file")));
        addLinkSpecForAbsoluteFilePath(linkSpecs, file, lineno, match, "file");
        CompileTask task(type, match.captured("description").trimmed(), file, lineno);
        task.column = match.captured("column").toInt();
        scheduleTask(task, 1);
        return {Status::Done, linkSpecs};
    }
    match = m_uicRegExp.match(lne);
    if (match.hasMatch()) {
        const QString fileName = match.captured(1);
        QString message = match.captured("msg").trimmed();
        Utils::FilePath filePath;
        LinkSpecs linkSpecs;
        bool isUicMessage = true;
        if (fileName == "uic" || fileName == "stdin") {
            message.prepend(": ").prepend(fileName);
        } else if (fileName.endsWith(".ui")) {
            filePath = absoluteFilePath(Utils::FilePath::fromUserInput(fileName));
            addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, -1, match, "file");
        } else {
            isUicMessage = false;
        }
        if (isUicMessage) {
            scheduleTask(CompileTask(Task::Warning, message, filePath), 1);
            return {Status::Done, linkSpecs};
        }
    }
    match = m_translationRegExp.match(line);
    if (match.hasMatch()) {
        Task::TaskType type = Task::Warning;
        if (match.captured("level") == QLatin1String("Error"))
            type = Task::Error;
        LinkSpecs linkSpecs;
        const Utils::FilePath file
                = absoluteFilePath(Utils::FilePath::fromUserInput(match.captured("file")));
        addLinkSpecForAbsoluteFilePath(linkSpecs, file, 0, match, "file");
        CompileTask task(type, match.captured("description"), file);
        scheduleTask(task, 1);
        return {Status::Done, linkSpecs};
    }
    match = m_qmlToolsRegExp.match(line);
    if (match.hasMatch()) {
        const Task::TaskType type = match.captured("level") == "Error" ? Task::Error
                                                                       : Task::Warning;
        const Utils::FilePath file
            = absoluteFilePath(Utils::FilePath::fromUserInput(match.captured("file")));
        bool ok;
        int lineno = match.captured("line").toInt(&ok);
        if (!ok)
            lineno = -1;
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, file, lineno, match, "file");
        CompileTask task(type, match.captured("description"), file, lineno,
                         match.captured("column").toInt());
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
    QTest::newRow("moc warning (Qt 6/Windows)")
            << QString::fromLatin1(R"(C:/Users/alportal/dev/qt-creator-qt6/src/plugins/qmlprofiler/qmlprofilerplugin.h(38:1): error: Plugin Metadata file "QmlProfiler.json" does not exist. Declaration will be ignored)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       R"(Plugin Metadata file "QmlProfiler.json" does not exist. Declaration will be ignored)",
                                       Utils::FilePath::fromUserInput("C:/Users/alportal/dev/qt-creator-qt6/src/plugins/qmlprofiler/qmlprofilerplugin.h"), 38, 1))
            << QString();
    QTest::newRow("moc warning (Qt 6/Unix)")
            << QString::fromLatin1(R"(/Users/alportal/dev/qt-creator-qt6/src/plugins/qmlprofiler/qmlprofilerplugin.h:38:1: error: Plugin Metadata file "QmlProfiler.json" does not exist. Declaration will be ignored)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       R"(Plugin Metadata file "QmlProfiler.json" does not exist. Declaration will be ignored)",
                                       Utils::FilePath::fromUserInput("/Users/alportal/dev/qt-creator-qt6/src/plugins/qmlprofiler/qmlprofilerplugin.h"), 38, 1))
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
    QTest::newRow("uic warning")
            << QString::fromLatin1("mainwindow.ui: Warning: The name 'pushButton' (QPushButton) is already in use, defaulting to 'pushButton1'.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                                "The name 'pushButton' (QPushButton) is already in use, defaulting to 'pushButton1'.",
                                Utils::FilePath::fromUserInput("mainwindow.ui")))
            << QString();
    QTest::newRow("translation")
            << QString::fromLatin1("Warning: dropping duplicate messages in '/some/place/qtcreator_fr.qm'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                                       QLatin1String("dropping duplicate messages"),
                                                       Utils::FilePath::fromUserInput(QLatin1String("/some/place/qtcreator_fr.qm")), -1))
            << QString();
    QTest::newRow("qmlsc/qmllint warning") // QTCREATORBUG-28720
            << QString::fromLatin1("Warning: Main.qml:4:1: Warnings occurred while importing module "
                                   "\"QtQuick.Controls\": [import]\"")
            << OutputParserTester::STDERR << QString() << QString()
            << (Tasks() << CompileTask(Task::Warning,
                                       "Warnings occurred while importing module \"QtQuick.Controls\": [import]\"",
                                       Utils::FilePath::fromUserInput("Main.qml"), 4, 1))
            << QString();
    QTest::newRow("qmlsc/qmllint error") // QTCREATORBUG-28720
            << QString::fromLatin1("Error: E:/foo/PerfProfilerFlameGraphView.qml:10:5: "
                                   "Could not compile binding for model: Cannot resolve property type  for binding on model")
            << OutputParserTester::STDERR << QString() << QString()
            << (Tasks() << CompileTask(Task::Error,
                                       "Could not compile binding for model: Cannot resolve property type  for binding on model",
                                       Utils::FilePath::fromUserInput("E:/foo/PerfProfilerFlameGraphView.qml"), 10, 5))
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
