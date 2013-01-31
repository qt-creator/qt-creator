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

#include <filesearch.h>

#include <QtTest>

QT_BEGIN_NAMESPACE
bool operator==(const Utils::FileSearchResult &r1, const Utils::FileSearchResult &r2)
{
    return r1.fileName == r2.fileName
            && r1.lineNumber == r2.lineNumber
            && r1.matchingLine == r2.matchingLine
            && r1.matchStart == r2.matchStart
            && r1.matchLength == r2.matchLength
            && r1.regexpCapturedTexts == r2.regexpCapturedTexts;
}
QT_END_NAMESPACE

class tst_FileSearch : public QObject
{
    Q_OBJECT

private slots:
    void multipleResults();
    void caseSensitive();
    void caseInSensitive();
    void matchCaseReplacement();
};

namespace {
    const char * const FILENAME = ":/tst_filesearch/testfile.txt";

    void test_helper(const Utils::FileSearchResultList &expectedResults,
                     const QString &term,
                     QTextDocument::FindFlags flags)
    {
        Utils::FileIterator *it = new Utils::FileIterator(QStringList() << QLatin1String(FILENAME), QList<QTextCodec *>() << QTextCodec::codecForLocale());
        QFutureWatcher<Utils::FileSearchResultList> watcher;
        QSignalSpy ready(&watcher, SIGNAL(resultsReadyAt(int,int)));
        watcher.setFuture(Utils::findInFiles(term, it, flags));
        watcher.future().waitForFinished();
        QTest::qWait(100); // process events
        QCOMPARE(ready.count(), 1);
        Utils::FileSearchResultList results = watcher.resultAt(0);
        QCOMPARE(results.count(), expectedResults.count());
        for (int i = 0; i < expectedResults.size(); ++i) {
            QCOMPARE(results.at(i), expectedResults.at(i));
        }
    }
}

void tst_FileSearch::multipleResults()
{
    Utils::FileSearchResultList expectedResults;
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 2, QLatin1String("search to find multiple find results"), 10, 4, QStringList());
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 2, QLatin1String("search to find multiple find results"), 24, 4, QStringList());
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 4, QLatin1String("here you find another result"), 9, 4, QStringList());
    test_helper(expectedResults, QLatin1String("find"), QTextDocument::FindFlags(0));
}

void tst_FileSearch::caseSensitive()
{
    Utils::FileSearchResultList expectedResults;
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 3, QLatin1String("search CaseSensitively for casesensitive"), 7, 13, QStringList());
    test_helper(expectedResults, QLatin1String("CaseSensitive"), QTextDocument::FindCaseSensitively);
}

void tst_FileSearch::caseInSensitive()
{
    Utils::FileSearchResultList expectedResults;
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 3, QLatin1String("search CaseSensitively for casesensitive"), 7, 13, QStringList());
    expectedResults << Utils::FileSearchResult(QLatin1String(FILENAME), 3, QLatin1String("search CaseSensitively for casesensitive"), 27, 13, QStringList());
    test_helper(expectedResults, QLatin1String("CaseSensitive"), QTextDocument::FindFlags(0));
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

QTEST_MAIN(tst_FileSearch)

#include "tst_filesearch.moc"
