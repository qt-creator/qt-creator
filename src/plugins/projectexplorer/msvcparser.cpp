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

#include "msvcparser.h"
#include "projectexplorerconstants.h"
#include "buildmanager.h"

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

using namespace Utils;

// As of MSVC 2015: "foo.cpp(42) :" -> "foo.cpp(42):"
static const char FILE_POS_PATTERN[] = "^(?:\\d+>)?(cl|LINK|.+[^ ]) ?: ";

static QPair<FileName, int> parseFileName(const QString &input)
{
    QString fileName = input;
    if (fileName.startsWith("LINK") || fileName.startsWith("cl"))
        return qMakePair(FileName(), -1);

    // Extract linenumber (if it is there):
    int linenumber = -1;
    if (fileName.endsWith(')')) {
        int pos = fileName.lastIndexOf('(');
        if (pos >= 0) {
            // clang-cl gives column, too: "foo.cpp(34,1)" as opposed to MSVC "foo.cpp(34)".
            int endPos = fileName.indexOf(',', pos + 1);
            if (endPos < 0)
                endPos = fileName.size() - 1;
            bool ok = false;
            const int n = fileName.midRef(pos + 1, endPos - pos - 1).toInt(&ok);
            if (ok) {
                fileName = fileName.left(pos);
                linenumber = n;
            }
        }
    }
    const QString normalized = FileUtils::normalizePathName(fileName);
    return qMakePair(FileName::fromUserInput(normalized), linenumber);
}

using namespace ProjectExplorer;

// nmake/jom messages.
static bool handleNmakeJomMessage(const QString &line, Task *task)
{
    int matchLength = 0;
    if (line.startsWith("Error:"))
        matchLength = 6;
    else if (line.startsWith("Warning:"))
        matchLength = 8;

    if (!matchLength)
        return false;

    *task = Task(Task::Error,
                 line.mid(matchLength).trimmed(), /* description */
                 FileName(), /* fileName */
                 -1, /* linenumber */
                 Constants::TASK_CATEGORY_COMPILE);
    return true;
}

static Task::TaskType taskType(const QString &category)
{
    Task::TaskType type = Task::Unknown;
    if (category == "warning")
        type = Task::Warning;
    else if (category == "error")
        type = Task::Error;
    return type;
}

MsvcParser::MsvcParser()
{
    setObjectName("MsvcParser");
    m_compileRegExp.setPattern(QString(FILE_POS_PATTERN)
                               + "(?:Command line |fatal )?(?:(warning|error) "
                                 "([A-Z]+\\d{4} ?: )|note: )(.*)$");
    QTC_CHECK(m_compileRegExp.isValid());
    m_additionalInfoRegExp.setPattern("^        (?:(could be |or )\\s*')?(.*)\\((\\d+)\\) : (.*)$");
    QTC_CHECK(m_additionalInfoRegExp.isValid());
}

void MsvcParser::stdOutput(const QString &line)
{
    QRegularExpressionMatch match = m_additionalInfoRegExp.match(line);
    if (line.startsWith("        ") && !match.hasMatch()) {
        if (m_lastTask.isNull())
            return;

        m_lastTask.description.append('\n');
        m_lastTask.description.append(line.mid(8));
        // trim trailing spaces:
        int i = 0;
        for (i = m_lastTask.description.length() - 1; i >= 0; --i) {
            if (!m_lastTask.description.at(i).isSpace())
                break;
        }
        m_lastTask.description.truncate(i + 1);

        if (m_lastTask.formats.isEmpty()) {
            QTextLayout::FormatRange fr;
            fr.start = m_lastTask.description.indexOf('\n') + 1;
            fr.length = m_lastTask.description.length() - fr.start;
            fr.format.setFontItalic(true);
            m_lastTask.formats.append(fr);
        } else {
            m_lastTask.formats[0].length = m_lastTask.description.length() - m_lastTask.formats[0].start;
        }
        ++m_lines;
        return;
    }

    if (processCompileLine(line))
        return;
    if (handleNmakeJomMessage(line, &m_lastTask)) {
        m_lines = 1;
        return;
    }
    if (match.hasMatch()) {
        QString description = match.captured(1)
                + match.captured(4).trimmed();
        if (!match.captured(1).isEmpty())
            description.chop(1); // Remove trailing quote
        m_lastTask = Task(Task::Unknown, description,
                          FileName::fromUserInput(match.captured(2)), /* fileName */
                          match.captured(3).toInt(), /* linenumber */
                          Constants::TASK_CATEGORY_COMPILE);
        m_lines = 1;
        return;
    }
    IOutputParser::stdOutput(line);
}

void MsvcParser::stdError(const QString &line)
{
    if (processCompileLine(line))
        return;
    // Jom outputs errors to stderr
    if (handleNmakeJomMessage(line, &m_lastTask)) {
        m_lines = 1;
        return;
    }
    IOutputParser::stdError(line);
}

Core::Id MsvcParser::id()
{
    return Core::Id("ProjectExplorer.OutputParser.Msvc");
}

bool MsvcParser::processCompileLine(const QString &line)
{
    doFlush();

    QRegularExpressionMatch match = m_compileRegExp.match(line);
    if (match.hasMatch()) {
        QPair<FileName, int> position = parseFileName(match.captured(1));
        m_lastTask = Task(taskType(match.captured(2)),
                          match.captured(3) + match.captured(4).trimmed(), // description
                          position.first, position.second,
                          Constants::TASK_CATEGORY_COMPILE);
        m_lines = 1;
        return true;
    }
    return false;
}

void MsvcParser::doFlush()
{
    if (m_lastTask.isNull())
        return;

    Task t = m_lastTask;
    m_lastTask.clear();
    emit addTask(t, m_lines, 1);
}

// --------------------------------------------------------------------------
// ClangClParser: The compiler errors look similar to MSVC, except that the
// column number is also given and there are no 4digit CXXXX error numbers.
// They are output to stderr.
// --------------------------------------------------------------------------

// ".\qwindowsgdinativeinterface.cpp(48,3) :  error: unknown type name 'errr'"
static inline QString clangClCompilePattern()
{
    return QLatin1String(FILE_POS_PATTERN) + " (warning|error): (.*)$";
}

ClangClParser::ClangClParser()
    : m_compileRegExp(clangClCompilePattern())
{
    setObjectName("ClangClParser");
    QTC_CHECK(m_compileRegExp.isValid());
}

void ClangClParser::stdOutput(const QString &line)
{
    if (handleNmakeJomMessage(line, &m_lastTask)) {
        m_linkedLines = 1;
        doFlush();
        return;
    }
    IOutputParser::stdOutput(line);
}

// Check for a code marker '~~~~ ^ ~~~~~~~~~~~~' underlining above code.
static inline bool isClangCodeMarker(const QString &trimmedLine)
{
    return trimmedLine.constEnd() ==
            std::find_if(trimmedLine.constBegin(), trimmedLine.constEnd(),
                         [] (QChar c) { return c != ' ' && c != '^' && c != '~'; });
}

void ClangClParser::stdError(const QString &lineIn)
{
    const QString line = IOutputParser::rightTrimmed(lineIn); // Strip \r\n.

    if (handleNmakeJomMessage(line, &m_lastTask)) {
        m_linkedLines = 1;
        doFlush();
        return;
    }

    // Finish a sequence of warnings/errors: "2 warnings generated."
    if (!line.isEmpty() && line.at(0).isDigit() && line.endsWith("generated.")) {
        doFlush();
        return;
    }

    // Start a new error message by a sequence of "In file included from " which is to be skipped.
    if (line.startsWith("In file included from ")) {
        doFlush();
        return;
    }

    QRegularExpressionMatch match = m_compileRegExp.match(line);
    if (match.hasMatch()) {
        doFlush();
        const QPair<FileName, int> position = parseFileName(match.captured(1));
        m_lastTask = Task(taskType(match.captured(2)), match.captured(3).trimmed(),
                          position.first, position.second,
                          Constants::TASK_CATEGORY_COMPILE);
        m_linkedLines = 1;
        return;
    }

    if (!m_lastTask.isNull()) {
        const QString trimmed = line.trimmed();
        if (isClangCodeMarker(trimmed)) {
            doFlush();
            return;
        }
        m_lastTask.description.append('\n');
        m_lastTask.description.append(trimmed);
        ++m_linkedLines;
        return;
    }

    IOutputParser::stdError(lineIn);
}

void ClangClParser::doFlush()
{
    if (!m_lastTask.isNull()) {
        emit addTask(m_lastTask, m_linkedLines, 1);
        m_lastTask.clear();
    }
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>
#   include "projectexplorer.h"
#   include "projectexplorer/outputparser_test.h"

namespace ProjectExplorer {

void ProjectExplorerPlugin::testMsvcOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << "Sometext" << OutputParserTester::STDOUT
            << "Sometext\n" << ""
            << QList<Task>()
            << "";
    QTest::newRow("pass-through stderr")
            << "Sometext" << OutputParserTester::STDERR
            << "" << "Sometext\n"
            << QList<Task>()
            << "";

    QTest::newRow("labeled error")
            << "qmlstandalone\\main.cpp(54) : error C4716: 'findUnresolvedModule' : must return a value"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "C4716: 'findUnresolvedModule' : must return a value",
                        FileName::fromUserInput("qmlstandalone\\main.cpp"), 54,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("labeled error-2015")
            << "qmlstandalone\\main.cpp(54): error C4716: 'findUnresolvedModule' : must return a value"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "C4716: 'findUnresolvedModule' : must return a value",
                        FileName::fromUserInput("qmlstandalone\\main.cpp"), 54,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("labeled error with number prefix")
            << "1>qmlstandalone\\main.cpp(54) : error C4716: 'findUnresolvedModule' : must return a value"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "C4716: 'findUnresolvedModule' : must return a value",
                        FileName::fromUserInput("qmlstandalone\\main.cpp"), 54,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("labeled warning")
            << "x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Warning,
                        "C4100: 'something' : unreferenced formal parameter",
                        FileName::fromUserInput("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp"), 69,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";


    QTest::newRow("labeled warning with number prefix")
            << "1>x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Warning,
                        "C4100: 'something' : unreferenced formal parameter",
                        FileName::fromUserInput("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp"), 69,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("additional information")
            << "x:\\src\\plugins\\texteditor\\icompletioncollector.h(50) : warning C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'\n"
               "        x:\\src\\plugins\\texteditor\\completionsupport.h(39) : see declaration of 'TextEditor::CompletionItem'"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Warning,
                        "C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'",
                        FileName::fromUserInput("x:\\src\\plugins\\texteditor\\icompletioncollector.h"), 50,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "see declaration of 'TextEditor::CompletionItem'",
                        FileName::fromUserInput("x:\\src\\plugins\\texteditor\\completionsupport.h"), 39,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("additional information with prefix")
            << "2>x:\\src\\plugins\\texteditor\\icompletioncollector.h(50) : warning C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'\n"
               "        x:\\src\\plugins\\texteditor\\completionsupport.h(39) : see declaration of 'TextEditor::CompletionItem'"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Warning,
                        "C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'",
                        FileName::fromUserInput("x:\\src\\plugins\\texteditor\\icompletioncollector.h"), 50,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "see declaration of 'TextEditor::CompletionItem'",
                        FileName::fromUserInput("x:\\src\\plugins\\texteditor\\completionsupport.h"), 39,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("fatal linker error")
            << "LINK : fatal error LNK1146: no argument specified with option '/LIBPATH:'"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "LNK1146: no argument specified with option '/LIBPATH:'",
                        FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    // This actually comes through stderr!
    QTest::newRow("command line warning")
            << "cl : Command line warning D9002 : ignoring unknown option '-fopenmp'"
            << OutputParserTester::STDERR
            << "" << ""
            << (QList<Task>()
                << Task(Task::Warning,
                        "D9002 : ignoring unknown option '-fopenmp'",
                        FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
    QTest::newRow("complex error")
            << "..\\untitled\\main.cpp(19) : error C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
               "        with\n"
               "        [\n"
               "            _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
               "        ]\n"
               "        No constructor could take the source type, or constructor overload resolution was ambiguous"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
                        "with\n"
                        "[\n"
                        "    _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
                        "]\n"
                        "No constructor could take the source type, or constructor overload resolution was ambiguous",
                        FileName::fromUserInput("..\\untitled\\main.cpp"), 19,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
    QTest::newRow("Linker error 1")
            << "main.obj : error LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main",
                        FileName::fromUserInput("main.obj"), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
    QTest::newRow("Linker error 2")
            << "debug\\Experimentation.exe : fatal error LNK1120: 1 unresolved externals"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "LNK1120: 1 unresolved externals",
                        FileName::fromUserInput("debug\\Experimentation.exe"), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("nmake error")
            << "Error: dependent '..\\..\\..\\..\\creator-2.5\\src\\plugins\\coreplugin\\ifile.h' does not exist."
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "dependent '..\\..\\..\\..\\creator-2.5\\src\\plugins\\coreplugin\\ifile.h' does not exist.",
                        FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
    QTest::newRow("jom error")
            << "Error: dependent 'main.cpp' does not exist."
            << OutputParserTester::STDERR
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "dependent 'main.cpp' does not exist.",
                        FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("Multiline error")
            << "c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility(2227) : warning C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'\n"
               "        c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility(2212) : see declaration of 'std::_Copy_impl'\n"
               "        symbolgroupvalue.cpp(2314) : see reference to function template instantiation '_OutIt std::copy<const unsigned char*,unsigned short*>(_InIt,_InIt,_OutIt)' being compiled\n"
               "        with\n"
               "        [\n"
               "            _OutIt=unsigned short *,\n"
               "            _InIt=const unsigned char *\n"
               "        ]"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Warning,
                        "C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'",
                        FileName::fromUserInput("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility"), 2227,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "see declaration of 'std::_Copy_impl'",
                        FileName::fromUserInput("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility"), 2212,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "see reference to function template instantiation '_OutIt std::copy<const unsigned char*,unsigned short*>(_InIt,_InIt,_OutIt)' being compiled\n"
                        "with\n"
                        "[\n"
                        "    _OutIt=unsigned short *,\n"
                        "    _InIt=const unsigned char *\n"
                        "]",
                        FileName::fromUserInput("symbolgroupvalue.cpp"), 2314,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";

    QTest::newRow("Ambiguous symbol")
            << "D:\\Project\\file.h(98) : error C2872: 'UINT64' : ambiguous symbol\n"
               "        could be 'C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\include\\basetsd.h(83) : unsigned __int64 UINT64'\n"
               "        or       'D:\\Project\\types.h(71) : Types::UINT64'"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "C2872: 'UINT64' : ambiguous symbol",
                        FileName::fromUserInput("D:\\Project\\file.h"), 98,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "could be unsigned __int64 UINT64",
                        FileName::fromUserInput("C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\include\\basetsd.h"), 83,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "or Types::UINT64",
                        FileName::fromUserInput("D:\\Project\\types.h"), 71,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
    QTest::newRow("ignore moc note")
            << "/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated."
            << OutputParserTester::STDERR
            << "" << "/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.\n"
            << (QList<ProjectExplorer::Task>())
            << "";

    QTest::newRow("error with note")
            << "main.cpp(7): error C2733: 'func': second C linkage of overloaded function not allowed\n"
               "main.cpp(6): note: see declaration of 'func'"
            << OutputParserTester::STDOUT
            << "" << ""
            << (QList<Task>()
                << Task(Task::Error,
                        "C2733: 'func': second C linkage of overloaded function not allowed",
                        FileName::fromUserInput("main.cpp"), 7,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        "see declaration of 'func'",
                        FileName::fromUserInput("main.cpp"), 6,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
}

void ProjectExplorerPlugin::testMsvcOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new MsvcParser);
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

void ProjectExplorerPlugin::testClangClOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

    const QString warning1 = "private field 'm_version' is not used [-Wunused-private-field]\n"
                             "const int m_version; //! majorVersion<<8 + minorVersion\n";
    const QString warning2 = "unused variable 'formatTextPlainC' [-Wunused-const-variable]\n"
                             "static const char formatTextPlainC[] = \"text/plain\";\n";
    const QString warning3 = "unused variable 'formatTextHtmlC' [-Wunused-const-variable]\n"
                             "static const char formatTextHtmlC[] = \"text/html\";\n";
    const QString error1 = "unknown type name 'errr'\n"
                           "  errr\n";
    const QString expectedError1 = "unknown type name 'errr'\n"
                                   "errr"; // Line 2 trimmed.
    const QString error2 =
            "expected unqualified-id\n"
            "void *QWindowsGdiNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs)\n";

    const QString clangClCompilerLog =
            "In file included from .\\qwindowseglcontext.cpp:40:\n"
            "./qwindowseglcontext.h(282,15) :  warning: "  + warning1 +
            "5 warnings generated.\n"
            ".\\qwindowsclipboard.cpp(60,19) :  warning: " + warning2 +
            "                  ^\n"
            ".\\qwindowsclipboard.cpp(61,19) :  warning: " + warning3 +
            "                  ^\n"
            "2 warnings generated.\n"
            ".\\qwindowsgdinativeinterface.cpp(48,3) :  error: " + error1 +
            "  ^\n"
            ".\\qwindowsgdinativeinterface.cpp(51,1) :  error: " + error2 +
            "^\n"
            "2 errors generated.\n";

    const QString ignoredStderr =
            "NMAKE : fatal error U1077: 'D:\\opt\\LLVM64_390\\bin\\clang-cl.EXE' : return code '0x1'\n"
            "Stop.";

    const QString input = clangClCompilerLog + ignoredStderr;
    const QString expectedStderr = ignoredStderr + '\n';

    QTest::newRow("error")
            << input
            << OutputParserTester::STDERR
            << "" << expectedStderr
            << (QList<Task>()
                << Task(Task::Warning, warning1.trimmed(),
                        FileName::fromUserInput("./qwindowseglcontext.h"), 282,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning, warning2.trimmed(),
                        FileName::fromUserInput(".\\qwindowsclipboard.cpp"), 60,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning, warning3.trimmed(),
                        FileName::fromUserInput(".\\qwindowsclipboard.cpp"), 61,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error, expectedError1,
                        FileName::fromUserInput(".\\qwindowsgdinativeinterface.cpp"), 48,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error, error2.trimmed(),
                        FileName::fromUserInput(".\\qwindowsgdinativeinterface.cpp"), 51,
                        Constants::TASK_CATEGORY_COMPILE))
            << "";
}

void ProjectExplorerPlugin::testClangClOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new ClangClParser);
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

} // namespace ProjectExplorer

#endif // WITH_TEST
