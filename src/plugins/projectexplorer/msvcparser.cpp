/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "msvcparser.h"
#include "projectexplorerconstants.h"
#include "buildmanager.h"

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

// As of MSVC 2015: "foo.cpp(42) :" -> "foo.cpp(42):"
static const char FILE_POS_PATTERN[] = "(cl|LINK|.+[^ ]) ?: ";
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
            bool ok = false;
            int n = fileName.mid(pos + 1, fileName.count() - pos - 2).toInt(&ok);
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
    if (line.startsWith(QLatin1String("Error:"))) {
        m_lastTask = Task(Task::Error,
                          line.mid(6).trimmed(), /* description */
                          Utils::FileName(), /* fileName */
                          -1, /* linenumber */
                          Constants::TASK_CATEGORY_COMPILE);
        m_lines = 1;
        return;
    }
    if (line.startsWith(QLatin1String("Warning:"))) {
        m_lastTask = Task(Task::Warning,
                          line.mid(8).trimmed(), /* description */
                          Utils::FileName(), /* fileName */
                          -1, /* linenumber */
                          Constants::TASK_CATEGORY_COMPILE);
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
    if (line.startsWith(QLatin1String("Error:"))) {
        m_lastTask = Task(Task::Error,
                          line.mid(6).trimmed(), /* description */
                          Utils::FileName(), /* fileName */
                          -1, /* linenumber */
                          Constants::TASK_CATEGORY_COMPILE);
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
        Task::TaskType type = Task::Unknown;
        const QString category = match.captured(3);
        if (category == QLatin1String("warning"))
            type = Task::Warning;
        else if (category == QLatin1String("error"))
            type = Task::Error;
        m_lastTask = Task(type, match.captured(4).trimmed() /* description */,
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

    QTest::newRow("labeled warning")
            << QString::fromLatin1("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter") << OutputParserTester::STDOUT
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

#endif // WITH_TEST
