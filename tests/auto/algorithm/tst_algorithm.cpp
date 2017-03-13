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

#include <utils/algorithm.h>

#include <QtTest>

class tst_Algorithm : public QObject
{
    Q_OBJECT

private slots:
    void transform();
    void sort();
};


int stringToInt(const QString &s)
{
    return s.toInt();
}

namespace {
struct Struct
{
    Struct(int m) : member(m) {}
    bool operator==(const Struct &other) const { return member == other.member; }

    int member;
};
}

void tst_Algorithm::transform()
{
    // same container type
    {
        // QList has standard inserter
        const QList<QString> strings({"1", "3", "132"});
        const QList<int> i1 = Utils::transform(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QList<int>({1, 3, 132}));
        const QList<int> i2 = Utils::transform(strings, stringToInt);
        QCOMPARE(i2, QList<int>({1, 3, 132}));
        const QList<int> i3 = Utils::transform(strings, &QString::size);
        QCOMPARE(i3, QList<int>({1, 1, 3}));
    }
    {
        // QStringList
        const QStringList strings({"1", "3", "132"});
        const QList<int> i1 = Utils::transform(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QList<int>({1, 3, 132}));
        const QList<int> i2 = Utils::transform(strings, stringToInt);
        QCOMPARE(i2, QList<int>({1, 3, 132}));
        const QList<int> i3 = Utils::transform(strings, &QString::size);
        QCOMPARE(i3, QList<int>({1, 1, 3}));
    }
    {
        // QSet internally needs special inserter
        const QSet<QString> strings({QString("1"), QString("3"), QString("132")});
        const QSet<int> i1 = Utils::transform(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QSet<int>({1, 3, 132}));
        const QSet<int> i2 = Utils::transform(strings, stringToInt);
        QCOMPARE(i2, QSet<int>({1, 3, 132}));
        const QSet<int> i3 = Utils::transform(strings, &QString::size);
        QCOMPARE(i3, QSet<int>({1, 3}));
    }

    // different container types
    {
        // QList to QSet
        const QList<QString> strings({"1", "3", "132"});
        const QSet<int> i1 = Utils::transform<QSet>(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QSet<int>({1, 3, 132}));
        const QSet<int> i2 = Utils::transform<QSet>(strings, stringToInt);
        QCOMPARE(i2, QSet<int>({1, 3, 132}));
        const QSet<int> i3 = Utils::transform<QSet>(strings, &QString::size);
        QCOMPARE(i3, QSet<int>({1, 3}));
    }
    {
        // QStringList to QSet
        const QStringList strings({"1", "3", "132"});
        const QSet<int> i1 = Utils::transform<QSet>(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QSet<int>({1, 3, 132}));
        const QSet<int> i2 = Utils::transform<QSet>(strings, stringToInt);
        QCOMPARE(i2, QSet<int>({1, 3, 132}));
        const QSet<int> i3 = Utils::transform<QSet>(strings, &QString::size);
        QCOMPARE(i3, QSet<int>({1, 3}));
    }
    {
        // QSet to QList
        const QSet<QString> strings({QString("1"), QString("3"), QString("132")});
        QList<int> i1 = Utils::transform<QList>(strings, [](const QString &s) { return s.toInt(); });
        Utils::sort(i1);
        QCOMPARE(i1, QList<int>({1, 3, 132}));
        QList<int> i2 = Utils::transform<QList>(strings, stringToInt);
        Utils::sort(i2);
        QCOMPARE(i2, QList<int>({1, 3, 132}));
        QList<int> i3 = Utils::transform<QList>(strings, &QString::size);
        Utils::sort(i3);
        QCOMPARE(i3, QList<int>({1, 1, 3}));
    }
    {
        const QList<Struct> list({4, 3, 2, 1, 2});
        const QList<int> trans = Utils::transform(list, &Struct::member);
        QCOMPARE(trans, QList<int>({4, 3, 2, 1, 2}));
    }
}

void tst_Algorithm::sort()
{
    QStringList s1({"3", "2", "1"});
    Utils::sort(s1);
    QCOMPARE(s1, QStringList({"1", "2", "3"}));
    QStringList s2({"13", "31", "22"});
    Utils::sort(s2, [](const QString &a, const QString &b) { return a[1] < b[1]; });
    QCOMPARE(s2, QStringList({"31", "22", "13"}));
    QList<QString> s3({"12345", "3333", "22"});
    Utils::sort(s3, &QString::size);
    QCOMPARE(s3, QList<QString>({"22", "3333", "12345"}));
    QList<Struct> s4({4, 3, 2, 1});
    Utils::sort(s4, &Struct::member);
    QCOMPARE(s4, QList<Struct>({1, 2, 3, 4}));
    // member function with pointers
    QList<QString> arr1({"12345", "3333", "22"});
    QList<QString *> s5({&arr1[0], &arr1[1], &arr1[2]});
    Utils::sort(s5, &QString::size);
    QCOMPARE(s5, QList<QString *>({&arr1[2], &arr1[1], &arr1[0]}));
    // member with pointers
    QList<Struct> arr2({4, 1, 3});
    QList<Struct *> s6({&arr2[0], &arr2[1], &arr2[2]});
    Utils::sort(s6, &Struct::member);
    QCOMPARE(s6, QList<Struct *>({&arr2[1], &arr2[2], &arr2[0]}));
}

QTEST_MAIN(tst_Algorithm)

#include "tst_algorithm.moc"
