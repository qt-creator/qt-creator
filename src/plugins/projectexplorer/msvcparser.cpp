/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "msvcparser.h"
#include "projectexplorerconstants.h"

namespace {
const char * const FILE_POS_PATTERN = "([^\\(]+)\\((\\d+)\\)\\s:";
}

using namespace ProjectExplorer;

MsvcParser::MsvcParser()
{
    m_compileRegExp.setPattern(QString::fromLatin1("^") + QLatin1String(FILE_POS_PATTERN) + QLatin1String("([^:\\d]+)\\s([A-Z]+(\\d+):.*)$"));
    m_compileRegExp.setMinimal(true);
    m_additionalInfoRegExp.setPattern(QString::fromLatin1("^        ") + QLatin1String(FILE_POS_PATTERN) + QLatin1String("\\s(.*)$"));
    m_additionalInfoRegExp.setMinimal(true);
    m_linkRegExp.setPattern(QString::fromLatin1("^") + QLatin1String(FILE_POS_PATTERN) + QLatin1String("[^:\\d]+(\\d+):(.*)$"));
    m_linkRegExp.setMinimal(true);
    m_nonFileRegExp.setPattern(QLatin1String("(^LINK|cl) : .*(error|warning) (.*)$"));
    m_nonFileRegExp.setMinimal(true);
}

void MsvcParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();
    if (m_compileRegExp.indexIn(lne) > -1 && m_compileRegExp.numCaptures() == 5) {
        Task task(Task::Unknown,
                  m_compileRegExp.cap(4) /* description */,
                  m_compileRegExp.cap(1) /* filename */,
                  m_compileRegExp.cap(2).toInt() /* linenumber */,
                  Constants::TASK_CATEGORY_COMPILE);
        if (m_compileRegExp.cap(3) == QLatin1String(" warning"))
            task.type = Task::Warning;
        else if (m_compileRegExp.cap(3) == QLatin1String(" error"))
            task.type = Task::Error;
        else
            task.type = toType(m_compileRegExp.cap(5).toInt());
        addTask(task);
        return;
    }
    if (m_additionalInfoRegExp.indexIn(line) > -1 && m_additionalInfoRegExp.numCaptures() == 3) {
        addTask(Task(Task::Unknown,
                     m_additionalInfoRegExp.cap(3),
                     m_additionalInfoRegExp.cap(1),
                     m_additionalInfoRegExp.cap(2).toInt(),
                     Constants::TASK_CATEGORY_COMPILE));
        return;
    }
    if (m_linkRegExp.indexIn(lne) > -1 && m_linkRegExp.numCaptures() == 3) {
        QString fileName = m_linkRegExp.cap(1);
        if (fileName.contains(QLatin1String("LINK"), Qt::CaseSensitive))
            fileName.clear();

        emit addTask(Task(toType(m_linkRegExp.cap(2).toInt()) /* task type */,
                          m_linkRegExp.cap(3) /* description */,
                          fileName /* filename */,
                          -1 /* line number */,
                          Constants::TASK_CATEGORY_COMPILE));
        return;
    }
    if (m_nonFileRegExp.indexIn(lne) > -1) {
        Task::TaskType type = Task::Unknown;
        if (m_nonFileRegExp.cap(2) == QLatin1String("warning"))
            type = Task::Warning;
        if (m_nonFileRegExp.cap(2) == QLatin1String("error"))
            type = Task::Error;
        emit addTask(Task(type, m_nonFileRegExp.cap(3) /* description */,
                          QString(), -1, Constants::TASK_CATEGORY_COMPILE));
        return;
    }
    IOutputParser::stdOutput(line);
}

void MsvcParser::stdError(const QString &line)
{
    if (m_nonFileRegExp.indexIn(line) > -1) {
        Task::TaskType type = Task::Unknown;
        if (m_nonFileRegExp.cap(2) == QLatin1String("warning"))
            type = Task::Warning;
        if (m_nonFileRegExp.cap(2) == QLatin1String("error"))
            type = Task::Error;
        emit addTask(Task(type, m_nonFileRegExp.cap(3) /* description */,
                          QString(), -1, Constants::TASK_CATEGORY_COMPILE));
        return;
    }
    IOutputParser::stdError(line);
}

Task::TaskType MsvcParser::toType(int number)
{
    // This is unfortunately not true for all possible kinds of errors, but better
    // than not having a fallback at all!
    if (number == 0)
        return Task::Unknown;
    else if (number > 4000 && number < 5000)
        return Task::Warning;
    else
        return Task::Error;
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

