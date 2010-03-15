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

#include "gnumakeparser.h"

#include "projectexplorerconstants.h"
#include "taskwindow.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

using namespace ProjectExplorer;

namespace {
    const char * const MAKE_PATTERN("^(mingw(32|64)-|g)?make(\\[\\d+\\])?:\\s");
}

GnuMakeParser::GnuMakeParser(const QString &dir) :
    m_alreadyFatal(false)
{
    m_makeDir.setPattern(QLatin1String(MAKE_PATTERN) +
                         QLatin1String("(\\w+) directory .(.+).$"));
    m_makeDir.setMinimal(true);
    m_makeLine.setPattern(QLatin1String(MAKE_PATTERN) + QLatin1String("(.*)$"));
    m_makeLine.setMinimal(true);
    m_makefileError.setPattern(QLatin1String("^(.*):(\\d+):\\s\\*\\*\\*\\s(.*)$"));
    m_makefileError.setMinimal(true);
    addDirectory(dir);
}

void GnuMakeParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();

    if (m_makeDir.indexIn(lne) > -1) {
        if (m_makeDir.cap(4) == "Leaving")
            removeDirectory(m_makeDir.cap(5));
        else
            addDirectory(m_makeDir.cap(5));
        return;
    }
    // Only ever report the first fatal message:
    // Everything else will be follow-up issues.
    if (m_makeLine.indexIn(lne) > -1) {
        if (!m_alreadyFatal) {
            QString message = m_makeLine.cap(4);
            Task task(Task::Warning,
                      message,
                      QString() /* filename */,
                      -1, /* line */
                      Constants::TASK_CATEGORY_BUILDSYSTEM);
            if (message.startsWith(QLatin1String("*** "))) {
                task.description = task.description.mid(4);
                task.type = Task::Error;
                m_alreadyFatal = true;
            }
            addTask(task);
        }
        return;
    }
    if (m_makefileError.indexIn(lne) > -1) {
        if (!m_alreadyFatal) {
            m_alreadyFatal = true;
            addTask(Task(Task::Error,
                         m_makefileError.cap(3),
                         m_makefileError.cap(1),
                         m_makefileError.cap(2).toInt(),
                         Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
        return;
    }

    IOutputParser::stdOutput(line);
}

void GnuMakeParser::addDirectory(const QString &dir)
{
    if (dir.isEmpty() || m_directories.contains(dir))
        return;
    m_directories.append(dir);
}

void GnuMakeParser::removeDirectory(const QString &dir)
{
    m_directories.removeAll(dir);
}

void GnuMakeParser::taskAdded(const Task &task)
{
    Task editable(task);
    QString filePath(QDir::cleanPath(task.file.trimmed()));

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
            editable.file = possibleFiles.first().filePath();
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

#   include <QtCore/QDebug>
#   include <QtCore/QUuid>

#   include "outputparser_test.h"
#   include "projectexplorer.h"
#   include "projectexplorerconstants.h"

#   include "metatypedeclarations.h"

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
            << QString::fromLatin1("Sometext") << QString()
            << QList<Task>()
            << QString()
            << QStringList();
    QTest::newRow("pass-through stderr")
            << QStringList()
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext")
            << QList<Task>()
            << QString()
            << QStringList();
    // make sure adding directories works (once;-)
    QTest::newRow("entering directory")
            << (QStringList() << QString::fromLatin1("/test/dir") )
            << QString::fromLatin1("make[4]: Entering directory `/home/code/build/qt/examples/opengl/grabber'\n"
                                   "make[4]: Entering directory `/home/code/build/qt/examples/opengl/grabber'\n")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << QList<Task>()
            << QString()
            << (QStringList() << QString::fromLatin1("/home/code/build/qt/examples/opengl/grabber") << QString::fromLatin1("/test/dir"));
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
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("No rule to make target `hello.c', needed by `hello.o'.  Stop."),
                        QString(), -1,
                        Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString()
            << QStringList();
    QTest::newRow("make warning")
            << QStringList()
            << QString::fromLatin1("make: warning: ignoring old commands for target `xxx'")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QString::fromLatin1("warning: ignoring old commands for target `xxx'"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString()
            << QStringList();
    QTest::newRow("multiple fatals")
            << QStringList()
            << QString::fromLatin1("make[3]: *** [.obj/debug-shared/gnumakeparser.o] Error 1\n"
                                   "make[3]: *** Waiting for unfinished jobs....\n"
                                   "make[2]: *** [sub-projectexplorer-make_default] Error 2")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("[.obj/debug-shared/gnumakeparser.o] Error 1"),
                        QString(), -1,
                        Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString()
            << QStringList();
    QTest::newRow("Makefile error")
            << QStringList()
            << QString::fromLatin1("Makefile:360: *** missing separator (did you mean TAB instead of 8 spaces?). Stop.")
            << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QString::fromLatin1("missing separator (did you mean TAB instead of 8 spaces?). Stop."),
                        QString::fromLatin1("Makefile"), 360,
                        Constants::TASK_CATEGORY_BUILDSYSTEM))
            << QString()
            << QStringList();
}

void ProjectExplorerPlugin::testGnuMakeParserParsing()
{
    OutputParserTester testbench;
    GnuMakeParser *childParser = new GnuMakeParser;
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
    foreach(const QString &dir, extraSearchDirs)
        childParser->addDirectory(dir);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);

    // make sure we still have all the original dirs
    QStringList newSearchDirs = childParser->searchDirectories();
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
                    QString(),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Error,
                    QLatin1String("no filename, no mangling"),
                    QString(),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE);
   QTest::newRow("no mangling")
            << QStringList()
            << QStringList()
            << Task(Task::Error,
                    QLatin1String("unknown filename, no mangling"),
                    QString::fromLatin1("some/path/unknown.cpp"),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Error,
                    QLatin1String("unknown filename, no mangling"),
                    QString::fromLatin1("some/path/unknown.cpp"),
                    -1,
                    Constants::TASK_CATEGORY_COMPILE);
    QTest::newRow("find file")
            << (QStringList() << "test/file.cpp")
            << (QStringList() << "test")
            << Task(Task::Error,
                    QLatin1String("mangling"),
                    QString::fromLatin1("file.cpp"),
                    10,
                    Constants::TASK_CATEGORY_COMPILE)
            << Task(Task::Error,
                    QLatin1String("mangling"),
                    QString::fromLatin1("$TMPDIR/test/file.cpp"),
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
    QString tempdir;
#if defined Q_OS_WIN
    tempdir = QDir::fromNativeSeparators(qgetenv("TEMP"));
#else
    tempdir = QLatin1String("/tmp");
#endif
    tempdir.append(QChar('/'));
    tempdir.append(QUuid::createUuid().toString());
    tempdir.append(QChar('/'));

    QDir filedir(tempdir);
    foreach (const QString &file, files) {
        Q_ASSERT(!file.startsWith(QChar('/')));
        Q_ASSERT(!file.contains(QLatin1String("../")));

        filedir.mkpath(file.left(file.lastIndexOf(QChar('/'))));

        QFile tempfile(tempdir + file);
        if (!tempfile.open(QIODevice::WriteOnly))
            continue;
        tempfile.write("Delete me again!");
        tempfile.close();
    }

    // setup search dirs:
    foreach (const QString &dir, searchDirectories) {
        Q_ASSERT(!dir.startsWith(QChar('/')));
        Q_ASSERT(!dir.contains(QLatin1String("../")));
        childParser->addDirectory(tempdir + dir);
    }

    // fix up output task file:
    if (outputTask.file.startsWith(QLatin1String("$TMPDIR/")))
        outputTask.file = outputTask.file.replace(QLatin1String("$TMPDIR/"), tempdir);

    // test mangling:
    testbench.testTaskMangling(inputTask, outputTask);

    // clean up:
    foreach (const QString &file, files)
        filedir.rmpath(tempdir + file);
}
#endif
