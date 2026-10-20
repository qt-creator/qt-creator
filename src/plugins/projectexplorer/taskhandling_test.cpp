// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "taskhandling_test.h"

#include "task.h"
#include "taskfile.h"
#include "taskmodel.h"

#include <utils/hostosinfo.h>
#include <utils/id.h>
#include <utils/link.h>
#include <utils/outputformatter.h>

#include <QTest>

namespace ProjectExplorer::Internal {

// Covers the task counting, filtering and .tasks file parsing that the former
// Squish system test suite_general/tst_tasks_handling exercised through the
// Issues pane, as well as linkified task details.
class TaskHandlingTest final : public QObject
{
    Q_OBJECT

private slots:
    void testTaskFiltering();
    void testTaskFileParsing_data();
    void testTaskFileParsing();
    void testTaskModelPopulationBenchmark();
    void testLinkedDetails_data();
    void testLinkedDetails();
};

void TaskHandlingTest::testTaskFiltering()
{
    const Utils::Id category = "Test.TaskModel";
    TaskModel model(nullptr);
    model.addCategory({category, "Test", "Test category", true, 0});

    const auto addTasks = [&model, category](Task::TaskType type, int count) {
        for (int i = 0; i < count; ++i)
            model.addTask(Task(type, "task", {}, -1, category));
    };
    addTasks(Task::Error, 3);
    addTasks(Task::Warning, 2);
    addTasks(Task::Unknown, 1);

    QCOMPARE(model.rowCount(), 6);
    QCOMPARE(model.taskCount(category), 6);
    QCOMPARE(model.errorTaskCount(category), 3);
    QCOMPARE(model.warningTaskCount(category), 2);

    TaskFilterModel filter(&model);
    QCOMPARE(filter.rowCount(), 6);

    // "Show Warnings" off hides warnings and unknowns (unknowns are grouped with
    // warnings), so only the errors remain.
    filter.setFilterIncludesWarnings(false);
    QCOMPARE(filter.rowCount(), 3);
    filter.setFilterIncludesWarnings(true);
    QCOMPARE(filter.rowCount(), 6);

    // Hiding errors leaves warnings and unknowns.
    filter.setFilterIncludesErrors(false);
    QCOMPARE(filter.rowCount(), 3);
    filter.setFilterIncludesErrors(true);
    QCOMPARE(filter.rowCount(), 6);

    // "My Tasks" off: excluding the category hides all of its tasks.
    filter.setFilteredCategories({category});
    QCOMPARE(filter.rowCount(), 0);
    filter.setFilteredCategories({});
    QCOMPARE(filter.rowCount(), 6);

    // Combination: excluding warnings and the category hides everything.
    filter.setFilterIncludesWarnings(false);
    filter.setFilteredCategories({category});
    QCOMPARE(filter.rowCount(), 0);
}

void TaskHandlingTest::testTaskFileParsing_data()
{
    QTest::addColumn<QByteArray>("line");
    QTest::addColumn<bool>("valid"); // whether a task is produced at all
    QTest::addColumn<int>("type"); // Task::TaskType
    QTest::addColumn<int>("lineNumber");
    QTest::addColumn<QString>("description");

    QTest::newRow("comment") << QByteArray("# a comment") << false << 0 << -1 << QString();
    QTest::newRow("description only")
        << QByteArray("just text") << true << int(Task::Unknown) << -1 << QString("just text");
    QTest::newRow("type and description")
        << QByteArray("warn\ta warning") << true << int(Task::Warning) << -1
        << QString("a warning");
    QTest::newRow("error type")
        << QByteArray("err\tan error") << true << int(Task::Error) << -1 << QString("an error");
    QTest::newRow("unknown type")
        << QByteArray("nonsense\tsome text") << true << int(Task::Unknown) << -1
        << QString("some text");
    QTest::newRow("file, type and description")
        << QByteArray("/path/file.cpp\terror\toops") << true << int(Task::Error) << -1
        << QString("oops");
    QTest::newRow("file, line, type and description")
        << QByteArray("/path/file.cpp\t42\twarn\tcareful") << true << int(Task::Warning) << 42
        << QString("careful");
    QTest::newRow("invalid line number")
        << QByteArray("/path/file.cpp\tNaN\terr\tboom") << true << int(Task::Error) << -1
        << QString("boom");
    QTest::newRow("escaped newline and tab")
        << QByteArray("err\tfirst\\nsecond\\tthird") << true << int(Task::Error) << -1
        << QString("first\nsecond\tthird");
}

void TaskHandlingTest::testTaskFileParsing()
{
    QFETCH(QByteArray, line);
    QFETCH(bool, valid);
    QFETCH(int, type);
    QFETCH(int, lineNumber);
    QFETCH(QString, description);

    const std::optional<Task> task = parseTaskFileLine(line, Utils::FilePath::fromString("/tmp"));
    QCOMPARE(task.has_value(), valid);
    if (!valid)
        return;

    QCOMPARE(int(task->type()), type);
    QCOMPARE(task->line(), lineNumber);
    QCOMPARE(task->description(), description);
}

void TaskHandlingTest::testTaskModelPopulationBenchmark()
{
    const Utils::Id category = "Test.TaskModel";
    QBENCHMARK {
        TaskModel model(nullptr);
        model.addCategory({category, "Test", "Test category", true, 0});
        for (int i = 0; i < 1100; ++i)
            model.addTask(Task(Task::Warning, "issue", {}, i, category));
    }
}

void TaskHandlingTest::testLinkedDetails_data()
{
    using namespace Utils;

    QTest::addColumn<Link>("location");
    QTest::addColumn<QString>("linkText"); // empty if no link is expected
    QTest::addColumn<int>("expectedColumn"); // as seen when following the link

    const FilePath localFile = FilePath::fromString(
        HostOsInfo::isWindowsHost() ? QString("C:/dir/main.cpp") : QString("/dir/main.cpp"));
    const FilePath remoteFile = FilePath::fromString("docker://someid/dir/main.cpp");

    QTest::newRow("no location") << Link() << QString() << 0;
    QTest::newRow("line, no column") << Link(localFile, 12, -1) << QString("main.cpp:12") << -1;
    QTest::newRow("line and first column")
        << Link(localFile, 12, 0) << QString("main.cpp:12:1") << 0;
    QTest::newRow("line and column") << Link(localFile, 12, 4) << QString("main.cpp:12:5") << 4;
    QTest::newRow("no line") << Link(localFile, 0, -1) << QString("main.cpp") << -1;
    QTest::newRow("non-local file")
        << Link(remoteFile, 12, 4) << QString("main.cpp:12:5") << 4;
}

void TaskHandlingTest::testLinkedDetails()
{
    QFETCH(Utils::Link, location);
    QFETCH(QString, linkText);
    QFETCH(int, expectedColumn);

    // The existing detail line makes sure the link range accounts for preceding text.
    Task task(Task::Error, "summary\nplain detail", {}, -1, {});
    task.addToDetails("text", location);
    if (linkText.isEmpty()) {
        QCOMPARE(task.details(), QStringList({"plain detail", "text"}));
        QVERIFY(task.formats().isEmpty());
        return;
    }

    // The link covers the file name and position, but not the text that follows them.
    QCOMPARE(task.details(), QStringList({"plain detail", linkText + ": text"}));
    QCOMPARE(task.formats().size(), 1);
    const QTextLayout::FormatRange &range = task.formats().first();
    QVERIFY(range.format.isAnchor());
    QCOMPARE(task.description().mid(range.start, range.length), linkText);

    // The link must lead back to the location when clicked.
    const QString href = range.format.anchorHref();
    QVERIFY(Utils::OutputLineParser::isLinkTarget(href));
    const Utils::Link target = Utils::OutputLineParser::parseLinkTarget(href);
    QCOMPARE(target.targetFilePath, location.targetFilePath);
    QCOMPARE(target.target.line, location.target.line);
    QCOMPARE(target.target.column, expectedColumn);

    QVERIFY(task.formattedDescription({Task::WithSummary, Task::WithLinks})
                .contains(QString("summary\nplain detail\n<a href=\"%1\">%2</a>: text")
                              .arg(href.toHtmlEscaped(), linkText)));
}

QObject *createTaskHandlingTest()
{
    return new TaskHandlingTest;
}

} // namespace ProjectExplorer::Internal

#include "taskhandling_test.moc"

#endif // WITH_TESTS
