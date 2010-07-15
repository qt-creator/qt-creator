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

#include "gccparser.h"
#include "ldparser.h"
#include "taskwindow.h"
#include "projectexplorerconstants.h"

using namespace ProjectExplorer;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "^(([A-Za-z]:)?[^:]+\\.[^:]+):";
    const char * const COMMAND_PATTERN = "^(.*[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(gcc|g\\+\\+)(-[0-9\\.]+)?(\\.exe)?: ";
}

GccParser::GccParser()
{
    m_regExp.setPattern(QString::fromLatin1(FILE_PATTERN) + QLatin1String("(\\d+):(\\d+:)?\\s((fatal |#)?(warning|error|note):?\\s)(.+)$"));
    m_regExp.setMinimal(true);

    m_regExpIncluded.setPattern("^.*from\\s([^:]+):(\\d+)(,|:)$");
    m_regExpIncluded.setMinimal(true);

    // optional path with trailing slash
    // optional arm-linux-none-thingy
    // name of executable
    // optional trailing version number
    // optional .exe postfix
    m_regExpGccNames.setPattern(COMMAND_PATTERN);
    m_regExpGccNames.setMinimal(true);

    appendOutputParser(new LdParser);
}

void GccParser::stdError(const QString &line)
{
    QString lne = line.trimmed();

    // Blacklist some lines to not handle them:
    if (lne.startsWith(QLatin1String("TeamBuilder ")) ||
        lne.startsWith(QLatin1String("distcc["))) {
        IOutputParser::stdError(line);
        return;
    }

    // Handle misc issues:
    if (lne.startsWith(QLatin1String("ERROR:")) ||
        lne == QLatin1String("* cpp failed")) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          QString() /* filename */,
                          -1 /* linenumber */,
                          Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (m_regExpGccNames.indexIn(lne) > -1) {
        QString description = lne.mid(m_regExpGccNames.matchedLength());
        Task task(Task::Error,
                  description,
                  QString(), /* filename */
                  -1, /* line */
                  Constants::TASK_CATEGORY_COMPILE);
        if (description.startsWith(QLatin1String("warning: "))) {
            task.type = Task::Warning;
            task.description = description.mid(9);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            task.description = description.mid(7);
        }
        emit addTask(task);
        return;
    } else if (m_regExp.indexIn(lne) > -1) {
        QString filename = m_regExp.cap(1);
        int lineno = m_regExp.cap(3).toInt();
        Task task(Task::Unknown,
                  m_regExp.cap(8) /* description */,
                  filename, lineno,
                  Constants::TASK_CATEGORY_COMPILE);
        if (m_regExp.cap(7) == QLatin1String("warning"))
            task.type = Task::Warning;
        else if (m_regExp.cap(7) == QLatin1String("error") ||
                 task.description.startsWith(QLatin1String("undefined reference to")))
            task.type = Task::Error;

        // Prepend "#warning" or "#error" if that triggered the match on (warning|error)
        // We want those to show how the warning was triggered
        if (m_regExp.cap(5).startsWith(QChar('#')))
            task.description = m_regExp.cap(5) + task.description;

        emit addTask(task);
        return;
    } else if (m_regExpIncluded.indexIn(lne) > -1) {
        emit addTask(Task(Task::Unknown,
                          lne /* description */,
                          m_regExpIncluded.cap(1) /* filename */,
                          m_regExpIncluded.cap(2).toInt() /* linenumber */,
                          Constants::TASK_CATEGORY_COMPILE));
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

void ProjectExplorerPlugin::testGccOutputParsers_data()
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

    QTest::newRow("GCCE error")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "/temp/test/untitled8/main.cpp:9: error: `sfasdf' undeclared (first use this function)\n"
                                   "/temp/test/untitled8/main.cpp:9: error: (Each undeclared identifier is reported only once for each function it appears in.)")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                                    QLatin1String("In function `int main(int, char**)':"),
                                    QLatin1String("/temp/test/untitled8/main.cpp"), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                                    QLatin1String("`sfasdf' undeclared (first use this function)"),
                                    QLatin1String("/temp/test/untitled8/main.cpp"), 9,
                                    Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                                    QLatin1String("(Each undeclared identifier is reported only once for each function it appears in.)"),
                                    QLatin1String("/temp/test/untitled8/main.cpp"), 9,
                                    Constants::TASK_CATEGORY_COMPILE)
                )
            << QString();
    QTest::newRow("GCCE warning")
            << QString::fromLatin1("/src/corelib/global/qglobal.h:1635: warning: inline function `QDebug qDebug()' used but never defined")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Warning,
                        QLatin1String("inline function `QDebug qDebug()' used but never defined"),
                        QLatin1String("/src/corelib/global/qglobal.h"), 1635,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("warning")
            << QString::fromLatin1("main.cpp:7:2: warning: Some warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("Some warning"),
                                                       QLatin1String("main.cpp"), 7,
                                                       Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("GCCE #error")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:7: #error Symbian error")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Error,
                                                       QLatin1String("#error Symbian error"),
                                                       QLatin1String("C:\\temp\\test\\untitled8\\main.cpp"), 7,
                                                       Constants::TASK_CATEGORY_COMPILE))
            << QString();
    // Symbian reports #warning(s) twice (using different syntax).
    QTest::newRow("GCCE #warning1")
            << QString::fromLatin1("C:\\temp\\test\\untitled8\\main.cpp:8: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("#warning Symbian warning"),
                                                       QLatin1String("C:\\temp\\test\\untitled8\\main.cpp"), 8,
                                                       Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("GCCE #warning2")
            << QString::fromLatin1("/temp/test/untitled8/main.cpp:8:2: warning: #warning Symbian warning")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("#warning Symbian warning"),
                                                       QLatin1String("/temp/test/untitled8/main.cpp"), 8,
                                                       Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("Undefined reference (debug)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:8: undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In function `main':"),
                        QLatin1String("main.o"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                        QLatin1String("C:\\temp\\test\\untitled8/main.cpp"), 8,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("collect2: ld returned 1 exit status"),
                        QString(), -1,
                                    Constants::TASK_CATEGORY_COMPILE)
                )
            << QString();
    QTest::newRow("Undefined reference (release)")
            << QString::fromLatin1("main.o: In function `main':\n"
                                   "C:\\temp\\test\\untitled8/main.cpp:(.text+0x40): undefined reference to `MainWindow::doSomething()'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In function `main':"),
                        QLatin1String("main.o"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `MainWindow::doSomething()'"),
                        QLatin1String("C:\\temp\\test\\untitled8/main.cpp"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("collect2: ld returned 1 exit status"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                )
            << QString();
    QTest::newRow("linker: dll format not recognized")
            << QString::fromLatin1("c:\\Qt\\4.6\\lib/QtGuid4.dll: file not recognized: File format not recognized")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("file not recognized: File format not recognized"),
                        QLatin1String("c:\\Qt\\4.6\\lib/QtGuid4.dll"), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("Invalid rpath")
            << QString::fromLatin1("g++: /usr/local/lib: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("/usr/local/lib: No such file or directory"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("Invalid rpath")
            << QString::fromLatin1("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp: In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2114: warning: unused variable 'index'\n"
                                   "../../../../master/src/plugins/debugger/gdb/gdbengine.cpp:2115: warning: unused variable 'handler'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In member function 'void Debugger::Internal::GdbEngine::handleBreakInsert2(const Debugger::Internal::GdbResponse&)':"),
                        QLatin1String("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning,
                        QLatin1String("unused variable 'index'"),
                        QLatin1String("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"), 2114,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning,
                        QLatin1String("unused variable 'handler'"),
                        QLatin1String("../../../../master/src/plugins/debugger/gdb/gdbengine.cpp"), 2115,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();    
    QTest::newRow("gnumakeparser.cpp errors")
            << QString::fromLatin1("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp: In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected primary-expression before ':' token\n"
                                   "/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp:264: error: expected ';' before ':' token")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In member function 'void ProjectExplorer::ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()':"),
                        QLatin1String("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("expected primary-expression before ':' token"),
                        QLatin1String("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"), 264,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("expected ';' before ':' token"),
                        QLatin1String("/home/code/src/creator/src/plugins/projectexplorer/gnumakeparser.cpp"), 264,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("distcc error(QTCREATORBUG-904)")
            << QString::fromLatin1("distcc[73168] (dcc_get_hostlist) Warning: no hostlist is set; can't distribute work\n"
                                   "distcc[73168] (dcc_build_somewhere) Warning: failed to distribute, running locally instead")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("distcc[73168] (dcc_get_hostlist) Warning: no hostlist is set; can't distribute work\n"
                                                "distcc[73168] (dcc_build_somewhere) Warning: failed to distribute, running locally instead")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("ld warning (QTCREATORBUG-905)")
            << QString::fromLatin1("ld: warning: Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Warning,
                         QLatin1String("Core::IEditor* QVariant::value<Core::IEditor*>() const has different visibility (hidden) in .obj/debug-shared/openeditorsview.o and (default) in .obj/debug-shared/editormanager.o"),
                         QString(), -1,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("ld fatal")
            << QString::fromLatin1("ld: fatal: Symbol referencing errors. No output written to testproject")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Error,
                         QLatin1String("Symbol referencing errors. No output written to testproject"),
                         QString(), -1,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("Teambuilder issues")
            << QString::fromLatin1("TeamBuilder Client:: error: could not find Scheduler, running Job locally...")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("TeamBuilder Client:: error: could not find Scheduler, running Job locally...")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("note")
            << QString::fromLatin1("/home/dev/creator/share/qtcreator/gdbmacros/gdbmacros.cpp:1079: note: initialized from here")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Unknown,
                         QLatin1String("initialized from here"),
                         QString::fromLatin1("/home/dev/creator/share/qtcreator/gdbmacros/gdbmacros.cpp"), 1079,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("static member function")
            << QString::fromLatin1("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c: In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':\n"
                                   "/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c:194: warning: suggest explicit braces to avoid ambiguous 'else'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Unknown,
                         QLatin1String("In static member function 'static std::_Rb_tree_node_base* std::_Rb_global<_Dummy>::_Rebalance_for_erase(std::_Rb_tree_node_base*, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&, std::_Rb_tree_node_base*&)':"),
                         QString::fromLatin1("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c"), -1,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Warning,
                         QLatin1String("suggest explicit braces to avoid ambiguous 'else'"),
                         QString::fromLatin1("/Qt/4.6.2-Symbian/s60sdk/epoc32/include/stdapis/stlport/stl/_tree.c"), 194,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("rm false positive")
            << QString::fromLatin1("rm: cannot remove `release/moc_mainwindow.cpp': No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString("rm: cannot remove `release/moc_mainwindow.cpp': No such file or directory")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("ranlib false positive")
            << QString::fromLatin1("ranlib: file: libSupport.a(HashTable.o) has no symbols")
            << OutputParserTester::STDERR
            << QString() << QString("ranlib: file: libSupport.a(HashTable.o) has no symbols")
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("ld: missing library")
            << QString::fromLatin1("/usr/bin/ld: cannot find -ldoesnotexist")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Error,
                         QLatin1String("cannot find -ldoesnotexist"),
                         QString(), -1,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("In function")
            << QString::fromLatin1("../../scriptbug/main.cpp: In function void foo(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:8: warning: unused variable c")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Unknown,
                         QLatin1String("In function void foo(i) [with i = double]:"),
                         QLatin1String("../../scriptbug/main.cpp"), -1,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from here"),
                         QLatin1String("../../scriptbug/main.cpp"), 22,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Warning,
                         QLatin1String("unused variable c"),
                         QLatin1String("../../scriptbug/main.cpp"), 8,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("instanciated from here")
            << QString::fromLatin1("main.cpp:10: instantiated from here  ")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from here"),
                         QLatin1String("main.cpp"), 10,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();
    QTest::newRow("In constructor")
            << QString::fromLatin1("/dev/creator/src/plugins/find/basetextfind.h: In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Unknown,
                         QLatin1String("In constructor 'Find::BaseTextFind::BaseTextFind(QTextEdit*)':"),
                         QLatin1String("/dev/creator/src/plugins/find/basetextfind.h"), -1,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("At global scope")
            << QString::fromLatin1("../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n"
                                   "../../scriptbug/main.cpp:5: warning: unused parameter v")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Unknown,
                         QLatin1String("At global scope:"),
                         QLatin1String("../../scriptbug/main.cpp"), -1,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Unknown,
                         QLatin1String("In instantiation of void bar(i) [with i = double]:"),
                         QLatin1String("../../scriptbug/main.cpp"), -1,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from void foo(i) [with i = double]"),
                         QLatin1String("../../scriptbug/main.cpp"), 8,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Unknown,
                         QLatin1String("instantiated from here"),
                         QLatin1String("../../scriptbug/main.cpp"), 22,
                         Constants::TASK_CATEGORY_COMPILE)
                 << Task(Task::Warning,
                         QLatin1String("unused parameter v"),
                         QLatin1String("../../scriptbug/main.cpp"), 5,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("gcc 4.5 fatal error")
            << QString::fromLatin1("/home/code/test.cpp:54:38: fatal error: test.moc: No such file or directory")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                 << Task(Task::Error,
                         QLatin1String("test.moc: No such file or directory"),
                         QLatin1String("/home/code/test.cpp"), 54,
                         Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("QTCREATORBUG-597")
            << QString::fromLatin1("debug/qplotaxis.o: In function `QPlotAxis':\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "M:\\Development\\x64\\QtPlot/qplotaxis.cpp:26: undefined reference to `vtable for QPlotAxis'\n"
                                   "collect2: ld returned 1 exit status")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In function `QPlotAxis':"),
                        QLatin1String("debug/qplotaxis.o"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `vtable for QPlotAxis'"),
                        QLatin1String("M:\\Development\\x64\\QtPlot/qplotaxis.cpp"), 26,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("undefined reference to `vtable for QPlotAxis'"),
                        QLatin1String("M:\\Development\\x64\\QtPlot/qplotaxis.cpp"), 26,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Error,
                        QLatin1String("collect2: ld returned 1 exit status"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

    QTest::newRow("instantiated from here should not be an error")
            << QString::fromLatin1("../stl/main.cpp: In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:\n"
                                   "../stl/main.cpp:38:   instantiated from here\n"
                                   "../stl/main.cpp:31: warning: returning reference to temporary\n"
                                   "../stl/main.cpp: At global scope:\n"
                                   "../stl/main.cpp:31: warning: unused parameter index")
            << OutputParserTester::STDERR
            << QString() << QString()
            << ( QList<ProjectExplorer::Task>()
                << Task(Task::Unknown,
                        QLatin1String("In member function typename _Vector_base<_Tp, _Alloc>::_Tp_alloc_type::const_reference Vector<_Tp, _Alloc>::at(int) [with _Tp = Point, _Alloc = Allocator<Point>]:"),
                        QLatin1String("../stl/main.cpp"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        QLatin1String("instantiated from here"),
                        QLatin1String("../stl/main.cpp"), 38,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning,
                        QLatin1String("returning reference to temporary"),
                        QLatin1String("../stl/main.cpp"), 31,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Unknown,
                        QLatin1String("At global scope:"),
                        QLatin1String("../stl/main.cpp"), -1,
                        Constants::TASK_CATEGORY_COMPILE)
                << Task(Task::Warning,
                        QLatin1String("unused parameter index"),
                        QLatin1String("../stl/main.cpp"), 31,
                        Constants::TASK_CATEGORY_COMPILE))
            << QString();

}

void ProjectExplorerPlugin::testGccOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new GccParser);
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
