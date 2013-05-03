/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangparser.h"
#include "ldparser.h"
#include "projectexplorerconstants.h"

using namespace ProjectExplorer;

// opt. drive letter + filename: (2 brackets)
static const char * const FILE_PATTERN = "(<command line>|([A-Za-z]:)?[^:]+\\.[^:]+)";

ClangParser::ClangParser() :
    m_commandRegExp(QLatin1String("^clang(\\+\\+)?: +(fatal +)?(warning|error|note): (.*)$")),
    m_inLineRegExp(QLatin1String("^In (.*) included from (.*):(\\d+):$")),
    m_messageRegExp(QLatin1String("^") + QLatin1String(FILE_PATTERN) + QLatin1String("(:(\\d+):\\d+|\\((\\d+)\\) *): +(fatal +)?(error|warning|note): (.*)$")),
    m_summaryRegExp(QLatin1String("^\\d+ (warnings?|errors?)( and \\d (warnings?|errors?))? generated.$")),
    m_expectSnippet(false)
{
    setObjectName(QLatin1String("ClangParser"));

    appendOutputParser(new LdParser);
}

void ClangParser::stdError(const QString &line)
{
    const QString lne = rightTrimmed(line);
    if (m_summaryRegExp.indexIn(lne) > -1) {
        doFlush();
        m_expectSnippet = false;
        return;
    }

    if (m_commandRegExp.indexIn(lne) > -1) {
        m_expectSnippet = true;
        Task task(Task::Error,
                  m_commandRegExp.cap(4),
                  Utils::FileName(), /* filename */
                  -1, /* line */
                  Core::Id(Constants::TASK_CATEGORY_COMPILE));
        if (m_commandRegExp.cap(3) == QLatin1String("warning"))
            task.type = Task::Warning;
        else if (m_commandRegExp.cap(3) == QLatin1String("note"))
            task.type = Task::Unknown;
        newTask(task);
        return;
    }

    if (m_inLineRegExp.indexIn(lne) > -1) {
        m_expectSnippet = true;
        newTask(Task(Task::Unknown,
                     lne.trimmed(),
                     Utils::FileName::fromUserInput(m_inLineRegExp.cap(2)), /* filename */
                     m_inLineRegExp.cap(3).toInt(), /* line */
                     Core::Id(Constants::TASK_CATEGORY_COMPILE)));
        return;
    }

    if (m_messageRegExp.indexIn(lne) > -1) {
        m_expectSnippet = true;
        bool ok = false;
        int lineNo = m_messageRegExp.cap(4).toInt(&ok);
        if (!ok)
            lineNo = m_messageRegExp.cap(5).toInt(&ok);
        Task task(Task::Error,
                  m_messageRegExp.cap(8),
                  Utils::FileName::fromUserInput(m_messageRegExp.cap(1)), /* filename */
                  lineNo,
                  Core::Id(Constants::TASK_CATEGORY_COMPILE));
        if (m_messageRegExp.cap(7) == QLatin1String("warning"))
            task.type = Task::Warning;
        else if (m_messageRegExp.cap(7) == QLatin1String("note"))
            task.type = Task::Unknown;
        newTask(task);
        return;
    }

    if (m_expectSnippet) {
        amendDescription(lne, true);
        return;
    }

    IOutputParser::stdError(line);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "metatypedeclarations.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testClangOutputParser_data()
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

    QTest::newRow("clang++ warning")
            << QString::fromLatin1("clang++: warning: argument unused during compilation: '-mthreads'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("argument unused during compilation: '-mthreads'"),
                        Utils::FileName(), -1,
                        categoryCompile))
            << QString();
    QTest::newRow("clang++ error")
            << QString::fromLatin1("clang++: error: no input files [err_drv_no_input_files]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("no input files [err_drv_no_input_files]"),
                        Utils::FileName(), -1,
                        categoryCompile))
            << QString();
    QTest::newRow("complex warning")
            << QString::fromLatin1("In file included from ..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h:45:\n"
                                   "..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h(1425) :  warning: unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                                   "class Q_CORE_EXPORT QSysInfo {\n"
                                   "      ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In file included from ..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h:45:"),
                        Utils::FileName::fromUserInput(QLatin1String("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h")), 45,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                                      "class Q_CORE_EXPORT QSysInfo {\n"
                                      "      ^"),
                        Utils::FileName::fromUserInput(QLatin1String("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h")), 1425,
                        categoryCompile))
            << QString();
        QTest::newRow("note")
                << QString::fromLatin1("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h:1289:27: note: instantiated from:\n"
                                       "#    define Q_CORE_EXPORT Q_DECL_IMPORT\n"
                                       "                          ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (QList<ProjectExplorer::Task>()
                    << Task(Task::Unknown,
                            QLatin1String("instantiated from:\n"
                                          "#    define Q_CORE_EXPORT Q_DECL_IMPORT\n"
                                          "                          ^"),
                            Utils::FileName::fromUserInput(QLatin1String("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h")), 1289,
                            categoryCompile))
                << QString();
        QTest::newRow("fatal error")
                << QString::fromLatin1("/usr/include/c++/4.6/utility:68:10: fatal error: 'bits/c++config.h' file not found\n"
                                       "#include <bits/c++config.h>\n"
                                       "         ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (QList<ProjectExplorer::Task>()
                    << Task(Task::Error,
                            QLatin1String("'bits/c++config.h' file not found\n"
                                          "#include <bits/c++config.h>\n"
                                          "         ^"),
                            Utils::FileName::fromUserInput(QLatin1String("/usr/include/c++/4.6/utility")), 68,
                            categoryCompile))
                << QString();

        QTest::newRow("line confusion")
                << QString::fromLatin1("/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp:567:51: warning: ?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                       "            int x = option->rect.x() + horizontal ? 2 : 6;\n"
                                       "                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (QList<ProjectExplorer::Task>()
                    << Task(Task::Warning,
                            QLatin1String("?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                          "            int x = option->rect.x() + horizontal ? 2 : 6;\n"
                                          "                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^"),
                            Utils::FileName::fromUserInput(QLatin1String("/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp")), 567,
                            categoryCompile))
                << QString();
}

void ProjectExplorerPlugin::testClangOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new ClangParser);
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
