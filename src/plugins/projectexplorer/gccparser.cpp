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

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

// opt. drive letter + filename: (2 brackets)
static const char FILE_PATTERN[] = "(<command[ -]line>|([A-Za-z]:)?[^:]+):";
static const char COMMAND_PATTERN[] = "^(.*?[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(gcc|g\\+\\+)(-[0-9\\.]+)?(\\.exe)?: ";

GccParser::GccParser()
{
    setObjectName(QLatin1String("GCCParser"));
    m_regExp.setPattern(QLatin1Char('^') + QLatin1String(FILE_PATTERN)
                        + QLatin1String("(\\d+):(\\d+:)?\\s+((fatal |#)?(warning|error|note):?\\s)?([^\\s].+)$"));
    QTC_CHECK(m_regExp.isValid());

    m_regExpIncluded.setPattern(QString::fromLatin1("\\bfrom\\s") + QLatin1String(FILE_PATTERN)
                                + QLatin1String("(\\d+)(:\\d+)?[,:]?$"));
    QTC_CHECK(m_regExpIncluded.isValid());

    // optional path with trailing slash
    // optional arm-linux-none-thingy
    // name of executable
    // optional trailing version number
    // optional .exe postfix
    m_regExpGccNames.setPattern(QLatin1String(COMMAND_PATTERN));
    QTC_CHECK(m_regExpGccNames.isValid());

    appendOutputParser(new Internal::LldParser);
    appendOutputParser(new LdParser);
}

void GccParser::stdError(const QString &line)
{
    QString lne = rightTrimmed(line);

    // Blacklist some lines to not handle them:
    if (lne.startsWith(QLatin1String("TeamBuilder ")) ||
        lne.startsWith(QLatin1String("distcc["))) {
        IOutputParser::stdError(line);
        return;
    }

    // Handle misc issues:
    if (lne.startsWith(QLatin1String("ERROR:")) ||
        lne == QLatin1String("* cpp failed")) {
        newTask(Task(Task::Error,
                     lne /* description */,
                     Utils::FilePath() /* filename */,
                     -1 /* linenumber */,
                     Constants::TASK_CATEGORY_COMPILE));
        return;
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
        Task task(type, description, Utils::FilePath(), /* filename */
                  -1, /* line */ Constants::TASK_CATEGORY_COMPILE);
        newTask(task);
        return;
    }

    match = m_regExp.match(lne);
    if (match.hasMatch()) {
        Utils::FilePath filename = Utils::FilePath::fromUserInput(match.captured(1));
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

        Task task(type, description, filename, lineno, Constants::TASK_CATEGORY_COMPILE);
        newTask(task);
        return;
    }

    match = m_regExpIncluded.match(lne);
    if (match.hasMatch()) {
        newTask(Task(Task::Unknown,
                     lne.trimmed() /* description */,
                     Utils::FilePath::fromUserInput(match.captured(1)) /* filename */,
                     match.captured(3).toInt() /* linenumber */,
                     Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (lne.startsWith(QLatin1Char(' '))) {
        amendDescription(lne, true);
        return;
    }

    doFlush();
    IOutputParser::stdError(line);
}

void GccParser::stdOutput(const QString &line)
{
    doFlush();
    IOutputParser::stdOutput(line);
}

Core::Id GccParser::id()
{
    return Core::Id("ProjectExplorer.OutputParser.Gcc");
}

void GccParser::newTask(const Task &task)
{
    doFlush();
    m_currentTask = task;
    m_lines = 1;
}

void GccParser::doFlush()
{
    if (m_currentTask.isNull())
        return;
    Task t = m_currentTask;
    m_currentTask.clear();
    emit addTask(t, m_lines, 1);
    m_lines = 0;
}

void GccParser::amendDescription(const QString &desc, bool monospaced)
{
    if (m_currentTask.isNull())
        return;
    int start = m_currentTask.description.count() + 1;
    m_currentTask.description.append(QLatin1Char('\n'));
    m_currentTask.description.append(desc);
    if (monospaced) {
        QTextLayout::FormatRange fr;
        fr.start = start;
        fr.length = desc.count() + 1;
        fr.format.setFont(TextEditor::TextEditorSettings::fontSettings().font());
        fr.format.setFontStyleHint(QFont::Monospace);
        m_currentTask.formats.append(fr);
    }
    ++m_lines;
    return;
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

    const Core::Id categoryCompile = Constants::TASK_CATEGORY_COMPILE;

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
                << Task(Task::Unknown,
                        QLatin1String("In function `int main(int, char**)':"),
                        Utils::FilePath::fromUserInput(QLatin1String("/temp/test/untitled8/main.cpp")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("`sfasdf' undeclared (first use this function)"),
                        Utils::FilePath::fromUserInput(QLatin1String("/temp/test/untitled8/main.cpp")), 9,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("(Each undeclared identifier is reported only once for each function it appears in.)"),
                        Utils::FilePath::fromUserInput(QLatin1String("/temp/test/untitled8/main.cpp")), 9,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("GCCE warning")
            << QString::fromLatin1("/src/corelib/global/qglobal.h:1635: warning: inline function `QDebug qDebug()' used but never defined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Warning,
                        QLatin1String("inline function `QDebug qDebug()' used but never defined"),
                        Utils::FilePath::fromUserInput(QLatin1String("/src/corelib/global/qglobal.h")), 1635,
                        categoryCompile))
            << QString();
    QTest::newRow("warning")
            << QString::fromLatin1("main.cpp:7:2: warning: Some warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks() << Task(Task::Warning,
                                                       QLatin1String("Some warning"),
                                                       Utils::FilePath::fromUserInput(QLatin1String("main.cpp")), 7,
                                                       categoryCompile))
            << QString();
    QTest::newRow("GCCE #error")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:7: #error Symbian error")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Error,
                        QLatin1String("#error Symbian error"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:\\temp\\test\\untitled8\\main.cpp")), 7,
                        categoryCompile))
            << QString();
    // Symbian reports #warning(s) twice (using different syntax).
    QTest::newRow("GCCE #warning1")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:8: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Warning,
                        QLatin1String("#warning Symbian warning"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:\\temp\\test\\untitled8\\main.cpp")), 8,
                        categoryCompile))
            << QString();
    QTest::newRow("GCCE #warning2")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp:8:2: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Warning,
                        QLatin1String("#warning Symbian warning"),
                        Utils::FilePath::fromUserInput(QLatin1String("/temp/test/untitled8/main.cpp")), 8,
                        categoryCompile))
            << QString();
    QTest::newRow("Undefined reference (debug)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In function `main':"),
                        Utils::FilePath::fromUserInput(QLatin1String("main.o")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:\\temp\\test\\untitled8/main.cpp")), 8,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("collect2: ld returned 1 exit status"),
                        Utils::FilePath(), -1,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("Undefined reference (release)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In function `main':"),
                        Utils::FilePath::fromUserInput(QLatin1String("main.o")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:\\temp\\test\\untitled8/main.cpp")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("collect2: ld returned 1 exit status"),
                        Utils::FilePath(), -1,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("linker: dll format not recognized")
            << QString::fromLatin1("c:\\Qt\\4.6\\lib/QtGuid4.dll: file not recognized: File format not recognized")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Error,
                        QLatin1String("file not recognized: File format not recognized"),
                        Utils::FilePath::fromUserInput(QLatin1String("c:\\Qt\\4.6\\lib/QtGuid4.dll")), -1,
                        categoryCompile))
            << QString();
    QTest::newRow("Invalid rpath")
            << QString::fromLatin1("g++: /usr/local/lib: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Error,
                        QLatin1String("/usr/local/lib: No such file or directory"),
                        Utils::FilePath(), -1,
                        categoryCompile))
            << QString();

    QTest::newRow("Invalid rpath")
            << QString::fromLatin1("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp: In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2114: warning: unused variable 'index'\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2115: warning: unused variable 'handler'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':"),
                        Utils::FilePath::fromUserInput(QLatin1String("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp")), -1,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("unused variable 'index'"),
                        Utils::FilePath::fromUserInput(QLatin1String("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp")), 2114,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("unused variable 'handler'"),
                        Utils::FilePath::fromUserInput(QLatin1String("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp")), 2115,
                        categoryCompile))
            << QString();
    QTest::newRow("gnumakeparser.cpp errors")
            << QString::fromLatin1("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp: In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected primary-expression before ':' token\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected ';' before ':' token")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':"),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("expected primary-expression before ':' token"),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp")), 264,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("expected ';' before ':' token"),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp")), 264,
                        categoryCompile))
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
            << ( Tasks()
                 << Task(Task::Warning,
                         QLatin1String("Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o"),
                         Utils::FilePath(), -1,
                         categoryCompile))
            << QString();
    QTest::newRow("ld fatal")
            << QString::fromLatin1("ld: fatal: Symbol referencing errors. No output written to testproject")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Error,
                         QLatin1String("Symbol referencing errors. No output written to testproject"),
                         Utils::FilePath(), -1,
                         categoryCompile))
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
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("initialized from here"),
                         Utils::FilePath::fromUserInput(QLatin1String("/home/dev/creator/share/qtcreator/debugger/dumper.cpp")), 1079,
                         categoryCompile))
            << QString();
    QTest::newRow("static member function")
            << QString::fromLatin1("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c: In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':\n"
                                   "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c:194: warning: suggest explicit braces to avoid ambiguous 'else'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':"),
                         Utils::FilePath::fromUserInput(QLatin1String("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c")), -1,
                         categoryCompile)
                 << Task(Task::Warning,
                         QLatin1String("suggest explicit braces to avoid ambiguous 'else'"),
                         Utils::FilePath::fromUserInput(QLatin1String("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c")), 194,
                         categoryCompile))
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
            << ( Tasks()
                 << Task(Task::Error,
                         QLatin1String("cannot find -ldoesnotexist"),
                         Utils::FilePath(), -1,
                         categoryCompile))
            << QString();
    QTest::newRow("In function")
            << QString::fromLatin1("../../scriptbug/main.cpp: In function void foo(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:8: warning: unused variable c")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("In function void foo(i) [with i = double]:"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), -1,
                         categoryCompile)
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from here"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), 22,
                         categoryCompile)
                 << Task(Task::Warning,
                         QLatin1String("unused variable c"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), 8,
                         categoryCompile))
            << QString();
    QTest::newRow("instanciated from here")
            << QString::fromLatin1("main.cpp:10: instantiated from here  ")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from here"),
                         Utils::FilePath::fromUserInput(QLatin1String("main.cpp")), 10,
                         categoryCompile))
            << QString();
    QTest::newRow("In constructor")
            << QString::fromLatin1("/dev/creator/src/plugins/find/basetextfind.h: In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':"),
                         Utils::FilePath::fromUserInput(QLatin1String("/dev/creator/src/plugins/find/basetextfind.h")), -1,
                         categoryCompile))
            << QString();

    QTest::newRow("At global scope")
            << QString::fromLatin1("../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:5: warning: unused parameter v")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("At global scope:"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), -1,
                         categoryCompile)
                 << Task(Task::Unknown,
                         QLatin1String("In instantiation of void bar(i) [with i = double]:"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), -1,
                         categoryCompile)
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from void foo(i) [with i = double]"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), 8,
                         categoryCompile)
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from here"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), 22,
                         categoryCompile)
                 << Task(Task::Warning,
                         QLatin1String("unused parameter v"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../scriptbug/main.cpp")), 5,
                         categoryCompile))
            << QString();

    QTest::newRow("gcc 4.5 fatal error")
            << QString::fromLatin1("/home/code/test.cpp:54:38: fatal error: test.moc: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Error,
                         QLatin1String("test.moc: No such file or directory"),
                         Utils::FilePath::fromUserInput(QLatin1String("/home/code/test.cpp")), 54,
                         categoryCompile))
            << QString();

    QTest::newRow("QTCREATORBUG-597")
            << QString::fromLatin1("debug/qplotaxis.o: In function `QPlotAxis':\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In function `QPlotAxis':"),
                        Utils::FilePath::fromUserInput(QLatin1String("debug/qplotaxis.o")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `vtable for QPlotAxis'"),
                        Utils::FilePath::fromUserInput(QLatin1String("M:\\Development\\x64\\QtPlot/qplotaxis.cpp")), 26,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `vtable for QPlotAxis'"),
                        Utils::FilePath::fromUserInput(QLatin1String("M:\\Development\\x64\\QtPlot/qplotaxis.cpp")), 26,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("collect2: ld returned 1 exit status"),
                        Utils::FilePath(), -1,
                        categoryCompile))
            << QString();

    QTest::newRow("instantiated from here should not be an error")
            << QString::fromLatin1("../stl/main.cpp: In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                                   "../stl/main.cpp:38:   instantiated from here\n"
                                   "../stl/main.cpp:31: warning: returning reference to temporary\n"
                                   "../stl/main.cpp: At global scope:\n"
                                   "../stl/main.cpp:31: warning: unused parameter index")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:"),
                        Utils::FilePath::fromUserInput(QLatin1String("../stl/main.cpp")), -1,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("instantiated from here"),
                        Utils::FilePath::fromUserInput(QLatin1String("../stl/main.cpp")), 38,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("returning reference to temporary"),
                        Utils::FilePath::fromUserInput(QLatin1String("../stl/main.cpp")), 31,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("At global scope:"),
                        Utils::FilePath::fromUserInput(QLatin1String("../stl/main.cpp")), -1,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("unused parameter index"),
                        Utils::FilePath::fromUserInput(QLatin1String("../stl/main.cpp")), 31,
                        categoryCompile))
            << QString();

    QTest::newRow("GCCE from lines")
            << QString::fromLatin1("In file included from C:/Symbian_SDK/epoc32/include/e32cmn.h:6792,\n"
                                   "                 from C:/Symbian_SDK/epoc32/include/e32std.h:25,\n"
                                   "C:/Symbian_SDK/epoc32/include/e32cmn.inl: In member function 'SSecureId::operator const TSecureId&() const':\n"
                                   "C:/Symbian_SDK/epoc32/include/e32cmn.inl:7094: warning: returning reference to temporary")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In file included from C:/Symbian_SDK/epoc32/include/e32cmn.h:6792,"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:/Symbian_SDK/epoc32/include/e32cmn.h")), 6792,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("from C:/Symbian_SDK/epoc32/include/e32std.h:25,"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:/Symbian_SDK/epoc32/include/e32std.h")), 25,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("In member function 'SSecureId::operator const TSecureId&() const':"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:/Symbian_SDK/epoc32/include/e32cmn.inl")), -1,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("returning reference to temporary"),
                        Utils::FilePath::fromUserInput(QLatin1String("C:/Symbian_SDK/epoc32/include/e32cmn.inl")), 7094,
                        categoryCompile))
            << QString();

    QTest::newRow("QTCREATORBUG-2206")
            << QString::fromLatin1("../../../src/XmlUg/targetdelete.c: At top level:")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("At top level:"),
                         Utils::FilePath::fromUserInput(QLatin1String("../../../src/XmlUg/targetdelete.c")), -1,
                         categoryCompile))
            << QString();

    QTest::newRow("GCCE 4: commandline, includes")
            << QString::fromLatin1("In file included from /Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h:15,\n"
                                   "                 from <command line>:26:\n"
                                   "/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh:1134:26: warning: no newline at end of file")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Unknown,
                         QLatin1String("In file included from /Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h:15,"),
                         Utils::FilePath::fromUserInput(QLatin1String("/Symbian/SDK/EPOC32/INCLUDE/GCCE/GCCE.h")), 15,
                         categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("from <command line>:26:"),
                        Utils::FilePath::fromUserInput(QLatin1String("<command line>")), 26,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("no newline at end of file"),
                        Utils::FilePath::fromUserInput(QLatin1String("/Symbian/SDK/epoc32/include/variant/Symbian_OS.hrh")), 1134,
                        categoryCompile))
            << QString();

    QTest::newRow("Linker fail (release build)")
            << QString::fromLatin1("release/main.o:main.cpp:(.text+0x42): undefined reference to `MainWindow::doSomething()'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Error,
                        QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                        Utils::FilePath::fromUserInput(QLatin1String("main.cpp")), -1,
                        categoryCompile))
            << QString();

    QTest::newRow("enumeration warning")
            << QString::fromLatin1("../../../src/shared/proparser/profileevaluator.cpp: In member function 'ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(const ProString&, const ProStringList&)':\n"
                                   "../../../src/shared/proparser/profileevaluator.cpp:2817:9: warning: case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In member function 'ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(const ProString&, const ProStringList&)':"),
                        Utils::FilePath::fromUserInput(QLatin1String("../../../src/shared/proparser/profileevaluator.cpp")), -1,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("case value '0' not in enumerated type 'ProFileEvaluator::Private::TestFunc'"),
                        Utils::FilePath::fromUserInput(QLatin1String("../../../src/shared/proparser/profileevaluator.cpp")), 2817,
                        categoryCompile))
            << QString();

    QTest::newRow("include with line:column info")
            << QString::fromLatin1("In file included from <command-line>:0:0:\n"
                                   "./mw.h:4:0: warning: \"STUPID_DEFINE\" redefined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In file included from <command-line>:0:0:"),
                        Utils::FilePath::fromUserInput(QLatin1String("<command-line>")), 0,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("\"STUPID_DEFINE\" redefined"),
                        Utils::FilePath::fromUserInput(QLatin1String("./mw.h")), 4,
                        categoryCompile))
            << QString();
    QTest::newRow("instanciation with line:column info")
            << QString::fromLatin1("file.h: In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':\n"
                                   "file.cpp:87:10: instantiated from here\n"
                                   "file.h:21:5: warning: comparison between signed and unsigned integer expressions [-Wsign-compare]")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In function 'void UnitTest::CheckEqual(UnitTest::TestResults&, const Expected&, const Actual&, const UnitTest::TestDetails&) [with Expected = unsigned int, Actual = int]':"),
                        Utils::FilePath::fromUserInput(QLatin1String("file.h")), -1,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("instantiated from here"),
                        Utils::FilePath::fromUserInput(QLatin1String("file.cpp")), 87,
                        categoryCompile)
                << Task(Task::Warning,
                        QLatin1String("comparison between signed and unsigned integer expressions [-Wsign-compare]"),
                        Utils::FilePath::fromUserInput(QLatin1String("file.h")), 21,
                        categoryCompile))
            << QString();
    QTest::newRow("linker error") // QTCREATORBUG-3107
            << QString::fromLatin1("cns5k_ins_parser_tests.cpp:(.text._ZN20CNS5kINSParserEngine21DropBytesUntilStartedEP14CircularBufferIhE[CNS5kINSParserEngine::DropBytesUntilStarted(CircularBuffer<unsigned char>*)]+0x6d): undefined reference to `CNS5kINSPacket::SOH_BYTE'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Error,
                        QLatin1String("undefined reference to `CNS5kINSPacket::SOH_BYTE'"),
                        Utils::FilePath::fromUserInput(QLatin1String("cns5k_ins_parser_tests.cpp")), -1,
                        categoryCompile))
            << QString();

    QTest::newRow("uic warning")
            << QString::fromLatin1("mainwindow.ui: Warning: The name 'pushButton' (QPushButton) is already in use, defaulting to 'pushButton1'.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Warning,
                         QLatin1String("The name 'pushButton' (QPushButton) is already in use, defaulting to 'pushButton1'."),
                         Utils::FilePath::fromUserInput(QLatin1String("mainwindow.ui")), -1,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("libimf warning")
            << QString::fromLatin1("libimf.so: warning: warning: feupdateenv is not implemented and will always fail")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                 << Task(Task::Warning,
                         QLatin1String("warning: feupdateenv is not implemented and will always fail"),
                         Utils::FilePath::fromUserInput(QLatin1String("libimf.so")), -1,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("gcc 4.8")
            << QString::fromLatin1("In file included from /home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp:31:0:\n"
                                   ".uic/ui_pluginerrorview.h:14:25: fatal error: QtGui/QAction: No such file or directory\n"
                                   " #include <QtGui/QAction>\n"
                                   "                         ^")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In file included from /home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp:31:0:"),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/code/src/creator/src/libs/extensionsystem/pluginerrorview.cpp")), 31,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("QtGui/QAction: No such file or directory\n"
                                      " #include <QtGui/QAction>\n"
                                      "                         ^"),
                        Utils::FilePath::fromUserInput(QLatin1String(".uic/ui_pluginerrorview.h")), 14,
                        categoryCompile))
            << QString();

    QTest::newRow("qtcreatorbug-9195")
            << QString::fromLatin1("In file included from /usr/include/qt4/QtCore/QString:1:0,\n"
                                   "                 from main.cpp:3:\n"
                                   "/usr/include/qt4/QtCore/qstring.h: In function 'void foo()':\n"
                                   "/usr/include/qt4/QtCore/qstring.h:597:5: error: 'QString::QString(const char*)' is private\n"
                                   "main.cpp:7:22: error: within this context")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In file included from /usr/include/qt4/QtCore/QString:1:0,"),
                        Utils::FilePath::fromUserInput(QLatin1String("/usr/include/qt4/QtCore/QString")), 1,
                        categoryCompile)
                 << Task(Task::Unknown,
                         QLatin1String("from main.cpp:3:"),
                         Utils::FilePath::fromUserInput(QLatin1String("main.cpp")), 3,
                         categoryCompile)
                 << Task(Task::Unknown,
                         QLatin1String("In function 'void foo()':"),
                         Utils::FilePath::fromUserInput(QLatin1String("/usr/include/qt4/QtCore/qstring.h")), -1,
                         categoryCompile)
                << Task(Task::Error,
                        QLatin1String("'QString::QString(const char*)' is private"),
                        Utils::FilePath::fromUserInput(QLatin1String("/usr/include/qt4/QtCore/qstring.h")), 597,
                        categoryCompile)
                 << Task(Task::Error,
                         QLatin1String("within this context"),
                         Utils::FilePath::fromUserInput(QLatin1String("main.cpp")), 7,
                         categoryCompile))
            << QString();

    QTest::newRow("ld: Multiple definition error")
            << QString::fromLatin1("foo.o: In function `foo()':\n"
                                   "/home/user/test/foo.cpp:2: multiple definition of `foo()'\n"
                                   "bar.o:/home/user/test/bar.cpp:4: first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Unknown,
                        QLatin1String("In function `foo()':"),
                        Utils::FilePath::fromUserInput(QLatin1String("foo.o")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("multiple definition of `foo()'"),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/user/test/foo.cpp")), 2,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("first defined here"),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/user/test/bar.cpp")), 4,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("collect2: error: ld returned 1 exit status"),
                        Utils::FilePath(), -1,
                        categoryCompile)
                )
            << QString();

    QTest::newRow("ld: .data section")
            << QString::fromLatin1("foo.o:(.data+0x0): multiple definition of `foo'\n"
                                   "bar.o:(.data+0x0): first defined here\n"
                                   "collect2: error: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Error,
                        QLatin1String("multiple definition of `foo'"),
                        Utils::FilePath::fromUserInput(QLatin1String("foo.o")), -1,
                        categoryCompile)
                << Task(Task::Unknown,
                        QLatin1String("first defined here"),
                        Utils::FilePath::fromUserInput(QLatin1String("bar.o")), -1,
                        categoryCompile)
                << Task(Task::Error,
                        QLatin1String("collect2: error: ld returned 1 exit status"),
                        Utils::FilePath(), -1,
                        categoryCompile)
                )
            << QString();

    QTest::newRow("ld: undefined member function reference")
            << "obj/gtest-clang-printing.o:gtest-clang-printing.cpp:llvm::VerifyDisableABIBreakingChecks: error: undefined reference to 'llvm::DisableABIBreakingChecks'"
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Error,
                        QLatin1String("error: undefined reference to 'llvm::DisableABIBreakingChecks'"),
                        Utils::FilePath::fromString("gtest-clang-printing.cpp"), -1,
                        categoryCompile)
                )
            << QString();

    const auto task = [categoryCompile](Task::TaskType type, const QString &msg,
                                        const QString &file = {}, int line = -1) {
        return Task(type, msg, FilePath::fromString(file), line, categoryCompile);
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
            << Tasks{
                   errorTask("ld.lld: error: undefined symbol: func()"),
                   unknownTask("referenced by test.cpp:5", "test.cpp", 5),
                   unknownTask("/tmp/ccg8pzRr.o:(main)",  "/tmp/ccg8pzRr.o"),
                   errorTask("collect2: error: ld returned 1 exit status")}
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
                << Task(Task::Warning,
                        QLatin1String("file: lib/libtest.a(Test0.cpp.o) has no symbols"),
                        Utils::FilePath(), -1,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("Mac: ranlib warning2")
            << QString::fromLatin1("/path/to/XCode/and/ranlib: file: lib/libtest.a(Test0.cpp.o) has no symbols")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Warning,
                        QLatin1String("file: lib/libtest.a(Test0.cpp.o) has no symbols"),
                        Utils::FilePath(), -1,
                        categoryCompile)
                )
            << QString();
    QTest::newRow("moc note")
            << QString::fromLatin1("/home/qtwebkithelpviewer.h:0: Note: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (Tasks()
                << Task(Task::Unknown,
                        QLatin1String("Note: No relevant classes found. No output generated."),
                        Utils::FilePath::fromUserInput(QLatin1String("/home/qtwebkithelpviewer.h")), 0,
                        categoryCompile)
                )
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
            << Tasks{Task(Task::Unknown,
                     "In file included from /usr/include/qt/QtCore/qlocale.h:43,",
                     Utils::FilePath::fromUserInput("/usr/include/qt/QtCore/qlocale.h"), 43,
                     categoryCompile),
               Task(Task::Unknown,
                    "from /usr/include/qt/QtCore/qtextstream.h:46,",
                    Utils::FilePath::fromUserInput("/usr/include/qt/QtCore/qtextstream.h"), 46,
                    categoryCompile),
               Task(Task::Unknown,
                    "from /qtc/src/shared/proparser/proitems.cpp:31:",
                    Utils::FilePath::fromUserInput("/qtc/src/shared/proparser/proitems.cpp"), 31,
                    categoryCompile),
               Task(Task::Unknown,
                    "In constructor ‘QVariant::QVariant(QVariant&&)’:",
                    Utils::FilePath::fromUserInput("/usr/include/qt/QtCore/qvariant.h"), -1,
                    categoryCompile),
               Task(Task::Warning,
                    "implicitly-declared ‘constexpr QVariant::Private& QVariant::Private::operator=(const QVariant::Private&)’ is deprecated [-Wdeprecated-copy]\n"
                    "  273 |     { other.d = Private(); }\n"
                    "      |                         ^",
                    Utils::FilePath::fromUserInput("/usr/include/qt/QtCore/qvariant.h"), 273,
                    categoryCompile),
               Task(Task::Unknown,
                    "because ‘QVariant::Private’ has user-provided ‘QVariant::Private::Private(const QVariant::Private&)’\n"
                    "  399 |         inline Private(const Private &other) Q_DECL_NOTHROW\n"
                    "      |                      ^~~~~~~)",
                    Utils::FilePath::fromUserInput("/usr/include/qt/QtCore/qvariant.h"), 399,
                    categoryCompile),
               Task(Task::Unknown,
                    "In function ‘int test(const shape&, const shape&)’:",
                    Utils::FilePath::fromUserInput("t.cc"), -1,
                    categoryCompile),
               Task(Task::Error,
                    "no match for ‘operator+’ (operand types are ‘boxed_value<double>’ and ‘boxed_value<double>’)\n"
                    "  14 |   return (width(s1) * height(s1)\n"
                    "     |           ~~~~~~~~~~~~~~~~~~~~~~\n"
                    "     |                     |\n"
                    "     |                     boxed_value<[...]>\n"
                    "  15 |    + width(s2) * height(s2));\n"
                    "     |    ^ ~~~~~~~~~~~~~~~~~~~~~~\n"
                    "     |                |\n"
                    "     |                boxed_value<[...]>",
                    Utils::FilePath::fromUserInput("t.cc"), 15,
                    categoryCompile),
               Task(Task::Error,
                    "‘string’ in namespace ‘std’ does not name a type\n"
                    "  1 | std::string test(void)\n"
                    "    |      ^~~~~~",
                    Utils::FilePath::fromUserInput("incomplete.c"), 1, categoryCompile),
               Task(Task::Unknown,
                    "‘std::string’ is defined in header ‘<string>’; did you forget to ‘#include <string>’?\n"
                    " +++ |+#include <string>\n"
                    "  1 | std::string test(void)",
                    Utils::FilePath::fromUserInput("incomplete.c"), 1, categoryCompile),
               Task(Task::Unknown,
                    "In function ‘caller’:",
                    Utils::FilePath::fromUserInput("param-type-mismatch.c"), -1, categoryCompile),
               Task(Task::Warning,
                    "passing argument 2 of ‘callee’ makes pointer from integer without a cast [-Wint-conversion]\n"
                    "  5 |   return callee(first, second, third);\n"
                    "    |                        ^~~~~~\n"
                    "    |                        |\n"
                    "    |                        int",
                    Utils::FilePath::fromUserInput("param-type-mismatch.c"), 5, categoryCompile),
               Task(Task::Unknown,
                    "expected ‘const char *’ but argument is of type ‘int’\n"
                    "  1 | extern int callee(int one, const char *two, float three);\n"
                    "    |                            ~~~~~~~~~~~~^~~",
                    Utils::FilePath::fromUserInput("param-type-mismatch.c"), 1, categoryCompile)}
            << QString();
}

void ProjectExplorerPlugin::testGccOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new GccParser);
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
