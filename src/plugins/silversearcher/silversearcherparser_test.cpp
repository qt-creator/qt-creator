// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "silversearcherparser.h"
#include "silversearcherparser_test.h"

#include <QtTest>

using namespace Utils;

namespace SilverSearcher {
namespace Internal {

SearchResultItem searchResult(const FilePath &fileName, const QString &matchingLine,
                              int lineNumber, int matchStart, int matchLength)
{
    SearchResultItem result;
    result.setFilePath(fileName);
    result.setLineText(matchingLine);
    result.setMainRange(lineNumber, matchStart, matchLength);
    result.setUseTextEditorFont(true);
    return result;
}

void OutputParserTest::test_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<SearchResultItems>("results");

    QTest::addRow("nothing") << QString("\n") << SearchResultItems();
    QTest::addRow("oneFileOneMatch")
        << QString(":/file/path/to/filename.h\n"
                   "1;1 5:match\n")
        << SearchResultItems{searchResult("/file/path/to/filename.h", "match", 1, 1, 5)};
    QTest::addRow("multipleFilesWithOneMatch")
        << QString(":/file/path/to/filename1.h\n"
                   "1;1 5:match\n"
                   "\n"
                   ":/file/path/to/filename2.h\n"
                   "2;2 5: match\n")
        << SearchResultItems{searchResult("/file/path/to/filename1.h", "match", 1, 1, 5),
                             searchResult("/file/path/to/filename2.h", " match", 2, 2, 5)};
    QTest::addRow("oneFileMultipleMatches")
        << QString(":/file/path/to/filename.h\n"
                   "1;1 5,7 5:match match\n")
        << SearchResultItems{searchResult("/file/path/to/filename.h", "match match", 1, 1, 5),
                             searchResult("/file/path/to/filename.h", "match match", 1, 7, 5)};
    QTest::addRow("multipleFilesWithMultipleMatches")
        << QString(":/file/path/to/filename1.h\n"
                   "1;1 5,7 5:match match\n"
                   "\n"
                   ":/file/path/to/filename2.h\n"
                   "2;2 5,8 5: match match\n")
        << SearchResultItems{searchResult("/file/path/to/filename1.h", "match match", 1, 1, 5),
                             searchResult("/file/path/to/filename1.h", "match match", 1, 7, 5),
                             searchResult("/file/path/to/filename2.h", " match match", 2, 2, 5),
                             searchResult("/file/path/to/filename2.h", " match match", 2, 8, 5)};
}

void OutputParserTest::test()
{
    QFETCH(QString, input);
    QFETCH(SearchResultItems, results);
    const SearchResultItems items = SilverSearcher::parse(input);
    QCOMPARE(items, results);
}

} // namespace Internal
} // namespace SilverSearcher
