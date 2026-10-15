// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filesearch.h>

#include <QFutureWatcher>
#include <QSignalSpy>
#include <QTest>

using namespace Utils;

class tst_FileSearch : public QObject
{
    Q_OBJECT
private slots:
    void multipleResults();
    void caseSensitive();
    void caseInSensitive();
    void wholeWords();
    void wholeWords_data();
    void emptyTerm();
    void matchCaseReplacement();
    void acrossLines();
    void acrossLines_data();
    void acrossLinesCapturedTexts();

private:
    const FilePath m_filePath = FilePath::fromString(":/tst_filesearch/testfile.txt");
};

SearchResultItem searchResult(const FilePath &fileName, const QString &matchingLine,
                              int lineNumber, int matchStart, int matchLength,
                              const QStringList &regexpCapturedTexts = {})
{
    SearchResultItem result;
    result.setFilePath(fileName);
    result.setLineText(matchingLine);
    result.setMainRange(lineNumber, matchStart, matchLength);
    result.setUserData(regexpCapturedTexts);
    result.setUseTextEditorFont(true);
    return result;
}

void test_helper(const FilePath &filePath, const SearchResultItems &expectedResults,
                 const QString &term, FindFlags flags = {})
{
    const FileListContainer container({filePath}, {TextEncoding::encodingForLocale()});
    QFutureWatcher<SearchResultItems> watcher;
    QSignalSpy ready(&watcher, &QFutureWatcherBase::resultsReadyAt);
    watcher.setFuture(Utils::findInFiles(term, container, flags, {}));
    watcher.future().waitForFinished();
    QTest::qWait(100); // process events
    QCOMPARE(ready.count(), 1);
    SearchResultItems results = watcher.resultAt(0);
    QCOMPARE(results.count(), expectedResults.count());
    for (int i = 0; i < expectedResults.size(); ++i)
        QCOMPARE(results.at(i), expectedResults.at(i));
}

// One entry per hit, as "line:column:length".
static QStringList searchHits(const QString &term, FindFlags flags, const QString &contents)
{
    QPromise<SearchResultItems> promise;
    const QFuture<SearchResultItems> future = promise.future();
    promise.start();
    Utils::searchInContents(promise, term, flags, FilePath::fromString("test.txt"), contents);
    promise.finish();

    QStringList hits;
    for (const SearchResultItems &items : future.results()) {
        for (const SearchResultItem &item : items) {
            const Text::Range range = item.mainRange();
            hits << QString("%1:%2:%3").arg(range.begin.line).arg(range.begin.column)
                        .arg(range.length(contents));
        }
    }
    return hits;
}

struct Hit
{
    Text::Position begin;
    Text::Position end;
    QString lineText;
};
Q_DECLARE_METATYPE(QList<Hit>)

static QList<Hit> searchAcross(const QString &term, FindFlags flags, const QString &contents)
{
    QPromise<SearchResultItems> promise;
    const QFuture<SearchResultItems> future = promise.future();
    promise.start();
    Utils::searchInContents(promise, term, flags, FilePath::fromString("test.txt"), contents);
    promise.finish();

    QList<Hit> hits;
    for (const SearchResultItems &items : future.results()) {
        for (const SearchResultItem &item : items)
            hits.append({item.mainRange().begin, item.mainRange().end, item.lineText()});
    }
    return hits;
}

void tst_FileSearch::multipleResults()
{
    SearchResultItems expectedResults;
    expectedResults << searchResult(m_filePath, "search to find multiple find results", 2, 10, 4);
    expectedResults << searchResult(m_filePath, "search to find multiple find results", 2, 24, 4);
    expectedResults << searchResult(m_filePath, "here you find another result", 4, 9, 4);
    test_helper(m_filePath, expectedResults, "find");

    expectedResults.clear();
    expectedResults << searchResult(m_filePath,
                                    "aaaaaaaa this line has 2 results for four a in a row",
                                    5, 0, 4);
    expectedResults << searchResult(m_filePath,
                                    "aaaaaaaa this line has 2 results for four a in a row",
                                    5, 4, 4);
    test_helper(m_filePath, expectedResults, "aaaa");

    expectedResults.clear();
    expectedResults << searchResult(m_filePath,
                                    "aaaaaaaa this line has 2 results for four a in a row",
                                    5, 0, 4, {"aaaa"});
    expectedResults << searchResult(m_filePath,
                                    "aaaaaaaa this line has 2 results for four a in a row",
                                    5, 4, 4, {"aaaa"});
    test_helper(m_filePath, expectedResults, "aaaa", FindRegularExpression);
}

void tst_FileSearch::caseSensitive()
{
    SearchResultItems expectedResults;
    expectedResults << searchResult(m_filePath, "search CaseSensitively for casesensitive",
                                    3, 7, 13);
    test_helper(m_filePath, expectedResults, "CaseSensitive", FindCaseSensitively);
}

void tst_FileSearch::caseInSensitive()
{
    SearchResultItems expectedResults;
    expectedResults << searchResult(m_filePath, "search CaseSensitively for casesensitive", 3, 7, 13);
    expectedResults << searchResult(m_filePath, "search CaseSensitively for casesensitive", 3, 27, 13);
    test_helper(m_filePath, expectedResults, "CaseSensitive");
}

void tst_FileSearch::wholeWords()
{
    QFETCH(QString, contents);
    QFETCH(QString, term);
    QFETCH(QString, regExpTerm);
    QFETCH(QStringList, expected);

    const FindFlags flags = FindWholeWords | FindCaseSensitively;
    QCOMPARE(searchHits(term, flags, contents), expected);
    // Both search implementations must answer the same query alike.
    QCOMPARE(searchHits(regExpTerm, flags | FindRegularExpression, contents), expected);
}

void tst_FileSearch::wholeWords_data()
{
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("term");
    QTest::addColumn<QString>("regExpTerm");
    QTest::addColumn<QStringList>("expected");

    // A term made of word characters keeps requiring a separator on both sides.
    QTest::newRow("word alone") << "foo bar" << "foo" << "foo" << QStringList{"1:0:3"};
    QTest::newRow("word glued after") << "foobar" << "foo" << "foo" << QStringList();
    QTest::newRow("word glued before") << "xfoo" << "foo" << "foo" << QStringList();
    QTest::newRow("word between punctuation") << "x-foo-y" << "foo" << "foo"
                                              << QStringList{"1:2:3"};
    QTest::newRow("two lines") << "foo\nfoo" << "foo" << "foo" << QStringList{"1:0:3", "2:0:3"};

    // A term starting with a non-word character is its own separator at the front.
    QTest::newRow("hash at line start") << "#include <a>" << "#include" << "\\#include"
                                        << QStringList{"1:0:8"};
    QTest::newRow("hash after word char") << "x#include <a>" << "#include" << "\\#include"
                                          << QStringList{"1:1:8"};
    QTest::newRow("underscore is a word char") << "x_foo" << "_foo" << "_foo" << QStringList();

    // A term ending with a non-word character is its own separator at the back.
    QTest::newRow("paren at term end") << "foo(1)" << "foo(" << "foo\\(" << QStringList{"1:0:4"};
    QTest::newRow("paren before non-word char") << "foo()" << "foo(" << "foo\\("
                                                << QStringList{"1:0:4"};
    QTest::newRow("paren after word char") << "xfoo(1)" << "foo(" << "foo\\(" << QStringList();
    QTest::newRow("plus at term end") << "c++" << "c++" << "c\\+\\+" << QStringList{"1:0:3"};
    QTest::newRow("plus before word char") << "c++x" << "c++" << "c\\+\\+"
                                           << QStringList{"1:0:3"};
    QTest::newRow("plus after word char") << "ac++" << "c++" << "c\\+\\+" << QStringList();
    QTest::newRow("non-word term only") << "a::b" << "::" << "::" << QStringList{"1:1:2"};
}

void tst_FileSearch::emptyTerm()
{
    QCOMPARE(searchHits({}, FindWholeWords, "foo bar"), QStringList());
    QCOMPARE(searchHits({}, FindWholeWords, {}), QStringList());
    QCOMPARE(searchHits({}, FindWholeWords, "\n\n"), QStringList());
}

void tst_FileSearch::matchCaseReplacement()
{
    QCOMPARE(Utils::matchCaseReplacement("", "foobar"), QString("foobar"));          //empty string

    QCOMPARE(Utils::matchCaseReplacement("testpad", "foobar"), QString("foobar"));   //lower case
    QCOMPARE(Utils::matchCaseReplacement("TESTPAD", "foobar"), QString("FOOBAR"));   //upper case
    QCOMPARE(Utils::matchCaseReplacement("Testpad", "foobar"), QString("Foobar"));   //capitalized
    QCOMPARE(Utils::matchCaseReplacement("tESTPAD", "foobar"), QString("fOOBAR"));   //un-capitalized
    QCOMPARE(Utils::matchCaseReplacement("tEsTpAd", "foobar"), QString("foobar"));   //mixed case, use replacement as specified
    QCOMPARE(Utils::matchCaseReplacement("TeStPaD", "foobar"), QString("foobar"));   //mixed case, use replacement as specified

    QCOMPARE(Utils::matchCaseReplacement("testpad", "fooBar"), QString("foobar"));   //lower case
    QCOMPARE(Utils::matchCaseReplacement("TESTPAD", "fooBar"), QString("FOOBAR"));   //upper case
    QCOMPARE(Utils::matchCaseReplacement("Testpad", "fooBar"), QString("Foobar"));   //capitalized
    QCOMPARE(Utils::matchCaseReplacement("tESTPAD", "fooBar"), QString("fOOBAR"));   //un-capitalized
    QCOMPARE(Utils::matchCaseReplacement("tEsTpAd", "fooBar"), QString("fooBar"));   //mixed case, use replacement as specified
    QCOMPARE(Utils::matchCaseReplacement("TeStPaD", "fooBar"), QString("fooBar"));   //mixed case, use replacement as specified

    //with common prefix
    QCOMPARE(Utils::matchCaseReplacement("pReFiXtestpad", "prefixfoobar"), QString("pReFiXfoobar"));   //lower case
    QCOMPARE(Utils::matchCaseReplacement("pReFiXTESTPAD", "prefixfoobar"), QString("pReFiXFOOBAR"));   //upper case
    QCOMPARE(Utils::matchCaseReplacement("pReFiXTestpad", "prefixfoobar"), QString("pReFiXFoobar"));   //capitalized
    QCOMPARE(Utils::matchCaseReplacement("pReFiXtESTPAD", "prefixfoobar"), QString("pReFiXfOOBAR"));   //un-capitalized
    QCOMPARE(Utils::matchCaseReplacement("pReFiXtEsTpAd", "prefixfoobar"), QString("pReFiXfoobar"));   //mixed case, use replacement as specified
    QCOMPARE(Utils::matchCaseReplacement("pReFiXTeStPaD", "prefixfoobar"), QString("pReFiXfoobar"));   //mixed case, use replacement as specified

    //with common suffix
    QCOMPARE(Utils::matchCaseReplacement("testpadSuFfIx", "foobarsuffix"), QString("foobarSuFfIx"));   //lower case
    QCOMPARE(Utils::matchCaseReplacement("TESTPADSuFfIx", "foobarsuffix"), QString("FOOBARSuFfIx"));   //upper case
    QCOMPARE(Utils::matchCaseReplacement("TestpadSuFfIx", "foobarsuffix"), QString("FoobarSuFfIx"));   //capitalized
    QCOMPARE(Utils::matchCaseReplacement("tESTPADSuFfIx", "foobarsuffix"), QString("fOOBARSuFfIx"));   //un-capitalized
    QCOMPARE(Utils::matchCaseReplacement("tEsTpAdSuFfIx", "foobarsuffix"), QString("foobarSuFfIx"));   //mixed case, use replacement as specified
    QCOMPARE(Utils::matchCaseReplacement("TeStPaDSuFfIx", "foobarsuffix"), QString("foobarSuFfIx"));   //mixed case, use replacement as specified

    //with common prefix and suffix
    QCOMPARE(Utils::matchCaseReplacement("pReFiXtestpadSuFfIx", "prefixfoobarsuffix"), QString("pReFiXfoobarSuFfIx"));   //lower case
    QCOMPARE(Utils::matchCaseReplacement("pReFiXTESTPADSuFfIx", "prefixfoobarsuffix"), QString("pReFiXFOOBARSuFfIx"));   //upper case
    QCOMPARE(Utils::matchCaseReplacement("pReFiXTestpadSuFfIx", "prefixfoobarsuffix"), QString("pReFiXFoobarSuFfIx"));   //capitalized
    QCOMPARE(Utils::matchCaseReplacement("pReFiXtESTPADSuFfIx", "prefixfoobarsuffix"), QString("pReFiXfOOBARSuFfIx"));   //un-capitalized
    QCOMPARE(Utils::matchCaseReplacement("pReFiXtEsTpAdSuFfIx", "prefixfoobarsuffix"), QString("pReFiXfoobarSuFfIx"));   //mixed case, use replacement as specified
    QCOMPARE(Utils::matchCaseReplacement("pReFiXTeStPaDSuFfIx", "prefixfoobarsuffix"), QString("pReFiXfoobarSuFfIx"));   //mixed case, use replacement as specified
}

void tst_FileSearch::acrossLines_data()
{
    QTest::addColumn<QString>("term");
    QTest::addColumn<int>("flags");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QList<Hit>>("expected");

    const int regex = FindRegularExpression;
    const QString text = "int a;\nint b;\nint c;\n";

    QTest::newRow("regex") << "a;\\nint b" << regex << text
        << QList<Hit>{{{1, 4}, {2, 5}, "int a;\nint b;"}};
    QTest::newRow("regex-crlf-in-pattern") << "a;\\r\\nint b" << regex << text
        << QList<Hit>{{{1, 4}, {2, 5}, "int a;\nint b;"}};
    QTest::newRow("regex-crlf-in-file") << "a;\\nint b" << regex
        << QString("int a;\r\nint b;\r\nint c;\r\n")
        << QList<Hit>{{{1, 4}, {2, 5}, "int a;\nint b;"}};
    QTest::newRow("three-lines") << "a;\\n.*\\n.*c" << regex << text
        << QList<Hit>{{{1, 4}, {3, 5}, "int a;\nint b;\nint c;"}};
    QTest::newRow("plain-literal-newline") << "a;\nint" << 0 << text
        << QList<Hit>{{{1, 4}, {2, 3}, "int a;\nint b;"}};
    QTest::newRow("plain-is-not-a-regex") << "a.\nint" << 0 << text << QList<Hit>{};
    QTest::newRow("case-insensitive") << "A;\\nINT" << regex << text
        << QList<Hit>{{{1, 4}, {2, 3}, "int a;\nint b;"}};
    QTest::newRow("case-sensitive") << "A;\\nint" << (regex | FindCaseSensitively) << text
        << QList<Hit>{};
    QTest::newRow("several") << "x\\ny" << regex << "x\ny\nx\ny\nxy"
        << QList<Hit>{{{1, 0}, {2, 1}, "x\ny"}, {{3, 0}, {4, 1}, "x\ny"}};
    QTest::newRow("whole-words-match") << "b;\\nint" << (regex | FindWholeWords) << text
        << QList<Hit>{{{2, 4}, {3, 3}, "int b;\nint c;"}};
    QTest::newRow("whole-words-reject") << "t b;\\nin" << (regex | FindWholeWords) << text
        << QList<Hit>{};
    QTest::newRow("line-anchors") << "^int b;\\n^int" << regex << text
        << QList<Hit>{{{2, 0}, {3, 3}, "int b;\nint c;"}};
    QTest::newRow("match-ends-after-newline") << "c;\\n" << regex << text
        << QList<Hit>{{{3, 4}, {4, 0}, "int c;\n"}};
}

void tst_FileSearch::acrossLines()
{
    QFETCH(QString, term);
    QFETCH(int, flags);
    QFETCH(QString, contents);
    QFETCH(QList<Hit>, expected);

    const QList<Hit> hits = searchAcross(term, FindFlags::fromInt(flags), contents);
    QCOMPARE(hits.size(), expected.size());
    for (int i = 0; i < expected.size(); ++i) {
        QCOMPARE(hits.at(i).begin.line, expected.at(i).begin.line);
        QCOMPARE(hits.at(i).begin.column, expected.at(i).begin.column);
        QCOMPARE(hits.at(i).end.line, expected.at(i).end.line);
        QCOMPARE(hits.at(i).end.column, expected.at(i).end.column);
        QCOMPARE(hits.at(i).lineText, expected.at(i).lineText);
    }
}

void tst_FileSearch::acrossLinesCapturedTexts()
{
    const auto userData = [](const QString &term, FindFlags flags) {
        QPromise<SearchResultItems> promise;
        const QFuture<SearchResultItems> future = promise.future();
        promise.start();
        Utils::searchInContents(promise, term, flags, FilePath::fromString("test.txt"),
                                "int a;\nint b;\n");
        promise.finish();
        QList<QStringList> data;
        for (const SearchResultItems &items : future.results()) {
            for (const SearchResultItem &item : items)
                data.append(item.userData().toStringList());
        }
        return data;
    };
    // The replacement of a search result expands the captures of a regular expression, but only
    // that: a plain text search leaves them empty, as the one for a single line does.
    QCOMPARE(userData("a;\nint", {}), (QList<QStringList>{QStringList()}));
    QCOMPARE(userData("(a);\nint", FindRegularExpression),
             (QList<QStringList>{QStringList{"a;\nint", "a"}}));
}

QTEST_GUILESS_MAIN(tst_FileSearch)

#include "tst_filesearch.moc"
