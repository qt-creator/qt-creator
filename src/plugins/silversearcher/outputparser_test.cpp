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
    const QList<Utils::FileSearchResult> items = ssop.parse();
    QCOMPARE(items, results);
}

} // namespace Internal
} // namespace SilverSearcher
