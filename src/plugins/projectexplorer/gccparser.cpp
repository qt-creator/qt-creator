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

#include "gccparser.h"
#include "ldparser.h"
#include "lldparser.h"
#include "task.h"
#include "projectexplorerconstants.h"
#include "buildmanager.h"

#include <utils/qtcassert.h>

#include <numeric>

using namespace ProjectExplorer;
using namespace Utils;

// opt. drive letter + filename: (2 brackets)
static const char FILE_PATTERN[] = "(<command[ -]line>|([A-Za-z]:)?[^:]+):";
static const char COMMAND_PATTERN[] = "^(.*?[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(gcc|g\\+\\+)(-[0-9\\.]+)?(\\.exe)?: ";

GccParser::GccParser()
{
    setObjectName(QLatin1String("GCCParser"));
    m_regExp.setPattern(QLatin1Char('^') + QLatin1String(FILE_PATTERN)
                        + QLatin1String("(?:(?:(\\d+):(\\d+:)?)|\\(.*\\):)\\s+((fatal |#)?(warning|error|note):?\\s)?([^\\s].+)$"));
    QTC_CHECK(m_regExp.isValid());

    m_regExpScope.setPattern(QLatin1Char('^') + FILE_PATTERN
                                    + "(?:(\\d+):)?(\\d+:)?\\s+((?:In .*function .*|At global scope):)$");
    QTC_CHECK(m_regExpScope.isValid());

    m_regExpIncluded.setPattern(QString::fromLatin1("\\bfrom\\s") + QLatin1String(FILE_PATTERN)
                                + QLatin1String("(\\d+)(:\\d+)?[,:]?$"));
    QTC_CHECK(m_regExpIncluded.isValid());

    m_regExpInlined.setPattern(QString::fromLatin1("\\binlined from\\s.* at ")
                               + FILE_PATTERN + "(\\d+)(:\\d+)?[,:]?$");
    QTC_CHECK(m_regExpInlined.isValid());

    // optional path with trailing slash
    // optional arm-linux-none-thingy
    // name of executable
    // optional trailing version number
    // optional .exe postfix
    m_regExpGccNames.setPattern(QLatin1String(COMMAND_PATTERN));
    QTC_CHECK(m_regExpGccNames.isValid());
}

Utils::Id GccParser::id()
{
    return Utils::Id("ProjectExplorer.OutputParser.Gcc");
}

QList<OutputLineParser *> GccParser::gccParserSuite()
{
    return {new GccParser, new Internal::LldParser, new LdParser};
}

void GccParser::createOrAmendTask(
        Task::TaskType type,
        const QString &description,
        const QString &originalLine,
        bool forceAmend,
        const FilePath &file,
        int line,
        const LinkSpecs &linkSpecs
        )
{
    const bool amend = !m_currentTask.isNull() && (forceAmend || isContinuation(originalLine));
    if (!amend) {
        flush();
        m_currentTask = CompileTask(type, description, file, line);
        m_currentTask.details.append(originalLine);
        m_linkSpecs = linkSpecs;
        m_lines = 1;
        return;
    }

    LinkSpecs adaptedLinkSpecs = linkSpecs;
    const int offset = std::accumulate(m_currentTask.details.cbegin(), m_currentTask.details.cend(),
            0, [](int total, const QString &line) { return total + line.length() + 1;});
    for (LinkSpec &ls : adaptedLinkSpecs)
        ls.startPos += offset;
    m_linkSpecs << adaptedLinkSpecs;
    m_currentTask.details.append(originalLine);

    // Check whether the new line is more relevant than the previous ones.
    if ((m_currentTask.type != Task::Error && type == Task::Error)
            || (m_currentTask.type == Task::Unknown && type != Task::Unknown)) {
        m_currentTask.type = type;
        m_currentTask.summary = description;
        if (!file.isEmpty()) {
            m_currentTask.setFile(file);
            m_currentTask.line = line;
        }
    }
    ++m_lines;
}

void GccParser::flush()
{
    if (m_currentTask.isNull())
        return;

    // If there is only one line of details, then it is the line that we generated
    // the summary from. Remove it, because it does not add any information.
    if (m_currentTask.details.count() == 1)
        m_currentTask.details.clear();

    setDetailsFormat(m_currentTask, m_linkSpecs);
    Task t = m_currentTask;
    m_currentTask.clear();
    m_linkSpecs.clear();
    scheduleTask(t, m_lines, 1);
    m_lines = 0;
}

OutputLineParser::Result GccParser::handleLine(const QString &line, OutputFormat type)
{
    if (type == StdOutFormat) {
        flush();
        return Status::NotHandled;
    }

    const QString lne = rightTrimmed(line);

    // Blacklist some lines to not handle them:
    if (lne.startsWith(QLatin1String("TeamBuilder ")) ||
        lne.startsWith(QLatin1String("distcc["))) {
        return Status::NotHandled;
    }

    // Handle misc issues:
    if (lne.startsWith(QLatin1String("ERROR:")) || lne == QLatin1String("* cpp failed")) {
        createOrAmendTask(Task::Error, lne, lne);
        return Status::InProgress;
    }

    QRegularExpressionMatch match = m_regExpGccNames.match(lne);
    if (match.hasMatch()) {
        QString description = lne.mid(match.capturedLength());
        Task::TaskType type = Task::Error;
        if (description.startsWith(QLatin1String("warning: "))) {
            type = Task::Warning;
            description = description.mid(9);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            description = description.mid(7);
        }
        createOrAmendTask(type, description, lne);
        return Status::InProgress;
    }

    match = m_regExpIncluded.match(lne);
    if (!match.hasMatch())
        match = m_regExpInlined.match(lne);
    if (match.hasMatch()) {
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(1)));
        const int lineNo = match.captured(3).toInt();
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, match, 1);
        createOrAmendTask(Task::Unknown, lne.trimmed(), lne, false, filePath, lineNo, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    match = m_regExp.match(lne);
    if (match.hasMatch()) {
        int lineno = match.captured(3).toInt();
        Task::TaskType type = Task::Unknown;
        QString description = match.captured(8);
        if (match.captured(7) == QLatin1String("warning"))
            type = Task::Warning;
        else if (match.captured(7) == QLatin1String("error") ||
                 description.startsWith(QLatin1String("undefined reference to")) ||
                 description.startsWith(QLatin1String("multiple definition of")))
            type = Task::Error;
        // Prepend "#warning" or "#error" if that triggered the match on (warning|error)
        // We want those to show how the warning was triggered
        if (match.captured(5).startsWith(QLatin1Char('#')))
            description = match.captured(5) + description;

        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(1)));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineno, match, 1);
        createOrAmendTask(type, description, lne, false, filePath, lineno, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    match = m_regExpScope.match(lne);
    if (match.hasMatch()) {
        const int lineno = match.captured(3).toInt();
        const QString description = match.captured(5);
        const FilePath filePath = absoluteFilePath(FilePath::fromUserInput(match.captured(1)));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineno, match, 1);
        createOrAmendTask(Task::Unknown, description, lne, false, filePath, lineno, linkSpecs);
        return {Status::InProgress, linkSpecs};
    }

    if ((lne.startsWith(' ') && !m_currentTask.isNull()) || isContinuation(lne)) {
        createOrAmendTask(Task::Unknown, lne, lne, true);
        return Status::InProgress;
    }

    flush();
    return Status::NotHandled;
}

bool GccParser::isContinuation(const QString &newLine) const
{
    return !m_currentTask.isNull()
            && (m_currentTask.details.last().endsWith(':')
                || m_currentTask.details.last().endsWith(',')
                || newLine.contains("within this context")
                || newLine.contains("note:"));
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "projectexplorer.h"
#   include "outputparser_test.h"

void ProjectExplorerPlugin::testGccOutputParsers_data()
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

    QTest::newRow("ar output")
            << QString::fromLatin1("../../../../x86/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-ar: creating lib/libSkyView.a") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("../../../../x86/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-ar: creating lib/libSkyView.a\n")
            << Tasks()
            << QString();

    QTest::newRow("GCCE error")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "/temp/test/untitled8/main.cpp:9: error: `sfasdf' undeclared (first use this function)\n"
                                   "/temp/test/untitled8/main.cpp:9: error: (Each undeclared identifier is reported only once for each function it appears in.)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "`sfasdf' undeclared (first use this function)\n"
                               "/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                               "/temp/test/untitled8/main.cpp:9: error: `sfasdf' undeclared (first use this function)",
                               FilePath::fromUserInput("/temp/test/untitled8/main.cpp"),
                               9)
                << CompileTask(Task::Error,
                               "(Each undeclared identifier is reported only once for each function it appears in.)",
                               FilePath::fromUserInput("/temp/test/untitled8/main.cpp"),
                               9))
            << QString();

    QTest::newRow("GCCE warning")
            << QString::fromLatin1("/src/corelib/global/qglobal.h:1635: warning: inline function `QDebug qDebug()' used but never defined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "inline function `QDebug qDebug()' used but never defined",
                               FilePath::fromUserInput("/src/corelib/global/qglobal.h"),
                               1635))
            << QString();

    QTest::newRow("warning")
            << QString::fromLatin1("main.cpp:7:2: warning: Some warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "Some warning",
                               FilePath::fromUserInput("main.cpp"),
                               7))
            << QString();

    QTest::newRow("GCCE #error")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:7: #error Symbian error")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "#error Symbian error",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8\\main.cpp"),
                               7))
            << QString();

    // Symbian reports #warning(s) twice (using different syntax).
    QTest::newRow("GCCE #warning1")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:8: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "#warning Symbian warning",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8\\main.cpp"),
                               8))
            << QString();

    QTest::newRow("GCCE #warning2")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp:8:2: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "#warning Symbian warning",
                               FilePath::fromUserInput("/temp/test/untitled8/main.cpp"),
                               8))
            << QString();

    QTest::newRow("Undefined reference (debug)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'\n"
                               "main.o: In function `main':\n"
                               "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8/main.cpp"),
                               8)
                << CompileTask(Task::Error,
                               "collect2: ld returned 1 exit status"))
            << QString();

    QTest::newRow("Undefined reference (release)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'\n"
                               "main.o: In function `main':\n"
                               "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("C:\\temp\\test\\untitled8/main.cpp"))
                << CompileTask(Task::Error,
                               "collect2: ld returned 1 exit status"))
            << QString();

    QTest::newRow("linker: dll format not recognized")
            << QString::fromLatin1("c:\\Qt\\4.6\\lib/QtGuid4.dll: file not recognized: File format not recognized")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "file not recognized: File format not recognized",
                               FilePath::fromUserInput("c:\\Qt\\4.6\\lib/QtGuid4.dll")))
            << QString();

    QTest::newRow("Invalid rpath")
            << QString::fromLatin1("g++: /usr/local/lib: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "/usr/local/lib: No such file or directory"))
            << QString();

    QTest::newRow("unused variable")
            << QString::fromLatin1("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp: In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2114: warning: unused variable 'index'\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2115: warning: unused variable 'handler'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "unused variable 'index'\n"
                               "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp: In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':\n"
                               "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2114: warning: unused variable 'index'",
                               FilePath::fromUserInput("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"),
                               2114)
                << CompileTask(Task::Warning,
                               "unused variable 'handler'",
                               FilePath::fromUserInput("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"),
                               2115))
            << QString();

    QTest::newRow("gnumakeparser.cpp errors")
            << QString::fromLatin1("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp: In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected primary-expression before ':' token\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected ';' before ':' token")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "expected primary-expression before ':' token\n"
                               "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp: In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':\n"
                               "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected primary-expression before ':' token",
                               FilePath::fromUserInput("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"),
                               264)
                << CompileTask(Task::Error,
                               "expected ';' before ':' token",
                               FilePath::fromUserInput("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"),
                               264))
            << QString();

    QTest::newRow("distcc error(QTCREATORBUG-904)")
            << QString::fromLatin1("distcc[73168] (dcc_get_hostlist) Warning: no hostlist is set; can't distribute work\n"
                                   "distcc[73168] (dcc_build_somewhere) Warning: failed to distribute, running locally instead")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("distcc[73168] (dcc_get_hostlist) Warning: no hostlist is set; can't distribute work\n"
                                                "distcc[73168] (dcc_build_somewhere) Warning: failed to distribute, running locally instead\n")
            << Tasks()
            << QString();

    QTest::newRow("ld warning (QTCREATORBUG-905)")
            << QString::fromLatin1("ld: warning: Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                         "Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o"))
            << QString();

    QTest::newRow("ld fatal")
            << QString::fromLatin1("ld: fatal: Symbol referencing errors. No output written to testproject")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Error,
                         "Symbol referencing errors. No output written to testproject"))
            << QString();

    QTest::newRow("Teambuilder issues")
            << QString::fromLatin1("TeamBuilder Client:: error: could not find Scheduler, running Job locally...")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("TeamBuilder Client:: error: could not find Scheduler, running Job locally...\n")
            << Tasks()
            << QString();

    QTest::newRow("note")
            << QString::fromLatin1("/home/dev/creator/share/qtcreator/debugger/dumper.cpp:1079: note: initialized from here")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "initialized from here",
                                FilePath::fromUserInput("/home/dev/creator/share/qtcreator/debugger/dumper.cpp"),
                                1079))
            << QString();

    QTest::newRow("static member function")
            << QString::fromLatin1("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c: In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':\n"
                                   "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c:194: warning: suggest explicit braces to avoid ambiguous 'else'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                                "suggest explicit braces to avoid ambiguous 'else'\n"
                                "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c: In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':\n"
                                "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c:194: warning: suggest explicit braces to avoid ambiguous 'else'",
                                FilePath::fromUserInput("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c"),
                                194))
            << QString();

    QTest::newRow("rm false positive")
            << QString::fromLatin1("rm: cannot remove `release/moc_mainwindow.cpp': No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("rm: cannot remove `release/moc_mainwindow.cpp': No such file or directory\n")
            << Tasks()
            << QString();

    QTest::newRow("ld: missing library")
            << QString::fromLatin1("/usr/bin/ld: cannot find -ldoesnotexist")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "cannot find -ldoesnotexist"))
            << QString();

    QTest::newRow("In function")
            << QString::fromLatin1("../../scriptbug/main.cpp: In function void foo(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:8: warning: unused variable c")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "In function void foo(i) [with i = double]:\n"
                                "../../scriptbug/main.cpp: In function void foo(i) [with i = double]:\n"
                                "../../scriptbug/main.cpp:22: instantiated from here",
                                FilePath::fromUserInput("../../scriptbug/main.cpp"))
                 << CompileTask(Task::Warning,
                                "unused variable c",
                                FilePath::fromUserInput("../../scriptbug/main.cpp"),
                                8))
            << QString();

    QTest::newRow("instanciated from here")
            << QString::fromLatin1("main.cpp:10: instantiated from here  ")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "instantiated from here",
                                FilePath::fromUserInput("main.cpp"),
                                10))
            << QString();

    QTest::newRow("In constructor")
            << QString::fromLatin1("/dev/creator/src/plugins/find/basetextfind.h: In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':",
                                FilePath::fromUserInput("/dev/creator/src/plugins/find/basetextfind.h")))
            << QString();

    QTest::newRow("At global scope")
            << QString::fromLatin1("../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:5: warning: unused parameter v")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "At global scope:\n"
                                "../../scriptbug/main.cpp: At global scope:\n"
                                "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]",
                                FilePath::fromUserInput("../../scriptbug/main.cpp"))
                << CompileTask(Task::Unknown,
                               "instantiated from here",
                               FilePath::fromUserInput("../../scriptbug/main.cpp"),
                               22)
                << CompileTask(Task::Warning,
                               "unused parameter v",
                               FilePath::fromUserInput("../../scriptbug/main.cpp"),
                               5))
            << QString();

    QTest::newRow("gcc 4.5 fatal error")
            << QString::fromLatin1("/home/code/test.cpp:54:38: fatal error: test.moc: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Error,
                                "test.moc: No such file or directory",
                                FilePath::fromUserInput("/home/code/test.cpp"),
                                54))
            << QString();

    QTest::newRow("QTCREATORBUG-597")
            << QString::fromLatin1("debug/qplotaxis.o: In function `QPlotAxis':\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `vtable for QPlotAxis'\n"
                               "debug/qplotaxis.o: In function `QPlotAxis':\n"
                               "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'",
                               FilePath::fromUserInput("M:\\Development\\x64\\QtPlot/qplotaxis.cpp"),
                               26)
                << CompileTask(Task::Error,
                               "undefined reference to `vtable for QPlotAxis'",
                               FilePath::fromUserInput("M:\\Development\\x64\\QtPlot/qplotaxis.cpp"),
                               26)
                << CompileTask(Task::Error,
                               "collect2: ld returned 1 exit status"))
            << QString();

    QTest::newRow("instantiated from here should not be an error")
            << QString::fromLatin1("../stl/main.cpp: In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                                   "../stl/main.cpp:38:   instantiated from here\n"
                                   "../stl/main.cpp:31: warning: returning reference to temporary\n"
                                   "../stl/main.cpp: At global scope:\n"
                                   "../stl/main.cpp:31: warning: unused parameter index")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Unknown,
                               "In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                               "../stl/main.cpp: In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                               "../stl/main.cpp:38:   instantiated from here",
                               FilePath::fromUserInput("../stl/main.cpp"), -1)
                << CompileTask(Task::Warning,
                               "returning reference to temporary",
                               FilePath::fromUserInput("../stl/main.cpp"), 31)
                << CompileTask(Task::Warning,
                               "unused parameter index\n"
                               "../stl/main.cpp: At global scope:\n"
                               "../stl/main.cpp:31: warning: unused parameter index",
                               FilePath::fromUserInput("../stl/main.cpp"), 31))
            << QString();

    QTest::newRow("GCCE from lines")
            << QString::fromLatin1("In file included from C:/Symbian_SDK/epoc32/include/e32cmn.h:6792,\n"
                                   "                 from C:/Symbian_SDK/epoc32/include/e32std.h:25,\n"
                                   "C:/Symbian_SDK/epoc32/include/e32cmn.inl: In member function 'SSecureId::operator const TSecureId&() const':\n"
                                   "C:/Symbian_SDK/epoc32/include/e32cmn.inl:7094: warning: returning reference to temporary")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(Task::Warning,
                                 "returning reference to temporary\n"
                                 "In file included from C:/Symbian_SDK/epoc32/include/e32cmn.h:6792,\n"
                                 "                 from C:/Symbian_SDK/epoc32/include/e32std.h:25,\n"
                                 "C:/Symbian_SDK/epoc32/include/e32cmn.inl: In member function 'SSecureId::operator const TSecureId&() const':\n"
                                 "C:/Symbian_SDK/epoc32/include/e32cmn.inl:7094: warning: returning reference to temporary",
                                 FilePath::fromUserInput("C:/Symbian_SDK/epoc32/include/e32cmn.inl"),
                                 7094)}
            << QString();

    QTest::newRow("QTCREATORBUG-2206")
            << QString::fromLatin1("../../../src/XmlUg/targetdelete.c: At top level:")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "At top level:",
                                FilePath::fromUserInput("../../../src/XmlUg/targetdelete.c")))
            << QString();

    QTest::newRow("GCCE 4: commandline, includes")
            << QString::fromLatin1("In file included from /Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h:15,\n"
                                   "                 from <command line>:26:\n"
                                   "/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh:1134:26: warning: no newline at end of file")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(
                      Task::Warning,
                      "no newline at end of file\n"
                      "In file included from /Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h:15,\n"
                      "                 from <command line>:26:\n"
                      "/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh:1134:26: warning: no newline at end of file",
                      FilePath::fromUserInput("/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh"),
                      1134)}
            << QString();

    QTest::newRow("Linker fail (release build)")
            << QString::fromLatin1("release/main.o:main.cpp:(.text+0x42): undefined reference to `MainWindow::doSomething()'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `MainWindow::doSomething()'",
                               FilePath::fromUserInput("main.cpp")))
            << QString();

    QTest::newRow("enumeration warning")
            << QString::fromLatin1("../../../src/shared/proparser/profileevaluator.cpp: In member function 'ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(const ProString&, const ProStringList&)':\n"
                                   "../../../src/shared/proparser/profileevaluator.cpp:2817:9: warning: case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                                "case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'\n"
                                "../../../src/shared/proparser/profileevaluator.cpp: In member function 'ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(const ProString&, const ProStringList&)':\n"
                                "../../../src/shared/proparser/profileevaluator.cpp:2817:9: warning: case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'",
                                FilePath::fromUserInput("../../../src/shared/proparser/profileevaluator.cpp"),
                                2817))
            << QString();

    QTest::newRow("include with line:column info")
            << QString::fromLatin1("In file included from <command-line>:0:0:\n"
                                   "./mw.h:4:0: warning: \"STUPID_DEFINE\" redefined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(
                   Task::Warning,
                   "\"STUPID_DEFINE\" redefined\n"
                   "In file included from <command-line>:0:0:\n"
                   "./mw.h:4:0: warning: \"STUPID_DEFINE\" redefined",
                   FilePath::fromUserInput("./mw.h"), 4)}
            << QString();

    QTest::newRow("instantiation with line:column info")
            << QString::fromLatin1("file.h: In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                   "file.cpp:87:10: instantiated from here\n"
                                   "file.h:21:5: warning: comparison between signed and unsigned integer expressions [-Wsign-compare]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Unknown,
                                "In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                "file.h: In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                "file.cpp:87:10: instantiated from here",
                                FilePath::fromUserInput("file.h"))
                 << CompileTask(Task::Warning,
                                "comparison between signed and unsigned integer expressions [-Wsign-compare]",
                                FilePath::fromUserInput("file.h"),
                                21))
            << QString();

    QTest::newRow("linker error") // QTCREATORBUG-3107
            << QString::fromLatin1("cns5k_ins_parser_tests.cpp:(.text._ZN20CNS5kINSParserEngine21DropBytesUntilStartedEP14CircularBufferIhE[CNS5kINSParserEngine::DropBytesUntilStarted(CircularBuffer<unsigned char>*)]+0x6d): undefined reference to `CNS5kINSPacket::SOH_BYTE'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "undefined reference to `CNS5kINSPacket::SOH_BYTE'",
                               FilePath::fromUserInput("cns5k_ins_parser_tests.cpp")))
            << QString();

    QTest::newRow("uic warning")
            << QString::fromLatin1("mainwindow.ui: Warning: The name 'pushButton' (QPushButton) is already in use, defaulting to 'pushButton1'.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                                "The name 'pushButton' (QPushButton) is already in use, defaulting to 'pushButton1'.",
                                FilePath::fromUserInput("mainwindow.ui")))
            << QString();

    QTest::newRow("libimf warning")
            << QString::fromLatin1("libimf.so: warning: warning: feupdateenv is not implemented and will always fail")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                 << CompileTask(Task::Warning,
                                "warning: feupdateenv is not implemented and will always fail",
                                FilePath::fromUserInput("libimf.so")))
            << QString();

    QTest::newRow("gcc 4.8")
            << QString::fromLatin1("In file included from /home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp:31:0:\n"
                                   ".uic/ui_pluginerrorview.h:14:25: fatal error: QtGui/QAction: No such file or directory\n"
                                   " #include <QtGui/QAction>\n"
                                   "                         ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(
                   Task::Error,
                   "QtGui/QAction: No such file or directory\n"
                   "In file included from /home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp:31:0:\n"
                   ".uic/ui_pluginerrorview.h:14:25: fatal error: QtGui/QAction: No such file or directory\n"
                   " #include <QtGui/QAction>\n"
                   "                         ^",
                   FilePath::fromUserInput(".uic/ui_pluginerrorview.h"), 14)}
            << QString();

    QTest::newRow("qtcreatorbug-9195")
            << QString::fromLatin1("In file included from /usr/include/qt4/QtCore/QString:1:0,\n"
                                   "                 from main.cpp:3:\n"
                                   "/usr/include/qt4/QtCore/qstring.h: In function 'void foo()':\n"
                                   "/usr/include/qt4/QtCore/qstring.h:597:5: error: 'QString::QString(const char*)' is private\n"
                                   "main.cpp:7:22: error: within this context")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(
                   Task::Error,
                   "'QString::QString(const char*)' is private\n"
                   "In file included from /usr/include/qt4/QtCore/QString:1:0,\n"
                   "                 from main.cpp:3:\n"
                   "/usr/include/qt4/QtCore/qstring.h: In function 'void foo()':\n"
                   "/usr/include/qt4/QtCore/qstring.h:597:5: error: 'QString::QString(const char*)' is private\n"
                   "main.cpp:7:22: error: within this context",
                   FilePath::fromUserInput("/usr/include/qt4/QtCore/qstring.h"), 597)}
            << QString();

    QTest::newRow("ld: Multiple definition error")
            << QString::fromLatin1("foo.o: In function `foo()':\n"
                                   "/home/user/test/foo.cpp:2: multiple definition of `foo()'\n"
                                   "bar.o:/home/user/test/bar.cpp:4: first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "multiple definition of `foo()'\n"
                               "foo.o: In function `foo()':\n"
                               "/home/user/test/foo.cpp:2: multiple definition of `foo()'",
                               FilePath::fromUserInput("/home/user/test/foo.cpp"),
                               2)
                << CompileTask(Task::Unknown,
                               "first defined here",
                               FilePath::fromUserInput("/home/user/test/bar.cpp"),
                               4)
                << CompileTask(Task::Error,
                               "collect2: error: ld returned 1 exit status"))
            << QString();

    QTest::newRow("ld: .data section")
            << QString::fromLatin1("foo.o:(.data+0x0): multiple definition of `foo'\n"
                                   "bar.o:(.data+0x0): first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "multiple definition of `foo'",
                               FilePath::fromUserInput("foo.o"), -1)
                << CompileTask(Task::Unknown,
                               "first defined here",
                               FilePath::fromUserInput("bar.o"), -1)
                << CompileTask(Task::Error,
                               "collect2: error: ld returned 1 exit status"))
            << QString();


    QTest::newRow("Undefined symbol (Apple ld)")
            << "Undefined symbols for architecture x86_64:\n"
               "  \"SvgLayoutTest()\", referenced from:\n"
               "      _main in main.cpp.o"
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks({CompileTask(Task::Error,
                                     "Undefined symbols for architecture x86_64:\n"
                                  "  \"SvgLayoutTest()\", referenced from:\n"
                                  "      _main in main.cpp.o",
                                  FilePath::fromString("main.cpp.o"))})
            << QString();

    QTest::newRow("ld: undefined member function reference")
            << "obj/gtest-clang-printing.o:gtest-clang-printing.cpp:llvm::VerifyDisableABIBreakingChecks: error: undefined reference to 'llvm::DisableABIBreakingChecks'"
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Error,
                               "error: undefined reference to 'llvm::DisableABIBreakingChecks'",
                               FilePath::fromString("gtest-clang-printing.cpp")))
            << QString();

    const auto task = [](Task::TaskType type, const QString &msg,
                                        const QString &file = {}, int line = -1) {
        return CompileTask(type, msg, FilePath::fromString(file), line);
    };
    const auto errorTask = [&task](const QString &msg, const QString &file = {}, int line = -1) {
        return task(Task::Error, msg, file, line);
    };
    const auto unknownTask = [&task](const QString &msg, const QString &file = {}, int line = -1) {
        return task(Task::Unknown, msg, file, line);
    };
    QTest::newRow("lld: undefined reference with debug info")
            << "ld.lld: error: undefined symbol: func()\n"
               ">>> referenced by test.cpp:5\n"
               ">>>               /tmp/ccg8pzRr.o:(main)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks({
                   errorTask("ld.lld: error: undefined symbol: func()"),
                   unknownTask("referenced by test.cpp:5", "test.cpp", 5),
                   unknownTask("/tmp/ccg8pzRr.o:(main)",  "/tmp/ccg8pzRr.o"),
                   errorTask("collect2: error: ld returned 1 exit status")})
            << QString();

    QTest::newRow("lld: undefined reference with debug info (more verbose format)")
            << "ld.lld: error: undefined symbol: someFunc()\n"
               ">>> referenced by main.cpp:10 (/tmp/untitled4/main.cpp:10)\n"
               ">>>               /tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o:(main)\n"
               "clang-8: error: linker command failed with exit code 1 (use -v to see invocation)"
            << OutputParserTester::STDERR << QString()
            << QString("clang-8: error: linker command failed with exit code 1 (use -v to see invocation)\n")
            << Tasks{
                   errorTask("ld.lld: error: undefined symbol: someFunc()"),
                   unknownTask("referenced by main.cpp:10 (/tmp/untitled4/main.cpp:10)",
                               "/tmp/untitled4/main.cpp", 10),
                   unknownTask("/tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o:(main)",
                               "/tmp/Debug4/untitled4.5abe06ac/3a52ce780950d4d9/main.cpp.o")}
            << QString();

    QTest::newRow("lld: undefined reference without debug info")
            << "ld.lld: error: undefined symbol: func()\n"
               ">>> referenced by test.cpp\n"
               ">>>               /tmp/ccvjyJph.o:(main)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: undefined symbol: func()"),
                   unknownTask("referenced by test.cpp", "test.cpp"),
                   unknownTask("/tmp/ccvjyJph.o:(main)",  "/tmp/ccvjyJph.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    if (HostOsInfo::isWindowsHost()) {
        QTest::newRow("lld: undefined reference with mingw")
                << "lld-link: error: undefined symbol: __Z4funcv\n"
                   ">>> referenced by C:\\Users\\orgads\\AppData\\Local\\Temp\\cccApKoz.o:(.text)\n"
                   "collect2.exe: error: ld returned 1 exit status"
                << OutputParserTester::STDERR << QString() << QString()
                << Tasks{
                       errorTask("lld-link: error: undefined symbol: __Z4funcv"),
                       unknownTask("referenced by C:\\Users\\orgads\\AppData\\Local\\Temp\\cccApKoz.o:(.text)",
                                   "C:/Users/orgads/AppData/Local/Temp/cccApKoz.o"),
                       errorTask("collect2.exe: error: ld returned 1 exit status")}
                << QString();
    }

    QTest::newRow("lld: multiple definitions with debug info")
            << "ld.lld: error: duplicate symbol: func()\n"
               ">>> defined at test1.cpp:1\n"
               ">>>            test1.o:(func())\n"
               ">>> defined at test1.cpp:1\n"
               ">>>            test1.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: duplicate symbol: func()"),
                   unknownTask("defined at test1.cpp:1", "test1.cpp", 1),
                   unknownTask("test1.o:(func())",  "test1.o"),
                   unknownTask("defined at test1.cpp:1", "test1.cpp", 1),
                   unknownTask("test1.o:(.text+0x0)",  "test1.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    QTest::newRow("lld: multiple definitions with debug info (more verbose format)")
            << "ld.lld: error: duplicate symbol: theFunc()\n"
               ">>> defined at file.cpp:1 (/tmp/untitled3/file.cpp:1)\n"
               ">>>            /tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o:(theFunc())\n"
               ">>> defined at main.cpp:5 (/tmp/untitled3/main.cpp:5)\n"
               ">>>            /tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: duplicate symbol: theFunc()"),
                   unknownTask("defined at file.cpp:1 (/tmp/untitled3/file.cpp:1)",
                               "/tmp/untitled3/file.cpp", 1),
                   unknownTask("/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o:(theFunc())",
                               "/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/file.cpp.o"),
                   unknownTask("defined at main.cpp:5 (/tmp/untitled3/main.cpp:5)",
                               "/tmp/untitled3/main.cpp", 5),
                   unknownTask("/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o:(.text+0x0)",
                               "/tmp/Debug/untitled3.dade828b/3a52ce780950d4d9/main.cpp.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    QTest::newRow("lld: multiple definitions without debug info")
            << "ld.lld: error: duplicate symbol: func()\n"
               ">>> defined at test1.cpp\n"
               ">>>            test1.o:(func())\n"
               ">>> defined at test1.cpp\n"
               ">>>            test1.o:(.text+0x0)\n"
               "collect2: error: ld returned 1 exit status"
            << OutputParserTester::STDERR << QString() << QString()
            << Tasks{
                   errorTask("ld.lld: error: duplicate symbol: func()"),
                   unknownTask("defined at test1.cpp", "test1.cpp"),
                   unknownTask("test1.o:(func())",  "test1.o"),
                   unknownTask("defined at test1.cpp", "test1.cpp"),
                   unknownTask("test1.o:(.text+0x0)",  "test1.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
            << QString();

    if (HostOsInfo::isWindowsHost()) {
        QTest::newRow("lld: multiple definitions with mingw")
                << "lld-link: error: duplicate symbol: __Z4funcv in test1.o and in test2.o\n"
                   "collect2.exe: error: ld returned 1 exit status"
                << OutputParserTester::STDERR << QString() << QString()
                << Tasks{
                       errorTask("lld-link: error: duplicate symbol: __Z4funcv in test1.o and in test2.o"),
                       errorTask("collect2.exe: error: ld returned 1 exit status", {})}
                << QString();
    }

    QTest::newRow("Mac: ranlib warning")
            << QString::fromLatin1("ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "file: lib/libtest.a(Test0.cpp.o) has no symbols"))
            << QString();

    QTest::newRow("Mac: ranlib warning2")
            << QString::fromLatin1("/path/to/XCode/and/ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << CompileTask(Task::Warning,
                               "file: lib/libtest.a(Test0.cpp.o) has no symbols"))
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

    QTest::newRow("GCC 9 output")
            << QString("In file included from /usr/include/qt/QtCore/qlocale.h:43,\n"
                       "                 from /usr/include/qt/QtCore/qtextstream.h:46,\n"
                       "                 from /qtc/src/shared/proparser/proitems.cpp:31:\n"
                       "/usr/include/qt/QtCore/qvariant.h: In constructor ‘QVariant::QVariant(QVariant&&)’:\n"
                       "/usr/include/qt/QtCore/qvariant.h:273:25: warning: implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                       "  273 |     { other.d = Private(); }\n"
                       "      |                         ^\n"
                       "/usr/include/qt/QtCore/qvariant.h:399:16: note: because ‘QVariant::Private’ has user-provided ‘QVariant::Private::Private(const QVariant::Private&)’\n"
                       "  399 |         inline Private(const Private &other) Q_DECL_NOTHROW\n"
                       "      |                      ^~~~~~~)\n"
                       "t.cc: In function ‘int test(const shape&, const shape&)’:\n"
                       "t.cc:15:4: error: no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                       "  14 |   return (width(s1) * height(s1)\n"
                       "     |           ~~~~~~~~~~~~~~~~~~~~~~\n"
                       "     |                     |\n"
                       "     |                     boxed_value<[...]>\n"
                       "  15 |    + width(s2) * height(s2));\n"
                       "     |    ^ ~~~~~~~~~~~~~~~~~~~~~~\n"
                       "     |                |\n"
                       "     |                boxed_value<[...]>\n"
                       "incomplete.c:1:6: error: ‘string’ in namespace ‘std’ does not name a type\n"
                       "  1 | std::string test(void)\n"
                       "    |      ^~~~~~\n"
                       "incomplete.c:1:1: note: ‘std::string’ is defined in header ‘<string>’; did you forget to ‘#include <string>’?\n"
                       " +++ |+#include <string>\n"
                       "  1 | std::string test(void)\n"
                       "param-type-mismatch.c: In function ‘caller’:\n"
                       "param-type-mismatch.c:5:24: warning: passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                       "  5 |   return callee(first, second, third);\n"
                       "    |                        ^~~~~~\n"
                       "    |                        |\n"
                       "    |                        int\n"
                       "param-type-mismatch.c:1:40: note: expected ‘const char *’ but argument is of type ‘int’\n"
                       "  1 | extern int callee(int one, const char *two, float three);\n"
                       "    |                            ~~~~~~~~~~~~^~~"
               )
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(Task::Warning,
                                 "implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                                 "In file included from /usr/include/qt/QtCore/qlocale.h:43,\n"
                                 "                 from /usr/include/qt/QtCore/qtextstream.h:46,\n"
                                 "                 from /qtc/src/shared/proparser/proitems.cpp:31:\n"
                                 "/usr/include/qt/QtCore/qvariant.h: In constructor ‘QVariant::QVariant(QVariant&&)’:\n"
                                 "/usr/include/qt/QtCore/qvariant.h:273:25: warning: implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                                 "  273 |     { other.d = Private(); }\n"
                                 "      |                         ^\n"
                                 "/usr/include/qt/QtCore/qvariant.h:399:16: note: because ‘QVariant::Private’ has user-provided ‘QVariant::Private::Private(const QVariant::Private&)’\n"
                                 "  399 |         inline Private(const Private &other) Q_DECL_NOTHROW\n"
                                 "      |                      ^~~~~~~)",
                                 FilePath::fromUserInput("/usr/include/qt/QtCore/qvariant.h"), 273),
                     CompileTask(Task::Error,
                                 "no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                                 "t.cc: In function ‘int test(const shape&, const shape&)’:\n"
                                 "t.cc:15:4: error: no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                                 "  14 |   return (width(s1) * height(s1)\n"
                                 "     |           ~~~~~~~~~~~~~~~~~~~~~~\n"
                                 "     |                     |\n"
                                 "     |                     boxed_value<[...]>\n"
                                 "  15 |    + width(s2) * height(s2));\n"
                                 "     |    ^ ~~~~~~~~~~~~~~~~~~~~~~\n"
                                 "     |                |\n"
                                 "     |                boxed_value<[...]>",
                                 FilePath::fromUserInput("t.cc"),
                                 15),
                      CompileTask(Task::Error,
                                  "‘string’ in namespace ‘std’ does not name a type\n"
                                  "incomplete.c:1:6: error: ‘string’ in namespace ‘std’ does not name a type\n"
                                  "  1 | std::string test(void)\n"
                                  "    |      ^~~~~~\n"
                                  "incomplete.c:1:1: note: ‘std::string’ is defined in header ‘<string>’; did you forget to ‘#include <string>’?\n"
                                  " +++ |+#include <string>\n"
                                  "  1 | std::string test(void)",
                                  FilePath::fromUserInput("incomplete.c"),
                                  1),
                      CompileTask(Task::Warning,
                                  "passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                                  "param-type-mismatch.c: In function ‘caller’:\n"
                                  "param-type-mismatch.c:5:24: warning: passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                                  "  5 |   return callee(first, second, third);\n"
                                  "    |                        ^~~~~~\n"
                                  "    |                        |\n"
                                  "    |                        int\n"
                                  "param-type-mismatch.c:1:40: note: expected ‘const char *’ but argument is of type ‘int’\n"
                                  "  1 | extern int callee(int one, const char *two, float three);\n"
                                  "    |                            ~~~~~~~~~~~~^~~",
                                  FilePath::fromUserInput("param-type-mismatch.c"), 5)}
            << QString();

    QTest::newRow(R"("inlined from")")
            << QString("In file included from smallstringvector.h:30,\n"
                       "                 from smallstringio.h:28,\n"
                       "                 from gtest-creator-printing.h:29,\n"
                       "                 from googletest.h:41,\n"
                       "                 from smallstring-test.cpp:26:\n"
                       "In member function ‘void Utils::BasicSmallString<Size>::append(Utils::SmallStringView) [with unsigned int Size = 31]’,\n"
                       "    inlined from ‘Utils::BasicSmallString<Size>& Utils::BasicSmallString<Size>::operator+=(Utils::SmallStringView) [with unsigned int Size = 31]’ at smallstring.h:471:15,\n"
                       "    inlined from ‘virtual void SmallString_AppendLongSmallStringToShortSmallString_Test::TestBody()’ at smallstring-test.cpp:850:63:\n"
                       "smallstring.h:465:21: warning: writing 1 byte into a region of size 0 [-Wstringop-overflow=]\n"
                       "  465 |         at(newSize) = 0;\n"
                       "      |         ~~~~~~~~~~~~^~~")
            << OutputParserTester::STDERR
            << QString() << QString()
            << Tasks{CompileTask(Task::Warning,
                                 "writing 1 byte into a region of size 0 [-Wstringop-overflow=]\n"
                                 "In file included from smallstringvector.h:30,\n"
                                                        "                 from smallstringio.h:28,\n"
                                                        "                 from gtest-creator-printing.h:29,\n"
                                                        "                 from googletest.h:41,\n"
                                                        "                 from smallstring-test.cpp:26:\n"
                                                        "In member function ‘void Utils::BasicSmallString<Size>::append(Utils::SmallStringView) [with unsigned int Size = 31]’,\n"
                                                        "    inlined from ‘Utils::BasicSmallString<Size>& Utils::BasicSmallString<Size>::operator+=(Utils::SmallStringView) [with unsigned int Size = 31]’ at smallstring.h:471:15,\n"
                                                        "    inlined from ‘virtual void SmallString_AppendLongSmallStringToShortSmallString_Test::TestBody()’ at smallstring-test.cpp:850:63:\n"
                                                        "smallstring.h:465:21: warning: writing 1 byte into a region of size 0 [-Wstringop-overflow=]\n"
                                                        "  465 |         at(newSize) = 0;\n"
                                                        "      |         ~~~~~~~~~~~~^~~",
                                 FilePath::fromUserInput("smallstring.h"), 465)}
            << QString();
}

void ProjectExplorerPlugin::testGccOutputParsers()
{
    OutputParserTester testbench;
    testbench.setLineParsers(GccParser::gccParserSuite());
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
