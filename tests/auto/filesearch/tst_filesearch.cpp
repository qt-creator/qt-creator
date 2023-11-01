// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filesearch.h>

#include <QTextCodec>
#include <QtTest>

using namespace Utils;

class tst_FileSearch : public QObject
{
    Q_OBJECT
private slots:
    void multipleResults();
    void caseSensitive();
    void caseInSensitive();
    void matchCaseReplacement();

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
                 const QString &term, Utils::FindFlags flags = {})
{
    const FileListContainer container({filePath}, {QTextCodec::codecForLocale()});
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

QTEST_GUILESS_MAIN(tst_FileSearch)

#include "tst_filesearch.moc"
