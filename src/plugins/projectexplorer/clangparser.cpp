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

#include "clangparser.h"
#include "ldparser.h"
#include "lldparser.h"
#include "projectexplorerconstants.h"

using namespace ProjectExplorer;
using namespace Utils;

static Task::TaskType taskType(const QString &capture)
{
    const QString lc = capture.toLower();
    if (lc == QLatin1String("error"))
        return Task::Error;
    if (lc == QLatin1String("warning"))
        return Task::Warning;
    return Task::Unknown;
}

// opt. drive letter + filename: (2 brackets)
static const char *const FILE_PATTERN = "(<command line>|([A-Za-z]:)?[^:]+\\.[^:]+)";

ClangParser::ClangParser() :
    m_commandRegExp(QLatin1String("^clang(\\+\\+)?: +(fatal +)?(warning|error|note): (.*)$")),
    m_inLineRegExp(QLatin1String("^In (.*?) included from (.*?):(\\d+):$")),
    m_messageRegExp(QLatin1Char('^') + QLatin1String(FILE_PATTERN) + QLatin1String("(:(\\d+):\\d+|\\((\\d+)\\) *): +(fatal +)?(error|warning|note): (.*)$")),
    m_summaryRegExp(QLatin1String("^\\d+ (warnings?|errors?)( and \\d (warnings?|errors?))? generated.$")),
    m_codesignRegExp(QLatin1String("^Code ?Sign error: (.*)$")),
    m_expectSnippet(false)
{
    setObjectName(QLatin1String("ClangParser"));
}

QList<OutputLineParser *> ClangParser::clangParserSuite()
{
    return {new ClangParser, new Internal::LldParser, new LdParser};
}

OutputLineParser::Result ClangParser::handleLine(const QString &line, OutputFormat type)
{
    if (type != StdErrFormat)
        return Status::NotHandled;
    const QString lne = rightTrimmed(line);
    QRegularExpressionMatch match = m_summaryRegExp.match(lne);
    if (match.hasMatch()) {
        flush();
        m_expectSnippet = false;
        return Status::Done;
    }

    match = m_commandRegExp.match(lne);
    if (match.hasMatch()) {
        m_expectSnippet = true;
        createOrAmendTask(taskType(match.captured(3)), match.captured(4), lne);
        return Status::InProgress;
    }

    match = m_inLineRegExp.match(lne);
    if (match.hasMatch()) {
        m_expectSnippet = true;
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(2)));
        const int lineNo = match.captured(3).toInt();
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, match, 2);
        createOrAmendTask(Task::Unknown, lne.trimmed(), lne, false, filePath, lineNo, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    match = m_messageRegExp.match(lne);
    if (match.hasMatch()) {
        m_expectSnippet = true;
        bool ok = false;
        int lineNo = match.captured(4).toInt(&ok);
        if (!ok)
            lineNo = match.captured(5).toInt(&ok);
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(1)));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, match, 1);
        createOrAmendTask(taskType(match.captured(7)), match.captured(8), lne, false,
                          filePath, lineNo, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    match = m_codesignRegExp.match(lne);
    if (match.hasMatch()) {
        m_expectSnippet = true;
        createOrAmendTask(Task::Error, match.captured(1), lne, false);
        return Status::InProgress;
    }

    if (m_expectSnippet) {
        createOrAmendTask(Task::Unknown, lne, lne, true);
        return Status::InProgress;
    }

    return Status::NotHandled;
}

Utils::Id ClangParser::id()
{
    return Utils::Id("ProjectExplorer.OutputParser.Clang");
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testClangOutputParser_data()
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

    QTest::newRow("clang++ warning")
            << QString::fromLatin1("clang++: warning: argument unused during compilation: '-mthreads'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "argument unused during compilation: '-mthreads'"))
            << QString();

    QTest::newRow("clang++ error")
            << QString::fromLatin1("clang++: error: no input files [err_drv_no_input_files]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "no input files [err_drv_no_input_files]"))
            << QString();

    QTest::newRow("complex warning")
            << QString::fromLatin1("In file included from ..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h:45:\n"
                                   "..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h(1425) :  warning: unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                                   "class Q_CORE_EXPORT QSysInfo {\n"
                                   "      ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(
                   Task::Warning,
                   "unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                   "In file included from ..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qnamespace.h:45:\n"
                   "..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h(1425) :  warning: unknown attribute 'dllimport' ignored [-Wunknown-attributes]\n"
                   "class Q_CORE_EXPORT QSysInfo {\n"
                   "      ^",
                   FilePath::fromUserInput("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h"),
                   1425)}
            << QString();

        QTest::newRow("note")
                << QString::fromLatin1("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h:1289:27: note: instantiated from:\n"
                                       "#    define Q_CORE_EXPORT Q_DECL_IMPORT\n"
                                       "                          ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (Tasks()
                    << CompileTask(Task::Unknown,
                                   "instantiated from:\n"
                                   "..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h:1289:27: note: instantiated from:\n"
                                   "#    define Q_CORE_EXPORT Q_DECL_IMPORT\n"
                                   "                          ^",
                                   FilePath::fromUserInput("..\\..\\..\\QtSDK1.1\\Desktop\\Qt\\4.7.3\\mingw\\include/QtCore/qglobal.h"),
                                   1289))
                << QString();

        QTest::newRow("fatal error")
                << QString::fromLatin1("/usr/include/c++/4.6/utility:68:10: fatal error: 'bits/c++config.h' file not found\n"
                                       "#include <bits/c++config.h>\n"
                                       "         ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (Tasks()
                    << CompileTask(Task::Error,
                                   "'bits/c++config.h' file not found\n"
                                   "/usr/include/c++/4.6/utility:68:10: fatal error: 'bits/c++config.h' file not found\n"
                                   "#include <bits/c++config.h>\n"
                                   "         ^",
                                   FilePath::fromUserInput("/usr/include/c++/4.6/utility"),
                                   68))
                << QString();

        QTest::newRow("line confusion")
                << QString::fromLatin1("/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp:567:51: warning: ?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                       "            int x = option->rect.x() + horizontal ? 2 : 6;\n"
                                       "                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (Tasks()
                    << CompileTask(Task::Warning,
                                   "?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                   "/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp:567:51: warning: ?: has lower precedence than +; + will be evaluated first [-Wparentheses]\n"
                                   "            int x = option->rect.x() + horizontal ? 2 : 6;\n"
                                   "                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^",
                                   FilePath::fromUserInput("/home/code/src/creator/src/plugins/coreplugin/manhattanstyle.cpp"),
                                   567))
                << QString();

        QTest::newRow("code sign error")
                << QString::fromLatin1("Check dependencies\n"
                                       "Code Sign error: No matching provisioning profiles found: No provisioning profiles with a valid signing identity (i.e. certificate and private key pair) were found.\n"
                                       "CodeSign error: code signing is required for product type 'Application' in SDK 'iOS 7.0'")
                << OutputParserTester::STDERR
                << QString() << QString::fromLatin1("Check dependencies\n")
                << (Tasks()
                    << CompileTask(Task::Error,
                                   "No matching provisioning profiles found: No provisioning profiles with a valid signing identity (i.e. certificate and private key pair) were found.")
                    << CompileTask(Task::Error,
                                   "code signing is required for product type 'Application' in SDK 'iOS 7.0'"))
                << QString();

        QTest::newRow("moc note")
                << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
                << OutputParserTester::STDERR
                << QString() << QString()
                << (Tasks()
                    << CompileTask(Task::Unknown,
                                   "Note: No relevant classes found. No output generated.",
                                   FilePath::fromUserInput("/home/qtwebkithelpviewer.h")))
                << QString();
}

void ProjectExplorerPlugin::testClangOutputParser()
{
    OutputParserTester testbench;
    testbench.setLineParsers(ClangParser::clangParserSuite());
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(Tasks, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}
#endif
