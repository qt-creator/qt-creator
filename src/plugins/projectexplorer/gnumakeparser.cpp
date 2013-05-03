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

#include "gnumakeparser.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include <QDir>
#include <QFile>

using namespace ProjectExplorer;

namespace {
    // optional full path, make executable name, optional exe extension, optional number in square brackets, colon space
    const char * const MAKE_PATTERN("^(([A-Za-z]:)?[/\\\\][^:]*[/\\\\])?(mingw(32|64)-|g)?make(.exe)?(\\[\\d+\\])?:\\s");
}

GnuMakeParser::GnuMakeParser() :
    m_suppressIssues(false),
    m_fatalErrorCount(0)
{
    setObjectName(QLatin1String("GnuMakeParser"));
    m_makeDir.setPattern(QLatin1String(MAKE_PATTERN) +
                         QLatin1String("(\\w+) directory .(.+).$"));
    m_makeDir.setMinimal(true);
    m_makeLine.setPattern(QLatin1String(MAKE_PATTERN) + QLatin1String("(\\*\\*\\*\\s)?(.*)$"));
    m_makeLine.setMinimal(true);
    m_makefileError.setPattern(QLatin1String("^(.*):(\\d+):\\s\\*\\*\\*\\s(.*)$"));
    m_makefileError.setMinimal(true);
}

void GnuMakeParser::setWorkingDirectory(const QString &workingDirectory)
{
    addDirectory(workingDirectory);
    IOutputParser::setWorkingDirectory(workingDirectory);
}

bool GnuMakeParser::hasFatalErrors() const
{
    return (m_fatalErrorCount > 0) || IOutputParser::hasFatalErrors();
}

void GnuMakeParser::stdOutput(const QString &line)
{
    const QString lne = rightTrimmed(line);

    if (m_makeDir.indexIn(lne) > -1) {
        if (m_makeDir.cap(7) == QLatin1String("Leaving"))
            removeDirectory(m_makeDir.cap(8));
        else
            addDirectory(m_makeDir.cap(8));
        return;
    }

    IOutputParser::stdOutput(line);
}

void GnuMakeParser::stdError(const QString &line)
{
    const QString lne = rightTrimmed(line);

    if (m_makefileError.indexIn(lne) > -1) {
        ++m_fatalErrorCount;
        if (!m_suppressIssues) {
            m_suppressIssues = true;
            emit addTask(Task(Task::Error,
                              m_makefileError.cap(3),
                              Utils::FileName::fromUserInput(m_makefileError.cap(1)),
                              m_makefileError.cap(2).toInt(),
                              Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
        }
        return;
    }
    if (m_makeLine.indexIn(lne) > -1) {
        if (!m_makeLine.cap(7).isEmpty())
            ++m_fatalErrorCount;
        if (!m_suppressIssues) {
            m_suppressIssues = true;
            QString description = m_makeLine.cap(8);
            Task::TaskType type = Task::Error;
            if (description.startsWith(QLatin1String("warning: "))) {
                description = description.mid(9);
                type = Task::Warning;
            }

            emit addTask(Task(type, description,
                              Utils::FileName() /* filename */,
                              -1, /* line */
                              Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
        }
        return;
    }

    IOutputParser::stdError(line);
}

void GnuMakeParser::addDirectory(const QString &dir)
{
    if (dir.isEmpty())
        return;
    m_directories.append(dir);
}

void GnuMakeParser::removeDirectory(const QString &dir)
{
    m_directories.removeOne(dir);
}

void GnuMakeParser::taskAdded(const Task &task)
{
    Task editable(task);

    if (task.type == Task::Error) {
        // assume that all make errors will be follow up errors:
        m_suppressIssues = true;
    }

    QString filePath(task.file.toString());

    if (!filePath.isEmpty() && !QDir::isAbsolutePath(filePath)) {
        QList<QFileInfo> possibleFiles;
        foreach (const QString &dir, m_directories) {
            QFileInfo candidate(dir + QLatin1Char('/') + filePath);
            if (candidate.exists()
                && !possibleFiles.contains(candidate)) {
                possibleFiles << candidate;
            }
        }
        if (possibleFiles.size() == 1)
            editable.file = Utils::FileName(possibleFiles.first());
        // Let the Makestep apply additional heuristics (based on
        // files in ther project) if we can not uniquely
        // identify the file!
    }

    IOutputParser::taskAdded(editable);
}

#if defined WITH_TESTS
QStringList GnuMakeParser::searchDirectories() const
{
    return m_directories;
}
#endif

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include <QUuid>

#   include "outputparser_test.h"
#   include "projectexplorer.h"
#   include "projectexplorerconstants.h"

#   include "metatypedeclarations.h"

GnuMakeParserTester::GnuMakeParserTester(GnuMakeParser *p, QObject *parent) :
    QObject(parent),
    parser(p)
{ }

void GnuMakeParserTester::parserIsAboutToBeDeleted()
{
    directories = parser->searchDirectories();
}

void ProjectExplorerPlugin::testGnuMakeParserParsing_data()
{
    QTest::addColumn<QStringList>("extraSearchDirs");
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");
    QTest::addColumn<QStringList>("additionalSearchDirs");

    QTest::newRow("pass-through stdout")
            << QStringList()
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString()
            << QStringList();
    QTest::newRow("pass-through stderr")
            << QStringList()
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString()
            << QStringList();
    QTest::newRow("pass-through gcc infos")
            << QStringList()
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here")
            << OutputParserTester::STDERR
            << QString()
            << QString::fromLatin1("/temp/test/untitled8/main.cpp: In function `int main(int, char**)':\n"
                                   "../../scriptbug/main.cpp: At global scope:\n"
                                   "../../scriptbug/main.cpp: In instantiation of void bar(i) [with i = double]:\n"
                                   "../../scriptbug/main.cpp:8: instantiated from void foo(i) [with i = double]\n"
                                   "../../scriptbug/main.cpp:22: instantiated from here\n")
            << QList<ProjectExplorer::Task>()
            << QString()
            << QStringList();

    // make sure adding directories works (once;-)
    QTest::newRow("entering directory")
            << (QStringList() << QString::fromLatin1("/test/dir") )
            << QString::fromLatin1("make[4]: Entering directory `/home/code/build/qt/examples/opengl/grabber'\n"
                                   "make[4]: Entering directory `/home/code/build/qt/examples/opengl/grabber'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << QList<Task>()
            << QString()
            << (QStringList() << QString::fromLatin1("/home/code/build/qt/examples/opengl/grabber")
                              << QString::fromLatin1("/home/code/build/qt/examples/opengl/grabber")
                              << QString::fromLatin1("/test/dir"));
    QTest::newRow("leaving directory")
            << (QStringList()  << QString::fromLatin1("/home/code/build/qt/examples/opengl/grabber") << QString::fromLatin1("/test/dir"))
            << QString::fromLatin1("make[4]: Leaving directory `/home/code/build/qt/examples/opengl/grabber'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << QList<Task>()
            << QString()
            << (QStringList() << QString::fromLatin1("/test/dir"));
    QTest::newRow("make error")
            << QStringList()
            << QString::fromLatin1("make: *** No rule to make target `hello.c', needed by `hello.o'.  Stop.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("No rule to make target `hello.c', needed by `hello.o'.  Stop."),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("multiple fatals")
            << QStringList()
            << QString::fromLatin1("make[3]: *** [.obj/debug-shared/gnumakeparser.o] Error 1\n"
                                   "make[3]: *** Waiting for unfinished jobs....\n"
                                   "make[2]: *** [sub-projectexplorer-make_default] Error 2")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("[.obj/debug-shared/gnumakeparser.o] Error 1"),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("Makefile error")
            << QStringList()
            << QString::fromLatin1("Makefile:360: *** missing separator (did you mean TAB instead of 8 spaces?). Stop.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("missing separator (did you mean TAB instead of 8 spaces?). Stop."),
                        Utils::FileName::fromUserInput(QLatin1String("Makefile")), 360,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("mingw32-make error")
            << QStringList()
            << QString::fromLatin1("mingw32-make[1]: *** [debug/qplotaxis.o] Error 1\n"
                                   "mingw32-make: *** [debug] Error 2")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("[debug/qplotaxis.o] Error 1"),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("mingw64-make error")
            << QStringList()
            << QString::fromLatin1("mingw64-make.exe[1]: *** [dynlib.inst] Error -1073741819")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("[dynlib.inst] Error -1073741819"),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("make warning")
            << QStringList()
            << QString::fromLatin1("make[2]: warning: jobserver unavailable: using -j1. Add `+' to parent make rule.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QString::fromLatin1("jobserver unavailable: using -j1. Add `+' to parent make rule."),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("pass-trough note")
            << QStringList()
            << QString::fromLatin1("/home/dev/creator/share/qtcreator/dumper/dumper.cpp:1079: note: initialized from here")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("/home/dev/creator/share/qtcreator/dumper/dumper.cpp:1079: note: initialized from here\n")
            << QList<ProjectExplorer::Task>()
            << QString()
            << QStringList();
    QTest::newRow("Full path make exe")
            << QStringList()
            << QString::fromLatin1("C:\\Qt\\4.6.2-Symbian\\s60sdk\\epoc32\\tools\\make.exe: *** [sis] Error 2")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("[sis] Error 2"),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
    QTest::newRow("missing g++")
            << QStringList()
            << QString::fromLatin1("make: g++: Command not found")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("g++: Command not found"),
                        Utils::FileName(), -1,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
}

void ProjectExplorerPlugin::testGnuMakeParserParsing()
{
    OutputParserTester testbench;
    GnuMakeParser *childParser = new GnuMakeParser;
    GnuMakeParserTester *tester = new GnuMakeParserTester(childParser);
    connect(&testbench, SIGNAL(aboutToDeleteParser()),
            tester, SLOT(parserIsAboutToBeDeleted()));

    testbench.appendOutputParser(childParser);
    QFETCH(QStringList, extraSearchDirs);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);
    QFETCH(QStringList, additionalSearchDirs);

    QStringList searchDirs = childParser->searchDirectories();

    // add extra directories:
    foreach (const QString &dir, extraSearchDirs)
        childParser->addDirectory(dir);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);

    // make sure we still have all the original dirs
    QStringList newSearchDirs = tester->directories;
    foreach (const QString &dir, searchDirs) {
        QVERIFY(newSearchDirs.contains(dir));
        newSearchDirs.removeOne(dir);
    }

    // make sure we have all additional dirs:
    foreach (const QString &dir, additionalSearchDirs) {
        QVERIFY(newSearchDirs.contains(dir));
        newSearchDirs.removeOne(dir);
    }
    // make sure we have no extra cruft:
    QVERIFY(newSearchDirs.isEmpty());
    delete tester;
}

void ProjectExplorerPlugin::testGnuMakeParserTaskMangling_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QStringList>("searchDirectories");
    QTest::addColumn<Task>("inputTask");
    QTest::addColumn<Task>("outputTask");

    QTest::newRow("no filename")
            << QStringList()
            << QStringList()
            << Task(Task::Error,
                    QLatin1String("no filename, no mangling"),
                    Utils::FileName(),
                    -1,
                    Core::Id(Constants::TASK_CATEGORY_COMPILE))
            << Task(Task::Error,
                    QLatin1String("no filename, no mangling"),
                    Utils::FileName(),
                    -1,
                    Core::Id(Constants::TASK_CATEGORY_COMPILE));
   QTest::newRow("no mangling")
            << QStringList()
            << QStringList()
            << Task(Task::Error,
                    QLatin1String("unknown filename, no mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("some/path/unknown.cpp")),
                    -1,
                    Core::Id(Constants::TASK_CATEGORY_COMPILE))
            << Task(Task::Error,
                    QLatin1String("unknown filename, no mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("some/path/unknown.cpp")),
                    -1,
                    Core::Id(Constants::TASK_CATEGORY_COMPILE));
    QTest::newRow("find file")
            << (QStringList(QLatin1String("test/file.cpp")))
            << (QStringList(QLatin1String("test")))
            << Task(Task::Error,
                    QLatin1String("mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("file.cpp")),
                    10,
                    Core::Id(Constants::TASK_CATEGORY_COMPILE))
            << Task(Task::Error,
                    QLatin1String("mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("$TMPDIR/test/file.cpp")),
                    10,
                    Core::Id(Constants::TASK_CATEGORY_COMPILE));
}

void ProjectExplorerPlugin::testGnuMakeParserTaskMangling()
{
    OutputParserTester testbench;
    GnuMakeParser *childParser = new GnuMakeParser;
    testbench.appendOutputParser(childParser);

    QFETCH(QStringList, files);
    QFETCH(QStringList, searchDirectories);
    QFETCH(Task, inputTask);
    QFETCH(Task, outputTask);

    // setup files:
    QString tempdir = QDir::tempPath();
    const QChar slash = QLatin1Char('/');
    tempdir.append(slash);
    tempdir.append(QUuid::createUuid().toString());
    tempdir.append(slash);

    QDir filedir(tempdir);
    foreach (const QString &file, files) {
        Q_ASSERT(!file.startsWith(slash));
        Q_ASSERT(!file.contains(QLatin1String("../")));

        filedir.mkpath(file.left(file.lastIndexOf(slash)));

        QFile tempfile(tempdir + file);
        if (!tempfile.open(QIODevice::WriteOnly))
            continue;
        tempfile.write("Delete me again!");
        tempfile.close();
    }

    // setup search dirs:
    foreach (const QString &dir, searchDirectories) {
        Q_ASSERT(!dir.startsWith(QLatin1Char('/')));
        Q_ASSERT(!dir.contains(QLatin1String("../")));
        childParser->addDirectory(tempdir + dir);
    }

    // fix up output task file:
    QString filePath = outputTask.file.toString();
    if (filePath.startsWith(QLatin1String("$TMPDIR/")))
        outputTask.file = Utils::FileName::fromString(filePath.replace(QLatin1String("$TMPDIR/"), tempdir));

    // test mangling:
    testbench.testTaskMangling(inputTask, outputTask);

    // clean up:
    foreach (const QString &file, files)
        filedir.rmpath(tempdir + file);
}
#endif
