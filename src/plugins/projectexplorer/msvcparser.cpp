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

// As of MSVC 2015: "foo.cpp(42) :" -> "foo.cpp(42):"
static const char FILE_POS_PATTERN[] = "(?:\\d+>)?(cl|LINK|.+[^ ]) ?: ";
static const char ERROR_PATTERN[] = "[A-Z]+\\d\\d\\d\\d ?:";

static QPair<Utils::FileName, int> parseFileName(const QString &input)
{
    QString fileName = input;
    if (fileName.startsWith(QLatin1String("LINK"))
            || fileName.startsWith(QLatin1String("cl")))
        return qMakePair(Utils::FileName(), -1);

    // Extract linenumber (if it is there):
    int linenumber = -1;
    if (fileName.endsWith(QLatin1Char(')'))) {
        int pos = fileName.lastIndexOf(QLatin1Char('('));
        if (pos >= 0) {
            // clang-cl gives column, too: "foo.cpp(34,1)" as opposed to MSVC "foo.cpp(34)".
            int endPos = fileName.indexOf(QLatin1Char(','), pos + 1);
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
    const QString normalized = Utils::FileUtils::normalizePathName(fileName);
    return qMakePair(Utils::FileName::fromUserInput(normalized), linenumber);
}

using namespace ProjectExplorer;

// nmake/jom messages.
static bool handleNmakeJomMessage(const QString &line, Task *task)
{
    int matchLength = 0;
    if (line.startsWith(QLatin1String("Error:")))
        matchLength = 6;
    else if (line.startsWith(QLatin1String("Warning:")))
        matchLength = 8;

    if (!matchLength)
        return false;

    *task = Task(Task::Error,
                 line.mid(matchLength).trimmed(), /* description */
                 Utils::FileName(), /* fileName */
                 -1, /* linenumber */
                 Constants::TASK_CATEGORY_COMPILE);
    return true;
}

static Task::TaskType taskType(const QString &category)
{
    Task::TaskType type = Task::Unknown;
    if (category == QLatin1String("warning"))
        type = Task::Warning;
    else if (category == QLatin1String("error"))
        type = Task::Error;
    return type;
}

MsvcParser::MsvcParser()
{
    setObjectName(QLatin1String("MsvcParser"));
    m_compileRegExp.setPattern(QString::fromLatin1("^") + QLatin1String(FILE_POS_PATTERN)
                               + QLatin1String("(Command line |fatal )?(warning|error) (")
                               + QLatin1String(ERROR_PATTERN) + QLatin1String(".*)$"));
    QTC_CHECK(m_compileRegExp.isValid());
    m_additionalInfoRegExp.setPattern(QString::fromLatin1("^        (?:(could be |or )\\s*')?(.*)\\((\\d+)\\) : (.*)$"));
    QTC_CHECK(m_additionalInfoRegExp.isValid());
}

void MsvcParser::stdOutput(const QString &line)
{
    QRegularExpressionMatch match = m_additionalInfoRegExp.match(line);
    if (line.startsWith(QLatin1String("        ")) && !match.hasMatch()) {
        if (m_lastTask.isNull())
            return;

        m_lastTask.description.append(QLatin1Char('\n'));
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
            fr.start = m_lastTask.description.indexOf(QLatin1Char('\n')) + 1;
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
                          Utils::FileName::fromUserInput(match.captured(2)), /* fileName */
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

bool MsvcParser::processCompileLine(const QString &line)
{
    doFlush();

    QRegularExpressionMatch match = m_compileRegExp.match(line);
    if (match.hasMatch()) {
        QPair<Utils::FileName, int> position = parseFileName(match.captured(1));
        m_lastTask = Task(taskType(match.captured(3)),
                          match.captured(4).trimmed() /* description */,
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
    return QLatin1Char('^') + QLatin1String(FILE_POS_PATTERN)
        + QLatin1String(" (warning|error): (")
        + QLatin1String(".*)$");
}

ClangClParser::ClangClParser()
   : m_compileRegExp(clangClCompilePattern())
{
    setObjectName(QLatin1String("ClangClParser"));
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
                     [] (QChar c) { return c != QLatin1Char(' ') && c != QLatin1Char('^') && c != QLatin1Char('~'); });
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
    if (!line.isEmpty() && line.at(0).isDigit() && line.endsWith(QLatin1String("generated."))) {
        doFlush();
        return;
    }

    // Start a new error message by a sequence of "In file included from " which is to be skipped.
    if (line.startsWith(QLatin1String("In file included from "))) {
        doFlush();
        return;
    }

    QRegularExpressionMatch match = m_compileRegExp.match(line);
    if (match.hasMatch()) {
        doFlush();
        const QPair<Utils::FileName, int> position = parseFileName(match.captured(1));
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
        m_lastTask.description.append(QLatin1Char('\n'));
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

using namespace ProjectExplorer::Internal;
using namespace ProjectExplorer;

void ProjectExplorerPlugin::testMsvcOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString();

    QTest::newRow("labeled error")
            << QString::fromLatin1("qmlstandalone\\main.cpp(54) : error C4716: 'findUnresolvedModule' : must return a value") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("C4716: 'findUnresolvedModule' : must return a value"),
                        Utils::FileName::fromUserInput(QLatin1String("qmlstandalone\\main.cpp")), 54,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("labeled error-2015")
            << QString::fromLatin1("qmlstandalone\\main.cpp(54): error C4716: 'findUnresolvedModule' : must return a value") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("C4716: 'findUnresolvedModule' : must return a value"),
                        Utils::FileName::fromUserInput(QLatin1String("qmlstandalone\\main.cpp")), 54,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("labeled error with number prefix")
            << QString::fromLatin1("1>qmlstandalone\\main.cpp(54) : error C4716: 'findUnresolvedModule' : must return a value") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("C4716: 'findUnresolvedModule' : must return a value"),
                        Utils::FileName::fromUserInput(QLatin1String("qmlstandalone\\main.cpp")), 54,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("labeled warning")
            << QString::fromLatin1("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QLatin1String("C4100: 'something' : unreferenced formal parameter"),
                        Utils::FileName::fromUserInput(QLatin1String("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp")), 69,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();


    QTest::newRow("labeled warning with number prefix")
            << QString::fromLatin1("1>x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QLatin1String("C4100: 'something' : unreferenced formal parameter"),
                        Utils::FileName::fromUserInput(QLatin1String("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp")), 69,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("additional information")
            << QString::fromLatin1("x:\\src\\plugins\\texteditor\\icompletioncollector.h(50) : warning C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'\n"
                                   "        x:\\src\\plugins\\texteditor\\completionsupport.h(39) : see declaration of 'TextEditor::CompletionItem'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QLatin1String("C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'"),
                        Utils::FileName::fromUserInput(QLatin1String("x:\\src\\plugins\\texteditor\\icompletioncollector.h")), 50,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        QLatin1String("see declaration of 'TextEditor::CompletionItem'"),
                        Utils::FileName::fromUserInput(QLatin1String("x:\\src\\plugins\\texteditor\\completionsupport.h")), 39,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("additional information with prefix")
            << QString::fromLatin1("2>x:\\src\\plugins\\texteditor\\icompletioncollector.h(50) : warning C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'\n"
                                   "        x:\\src\\plugins\\texteditor\\completionsupport.h(39) : see declaration of 'TextEditor::CompletionItem'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QLatin1String("C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'"),
                        Utils::FileName::fromUserInput(QLatin1String("x:\\src\\plugins\\texteditor\\icompletioncollector.h")), 50,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        QLatin1String("see declaration of 'TextEditor::CompletionItem'"),
                        Utils::FileName::fromUserInput(QLatin1String("x:\\src\\plugins\\texteditor\\completionsupport.h")), 39,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("fatal linker error")
            << QString::fromLatin1("LINK : fatal error LNK1146: no argument specified with option '/LIBPATH:'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("LNK1146: no argument specified with option '/LIBPATH:'"),
                        Utils::FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    // This actually comes through stderr!
    QTest::newRow("command line warning")
            << QString::fromLatin1("cl : Command line warning D9002 : ignoring unknown option '-fopenmp'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QLatin1String("D9002 : ignoring unknown option '-fopenmp'"),
                        Utils::FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("complex error")
            << QString::fromLatin1("..\\untitled\\main.cpp(19) : error C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
                                   "        with\n"
                                   "        [\n"
                                   "            _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
                                   "        ]\n"
                                   "        No constructor could take the source type, or constructor overload resolution was ambiguous")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
                                      "with\n"
                                      "[\n"
                                      "    _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
                                      "]\n"
                                      "No constructor could take the source type, or constructor overload resolution was ambiguous"),
                        Utils::FileName::fromUserInput(QLatin1String("..\\untitled\\main.cpp")), 19,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("Linker error 1")
            << QString::fromLatin1("main.obj : error LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main"),
                        Utils::FileName::fromUserInput(QLatin1String("main.obj")), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("Linker error 2")
            << QString::fromLatin1("debug\\Experimentation.exe : fatal error LNK1120: 1 unresolved externals")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("LNK1120: 1 unresolved externals"),
                        Utils::FileName::fromUserInput(QLatin1String("debug\\Experimentation.exe")), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("nmake error")
            << QString::fromLatin1("Error: dependent '..\\..\\..\\..\\creator-2.5\\src\\plugins\\coreplugin\\ifile.h' does not exist.")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("dependent '..\\..\\..\\..\\creator-2.5\\src\\plugins\\coreplugin\\ifile.h' does not exist."),
                        Utils::FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("jom error")
            << QString::fromLatin1("Error: dependent 'main.cpp' does not exist.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("dependent 'main.cpp' does not exist."),
                        Utils::FileName(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("Multiline error")
            << QString::fromLatin1("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility(2227) : warning C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'\n"
                                   "        c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility(2212) : see declaration of 'std::_Copy_impl'\n"
                                   "        symbolgroupvalue.cpp(2314) : see reference to function template instantiation '_OutIt std::copy<const unsigned char*,unsigned short*>(_InIt,_InIt,_OutIt)' being compiled\n"
                                   "        with\n"
                                   "        [\n"
                                   "            _OutIt=unsigned short *,\n"
                                   "            _InIt=const unsigned char *\n"
                                   "        ]")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QLatin1String("C4996: 'std::_Copy_impl': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'"),
                        Utils::FileName::fromUserInput(QLatin1String("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility")), 2227,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        QLatin1String("see declaration of 'std::_Copy_impl'"),
                        Utils::FileName::fromUserInput(QLatin1String("c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\INCLUDE\\xutility")), 2212,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        QLatin1String("see reference to function template instantiation '_OutIt std::copy<const unsigned char*,unsigned short*>(_InIt,_InIt,_OutIt)' being compiled\n"
                                      "with\n"
                                      "[\n"
                                      "    _OutIt=unsigned short *,\n"
                                      "    _InIt=const unsigned char *\n"
                                      "]"),
                        Utils::FileName::fromUserInput(QLatin1String("symbolgroupvalue.cpp")), 2314,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("Ambiguous symbol")
            << QString::fromLatin1("D:\\Project\\file.h(98) : error C2872: 'UINT64' : ambiguous symbol\n"
                                   "        could be 'C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\include\\basetsd.h(83) : unsigned __int64 UINT64'\n"
                                   "        or       'D:\\Project\\types.h(71) : Types::UINT64'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("C2872: 'UINT64' : ambiguous symbol"),
                        Utils::FileName::fromUserInput(QLatin1String("D:\\Project\\file.h")), 98,
                        Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Unknown,
                    QLatin1String("could be unsigned __int64 UINT64"),
                    Utils::FileName::fromUserInput(QLatin1String("C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\include\\basetsd.h")), 83,
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Unknown,
                    QLatin1String("or Types::UINT64"),
                    Utils::FileName::fromUserInput(QLatin1String("D:\\Project\\types.h")), 71,
                    Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("ignore moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.\n")
            << (QList<ProjectExplorer::Task>())
            << QString();
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

    const QString warning1 = QLatin1String(
"private field 'm_version' is not used [-Wunused-private-field]\n"
"const int m_version; //! majorVersion<<8 + minorVersion");
    const QString warning2 = QLatin1String(
"unused variable 'formatTextPlainC' [-Wunused-const-variable]\n"
"static const char formatTextPlainC[] = \"text/plain\";");
    const QString warning3 = QLatin1String(
"unused variable 'formatTextHtmlC' [-Wunused-const-variable]\n"
"static const char formatTextHtmlC[] = \"text/html\";");
    const QString error1 = QLatin1String(
"unknown type name 'errr'\n"
"  errr");
    const QString expectedError1 = QLatin1String(
"unknown type name 'errr'\n"
"errr"); // Line 2 trimmed.
    const QString error2 = QLatin1String(
"expected unqualified-id\n"
"void *QWindowsGdiNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs)");

    const QString clangClCompilerLog = QLatin1String(
"In file included from .\\qwindowseglcontext.cpp:40:\n"
"./qwindowseglcontext.h(282,15) :  warning: ")  + warning1 + QLatin1String("\n"
"5 warnings generated.\n"
".\\qwindowsclipboard.cpp(60,19) :  warning: ") + warning2 + QLatin1String("\n"
"                  ^\n"
".\\qwindowsclipboard.cpp(61,19) :  warning: ") + warning3 + QLatin1String("\n"
"                  ^\n"
"2 warnings generated.\n"
".\\qwindowsgdinativeinterface.cpp(48,3) :  error: ") + error1 + QLatin1String("\n"
"  ^\n"
".\\qwindowsgdinativeinterface.cpp(51,1) :  error: ") + error2 + QLatin1String("\n"
"^\n"
"2 errors generated.\n");

    const QString ignoredStderr = QLatin1String(
"NMAKE : fatal error U1077: 'D:\\opt\\LLVM64_390\\bin\\clang-cl.EXE' : return code '0x1'\n"
"Stop.");

    const QString input = clangClCompilerLog + ignoredStderr;
    const QString expectedStderr = ignoredStderr + QLatin1Char('\n');

    QTest::newRow("error")
            << input
            << OutputParserTester::STDERR
            << QString() << expectedStderr
            << (QList<Task>()
                << Task(Task::Warning, warning1,
                        Utils::FileName::fromUserInput(QLatin1String("./qwindowseglcontext.h")), 282,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning, warning2,
                        Utils::FileName::fromUserInput(QLatin1String(".\\qwindowsclipboard.cpp")), 60,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning, warning3,
                        Utils::FileName::fromUserInput(QLatin1String(".\\qwindowsclipboard.cpp")), 61,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error, expectedError1,
                        Utils::FileName::fromUserInput(QLatin1String(".\\qwindowsgdinativeinterface.cpp")), 48,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error, error2,
                        Utils::FileName::fromUserInput(QLatin1String(".\\qwindowsgdinativeinterface.cpp")), 51,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
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

#endif // WITH_TEST
