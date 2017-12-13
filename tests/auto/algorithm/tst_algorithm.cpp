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

#include <QtTest>

#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <valarray>

// must get included after the containers above or gcc4.9 will have a problem using
// initializer_list related code on the templates inside algorithm.h
#include <utils/algorithm.h>

class tst_Algorithm : public QObject
{
    Q_OBJECT

private slots:
    void anyOf();
    void transform();
    void sort();
    void contains();
    void findOr();
    void findOrDefault();
};


int stringToInt(const QString &s)
{
    return s.toInt();
}

namespace {

struct BaseStruct
{
    BaseStruct(int m) : member(m) {}
    bool operator==(const BaseStruct &other) const { return member == other.member; }

    int member;
};

struct Struct : public BaseStruct
{
    Struct(int m) : BaseStruct(m) {}
    bool isOdd() const { return member % 2 == 1; }
    bool isEven() const { return !isOdd(); }

    int getMember() const { return member; }

};
}

void tst_Algorithm::anyOf()
{
    {
        const QList<QString> strings({"1", "3", "132"});
        QVERIFY(Utils::anyOf(strings, [](const QString &s) { return s == "132"; }));
        QVERIFY(!Utils::anyOf(strings, [](const QString &s) { return s == "1324"; }));
    }
    {
        const QList<Struct> list({2, 4, 6, 8});
        QVERIFY(Utils::anyOf(list, &Struct::isEven));
        QVERIFY(!Utils::anyOf(list, &Struct::isOdd));
    }
    {
        const QList<Struct> list({0, 0, 0, 0, 1, 0, 0});
        QVERIFY(Utils::anyOf(list, &Struct::member));
    }
    {
        const QList<Struct> list({0, 0, 0, 0, 0, 0, 0});
        QVERIFY(!Utils::anyOf(list, &Struct::member));
    }

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
    {
        // QList -> std::vector
        const QList<int> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform<std::vector>(v, [](int i) { return i + 1; });
        QCOMPARE(trans, std::vector<int>({2, 3, 4, 5}));
    }
    {
        // QList -> std::vector
        const QList<Struct> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform<std::vector>(v, &Struct::getMember);
        QCOMPARE(trans, std::vector<int>({1, 2, 3, 4}));
    }
    {
        // QList -> std::vector
        const QList<Struct> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform<std::vector>(v, &Struct::member);
        QCOMPARE(trans, std::vector<int>({1, 2, 3, 4}));
    }
    {
        // std::vector -> QList
        const std::vector<int> v({1, 2, 3, 4});
        const QList<int> trans = Utils::transform<QList>(v, [](int i) { return i + 1; });
        QCOMPARE(trans, QList<int>({2, 3, 4, 5}));
    }
    {
        // std::vector -> QList
        const std::vector<Struct> v({1, 2, 3, 4});
        const QList<int> trans = Utils::transform<QList>(v, &Struct::getMember);
        QCOMPARE(trans, QList<int>({1, 2, 3, 4}));
    }
    {
        // std::vector -> QList
        const std::vector<Struct> v({1, 2, 3, 4});
        const QList<int> trans = Utils::transform<QList>(v, &Struct::member);
        QCOMPARE(trans, QList<int>({1, 2, 3, 4}));
    }
    {
        // std::deque -> std::vector
        const std::deque<int> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform<std::vector>(v, [](int i) { return i + 1; });
        QCOMPARE(trans, std::vector<int>({2, 3, 4, 5}));
    }
    {
        // std::deque -> std::vector
        const std::deque<Struct> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform<std::vector>(v, &Struct::getMember);
        QCOMPARE(trans, std::vector<int>({1, 2, 3, 4}));
    }
    {
        // std::deque -> std::vector
        const std::deque<Struct> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform<std::vector>(v, &Struct::member);
        QCOMPARE(trans, std::vector<int>({1, 2, 3, 4}));
    }
    {
        // std::vector -> std::vector
        const std::vector<int> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform(v, [](int i) { return i + 1; });
        QCOMPARE(trans, std::vector<int>({2, 3, 4, 5}));
    }
    {
        // std::vector -> std::vector
        const std::vector<Struct> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform(v, &Struct::getMember);
        QCOMPARE(trans, std::vector<int>({1, 2, 3, 4}));
    }
    {
        // std::vector -> std::vector
        const std::vector<Struct> v({1, 2, 3, 4});
        const std::vector<int> trans = Utils::transform(v, &Struct::member);
        QCOMPARE(trans, std::vector<int>({1, 2, 3, 4}));
    }
    {
        // std::unordered_map -> QList
        std::unordered_map<int, double> m;
        m.emplace(1, 1.5);
        m.emplace(3, 2.5);
        m.emplace(5, 3.5);
        QList<double> trans = Utils::transform<QList>(m, [](const std::pair<int, double> &in) {
            return in.first * in.second;
        });
        Utils::sort(trans);
        QCOMPARE(trans, QList<double>({1.5, 7.5, 17.5}));
    }
    {
        // specific result container with one template parameter (QVector)
        std::vector<int> v({1, 2, 3, 4});
        const QVector<BaseStruct *> trans = Utils::transform<QVector<BaseStruct *>>(v, [](int i) {
            return new Struct(i);
        });
        QCOMPARE(trans.size(), 4);
        QCOMPARE(trans.at(0)->member, 1);
        QCOMPARE(trans.at(1)->member, 2);
        QCOMPARE(trans.at(2)->member, 3);
        QCOMPARE(trans.at(3)->member, 4);
        qDeleteAll(trans);
    }
    {
        // specific result container with one of two template parameters (std::vector)
        std::vector<int> v({1, 2, 3, 4});
        const std::vector<BaseStruct *> trans
            = Utils::transform<std::vector<BaseStruct *>>(v, [](int i) { return new Struct(i); });
        QCOMPARE(trans.size(), static_cast<std::vector<int>::size_type>(4ul));
        QCOMPARE(trans.at(0)->member, 1);
        QCOMPARE(trans.at(1)->member, 2);
        QCOMPARE(trans.at(2)->member, 3);
        QCOMPARE(trans.at(3)->member, 4);
        qDeleteAll(trans);
    }
    {
        // specific result container with two template parameters (std::vector)
        std::vector<int> v({1, 2, 3, 4});
        const std::vector<BaseStruct *, std::allocator<BaseStruct *>> trans
            = Utils::transform<std::vector<BaseStruct *, std::allocator<BaseStruct *>>>(v, [](int i) {
                  return new Struct(i);
              });
        QCOMPARE(trans.size(), static_cast<std::vector<int>::size_type>(4ul));
        QCOMPARE(trans.at(0)->member, 1);
        QCOMPARE(trans.at(1)->member, 2);
        QCOMPARE(trans.at(2)->member, 3);
        QCOMPARE(trans.at(3)->member, 4);
        qDeleteAll(trans);
    }
    {
        // specific result container with member function
        QList<Struct> v({1, 2, 3, 4});
        const QVector<double> trans = Utils::transform<QVector<double>>(v, &Struct::getMember);
        QCOMPARE(trans, QVector<double>({1.0, 2.0, 3.0, 4.0}));
    }
    {
        // specific result container with member
        QList<Struct> v({1, 2, 3, 4});
        const QVector<double> trans = Utils::transform<QVector<double>>(v, &Struct::member);
        QCOMPARE(trans, QVector<double>({1.0, 2.0, 3.0, 4.0}));
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
    // std::array:
    std::array<int, 4> array = {{4, 10, 8, 1}};
    Utils::sort(array);
    std::array<int, 4> arrayResult = {{1, 4, 8, 10}};
    QCOMPARE(array, arrayResult);
    // valarray (no begin/end member functions):
    std::valarray<int> valarray(array.data(), array.size());
    std::valarray<int> valarrayResult(arrayResult.data(), arrayResult.size());
    Utils::sort(valarray);
    QCOMPARE(valarray.size(), valarrayResult.size());
    for (size_t i = 0; i < valarray.size(); ++i)
        QCOMPARE(valarray[i], valarrayResult[i]);
}

void tst_Algorithm::contains()
{
    std::vector<int> v1{1, 2, 3, 4};
    QVERIFY(Utils::contains(v1, [](int i) { return i == 2; }));
    QVERIFY(!Utils::contains(v1, [](int i) { return i == 5; }));
    std::vector<Struct> structs = {2, 4, 6, 8};
    QVERIFY(Utils::contains(structs, &Struct::isEven));
    QVERIFY(!Utils::contains(structs, &Struct::isOdd));
    QList<Struct> structQlist = {2, 4, 6, 8};
    QVERIFY(Utils::contains(structQlist, &Struct::isEven));
    QVERIFY(!Utils::contains(structQlist, &Struct::isOdd));
    std::vector<std::unique_ptr<int>> v2;
    v2.emplace_back(std::make_unique<int>(1));
    v2.emplace_back(std::make_unique<int>(2));
    v2.emplace_back(std::make_unique<int>(3));
    v2.emplace_back(std::make_unique<int>(4));
    QVERIFY(Utils::contains(v2, [](const std::unique_ptr<int> &ip) { return *ip == 2; }));
    QVERIFY(!Utils::contains(v2, [](const std::unique_ptr<int> &ip) { return *ip == 5; }));
    // Find pointers in unique_ptrs:
    QVERIFY(Utils::contains(v2, v2.back().get()));
    int foo = 42;
    QVERIFY(!Utils::contains(v2, &foo));
}

void tst_Algorithm::findOr()
{
    std::vector<int> v1{1, 2, 3, 4};
    QCOMPARE(Utils::findOr(v1, 6, [](int i) { return i == 2; }), 2);
    QCOMPARE(Utils::findOr(v1, 6, [](int i) { return i == 5; }), 6);
    std::vector<std::unique_ptr<int>> v2;
    v2.emplace_back(std::make_unique<int>(1));
    v2.emplace_back(std::make_unique<int>(2));
    v2.emplace_back(std::make_unique<int>(3));
    v2.emplace_back(std::make_unique<int>(4));
    int def = 6;
    QCOMPARE(Utils::findOr(v2, &def, [](const std::unique_ptr<int> &ip) { return *ip == 2; }), v2.at(1).get());
    QCOMPARE(Utils::findOr(v2, &def, [](const std::unique_ptr<int> &ip) { return *ip == 5; }), &def);
    std::vector<std::unique_ptr<Struct>> v3;
    v3.emplace_back(std::make_unique<Struct>(1));
    v3.emplace_back(std::make_unique<Struct>(3));
    v3.emplace_back(std::make_unique<Struct>(5));
    v3.emplace_back(std::make_unique<Struct>(7));
    Struct defS(6);
    QCOMPARE(Utils::findOr(v3, &defS, &Struct::isOdd), v3.at(0).get());
    QCOMPARE(Utils::findOr(v3, &defS, &Struct::isEven), &defS);

    std::vector<std::shared_ptr<Struct>> v4;
    v4.emplace_back(std::make_shared<Struct>(1));
    v4.emplace_back(std::make_shared<Struct>(3));
    v4.emplace_back(std::make_shared<Struct>(5));
    v4.emplace_back(std::make_shared<Struct>(7));
    std::shared_ptr<Struct> sharedDefS = std::make_shared<Struct>(6);
    QCOMPARE(Utils::findOr(v4, sharedDefS, &Struct::isOdd), v4.at(0));
    QCOMPARE(Utils::findOr(v4, sharedDefS, &Struct::isEven), sharedDefS);
}

void tst_Algorithm::findOrDefault()
{
    std::vector<int> v1{1, 2, 3, 4};
    QCOMPARE(Utils::findOrDefault(v1, [](int i) { return i == 2; }), 2);
    QCOMPARE(Utils::findOrDefault(v1, [](int i) { return i == 5; }), 0);
    std::vector<std::unique_ptr<int>> v2;
    v2.emplace_back(std::make_unique<int>(1));
    v2.emplace_back(std::make_unique<int>(2));
    v2.emplace_back(std::make_unique<int>(3));
    v2.emplace_back(std::make_unique<int>(4));
    QCOMPARE(Utils::findOrDefault(v2, [](const std::unique_ptr<int> &ip) { return *ip == 2; }), v2.at(1).get());
    QCOMPARE(Utils::findOrDefault(v2, [](const std::unique_ptr<int> &ip) { return *ip == 5; }), static_cast<int*>(0));
    std::vector<std::unique_ptr<Struct>> v3;
    v3.emplace_back(std::make_unique<Struct>(1));
    v3.emplace_back(std::make_unique<Struct>(3));
    v3.emplace_back(std::make_unique<Struct>(5));
    v3.emplace_back(std::make_unique<Struct>(7));
    QCOMPARE(Utils::findOrDefault(v3, &Struct::isOdd), v3.at(0).get());
    QCOMPARE(Utils::findOrDefault(v3, &Struct::isEven), static_cast<Struct*>(nullptr));

    std::vector<std::shared_ptr<Struct>> v4;
    v4.emplace_back(std::make_shared<Struct>(1));
    v4.emplace_back(std::make_shared<Struct>(3));
    v4.emplace_back(std::make_shared<Struct>(5));
    v4.emplace_back(std::make_shared<Struct>(7));
    QCOMPARE(Utils::findOrDefault(v4, &Struct::isOdd), v4.at(0));
    QCOMPARE(Utils::findOrDefault(v4, &Struct::isEven), std::shared_ptr<Struct>());
}

QTEST_MAIN(tst_Algorithm)

#include "tst_algorithm.moc"
