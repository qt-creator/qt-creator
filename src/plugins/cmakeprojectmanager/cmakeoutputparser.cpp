// Copyright (C) 2016 Axonian LLC.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeoutputparser.h"

#include "cmakeprojectmanagertr.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

const char COMMON_ERROR_PATTERN[] = "^CMake Error at (.*?):([0-9]*?)( \\((.*?)\\))?:";
const char NEXT_SUBERROR_PATTERN[] = "^CMake Error in (.*?):";
const char COMMON_WARNING_PATTERN[] = "^CMake Warning (\\(dev\\) )?at (.*?):([0-9]*?)( \\((.*?)\\))?:";
const char LOCATION_LINE_PATTERN[] = ":(\\d+?):(?:(\\d+?))?$";
const char SOURCE_LINE_AND_FUNCTION_PATTERN[] = "  (.*?):([0-9]*?)( \\((.*?)\\))";

CMakeOutputParser::CMakeOutputParser()
{
    m_commonError.setPattern(QLatin1String(COMMON_ERROR_PATTERN));
    QTC_CHECK(m_commonError.isValid());

    m_nextSubError.setPattern(QLatin1String(NEXT_SUBERROR_PATTERN));
    QTC_CHECK(m_nextSubError.isValid());

    m_commonWarning.setPattern(QLatin1String(COMMON_WARNING_PATTERN));
    QTC_CHECK(m_commonWarning.isValid());

    m_locationLine.setPattern(QLatin1String(LOCATION_LINE_PATTERN));
    QTC_CHECK(m_locationLine.isValid());

    m_sourceLineAndFunction.setPattern(QLatin1String(SOURCE_LINE_AND_FUNCTION_PATTERN));
    QTC_CHECK(m_sourceLineAndFunction.isValid());
}

void CMakeOutputParser::setSourceDirectory(const FilePath &sourceDir)
{
    if (m_sourceDirectory)
        emit searchDirExpired(m_sourceDirectory.value());
    m_sourceDirectory = sourceDir;
    emit newSearchDirFound(sourceDir);
}

FilePath CMakeOutputParser::resolvePath(const QString &path) const
{
    if (m_sourceDirectory)
        return m_sourceDirectory->resolvePath(path);
    return FilePath::fromUserInput(path);
}

OutputLineParser::Result CMakeOutputParser::handleLine(const QString &line, OutputFormat type)
{
    if (line.startsWith("ninja: build stopped")) {
        m_lastTask = BuildSystemTask(Task::Error, line);
        m_lines = 1;
        flush();
        return Status::Done;
    }

    if (type != StdErrFormat)
        return Status::NotHandled;

    QRegularExpressionMatch match;
    QString trimmedLine = rightTrimmed(line);
    switch (m_expectTripleLineErrorData) {
    case NONE: {
        if (trimmedLine.isEmpty() && !m_lastTask.isNull()) {
            if (m_skippedFirstEmptyLine) {
                flush();
                return Status::InProgress;
            }
            m_skippedFirstEmptyLine = true;
            return Status::InProgress;
        }
        QScopeGuard cleanup([this] {
            if (m_skippedFirstEmptyLine)
                m_skippedFirstEmptyLine = false;
        });

        match = m_commonError.match(trimmedLine);
        if (match.hasMatch()) {
            const FilePath path = resolvePath(match.captured(1));

            m_lastTask = BuildSystemTask(Task::Error,
                                         QString(),
                                         absoluteFilePath(path),
                                         match.captured(2).toInt());
            m_lines = 1;
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(
                linkSpecs, m_lastTask.file, m_lastTask.line, m_lastTask.column, match, 1);

            m_errorOrWarningLine.file = m_lastTask.file;
            m_errorOrWarningLine.line = m_lastTask.line;
            m_errorOrWarningLine.function = match.captured(3);

            return {Status::InProgress, linkSpecs};
        }
        match = m_nextSubError.match(trimmedLine);
        if (match.hasMatch()) {
            m_lastTask = BuildSystemTask(Task::Error, QString(),
                                         absoluteFilePath(FilePath::fromUserInput(match.captured(1))));
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(
                linkSpecs, m_lastTask.file, m_lastTask.line, m_lastTask.column, match, 1);
            m_lines = 1;
            return {Status::InProgress, linkSpecs};
        }
        match = m_commonWarning.match(trimmedLine);
        if (match.hasMatch()) {
            const FilePath path = resolvePath(match.captured(2));
            m_lastTask = BuildSystemTask(Task::Warning,
                                         QString(),
                                         absoluteFilePath(path),
                                         match.captured(3).toInt());
            m_lines = 1;
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(
                linkSpecs, m_lastTask.file, m_lastTask.line, m_lastTask.column, match, 1);

            m_errorOrWarningLine.file = m_lastTask.file;
            m_errorOrWarningLine.line = m_lastTask.line;
            m_errorOrWarningLine.function = match.captured(4);

            return {Status::InProgress, linkSpecs};
        } else if (trimmedLine.startsWith(QLatin1String("  ")) && !m_lastTask.isNull()) {
            if (m_callStackDetected) {
                match = m_sourceLineAndFunction.match(trimmedLine);
                if (match.hasMatch()) {
                    CallStackLine stackLine;
                    stackLine.file = absoluteFilePath(resolvePath(match.captured(1)));
                    stackLine.line = match.captured(2).toInt();
                    stackLine.function = match.captured(3);
                    m_callStack << stackLine;
                }
            } else {
                if (m_skippedFirstEmptyLine)
                    m_lastTask.details.append(QString());
                m_lastTask.details.append(trimmedLine.mid(2));
            }
            return {Status::InProgress};
        } else if (trimmedLine.endsWith(QLatin1String("in cmake code at"))) {
            m_expectTripleLineErrorData = LINE_LOCATION;
            flush();
            const Task::TaskType type =
                    trimmedLine.contains(QLatin1String("Error")) ? Task::Error : Task::Warning;
            m_lastTask = BuildSystemTask(type, QString());
            return Status::InProgress;
        } else if (trimmedLine.startsWith("CMake Error: ")) {
            m_lastTask = BuildSystemTask(Task::Error, trimmedLine.mid(13));
            m_lines = 1;
            return Status::InProgress;
        } else if (trimmedLine.startsWith("-- ") || trimmedLine.startsWith(" * ")) {
            // Do not pass on lines starting with "-- " or "* ". Those are typical CMake output
            return Status::InProgress;
        } else if (trimmedLine.startsWith("Call Stack (most recent call first):")) {
            m_callStackDetected = true;
            return {Status::InProgress};
        }
        return Status::NotHandled;
    }
    case LINE_LOCATION:
        {
            match = m_locationLine.match(trimmedLine);
            QTC_CHECK(match.hasMatch());
            m_lastTask.file = absoluteFilePath(FilePath::fromUserInput(
                                                   trimmedLine.mid(0, match.capturedStart())));
            m_lastTask.line = match.captured(1).toInt();
            m_expectTripleLineErrorData = LINE_DESCRIPTION;
            LinkSpecs linkSpecs;
            addLinkSpecForAbsoluteFilePath(
                linkSpecs,
                m_lastTask.file,
                m_lastTask.line,
                m_lastTask.column,
                0,
                match.capturedStart());
            return {Status::InProgress, linkSpecs};
        }
    case LINE_DESCRIPTION:
        m_lastTask.summary = trimmedLine;
        if (trimmedLine.endsWith(QLatin1Char('\"')))
            m_expectTripleLineErrorData = LINE_DESCRIPTION2;
        else {
            m_expectTripleLineErrorData = NONE;
            flush();
            return Status::Done;
        }
        return Status::InProgress;
    case LINE_DESCRIPTION2:
        m_lastTask.details.append(trimmedLine);
        m_expectTripleLineErrorData = NONE;
        flush();
        return Status::Done;
    }
    return Status::NotHandled;
}

void CMakeOutputParser::flush()
{
    if (m_lastTask.isNull())
        return;

    Task t = m_lastTask;
    m_lastTask.clear();

    if (t.summary.isEmpty() && !t.details.isEmpty())
        t.summary = t.details.takeFirst();
    m_lines += t.details.count();

    if (!m_callStack.isEmpty()) {
        t.file = m_callStack.last().file;
        t.line = m_callStack.last().line;

        LinkSpecs specs;
        t.details << QString();
        t.details << Tr::tr("Call stack:");
        m_lines += 2;

        m_callStack.push_front(m_errorOrWarningLine);

        int offset = t.details.join('\n').size();
        Utils::reverseForeach(m_callStack, [&](const auto &line) {
            const QString fileAndLine = QString("%1:%2").arg(line.file.path()).arg(line.line);
            const QString completeLine = QString("  %1%2").arg(fileAndLine).arg(line.function);

            // newline and "  "
            offset += 3;
            specs.append(LinkSpec{offset,
                                  int(fileAndLine.length()),
                                  createLinkTarget(line.file, line.line, -1)});

            t.details << completeLine;
            offset += completeLine.length() - 2;
            ++m_lines;
        });

        setDetailsFormat(t, specs);
    }

    scheduleTask(t, m_lines, 1);
    m_lines = 0;

    m_callStack.clear();
    m_callStackDetected = false;
}

} // CMakeProjectManager

#ifdef WITH_TESTS

#include <projectexplorer/outputparser_test.h>

#include <QTest>

namespace CMakeProjectManager::Internal {

class CMakeOutputParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCMakeOutputParser_data();
    void testCMakeOutputParser();
};

void CMakeOutputParserTest::testCMakeOutputParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QStringList>("childStdOutLines");
    QTest::addColumn<QStringList>("childStdErrLines");
    QTest::addColumn<Tasks>("tasks");

    // negative tests
    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QStringList("Sometext") << QStringList()
            << Tasks();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QStringList() << QStringList("Sometext")
            << Tasks();

    // positive tests
    QTest::newRow("add custom target")
            << QString::fromLatin1("CMake Error at src/1/app/CMakeLists.txt:70 (add_custom_target):\n"
                                   "  Cannot find source file:\n\n"
                                   "    unknownFile.qml\n\n"
                                   "  Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   "  .hxx .in .txx\n\n\n"
                                   "CMake Error in src/1/app/CMakeLists.txt:\n"
                                   "  Cannot find source file:\n\n"
                                   "    CMakeLists.txt2\n\n"
                                   "  Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   "  .hxx .in .txx\n\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Cannot find source file:\n\n"
                                   "  unknownFile.qml\n\n"
                                   "Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   ".hxx .in .txx",
                                    FilePath::fromUserInput("src/1/app/CMakeLists.txt"), 70)
                << BuildSystemTask(Task::Error,
                                   "Cannot find source file:\n\n"
                                   "  CMakeLists.txt2\n\n"
                                   "Tried extensions "
                                   ".c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   ".hxx .in .txx",
                                   FilePath::fromUserInput("src/1/app/CMakeLists.txt"), -1));

    QTest::newRow("add subdirectory")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (add_subdirectory):\n"
                                   "  add_subdirectory given source \"app1\" which is not an existing directory.\n\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "add_subdirectory given source \"app1\" which is not an existing directory.",
                                   FilePath::fromUserInput("src/1/CMakeLists.txt"), 8));

    QTest::newRow("unknown command")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (i_am_wrong_command):\n"
                                   "  Unknown CMake command \"i_am_wrong_command\".\n\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Unknown CMake command \"i_am_wrong_command\".",
                                   FilePath::fromUserInput("src/1/CMakeLists.txt"), 8));

    QTest::newRow("incorrect arguments")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (message):\n"
                                   "  message called with incorrect number of arguments\n\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "message called with incorrect number of arguments",
                                   FilePath::fromUserInput("src/1/CMakeLists.txt"), 8));

    QTest::newRow("cmake error")
            << QString::fromLatin1("CMake Error: Error in cmake code at\n"
                                   "/test/path/CMakeLists.txt:9:\n"
                                   "Parse error.  Expected \"(\", got newline with text \"\n"
                                   "\".")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Parse error.  Expected \"(\", got newline with text \"\n\".",
                                   FilePath::fromUserInput("/test/path/CMakeLists.txt"), 9));

    QTest::newRow("cmake error2")
            << QString::fromLatin1("CMake Error: Error required internal CMake variable not set, cmake may be not be built correctly.\n"
                                   "Missing variable is:\n"
                                   "CMAKE_MAKE_PROGRAM\n\n") // FIXME: Test does not pass without extra newline
            << OutputParserTester::STDERR
            << QStringList() << QStringList{"Missing variable is:", "CMAKE_MAKE_PROGRAM"}
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Error required internal CMake variable not set, "
                                   "cmake may be not be built correctly."));

    QTest::newRow("cmake error at")
            << QString::fromLatin1("CMake Error at CMakeLists.txt:4:\n"
                                   "  Parse error.  Expected \"(\", got newline with text \"\n"
                                   "\n"
                                   "  \".\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Error,
                                   "Parse error.  Expected \"(\", got newline with text \"\n"
                                   "\n"
                                   "\".",
                                   FilePath::fromUserInput("CMakeLists.txt"), 4));

    QTest::newRow("cmake syntax warning")
            << QString::fromLatin1("Syntax Warning in cmake code at\n"
                                   "/test/path/CMakeLists.txt:9:15\n"
                                   "Argument not separated from preceding token by whitespace.")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "Argument not separated from preceding token by whitespace.",
                                   FilePath::fromUserInput("/test/path/CMakeLists.txt"), 9));

    QTest::newRow("cmake warning")
            << QString::fromLatin1("CMake Warning at CMakeLists.txt:13 (message):\n"
                                   "  this is a warning\n\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "this is a warning",
                                   FilePath::fromUserInput("CMakeLists.txt"), 13));

    QTest::newRow("cmake author warning")
            << QString::fromLatin1("CMake Warning (dev) at CMakeLists.txt:15 (message):\n"
                                   "  this is an author warning\n\n")
            << OutputParserTester::STDERR
            << QStringList() << QStringList()
            << (Tasks()
                << BuildSystemTask(Task::Warning,
                                   "this is an author warning",
                                   FilePath::fromUserInput("CMakeLists.txt"), 15));

    QTest::newRow("eat normal CMake output")
        << QString::fromLatin1("-- Qt5 install prefix: /usr/lib\n"
                               " * Plugin componentsplugin, with CONDITION TARGET QmlDesigner")
        << OutputParserTester::STDERR << QStringList() << QStringList() << (Tasks());

    QTest::newRow("cmake call-stack")
        << QString::fromLatin1(
               "CMake Error at /Qt/6.5.3/mingw_64/lib/cmake/Qt6Core/Qt6CoreMacros.cmake:588 "
               "(add_executable):\n"
               "\n"
               "  Cannot find source file:\n"
               "\n"
               "    not-existing\n"
               "\n"
               "  Tried extensions .c .C .c++ .cc .cpp .cxx .cu .mpp .m .M .mm .ixx .cppm\n"
               "  .ccm .cxxm .c++m .h .hh .h++ .hm .hpp .hxx .in .txx .f .F .for .f77 .f90\n"
               "  .f95 .f03 .hip .ispc\n"
               "Call Stack (most recent call first):\n"
               "  /Qt/6.5.3/mingw_64/lib/cmake/Qt6Core/Qt6CoreMacros.cmake:549 "
               "(_qt_internal_create_executable)\n"
               "  /Qt/6.5.3/mingw_64/lib/cmake/Qt6Core/Qt6CoreMacros.cmake:741 "
               "(qt6_add_executable)\n"
               "  /Projects/Test-Project/CMakeLists.txt:13 (qt_add_executable)\n")
        << OutputParserTester::STDERR << QStringList() << QStringList()
        << (Tasks() << BuildSystemTask(
                Task::Error,
                "\n"
                "Cannot find source file:\n"
                "\n"
                "  not-existing\n"
                "\n"
                "Tried extensions .c .C .c++ .cc .cpp .cxx .cu .mpp .m .M .mm .ixx .cppm\n"
                ".ccm .cxxm .c++m .h .hh .h++ .hm .hpp .hxx .in .txx .f .F .for .f77 .f90\n"
                ".f95 .f03 .hip .ispc\n"
                "\n"
                "Call stack:\n"
                "  /Projects/Test-Project/CMakeLists.txt:13 (qt_add_executable)\n"
                "  /Qt/6.5.3/mingw_64/lib/cmake/Qt6Core/Qt6CoreMacros.cmake:741 "
                "(qt6_add_executable)\n"
                "  /Qt/6.5.3/mingw_64/lib/cmake/Qt6Core/Qt6CoreMacros.cmake:549 "
                "(_qt_internal_create_executable)\n"
                "  /Qt/6.5.3/mingw_64/lib/cmake/Qt6Core/Qt6CoreMacros.cmake:588 "
                "(add_executable)",
                FilePath::fromUserInput("/Projects/Test-Project/CMakeLists.txt"),
                13));
}

void CMakeOutputParserTest::testCMakeOutputParser()
{
    OutputParserTester testbench;
    testbench.addLineParser(new CMakeOutputParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QStringList, childStdOutLines);
    QFETCH(QStringList, childStdErrLines);

    testbench.testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines);
}

QObject *createCMakeOutputParserTest()
{
    return new CMakeOutputParserTest;
}

} // CMakeProjectManager::Internal

#endif

#include "cmakeoutputparser.moc"
