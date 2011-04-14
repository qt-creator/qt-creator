/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "msvcparser.h"
#include "projectexplorerconstants.h"

namespace {
const char * const FILE_POS_PATTERN = "(cl|LINK|[^\\(]+)(\\((\\d+)\\))?";
const char * const ERROR_PATTERN = "[A-Z]+\\d\\d\\d\\d ?:";
}

using namespace ProjectExplorer;

MsvcParser::MsvcParser()
{
    setObjectName(QLatin1String("MsvcParser"));
    m_compileRegExp.setPattern(QString::fromLatin1("^") + QLatin1String(FILE_POS_PATTERN)
                               + QLatin1String(" : .*(warning|error) (")
                               + QLatin1String(ERROR_PATTERN) + QLatin1String(".*)$"));
    m_compileRegExp.setMinimal(true);
    m_additionalInfoRegExp.setPattern(QString::fromLatin1("^        ")
                                      + QLatin1String(FILE_POS_PATTERN)
                                      + QLatin1String(" : (.*)$"));
    m_additionalInfoRegExp.setMinimal(true);
}

MsvcParser::~MsvcParser()
{
    sendQueuedTask();
}

void MsvcParser::stdOutput(const QString &line)
{
    if (line.startsWith(QLatin1String("        ")) && m_additionalInfoRegExp.indexIn(line) < 0) {
        if (m_lastTask.isNull())
            return;

        m_lastTask.description.append(QChar('\n'));
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
        return;
    }

    if (processCompileLine(line))
        return;
    if (m_additionalInfoRegExp.indexIn(line) > -1) {
        m_lastTask = Task(Task::Unknown,
                          m_additionalInfoRegExp.cap(4).trimmed(), /* description */
                          m_additionalInfoRegExp.cap(1), /* fileName */
                          m_additionalInfoRegExp.cap(3).toInt(), /* linenumber */
                          Constants::TASK_CATEGORY_COMPILE);
        return;
    }
    IOutputParser::stdOutput(line);
}

void MsvcParser::stdError(const QString &line)
{
    if (processCompileLine(line))
        return;
    IOutputParser::stdError(line);
}

bool MsvcParser::processCompileLine(const QString &line)
{
    sendQueuedTask();

    if (m_compileRegExp.indexIn(line) > -1) {
        QString fileName = m_compileRegExp.cap(1);
        if (fileName == QLatin1String("LINK") || fileName == QLatin1String("cl"))
            fileName.clear();
        int linenumber = -1;
        if (!m_compileRegExp.cap(3).isEmpty())
            linenumber = m_compileRegExp.cap(3).toInt();
        m_lastTask = Task(Task::Unknown,
                          m_compileRegExp.cap(5).trimmed() /* description */,
                          fileName, linenumber,
                          Constants::TASK_CATEGORY_COMPILE);
        if (m_compileRegExp.cap(4) == QLatin1String("warning"))
            m_lastTask.type = Task::Warning;
        else if (m_compileRegExp.cap(4) == QLatin1String("error"))
            m_lastTask.type = Task::Error;

        return true;
    }
    return false;
}

void MsvcParser::sendQueuedTask()
{
    if (m_lastTask.isNull())
        return;

    addTask(m_lastTask);
    m_lastTask = Task();
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"

#   include "projectexplorer/outputparser_test.h"

using namespace ProjectExplorer::Internal;

void ProjectExplorerPlugin::testMsvcOutputParsers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext")
            << QList<ProjectExplorer::Task>()
            << QString();

    QTest::newRow("labeled error")
            << QString::fromLatin1("qmlstandalone\\main.cpp(54) : error C4716: 'findUnresolvedModule' : must return a value") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Error,
                                                       QLatin1String("C4716: 'findUnresolvedModule' : must return a value"),
                                                       QLatin1String("qmlstandalone\\main.cpp"), 54,
                                                       QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("labeled warning")
            << QString::fromLatin1("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp(69) : warning C4100: 'something' : unreferenced formal parameter") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("C4100: 'something' : unreferenced formal parameter"),
                                                       QLatin1String("x:\\src\\plugins\\projectexplorer\\msvcparser.cpp"), 69,
                                                       QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("additional information")
            << QString::fromLatin1("x:\\src\\plugins\\texteditor\\icompletioncollector.h(50) : warning C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'\n"
                                   "        x:\\src\\plugins\\texteditor\\completionsupport.h(39) : see declaration of 'TextEditor::CompletionItem'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("C4099: 'TextEditor::CompletionItem' : type name first seen using 'struct' now seen using 'class'"),
                        QLatin1String("x:\\src\\plugins\\texteditor\\icompletioncollector.h"), 50,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
                << Task(Task::Unknown,
                        QLatin1String("see declaration of 'TextEditor::CompletionItem'"),
                        QLatin1String("x:\\src\\plugins\\texteditor\\completionsupport.h"), 39,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    QTest::newRow("fatal linker error")
            << QString::fromLatin1("LINK : fatal error LNK1146: no argument specified with option '/LIBPATH:'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("LNK1146: no argument specified with option '/LIBPATH:'"),
                        QString(), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();

    // This actually comes through stderr!
    QTest::newRow("command line warning")
            << QString::fromLatin1("cl : Command line warning D9002 : ignoring unknown option '-fopenmp'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("D9002 : ignoring unknown option '-fopenmp'"),
                        QString(), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
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
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("C2440: 'initializing' : cannot convert from 'int' to 'std::_Tree<_Traits>::iterator'\n"
                                      "with\n"
                                      "[\n"
                                      "    _Traits=std::_Tmap_traits<int,double,std::less<int>,std::allocator<std::pair<const int,double>>,false>\n"
                                      "]\n"
                                      "No constructor could take the source type, or constructor overload resolution was ambiguous"),
                        QLatin1String("..\\untitled\\main.cpp"), 19,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("Linker error 1")
            << QString::fromLatin1("main.obj : error LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("LNK2019: unresolved external symbol \"public: void __thiscall Data::doit(void)\" (?doit@Data@@QAEXXZ) referenced in function _main"),
                        QLatin1String("main.obj"), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
            << QString();
    QTest::newRow("Linker error 2")
            << QString::fromLatin1("debug\\Experimentation.exe : fatal error LNK1120: 1 unresolved externals")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("LNK1120: 1 unresolved externals"),
                        QLatin1String("debug\\Experimentation.exe"), -1,
                        QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)))
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
#endif

