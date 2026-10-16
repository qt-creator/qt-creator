// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ripgrepengine_test.h"

#include "findinfilesripgrep.h"

#include <coreplugin/ioutputpane.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/filesearch.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace TextEditor;
using namespace Utils;

namespace Ripgrep::Internal {

// "file:begin line:column-end line:column|line text|captures", the files being in one directory
static QStringList hits(const SearchResultItems &items)
{
    QStringList result;
    for (const SearchResultItem &item : items) {
        const Text::Range range = item.mainRange();
        result << QString("%1:%2:%3-%4:%5|%6|%7")
                      .arg(item.filePath()->fileName().normalized(QString::NormalizationForm_C))
                      .arg(range.begin.line).arg(range.begin.column)
                      .arg(range.end.line).arg(range.end.column)
                      .arg(item.lineText(),
                           item.userData().toStringList().isEmpty() ? QString("-") : QString("captures"));
    }
    result.sort();
    return result;
}

static bool runsSuccessfully(const CommandLine &command)
{
    Process process;
    process.setCommand(command);
    process.setEnvironment(Environment::systemEnvironment());
    process.start();
    using namespace std::chrono_literals;
    return process.waitForFinished(5s) && process.result() == ProcessResult::FinishedWithSuccess;
}

void EngineTest::initTestCase()
{
    if (!runsSuccessfully({"rg", {"--version"}}))
        QSKIP("ripgrep is not available.");
    m_hasPcre2 = runsSuccessfully({"rg", {"--pcre2-version"}});
}

// The files to search, in \a dir
static void writeFiles(const QTemporaryDir &dir)
{
    const auto write = [&dir](const QString &name, const QByteArray &contents) {
        QFile file(dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    };
    write("code.cpp", "int a;\nint b;\nint c;\n");
    write("dots.txt", "a.b\naxb\n");
    write("parens.txt", "f(\n)\n");
    write("crlf.txt", "x\r\ny\r\nx\r\ny\r\n");
    write("FOO.CPP", "float e;\n");
    write("brackets.txt", "x?y\n");
    write("\u00fcn\u00ef.txt", QString("caf\u00e9 ab\n").toUtf8());
    write("lookaround.txt", "foobar\nfoobaz\n");
    QVERIFY(QDir(dir.path()).mkpath("src/build"));
    write("src/app.cpp", "float d;\n");
    write("src/build/inner.cpp", "float d;\n");
    QVERIFY(QDir(dir.path()).mkpath("src/sub"));
    write("src/sub/deep.cpp", "double g;\n");
}

static SearchResultItems search(const FileFindParameters &parameters,
                                const QString &searchOptions = {})
{
    const QFuture<SearchResultItems> future = findWithRipgrep(parameters, searchOptions);
    [&future] { QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 20000); }();
    SearchResultItems items;
    for (const SearchResultItems &chunk : future.results())
        items.append(chunk);
    return items;
}

void EngineTest::test_data()
{
    QTest::addColumn<QString>("term");
    QTest::addColumn<int>("flags");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<QStringList>("exclusionFilters");
    QTest::addColumn<QStringList>("expected");

    const int regex = FindRegularExpression;
    const int caseSensitive = FindCaseSensitively;
    const QStringList all = {"*"};

    QTest::newRow("plain-single-line") << "int b" << 0 << all << QStringList()
        << QStringList{"code.cpp:2:0-2:5|int b;|-"};
    QTest::newRow("plain-is-not-a-regex") << "a.b" << 0 << all << QStringList()
        << QStringList{"dots.txt:1:0-1:3|a.b|-"};
    QTest::newRow("plain-across-lines") << "a;\nint" << 0 << all << QStringList()
        << QStringList{"code.cpp:1:4-2:3|int a;\nint b;|-"};
    QTest::newRow("plain-across-lines-with-metacharacters") << "(\n)" << 0 << all << QStringList()
        << QStringList{"parens.txt:1:1-2:1|f(\n)|-"};
    QTest::newRow("regex-across-lines") << "(a);\\nint" << regex << all << QStringList{"dots.txt"}
        << QStringList{"code.cpp:1:4-2:3|int a;\nint b;|captures"};
    QTest::newRow("regex-with-a-crlf-across-lines") << "(a);\r\nint" << regex << all
        << QStringList{"dots.txt"}
        << QStringList{"code.cpp:1:4-2:3|int a;\nint b;|captures"};
    QTest::newRow("pcre2-look-ahead") << "foo(?=bar)" << regex << all << QStringList()
        << QStringList{"lookaround.txt:1:0-1:3|foobar|captures"};
    QTest::newRow("pcre2-look-behind") << "(?<=foo)ba(r)" << regex << all << QStringList()
        << QStringList{"lookaround.txt:1:3-1:6|foobar|captures"};
    QTest::newRow("pcre2-back-reference") << "(o)\\1" << regex << QStringList{"*.txt"}
        << QStringList()
        << QStringList{"lookaround.txt:1:1-1:3|foobar|captures",
                       "lookaround.txt:2:1-2:3|foobaz|captures"};
    QTest::newRow("regex-across-three-lines") << "a;\\n.*\\n.*c" << regex << all
        << QStringList{"dots.txt"}
        << QStringList{"code.cpp:1:4-3:5|int a;\nint b;\nint c;|captures"};
    QTest::newRow("case-insensitive") << "A;\nINT" << 0 << all << QStringList{"dots.txt"}
        << QStringList{"code.cpp:1:4-2:3|int a;\nint b;|-"};
    QTest::newRow("case-sensitive") << "A;\nINT" << caseSensitive << all << QStringList()
        << QStringList();
    QTest::newRow("whole-words") << "in" << int(FindWholeWords) << all << QStringList() << QStringList();
    QTest::newRow("crlf-in-the-pattern") << "x\\r\\ny" << regex << all << QStringList()
        << QStringList{"crlf.txt:1:0-2:1|x\ny|captures", "crlf.txt:3:0-4:1|x\ny|captures"};
    QTest::newRow("any-line-break") << "x\\Ry" << regex << all << QStringList()
        << QStringList{"crlf.txt:1:0-2:1|x\ny|captures", "crlf.txt:3:0-4:1|x\ny|captures"};
    QTest::newRow("plain-across-lines-in-a-crlf-file") << "x\ny" << 0 << all << QStringList()
        << QStringList{"crlf.txt:1:0-2:1|x\ny|-", "crlf.txt:3:0-4:1|x\ny|-"};
    QTest::newRow("regex-line-break-in-a-crlf-file") << "x\\ny" << regex << all << QStringList()
        << QStringList{"crlf.txt:1:0-2:1|x\ny|captures", "crlf.txt:3:0-4:1|x\ny|captures"};
    QTest::newRow("line-break-in-a-character-class") << "x[\\n]y" << regex << all << QStringList()
        << QStringList();
    QTest::newRow("line-break-in-a-character-class-and-a-literal-question-mark")
        << "x[\\r\\n]+y" << regex << all << QStringList()
        << QStringList{"crlf.txt:1:0-2:1|x\ny|captures", "crlf.txt:3:0-4:1|x\ny|captures"};
    QTest::newRow("default-exclusion-filter") << "float" << 0
        << QStringList{"*.cpp", "*.h"}
        << QStringList{"*/.git/*", "*/.cvs/*", "*/.svn/*", "*.autosave", "*/build/*"}
        << QStringList{"FOO.CPP:1:0-1:5|float e;|-", "app.cpp:1:0-1:5|float d;|-"};
    QTest::newRow("name-filter-with-a-star-between-slashes") << "double" << 0
        << QStringList{"*/src/*.cpp"} << QStringList()
        << QStringList{"deep.cpp:1:0-1:6|double g;|-"};
    QTest::newRow("name-filter-with-a-star-in-a-directory") << "double" << 0
        << QStringList{"*sub*"} << QStringList()
        << QStringList{"deep.cpp:1:0-1:6|double g;|-"};
    QTest::newRow("name-filter-with-a-star-after-a-directory-start") << "double" << 0
        << QStringList{"*/src*"} << QStringList()
        << QStringList{"deep.cpp:1:0-1:6|double g;|-"};
    QTest::newRow("name-filter-with-a-star-before-a-directory-end") << "double" << 0
        << QStringList{"*src/*"} << QStringList()
        << QStringList{"deep.cpp:1:0-1:6|double g;|-"};
    QTest::newRow("name-filter-ignores-case") << "float" << 0 << QStringList{"*.CPP"} << QStringList()
        << QStringList{"FOO.CPP:1:0-1:5|float e;|-", "app.cpp:1:0-1:5|float d;|-",
                       "inner.cpp:1:0-1:5|float d;|-"};
    QTest::newRow("non-ascii") << "ab" << 0 << all << QStringList()
        << QStringList{"\u00fcn\u00ef.txt:1:5-1:7|caf\u00e9 ab|-"};
    QTest::newRow("name-filter") << "int" << 0 << QStringList{"*.cpp"} << QStringList()
        << QStringList{"code.cpp:1:0-1:3|int a;|-", "code.cpp:2:0-2:3|int b;|-",
                       "code.cpp:3:0-3:3|int c;|-"};
    QTest::newRow("exclusion-filter") << "int" << 0 << all << QStringList{"*.cpp"}
        << QStringList();
    QTest::newRow("exclusion-filter-matching-a-directory-name") << "float" << 0 << all
        << QStringList{"src*"}
        << QStringList{"FOO.CPP:1:0-1:5|float e;|-", "app.cpp:1:0-1:5|float d;|-",
                       "inner.cpp:1:0-1:5|float d;|-"};
    QTest::newRow("exclusion-filter-matching-a-directory-in-the-path") << "float" << 0 << all
        << QStringList{"*src*"} << QStringList{"FOO.CPP:1:0-1:5|float e;|-"};
}

void EngineTest::test()
{
    QFETCH(QString, term);
    QFETCH(int, flags);
    QFETCH(QStringList, nameFilters);
    QFETCH(QStringList, exclusionFilters);
    QFETCH(QStringList, expected);

    if (QByteArray(QTest::currentDataTag()).startsWith("pcre2-") && !m_hasPcre2)
        QSKIP("ripgrep was built without PCRE2.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFiles(dir);
    if (QTest::currentTestFailed())
        return;

    FileFindParameters parameters;
    parameters.text = term;
    parameters.flags = FindFlags::fromInt(flags);
    parameters.nameFilters = nameFilters;
    parameters.exclusionFilters = exclusionFilters;
    parameters.searchDir = FilePath::fromString(dir.path());

    const SearchResultItems items = search(parameters);
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(hits(items), expected);
}

void EngineTest::testSearchOptions_data()
{
    QTest::addColumn<QString>("searchOptions");
    QTest::addColumn<QStringList>("expected");

    const QStringList allHits = {"FOO.CPP:1:0-1:5|float e;|-", "app.cpp:1:0-1:5|float d;|-",
                                 "inner.cpp:1:0-1:5|float d;|-"};
    QTest::newRow("none") << QString() << allHits;
    QTest::newRow("glob") << "-g !*.cpp" << QStringList{"FOO.CPP:1:0-1:5|float e;|-"};
    QTest::newRow("two-spaces") << "-g  !*.cpp" << QStringList{"FOO.CPP:1:0-1:5|float e;|-"};
    QTest::newRow("quoted-glob") << "-g \"!*.cpp\"" << QStringList{"FOO.CPP:1:0-1:5|float e;|-"};
    QTest::newRow("quoted-glob-with-a-space") << "-g \"!{*.cpp,a b}\""
        << QStringList{"FOO.CPP:1:0-1:5|float e;|-"};
}

void EngineTest::testSearchOptions()
{
    QFETCH(QString, searchOptions);
    QFETCH(QStringList, expected);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFiles(dir);
    if (QTest::currentTestFailed())
        return;

    FileFindParameters parameters;
    parameters.text = "float"; // No name filters, which would be globs that win over the options
    parameters.searchDir = FilePath::fromString(dir.path());

    const SearchResultItems items = search(parameters, searchOptions);
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(hits(items), expected);
}

static Core::IOutputPane *generalMessagesPane()
{
    return Utils::findOrDefault(Core::IOutputPane::allOutputPanes(), [](Core::IOutputPane *pane) {
        return pane->id() == "GeneralMessages";
    });
}

void EngineTest::testErrorIsReported_data()
{
    QTest::addColumn<QString>("searchOptions");
    QTest::addColumn<QString>("reported");

    QTest::newRow("unknown-option") << "--no-such-option" << "no-such-option";
    // Searching without the glob would find the files that it leaves out
    QTest::newRow("unclosed-quote") << "-g \"!*.cpp" << "-g \"!*.cpp";
}

void EngineTest::testErrorIsReported()
{
    QFETCH(QString, searchOptions);
    QFETCH(QString, reported);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFiles(dir);
    if (QTest::currentTestFailed())
        return;

    Core::IOutputPane *pane = generalMessagesPane();
    QVERIFY(pane);
    QSignalSpy flashed(pane, &Core::IOutputPane::flashButton);
    QStringList messages;
    QObject guard;
    Core::MessageManager::addObserver(&guard, [&messages](const QString &message) {
        messages << message;
    });

    FileFindParameters parameters;
    parameters.text = "float"; // No name filters, which would be globs that win over the options
    parameters.searchDir = FilePath::fromString(dir.path());

    // A search that finds nothing is no error
    QCOMPARE(search(parameters, "-g !*"), SearchResultItems());
    QCoreApplication::processEvents();
    QCOMPARE(messages, QStringList());

    QCOMPARE(search(parameters, searchOptions), SearchResultItems());
    QTRY_VERIFY(Utils::anyOf(messages, [&reported](const QString &message) {
        return message.contains("Ripgrep") && message.contains(reported);
    }));
    QVERIFY(flashed.count() > 0);
}

// ripgrep exits with an error also if it could not read some of the files, but the results of
// the others are complete.
void EngineTest::testUnreadableDirectory()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The permissions of a directory cannot be taken away the same way on Windows.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFiles(dir);
    if (QTest::currentTestFailed())
        return;
    const QString locked = dir.filePath("locked");
    QVERIFY(QDir(dir.path()).mkpath("locked"));
    QFile hidden(locked + "/hidden.cpp");
    QVERIFY(hidden.open(QIODevice::WriteOnly));
    hidden.write("double h;\n");
    hidden.close();
    const QFile::Permissions permissions = QFile::permissions(locked);
    QVERIFY(QFile::setPermissions(locked, {}));
    const auto restore = qScopeGuard([&] { QFile::setPermissions(locked, permissions); });
    if (QFile(hidden.fileName()).open(QIODevice::ReadOnly))
        QSKIP("The permissions of the directory are not enforced, like for root.");

    Core::IOutputPane *pane = generalMessagesPane();
    QVERIFY(pane);
    QSignalSpy flashed(pane, &Core::IOutputPane::flashButton);
    QStringList messages;
    QObject guard;
    Core::MessageManager::addObserver(&guard, [&messages](const QString &message) {
        messages << message;
    });

    FileFindParameters parameters;
    parameters.text = "double";
    parameters.searchDir = FilePath::fromString(dir.path());
    QCOMPARE(hits(search(parameters)), QStringList{"deep.cpp:1:0-1:6|double g;|-"});

    // The pane flashes, if at all, before the message reaches the observers
    QTRY_VERIFY(Utils::anyOf(messages, [](const QString &message) {
        return message.contains("Ripgrep") && message.contains("locked");
    }));
    QCOMPARE(flashed.count(), 0);
}

void EngineTest::testEngineIsEnabled()
{
    // The check whether rg runs does not block the start-up, the engine is enabled afterwards
    const SearchEngine *engine = ripgrepSearchEngine();
    QVERIFY(engine);
    QTRY_VERIFY(engine->isEnabled());
}

} // namespace Ripgrep::Internal
