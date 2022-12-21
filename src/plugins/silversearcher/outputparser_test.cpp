// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputparser_test.h"
#include "silversearcheroutputparser.h"

#include <QtTest>

using namespace Utils;

namespace SilverSearcher {
namespace Internal {

void OutputParserTest::test_data()
{
    QTest::addColumn<QString>("parserOutput");
    QTest::addColumn<FileSearchResultList>("results");

    QTest::addRow("nothing") << QString("\n") << FileSearchResultList();
    QTest::addRow("oneFileOneMatch")
            << QString(":/file/path/to/filename.h\n"
                       "1;1 5:match\n")
            << FileSearchResultList({{"/file/path/to/filename.h", 1, "match", 1, 5, {}}});
    QTest::addRow("multipleFilesWithOneMatch")
            << QString(":/file/path/to/filename1.h\n"
                       "1;1 5:match\n"
                       "\n"
                       ":/file/path/to/filename2.h\n"
                       "2;2 5: match\n")
            << FileSearchResultList({{"/file/path/to/filename1.h", 1, "match", 1, 5, {}},
                                     {"/file/path/to/filename2.h", 2, " match", 2, 5, {}}});
    QTest::addRow("oneFileMultipleMatches")
            << QString(":/file/path/to/filename.h\n"
                       "1;1 5,7 5:match match\n")
            << FileSearchResultList({{"/file/path/to/filename.h", 1, "match match", 1, 5, {}},
                                     {"/file/path/to/filename.h", 1, "match match", 7, 5, {}}});
    QTest::addRow("multipleFilesWithMultipleMatches")
            << QString(":/file/path/to/filename1.h\n"
                       "1;1 5,7 5:match match\n"
                       "\n"
                       ":/file/path/to/filename2.h\n"
                       "2;2 5,8 5: match match\n")
            << FileSearchResultList({{"/file/path/to/filename1.h", 1, "match match", 1, 5, {}},
                                     {"/file/path/to/filename1.h", 1, "match match", 7, 5, {}},
                                     {"/file/path/to/filename2.h", 2, " match match", 2, 5, {}},
                                     {"/file/path/to/filename2.h", 2, " match match", 8, 5, {}}});
}

void OutputParserTest::test()
{
    QFETCH(QString, parserOutput);
    QFETCH(FileSearchResultList, results);
    SilverSearcher::SilverSearcherOutputParser ssop(parserOutput);
    const FileSearchResultList items = ssop.parse();
    QCOMPARE(items, results);
}

} // namespace Internal
} // namespace SilverSearcher
