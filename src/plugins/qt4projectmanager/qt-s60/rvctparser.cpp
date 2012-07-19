/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "rvctparser.h"
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;
using namespace Qt4ProjectManager;

RvctParser::RvctParser() :
    m_task(0)
{
    setObjectName(QLatin1String("RvctParser"));
    // Start of a error or warning:
    m_warningOrError.setPattern(QLatin1String("^\"([^\\(\\)]+[^\\d])\", line (\\d+):(\\s(Warning|Error):)\\s+([^\\s].*)$"));
    m_warningOrError.setMinimal(true);

    // Last message for any file with warnings/errors.
    m_wrapUpTask.setPattern(QLatin1String("^([^\\(\\)]+[^\\d]):\\s(\\d+) warnings?,\\s(\\d+) errors?$"));
    m_wrapUpTask.setMinimal(true);

    // linker problems:
    m_genericProblem.setPattern(QLatin1String("^(Error|Warning): (.*)$"));
    m_genericProblem.setMinimal(true);
}

RvctParser::~RvctParser()
{
    sendTask();
}

void RvctParser::stdError(const QString &line)
{
    QString lne = line.trimmed();
    if (m_genericProblem.indexIn(lne) > -1) {
        sendTask();

        m_task = new Task(Task::Error,
                          m_genericProblem.cap(2) /* description */,
                          Utils::FileName(),
                          -1 /* linenumber */,
                          Core::Id(TASK_CATEGORY_COMPILE));
        if (m_warningOrError.cap(4) == QLatin1String("Warning"))
            m_task->type = Task::Warning;
        else if (m_warningOrError.cap(4) == QLatin1String("Error"))
            m_task->type = Task::Error;

        return;
   }
   if (m_warningOrError.indexIn(lne) > -1) {
       sendTask();

       m_task = new Task(Task::Unknown,
                         m_warningOrError.cap(5) /* description */,
                         Utils::FileName::fromUserInput(m_warningOrError.cap(1)) /* file */,
                         m_warningOrError.cap(2).toInt() /* line */,
                         Core::Id(TASK_CATEGORY_COMPILE));
       if (m_warningOrError.cap(4) == QLatin1String("Warning"))
           m_task->type = Task::Warning;
       else if (m_warningOrError.cap(4) == QLatin1String("Error"))
           m_task->type = Task::Error;
       return;
   }

   if (m_wrapUpTask.indexIn(lne) > -1) {
       sendTask();
       return;
   }
   if (m_task) {
       QString description = line;
       if (description.startsWith(QLatin1String("  ")))
           description = description.mid(2);
       if (description.endsWith(QLatin1Char('\n')))
           description.chop(1);
       if (m_task->formats.isEmpty()) {
           QTextLayout::FormatRange fr;
           fr.start = m_task->description.count(); // incl. '\n' we are about to add!
           fr.length = description.count() - 1;
           fr.format.setFontItalic(true);
           m_task->formats.append(fr);
       } else {
           m_task->formats[0].length += description.count() - 2 + 1;
       }
       m_task->description += QLatin1Char('\n') + description;

       // Wrap up license error:
       if (description.endsWith(QLatin1String("at \"www.macrovision.com\".")))
           sendTask();

       return;
   }
   IOutputParser::stdError(line);
}

void RvctParser::sendTask()
{
    if (!m_task)
        return;
    emit addTask(*m_task);
    delete m_task;
    m_task = 0;
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qt4projectmanagerplugin.h"
#   include <projectexplorer/metatypedeclarations.h>
#   include <projectexplorer/outputparser_test.h>

using namespace Qt4ProjectManager::Internal;

void Qt4ProjectManagerPlugin::testRvctOutputParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

    const Core::Id categoryCompile = Core::Id(Constants::TASK_CATEGORY_COMPILE);
    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<ProjectExplorer::Task>()
            << QString();

    QTest::newRow("Rvct warning")
            << QString::fromLatin1("\"../../../../s60-sdk/epoc32/include/stdapis/stlport/stl/_limits.h\", line 256: Warning:  #68-D: integer conversion resulted in a change of sign\n"
                                   "    : public _Integer_limits<char, CHAR_MIN, CHAR_MAX, -1, true>\n"
                                   "                                   ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("#68-D: integer conversion resulted in a change of sign\n"
                                      "  : public _Integer_limits<char, CHAR_MIN, CHAR_MAX, -1, true>\n"
                                      "                                 ^"),
                        Utils::FileName::fromUserInput("../../../../s60-sdk/epoc32/include/stdapis/stlport/stl/_limits.h"), 256,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("Rvct error")
            << QString::fromLatin1("\"mainwindow.cpp\", line 22: Error:  #20: identifier \"e\" is undefined\n"
                                   "      delete ui;e\n"
                                   "                ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("#20: identifier \"e\" is undefined\n"
                                      "    delete ui;e\n"
                                      "              ^"),
                        Utils::FileName::fromUserInput("mainwindow.cpp"), 22,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("Rvct linking error")
            << QString::fromLatin1("Error: L6218E: Undefined symbol MainWindow::sth() (referred from mainwindow.o)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("L6218E: Undefined symbol MainWindow::sth() (referred from mainwindow.o)"),
                        Utils::FileName(), -1,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("Rvct license error")
            << QString::fromLatin1("Error: C3397E: Cannot obtain license for Compiler (feature compiler) with license version >= 2.2:\n"
                                   "Cannot find license file.\n"
                                   " The license files (or license server system network addresses) attempted are \n"
                                   "listed below.  Use LM_LICENSE_FILE to use a different license file,\n"
                                   " or contact your software provider for a license file.\n"
                                   "Feature:       compiler\n"
                                   "Filename:      /usr/local/flexlm/licenses/license.dat\n"
                                   "License path:  /usr/local/flexlm/licenses/license.dat\n"
                                   "FLEXnet Licensing error:-1,359.  System Error: 2 \"No such file or directory\"\n"
                                   "For further information, refer to the FLEXnet Licensing End User Guide,\n"
                                   "available at \"www.macrovision.com\".")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("C3397E: Cannot obtain license for Compiler (feature compiler) with license version >= 2.2:\n"
                                      "Cannot find license file.\n"
                                      " The license files (or license server system network addresses) attempted are \n"
                                      "listed below.  Use LM_LICENSE_FILE to use a different license file,\n"
                                      " or contact your software provider for a license file.\n"
                                      "Feature:       compiler\n"
                                      "Filename:      /usr/local/flexlm/licenses/license.dat\n"
                                      "License path:  /usr/local/flexlm/licenses/license.dat\n"
                                      "FLEXnet Licensing error:-1,359.  System Error: 2 \"No such file or directory\"\n"
                                      "For further information, refer to the FLEXnet Licensing End User Guide,\n"
                                      "available at \"www.macrovision.com\"."),
                        Utils::FileName(), -1,
                        categoryCompile)
                )
            << QString();
}

void Qt4ProjectManagerPlugin::testRvctOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new RvctParser);
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
