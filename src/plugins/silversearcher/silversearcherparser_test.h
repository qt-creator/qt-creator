// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace SilverSearcher::Internal {

class OutputParserTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();
    void testChunksEndingInsideALine();
    void testCarriageReturns();
    void testOnlyHitOnItsLineIsNotConfirmed();
    void testWholeMatchIsReused();
    void testHitCountedFromEarlierLine();
    void testRunIsBounded();
    void testUnresolvedHitsAreBounded();
    void testLongRunOfMatchesAcrossLines();
    void testLongMatch();
    void testRegularExpressionUserData();
    void testPlainTextAcrossLinesInCrlfFile();
    void testArguments();
    void testSearchAcrossLinesInCrlfFile();
};

} // namespace SilverSearcher::Internal
