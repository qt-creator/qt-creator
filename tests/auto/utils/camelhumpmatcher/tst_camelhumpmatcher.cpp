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
    QTest::newRow("question-wildcard") << "Lon?Ca" << "VeryLongCamelHump" << 4;
    QTest::newRow("unmatched-question-wildcard") << "Long?Ca" << "VeryLongCamelHump" << -1;
    QTest::newRow("asterix-wildcard") << "Long*Ca" << "VeryLongCamelHump" << 4;
    QTest::newRow("empty-asterix-wildcard") << "Lo*Ca" << "VeryLongCamelHump" << 4;
}

QTEST_APPLESS_MAIN(tst_CamelHumpMatcher)
#include "tst_camelhumpmatcher.moc"
