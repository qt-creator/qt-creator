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

#include "gnumakeparser.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QFile>

using namespace ProjectExplorer;

namespace {
    // optional full path, make executable name, optional exe extension, optional number in square brackets, colon space
    const char * const MAKEEXEC_PATTERN("^(.*?[/\\\\])?(mingw(32|64)-|g)?make(.exe)?(\\[\\d+\\])?:\\s");
    const char * const MAKEFILE_PATTERN("^((.*?[/\\\\])?[Mm]akefile(\\.[a-zA-Z]+)?):(\\d+):\\s");
}

GnuMakeParser::GnuMakeParser()
{
    setObjectName(QLatin1String("GnuMakeParser"));
    m_makeDir.setPattern(QLatin1String(MAKEEXEC_PATTERN) +
                         QLatin1String("(\\w+) directory .(.+).$"));
    QTC_CHECK(m_makeDir.isValid());
    m_makeLine.setPattern(QLatin1String(MAKEEXEC_PATTERN) + QLatin1String("(.*)$"));
    QTC_CHECK(m_makeLine.isValid());
    m_errorInMakefile.setPattern(QLatin1String(MAKEFILE_PATTERN) + QLatin1String("(.*)$"));
    QTC_CHECK(m_errorInMakefile.isValid());
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

    QRegularExpressionMatch match = m_makeDir.match(lne);
    if (match.hasMatch()) {
        if (match.captured(6) == QLatin1String("Leaving"))
            removeDirectory(match.captured(7));
        else
            addDirectory(match.captured(7));
        return;
    }

    IOutputParser::stdOutput(line);
}

class Result {
public:
    Result() : isFatal(false), type(Task::Error) { }

    QString description;
    bool isFatal;
    Task::TaskType type;
};

static Result parseDescription(const QString &description)
{
    Result result;
    if (description.startsWith(QLatin1String("warning: "), Qt::CaseInsensitive)) {
        result.description = description.mid(9);
        result.type = Task::Warning;
        result.isFatal = false;
    } else if (description.startsWith(QLatin1String("*** "))) {
        result.description = description.mid(4);
        result.type = Task::Error;
        result.isFatal = true;
    } else {
        result.description = description;
        result.type = Task::Error;
        result.isFatal = false;
    }
    return result;
}

void GnuMakeParser::stdError(const QString &line)
{
    const QString lne = rightTrimmed(line);

    QRegularExpressionMatch match = m_errorInMakefile.match(lne);
    if (match.hasMatch()) {
        Result res = parseDescription(match.captured(5));
        if (res.isFatal)
            ++m_fatalErrorCount;
        if (!m_suppressIssues) {
            taskAdded(Task(res.type, res.description,
                           Utils::FileName::fromUserInput(match.captured(1)) /* filename */,
                           match.captured(4).toInt(), /* line */
                           Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)), 1, 0);
        }
        return;
    }
    match = m_makeLine.match(lne);
    if (match.hasMatch()) {
        Result res = parseDescription(match.captured(6));
        if (res.isFatal)
            ++m_fatalErrorCount;
        if (!m_suppressIssues) {
            Task task = Task(res.type, res.description,
                             Utils::FileName() /* filename */, -1, /* line */
                             Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
            taskAdded(task, 1, 0);
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

void GnuMakeParser::taskAdded(const Task &task, int linkedLines, int skippedLines)
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

    IOutputParser::taskAdded(editable, linkedLines, skippedLines);
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
            << QList<Task>()
            << QString()
            << QStringList();

    // make sure adding directories works (once;-)
    QTest::newRow("entering directory")
            << QStringList("/test/dir")
            << QString::fromLatin1("make[4]: Entering directory `/home/code/build/qt/examples/opengl/grabber'\n"
                                   "make[4]: Entering directory `/home/code/build/qt/examples/opengl/grabber'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << QList<Task>()
            << QString()
            << QStringList({"/home/code/build/qt/examples/opengl/grabber",
                            "/home/code/build/qt/examples/opengl/grabber", "/test/dir"});
    QTest::newRow("leaving directory")
            << QStringList({"/home/code/build/qt/examples/opengl/grabber", "/test/dir"})
            << QString::fromLatin1("make[4]: Leaving directory `/home/code/build/qt/examples/opengl/grabber'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << QList<Task>()
            << QString()
            << QStringList("/test/dir");
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
            << QString::fromLatin1("/home/dev/creator/share/qtcreator/debugger/dumper.cpp:1079: note: initialized from here")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("/home/dev/creator/share/qtcreator/debugger/dumper.cpp:1079: note: initialized from here\n")
            << QList<Task>()
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
    QTest::newRow("warning in Makefile")
            << QStringList()
            << QString::fromLatin1("Makefile:794: warning: overriding commands for target `xxxx.app/Contents/Info.plist'")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QString::fromLatin1("overriding commands for target `xxxx.app/Contents/Info.plist'"),
                        Utils::FileName::fromString(QLatin1String("Makefile")), 794,
                        Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)))
            << QString()
            << QStringList();
}

void ProjectExplorerPlugin::testGnuMakeParserParsing()
{
    OutputParserTester testbench;
    GnuMakeParser *childParser = new GnuMakeParser;
    GnuMakeParserTester *tester = new GnuMakeParserTester(childParser);
    connect(&testbench, &OutputParserTester::aboutToDeleteParser,
            tester, &GnuMakeParserTester::parserIsAboutToBeDeleted);

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
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Error,
                    QLatin1String("no filename, no mangling"),
                    Utils::FileName(),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE);
   QTest::newRow("no mangling")
            << QStringList()
            << QStringList()
            << Task(Task::Error,
                    QLatin1String("unknown filename, no mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("some/path/unknown.cpp")),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Error,
                    QLatin1String("unknown filename, no mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("some/path/unknown.cpp")),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE);
    QTest::newRow("find file")
            << QStringList("test/file.cpp")
            << QStringList("test")
            << Task(Task::Error,
                    QLatin1String("mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("file.cpp")),
                    10,
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Error,
                    QLatin1String("mangling"),
                    Utils::FileName::fromUserInput(QLatin1String("$TMPDIR/test/file.cpp")),
                    10,
                    Constants::TASK_CATEGORY_COMPILE);
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
    const QString tempdir
            = Utils::TemporaryDirectory::masterDirectoryPath() + '/' + QUuid::createUuid().toString() + '/';
    QDir filedir(tempdir);
    foreach (const QString &file, files) {
        Q_ASSERT(!file.startsWith('/'));
        Q_ASSERT(!file.contains("../"));

        filedir.mkpath(file.left(file.lastIndexOf('/')));

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
