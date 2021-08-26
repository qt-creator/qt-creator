/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <utils/indexedcontainerproxyconstiterator.h>

#include <QtTest>

#include <string>
#include <vector>

using namespace Utils;

class tst_IndexedContainerProxyConstIterator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConstruction();
    void testDereference();
    void testIndexing();
    void testComparisons();
    void testIncrement();
    void testDecrement();
    void testPlus();
    void testMinus();
    void testIteration();

private:
    using StringContainer = std::vector<std::string>;
    using StringIterator = IndexedContainerProxyConstIterator<StringContainer>;
    StringContainer strings;

    using BoolContainer = std::vector<bool>;
    using BoolIterator = IndexedContainerProxyConstIterator<BoolContainer>;
    BoolContainer bools;
};

void tst_IndexedContainerProxyConstIterator::initTestCase()
{
    strings = {"abc", "defgh", "ijk"};
    bools = {false, true, false};
}

void tst_IndexedContainerProxyConstIterator::testConstruction()
{
    StringIterator strIt(strings, 0);
    QCOMPARE(*strIt, "abc");

    StringIterator strIt2(strings, 1);
    QCOMPARE(*strIt2, "defgh");

    BoolIterator boolIt(bools, 0);
    QCOMPARE(*boolIt, false);

    BoolIterator boolIt2(bools, 1);
    QCOMPARE(*boolIt2, true);
}

void tst_IndexedContainerProxyConstIterator::testDereference()
{
    StringIterator strIt(strings, 0);
    QCOMPARE(*strIt, "abc");
    QCOMPARE(int(strIt->length()), 3);

    BoolIterator boolIt(bools, 0);
    QCOMPARE(*boolIt, false);
}

void tst_IndexedContainerProxyConstIterator::testIndexing()
{
    StringIterator strIt(strings, 0);
    QCOMPARE(strIt[2], "ijk");

    BoolIterator boolIt(bools, 0);
    QCOMPARE(boolIt[2], false);
}

void tst_IndexedContainerProxyConstIterator::testComparisons()
{
    StringIterator strIt(strings, 0);
    StringIterator strIt2(strings, 0);
    StringIterator strIt3(strings, 1);

    QVERIFY(strIt == strIt);
    QVERIFY(!(strIt != strIt));
    QVERIFY(!(strIt < strIt));
    QVERIFY(strIt <= strIt);
    QVERIFY(!(strIt > strIt));
    QVERIFY(strIt >= strIt);

    QVERIFY(strIt == strIt2);
    QVERIFY(!(strIt != strIt2));
    QVERIFY(!(strIt < strIt2));
    QVERIFY(strIt <= strIt2);
    QVERIFY(!(strIt > strIt2));
    QVERIFY(strIt >= strIt2);

    QVERIFY(!(strIt == strIt3));
    QVERIFY(strIt != strIt3);
    QVERIFY(strIt < strIt3);
    QVERIFY(strIt <= strIt3);
    QVERIFY(!(strIt > strIt3));
    QVERIFY(!(strIt >= strIt3));

    QVERIFY(!(strIt3 == strIt));
    QVERIFY(strIt3 != strIt);
    QVERIFY(!(strIt3 < strIt));
    QVERIFY(!(strIt3 <= strIt));
    QVERIFY(strIt3 > strIt);
    QVERIFY(strIt3 >= strIt);
}

void tst_IndexedContainerProxyConstIterator::testIncrement()
{
    StringIterator strIt(strings, 0);
    QCOMPARE(*(++strIt), "defgh");
    QCOMPARE(*(strIt++), "defgh");
    QCOMPARE(*strIt, "ijk");

    BoolIterator boolIt(bools, 0);
    QCOMPARE(*(++boolIt), true);
    QCOMPARE(*(boolIt++), true);
    QCOMPARE(*boolIt, false);
}

void tst_IndexedContainerProxyConstIterator::testDecrement()
{
    StringIterator strIt(strings, 3);
    QCOMPARE(*(--strIt), "ijk");
    QCOMPARE(*(strIt--), "ijk");
    QCOMPARE(*strIt, "defgh");

    BoolIterator boolIt(bools, 3);
    QCOMPARE(*(--boolIt), false);
    QCOMPARE(*(boolIt--), false);
    QCOMPARE(*boolIt, true);
}

void tst_IndexedContainerProxyConstIterator::testPlus()
{
    StringIterator strIt(strings, 1);
    QCOMPARE(*(strIt + 1), "ijk");
    QCOMPARE(*(1 + strIt), "ijk");
    strIt += 1;
    QCOMPARE(*strIt, "ijk");

    BoolIterator boolIt(bools, 1);
    QCOMPARE(*(boolIt + 1), false);
    QCOMPARE(*(1 + boolIt), false);
    boolIt += 1;
    QCOMPARE(*boolIt, false);
}

void tst_IndexedContainerProxyConstIterator::testMinus()
{
    StringIterator strIt(strings, 1);
    QCOMPARE(*(strIt - 1), "abc");
    strIt -= 1;
    QCOMPARE(*strIt, "abc");

    BoolIterator boolIt(bools, 1);
    QCOMPARE(*(boolIt - 1), false);
    boolIt -= 1;
    QCOMPARE(*boolIt, false);
}

void tst_IndexedContainerProxyConstIterator::testIteration()
{
    StringIterator strBegin(strings, 0);
    StringIterator strEnd(strings, strings.size());
    StringContainer stringsCopy;
    for (StringIterator it = strBegin; it != strEnd; ++it)
        stringsCopy.push_back(*it);
    QCOMPARE(stringsCopy, strings);

    BoolIterator boolBegin(bools, 0);
    BoolIterator boolEnd(bools, bools.size());
    BoolContainer boolsCopy;
    for (BoolIterator it = boolBegin; it != boolEnd; ++it)
        boolsCopy.push_back(*it);
    QCOMPARE(boolsCopy, bools);
}

QTEST_MAIN(tst_IndexedContainerProxyConstIterator)

#include "tst_indexedcontainerproxyconstiterator.moc"
