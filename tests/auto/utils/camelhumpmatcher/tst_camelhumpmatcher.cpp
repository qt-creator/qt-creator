/**************************************************************************
**
** Copyright (C) 2017 BlackBerry Limited <qt@blackberry.com>
** Copyright (C) 2017 Andre Hartmann <aha_1980@gmx.de>
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

#include <utils/camelhumpmatcher.h>

#include <QtTest>

class tst_CamelHumpMatcher : public QObject
{
    Q_OBJECT

private slots:
    void camelHumpMatcher();
    void camelHumpMatcher_data();
    void highlighting();
    void highlighting_data();
};

void tst_CamelHumpMatcher::camelHumpMatcher()
{
    QFETCH(QString, pattern);
    QFETCH(QString, candidate);
    QFETCH(int, expectedIndex);

    const QRegularExpression regExp = CamelHumpMatcher::createCamelHumpRegExp(pattern);
    const QRegularExpressionMatch match = regExp.match(candidate);
    QCOMPARE(match.capturedStart(), expectedIndex);
}

void tst_CamelHumpMatcher::camelHumpMatcher_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("candidate");
    QTest::addColumn<int>("expectedIndex");

    QTest::newRow("underscore") << "vl" << "very_long_camel_hump" << 0;
    QTest::newRow("underscore-uppercase") << "vl" << "VERY_LONG_CAMEL_HUMP" << 0;
    QTest::newRow("exact") << "VeryLongCamelHump" << "VeryLongCamelHump" << 0;
    QTest::newRow("prefix-segments") << "velo" << "very_long_Camel_hump" << 0;
    QTest::newRow("humps") << "vlc" << "VeryLongCamelHump" << 0;
    QTest::newRow("case-insensitive-humps") << "VlCh" << "VeryLongCamelHump" << 0;
    QTest::newRow("incorrect-hump") << "vxc" << "VeryLongCamelHump" << -1;
    QTest::newRow("skipped-hump") << "vc" << "VeryLongCamelHump" << -1;
    QTest::newRow("middle-humps") << "lc" << "VeryLongCamelHump" << 4;
    QTest::newRow("incorrect-hump") << "lyn" << "VeryLongCamelHump" << -1;
    QTest::newRow("humps") << "VL" << "VeryLongCamelHump" << 0;
    QTest::newRow("skipped-humps-upper") << "VH" << "VeryLongCamelHump" << -1;
    QTest::newRow("numbers") << "4" << "Test4Fun" << 4;
    QTest::newRow("question-wildcard") << "Lon?Ca" << "VeryLongCamelHump" << 4;
    QTest::newRow("unmatched-question-wildcard") << "Long?Ca" << "VeryLongCamelHump" << -1;
    QTest::newRow("asterisk-wildcard") << "Long*Ca" << "VeryLongCamelHump" << 4;
    QTest::newRow("empty-asterisk-wildcard") << "Lo*Ca" << "VeryLongCamelHump" << 4;
    QTest::newRow("no-partial") << "NCH" << "LongCamelHump" << -1;
    QTest::newRow("middle-after-number") << "CH" << "Long1CamelHump" << 5;
    QTest::newRow("middle-after-underscore") << "CH" << "long_camel_hump" << 5;
    QTest::newRow("middle-after-underscore-uppercase") << "CH" << "LONG_CAMEL_HUMP" << 5;
    QTest::newRow("middle-continued") << "cahu" << "LongCamelHump" << 4;
    QTest::newRow("middle-no-hump") << "window" << "mainwindow.cpp" << 4;
    QTest::newRow("case-insensitive") << "window" << "MAINWINDOW.cpp" << 4;
    QTest::newRow("case-insensitive-2") << "wINDow" << "MainwiNdow.cpp" << 4;
}

typedef QVector<int> MatchStart;
typedef QVector<int> MatchLength;

void tst_CamelHumpMatcher::highlighting()
{
    QFETCH(QString, pattern);
    QFETCH(QString, candidate);
    QFETCH(MatchStart, matchStart);
    QFETCH(MatchLength, matchLength);

    const QRegularExpression regExp = CamelHumpMatcher::createCamelHumpRegExp(pattern);
    const QRegularExpressionMatch match = regExp.match(candidate);
    const CamelHumpMatcher::HighlightingPositions positions =
            CamelHumpMatcher::highlightingPositions(match);

    QCOMPARE(positions.starts.size(), matchStart.size());
    for (int i = 0; i < positions.starts.size(); ++i) {
        QCOMPARE(positions.starts.at(i), matchStart.at(i));
        QCOMPARE(positions.lengths.at(i), matchLength.at(i));
    }
}

void tst_CamelHumpMatcher::highlighting_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("candidate");
    QTest::addColumn<MatchStart>("matchStart");
    QTest::addColumn<MatchLength>("matchLength");

    QTest::newRow("prefix-snake") << "very" << "very_long_camel_hump"
                                  << MatchStart{0} << MatchLength{4};
    QTest::newRow("middle-snake") << "long" << "very_long_camel_hump"
                                  << MatchStart{5} << MatchLength{4};
    QTest::newRow("suffix-snake") << "hump" << "very_long_camel_hump"
                                  << MatchStart{16} << MatchLength{4};
    QTest::newRow("prefix-camel") << "very" << "VeryLongCamelHump"
                                  << MatchStart{0} << MatchLength{4};
    QTest::newRow("middle-camel") << "Long" << "VeryLongCamelHump"
                                  << MatchStart{4} << MatchLength{4};
    QTest::newRow("suffix-camel") << "Hump" << "VeryLongCamelHump"
                                  << MatchStart{13} << MatchLength{4};
    QTest::newRow("humps-camel")  << "vlch" << "VeryLongCamelHump"
                                  << MatchStart{0, 4, 8, 13} << MatchLength{1, 1, 1, 1};
    QTest::newRow("humps-camel-lower") << "vlch" << "veryLongCamelHump"
                                       << MatchStart{0, 4, 8, 13} << MatchLength{1, 1, 1, 1};
    QTest::newRow("humps-snake") << "vlch" << "very_long_camel_hump"
                                 << MatchStart{0, 5, 10, 16} << MatchLength{1, 1, 1, 1};
    QTest::newRow("humps-middle") << "lc" << "VeryLongCamelHump"
                                  << MatchStart{4, 8} << MatchLength{1, 1};
    QTest::newRow("humps-last") << "h" << "VeryLongCamelHump"
                                << MatchStart{13} << MatchLength{1};
    QTest::newRow("humps-continued") << "LoCa" << "VeryLongCamelHump"
                                     << MatchStart{4, 8} << MatchLength{2, 2};
    QTest::newRow("numbers") << "4" << "TestJust4Fun"
                             << MatchStart{8} << MatchLength{1};
    QTest::newRow("wildcard-asterisk") << "Lo*Hu" << "VeryLongCamelHump"
                                       << MatchStart{4, 13} << MatchLength{2, 2};
    QTest::newRow("wildcard-question") << "Lo?g" << "VeryLongCamelHump"
                                       << MatchStart{4, 7} << MatchLength{2, 1};
    QTest::newRow("middle-no-hump") << "window" << "mainwindow.cpp"
                                    << MatchStart{4} << MatchLength{6};
}

QTEST_APPLESS_MAIN(tst_CamelHumpMatcher)
#include "tst_camelhumpmatcher.moc"
