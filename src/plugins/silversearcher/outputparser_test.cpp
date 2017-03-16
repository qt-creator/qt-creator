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

namespace SilverSearcher {
namespace Internal {

void OutputParserTest::testNoResults()
{
    const char parserOutput[] = "\n";
    const QByteArray output(parserOutput);
    SilverSearcher::SilverSearcherOutputParser ssop(output);
    const QList<Utils::FileSearchResult> items = ssop.parse();
    QCOMPARE(items.size(), 0);
}

void OutputParserTest::testOneFileWithOneMatch()
{
    const char parserOutput[] = ":/file/path/to/filename.h\n"
                                "1;1 5:match\n";
    const QByteArray output(parserOutput);
    SilverSearcher::SilverSearcherOutputParser ssop(output);
    const QList<Utils::FileSearchResult> items = ssop.parse();
    QCOMPARE(items.size(), 1);
    QCOMPARE(items[0].fileName, QStringLiteral("/file/path/to/filename.h"));
    QCOMPARE(items[0].lineNumber, 1);
    QCOMPARE(items[0].matchingLine, QStringLiteral("match"));
    QCOMPARE(items[0].matchStart, 1);
    QCOMPARE(items[0].matchLength, 5);
}

void OutputParserTest::testMultipleFilesWithOneMatch()
{
    const char parserOutput[] = ":/file/path/to/filename1.h\n"
                                "1;1 5:match\n"
                                "\n"
                                ":/file/path/to/filename2.h\n"
                                "2;2 5: match\n"
            ;
    const QByteArray output(parserOutput);
    SilverSearcher::SilverSearcherOutputParser ssop(output);
    const QList<Utils::FileSearchResult> items = ssop.parse();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].fileName, QStringLiteral("/file/path/to/filename1.h"));
    QCOMPARE(items[0].lineNumber, 1);
    QCOMPARE(items[0].matchingLine, QStringLiteral("match"));
    QCOMPARE(items[0].matchStart, 1);
    QCOMPARE(items[0].matchLength, 5);

    QCOMPARE(items[1].fileName, QStringLiteral("/file/path/to/filename2.h"));
    QCOMPARE(items[1].lineNumber, 2);
    QCOMPARE(items[1].matchingLine, QStringLiteral(" match"));
    QCOMPARE(items[1].matchStart, 2);
    QCOMPARE(items[1].matchLength, 5);
}

void OutputParserTest::testOneFileWithMultipleMatches()
{
    const char parserOutput[] = ":/file/path/to/filename.h\n"
                                "1;1 5,7 5:match match\n";
    const QByteArray output(parserOutput);
    SilverSearcher::SilverSearcherOutputParser ssop(output);
    const QList<Utils::FileSearchResult> items = ssop.parse();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].fileName, QStringLiteral("/file/path/to/filename.h"));
    QCOMPARE(items[0].lineNumber, 1);
    QCOMPARE(items[0].matchingLine, QStringLiteral("match match"));
    QCOMPARE(items[0].matchStart, 1);
    QCOMPARE(items[0].matchLength, 5);

    QCOMPARE(items[1].fileName, QStringLiteral("/file/path/to/filename.h"));
    QCOMPARE(items[1].lineNumber, 1);
    QCOMPARE(items[1].matchingLine, QStringLiteral("match match"));
    QCOMPARE(items[1].matchStart, 7);
    QCOMPARE(items[1].matchLength, 5);
}

void OutputParserTest::testMultipleFilesWithMultipleMatches()
{
    const char parserOutput[] = ":/file/path/to/filename1.h\n"
                                "1;1 5,7 5:match match\n"
                                "\n"
                                ":/file/path/to/filename2.h\n"
                                "2;2 5,8 5: match match\n";
    const QByteArray output(parserOutput);
    SilverSearcher::SilverSearcherOutputParser ssop(output);
    const QList<Utils::FileSearchResult> items = ssop.parse();
    QCOMPARE(items.size(), 4);
    QCOMPARE(items[0].fileName, QStringLiteral("/file/path/to/filename1.h"));
    QCOMPARE(items[0].lineNumber, 1);
    QCOMPARE(items[0].matchingLine, QStringLiteral("match match"));
    QCOMPARE(items[0].matchStart, 1);
    QCOMPARE(items[0].matchLength, 5);

    QCOMPARE(items[1].fileName, QStringLiteral("/file/path/to/filename1.h"));
    QCOMPARE(items[1].lineNumber, 1);
    QCOMPARE(items[1].matchingLine, QStringLiteral("match match"));
    QCOMPARE(items[1].matchStart, 7);
    QCOMPARE(items[1].matchLength, 5);

    QCOMPARE(items[2].fileName, QStringLiteral("/file/path/to/filename2.h"));
    QCOMPARE(items[2].lineNumber, 2);
    QCOMPARE(items[2].matchingLine, QStringLiteral(" match match"));
    QCOMPARE(items[2].matchStart, 2);
    QCOMPARE(items[2].matchLength, 5);

    QCOMPARE(items[3].fileName, QStringLiteral("/file/path/to/filename2.h"));
    QCOMPARE(items[3].lineNumber, 2);
    QCOMPARE(items[3].matchingLine, QStringLiteral(" match match"));
    QCOMPARE(items[3].matchStart, 8);
    QCOMPARE(items[3].matchLength, 5);
}

} // namespace Internal
} // namespace SilverSearcher
