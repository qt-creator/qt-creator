// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaparser.h"

#include <projectexplorer/task.h>

#ifdef WITH_TESTS
#include <projectexplorer/outputparser_test.h>
#include <QTest>
#endif

#include <QRegularExpression>

#include <optional>

using namespace ProjectExplorer;
using namespace Utils;

namespace Java {

JavaParser::JavaParser()
{ }

void JavaParser::setProjectFileList(const FilePaths &fileList)
{
    m_fileList = fileList;
}

void JavaParser::setBuildDirectory(const FilePath &buildDirectory)
{
    m_buildDirectory = buildDirectory;
}

void JavaParser::setSourceDirectory(const FilePath &sourceDirectory)
{
    m_sourceDirectory = sourceDirectory;
}

// The kind javac names a diagnostic with, which it has been doing since
// JDK 6 - and translating along with the rest of the diagnostic, the JDK
// shipping catalogs of several languages. A word this does not know is one
// of those, and it is for the user to read rather than for this to tell an
// error from a warning by.
static std::optional<Task::TaskType> taskTypeOfKind(const QString &kind)
{
    if (kind.compare("error", Qt::CaseInsensitive) == 0)
        return Task::Error;
    if (kind.compare("warning", Qt::CaseInsensitive) == 0)
        return Task::Warning;
    if (kind.compare("note", Qt::CaseInsensitive) == 0)
        return Task::Unknown;
    return {};
}

OutputLineParser::Result JavaParser::handleLine(const QString &line, OutputFormat)
{
    // What follows the line number is the diagnostic. A column there is
    // another tool's doing - checkstyle prints one, and javac itself where
    // it is asked for raw diagnostics - and such a line is left to the
    // parser that reads that form.
    static const QRegularExpression javaRegExp(
        R"(^(?<prefix>.*\[javac\]\s+)?(?<file>.*\.java):(?<line>\d+):(?!\d)\s*(?<text>.+)$)");

    const QRegularExpressionMatch match = javaRegExp.match(line);
    if (!match.hasMatch())
        return Status::NotHandled;

    bool ok;
    int lineno = match.captured("line").toInt(&ok);
    if (!ok)
        lineno = -1;

    FilePath file = FilePath::fromUserInput(match.captured("file"));
    if (file.isChildOf(m_buildDirectory)) {
        FilePath relativePath = file.relativeChildPath(m_buildDirectory);
        file = m_sourceDirectory.resolvePath(relativePath);
    }
    if (file.isRelativePath()) {
        for (const FilePath &fp : std::as_const(m_fileList))
            if (fp.endsWith(file.path())) {
                file = fp;
                break;
            }
    }

    static const QRegularExpression kindRegExp(R"(^(?<kind>\S+):\s+(?<text>.+)$)");

    // A diagnostic that names no kind is an error, which is what javac
    // printed before JDK 6 and all it printed that way. One that names a
    // kind in a language this does not read keeps it in its text and is
    // reported without one rather than as an error it may well not be.
    QString text = match.captured("text").trimmed();
    Task::TaskType taskType = Task::Error;
    const QRegularExpressionMatch kindMatch = kindRegExp.match(text);
    if (kindMatch.hasMatch()) {
        const std::optional<Task::TaskType> kind = taskTypeOfKind(kindMatch.captured("kind"));
        taskType = kind.value_or(Task::Unknown);
        if (kind)
            text = kindMatch.captured("text");
    }

    CompileTask task(taskType, text, absoluteFilePath(file), lineno);
    LinkSpecs linkSpecs;
    addLinkSpecForAbsoluteFilePath(
        linkSpecs, task.file(), task.line(), task.column(), match, "file");
    scheduleTask(task, 1);
    return {Status::Done, linkSpecs};
}

#ifdef WITH_TESTS
namespace Internal {

class JavaParserTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<Tasks>("tasks");

        QTest::newRow("pass-through") << QString("Sometext") << Tasks();

        QTest::newRow("error of javac")
            << QString("/src/Greeter.java:11: error: cannot find symbol")
            << Tasks{CompileTask(Task::Error,
                                 "cannot find symbol",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 11)};

        QTest::newRow("warning of javac")
            << QString("/src/Greeter.java:3: warning: [deprecation] foo() is deprecated")
            << Tasks{CompileTask(Task::Warning,
                                 "[deprecation] foo() is deprecated",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 3)};

        // What Ant and Gradle print, which is what the Android package build
        // of Qt Creator has always parsed.
        QTest::newRow("error through Ant")
            << QString("    [javac] /src/Greeter.java:11: error: cannot find symbol")
            << Tasks{CompileTask(Task::Error,
                                 "cannot find symbol",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 11)};

        // A JDK older than 6 names no kind, and the lines that follow a
        // diagnostic are not diagnostics of their own.
        QTest::newRow("error without a kind")
            << QString("/src/Greeter.java:11: cannot find symbol")
            << Tasks{CompileTask(Task::Error,
                                 "cannot find symbol",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 11)};

        // What javac calls a note it capitalizes, and what an annotation
        // processor reports that way is of no kind rather than an error.
        QTest::newRow("note of an annotation processor")
            << QString("/src/Greeter.java:5: Note: generated GreeterBuilder")
            << Tasks{CompileTask(Task::Unknown,
                                 "generated GreeterBuilder",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 5)};

        // Android lint runs over the same sources and names its kind the
        // same way, which is what it says it is.
        QTest::newRow("warning of lint")
            << QString("/src/Greeter.java:12: Warning: Use TextView [SetTextI18n]")
            << Tasks{CompileTask(Task::Warning,
                                 "Use TextView [SetTextI18n]",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 12)};

        // javac translates the kind along with the rest of the diagnostic,
        // the JDK shipping a Japanese catalog among others. What it says
        // there is the user's to read, and no error for having been said in
        // a language this does not know.
        QTest::newRow("kind in another language")
            << QString("/src/Greeter.java:3: \u8b66\u544a: [deprecation] foo() is deprecated")
            << Tasks{CompileTask(Task::Unknown,
                                 "\u8b66\u544a: [deprecation] foo() is deprecated",
                                 FilePath::fromUserInput("/src/Greeter.java"),
                                 3)};

        // A column after the line number is the form of checkstyle and of
        // the raw diagnostics of javac, which the parsers that read it make
        // more of than this one would.
        QTest::newRow("diagnostic with a column")
            << QString("/src/Greeter.java:12:5: Missing a Javadoc comment") << Tasks();

        QTest::newRow("summary line") << QString("1 error") << Tasks();
        QTest::newRow("source line of a diagnostic")
            << QString("        return \"Hello from \" + nam;") << Tasks();
    }

    void test()
    {
        OutputParserTester testbench;
        testbench.setLineParsers({new JavaParser});
        QFETCH(QString, input);
        QFETCH(Tasks, tasks);

        testbench.testParsing(input,
                              OutputParserTester::STDOUT,
                              tasks,
                              tasks.isEmpty() ? QStringList(input) : QStringList(),
                              QStringList());
    }
};

QObject *createJavaParserTest()
{
    return new JavaParserTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace Java

#ifdef WITH_TESTS
#include <javaparser.moc>
#endif
