// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <array>
#include <deque>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
    void toReferences();
    void take();
    void sorted();
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
        const QList<qsizetype> i3 = Utils::transform(strings, &QString::size);
        QCOMPARE(i3, QList<qsizetype>({1, 1, 3}));
    }
    {
        // QStringList
        const QStringList strings({"1", "3", "132"});
        const QList<int> i1 = Utils::transform(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QList<int>({1, 3, 132}));
        const QList<int> i2 = Utils::transform(strings, stringToInt);
        QCOMPARE(i2, QList<int>({1, 3, 132}));
        const QList<qsizetype> i3 = Utils::transform(strings, &QString::size);
        QCOMPARE(i3, QList<qsizetype>({1, 1, 3}));
    }
    {
        // QSet internally needs special inserter
        const QSet<QString> strings({QString("1"), QString("3"), QString("132")});
        const QSet<int> i1 = Utils::transform(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QSet<int>({1, 3, 132}));
        const QSet<int> i2 = Utils::transform(strings, stringToInt);
        QCOMPARE(i2, QSet<int>({1, 3, 132}));
        const QSet<qsizetype> i3 = Utils::transform(strings, &QString::size);
        QCOMPARE(i3, QSet<qsizetype>({1, 3}));
    }

    // different container types
    {
        // QList to QSet
        const QList<QString> strings({"1", "3", "132"});
        const QSet<int> i1 = Utils::transform<QSet>(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QSet<int>({1, 3, 132}));
        const QSet<int> i2 = Utils::transform<QSet>(strings, stringToInt);
        QCOMPARE(i2, QSet<int>({1, 3, 132}));
        const QSet<qsizetype> i3 = Utils::transform<QSet>(strings, &QString::size);
        QCOMPARE(i3, QSet<qsizetype>({1, 3}));
    }
    {
        // QStringList to QSet
        const QStringList strings({"1", "3", "132"});
        const QSet<int> i1 = Utils::transform<QSet>(strings, [](const QString &s) { return s.toInt(); });
        QCOMPARE(i1, QSet<int>({1, 3, 132}));
        const QSet<int> i2 = Utils::transform<QSet>(strings, stringToInt);
        QCOMPARE(i2, QSet<int>({1, 3, 132}));
        const QSet<qsizetype> i3 = Utils::transform<QSet>(strings, &QString::size);
        QCOMPARE(i3, QSet<qsizetype>({1, 3}));
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
        QList<qsizetype> i3 = Utils::transform<QList>(strings, &QString::size);
        QCOMPARE(Utils::sorted(i3), QList<qsizetype>({1, 1, 3}));
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
    {
        // non-const container and function parameter
        // same container type
        std::vector<Struct> v({1, 2, 3, 4});
        const std::vector<std::reference_wrapper<Struct>> trans
            = Utils::transform(v, [](Struct &s) { return std::ref(s); });
        // different container type
        QVector<Struct> v2({1, 2, 3, 4});
        const std::vector<std::reference_wrapper<Struct>> trans2
            = Utils::transform<std::vector>(v, [](Struct &s) { return std::ref(s); });
        // temporaries
        const auto tempv = [] { return QList<Struct>({1, 2, 3, 4}); };
        // temporary with member function
        const QList<int> trans3 = Utils::transform(tempv(), &Struct::getMember);
        const std::vector<int> trans4 = Utils::transform<std::vector>(tempv(), &Struct::getMember);
        const std::vector<int> trans5 = Utils::transform<std::vector<int>>(tempv(), &Struct::getMember);
        // temporary with member
        const QList<int> trans6 = Utils::transform(tempv(), &Struct::member);
        const std::vector<int> trans7 = Utils::transform<std::vector>(tempv(), &Struct::member);
        const std::vector<int> trans8 = Utils::transform<std::vector<int>>(tempv(), &Struct::member);
        // temporary with function
        const QList<int> trans9 = Utils::transform(tempv(),
                                                   [](const Struct &s) { return s.getMember(); });
        const std::vector<int> trans10 = Utils::transform<std::vector>(tempv(), [](const Struct &s) {
            return s.getMember();
        });
        const std::vector<int> trans11 = Utils::transform<std::vector<int>>(tempv(),
                                                                            [](const Struct &s) {
                                                                                return s.getMember();
                                                                            });
    }
    // target containers without reserve(...)
    {
        // std::vector -> std::deque
        const std::vector<int> v({1, 2, 3, 4});
        const std::deque<int> trans = Utils::transform<std::deque>(v, [](int i) { return i + 1; });
        QCOMPARE(trans, std::deque<int>({2, 3, 4, 5}));
    }
    {
        // std::vector -> std::list
        const std::vector<int> v({1, 2, 3, 4});
        const std::list<int> trans = Utils::transform<std::list>(v, [](int i) { return i + 1; });
        QCOMPARE(trans, std::list<int>({2, 3, 4, 5}));
    }
    {
        // std::vector -> std::set
        const std::vector<int> v({1, 2, 3, 4});
        const std::set<int> trans = Utils::transform<std::set<int>>(v, [](int i) { return i + 1; });
        QCOMPARE(trans, std::set<int>({2, 3, 4, 5}));
    }
    // various map/set/hash without push_back
    {
        // std::vector -> std::map
        const std::vector<int> v({1, 2, 3, 4});
        const std::map<int, int> trans = Utils::transform<std::map<int, int>>(v, [](int i) {
            return std::make_pair(i, i + 1);
        });
        const std::map<int, int> expected({{1, 2}, {2, 3}, {3, 4}, {4, 5}});
        QCOMPARE(trans, expected);
    }
    {
        // std::vector -> std::unordered_set
        const std::vector<int> v({1, 2, 3, 4});
        const std::unordered_set<int> trans = Utils::transform<std::unordered_set<int>>(v, [](int i) {
            return i + 1;
        });
        QCOMPARE(trans, std::unordered_set<int>({2, 3, 4, 5}));
    }
    {
        // std::vector -> std::unordered_map
        const std::vector<int> v({1, 2, 3, 4});
        const std::unordered_map<int, int> trans
            = Utils::transform<std::unordered_map<int, int>>(v, [](int i) {
                  return std::make_pair(i, i + 1);
              });
        const std::unordered_map<int, int> expected({{1, 2}, {2, 3}, {3, 4}, {4, 5}});
        QCOMPARE(trans, expected);
    }
    {
        // std::vector -> QMap using std::pair
        const std::vector<int> v({1, 2, 3, 4});
        const QMap<int, int> trans = Utils::transform<QMap<int, int>>(v, [](int i) {
            return std::make_pair(i, i + 1);
        });
        const QMap<int, int> expected({{1, 2}, {2, 3}, {3, 4}, {4, 5}});
        QCOMPARE(trans, expected);
    }
    {
        // std::vector -> QMap using QPair
        const std::vector<int> v({1, 2, 3, 4});
        const QMap<int, int> trans = Utils::transform<QMap<int, int>>(v, [](int i) {
            return qMakePair(i, i + 1);
        });
        const QMap<int, int> expected({{1, 2}, {2, 3}, {3, 4}, {4, 5}});
        QCOMPARE(trans, expected);
    }
    {
        // std::vector -> QHash using std::pair
        const std::vector<int> v({1, 2, 3, 4});
        const QHash<int, int> trans = Utils::transform<QHash<int, int>>(v, [](int i) {
            return std::make_pair(i, i + 1);
        });
        const QHash<int, int> expected({{1, 2}, {2, 3}, {3, 4}, {4, 5}});
        QCOMPARE(trans, expected);
    }
    {
        // std::vector -> QHash using QPair
        const std::vector<int> v({1, 2, 3, 4});
        const QHash<int, int> trans = Utils::transform<QHash<int, int>>(v, [](int i) {
            return qMakePair(i, i + 1);
        });
        const QHash<int, int> expected({{1, 2}, {2, 3}, {3, 4}, {4, 5}});
        QCOMPARE(trans, expected);
    }
    {
        // std::vector -> std::vector appending container
        const std::vector<int> v({1, 2, 3, 4});
        const auto trans = Utils::transform<std::vector<int>>(v, [](int i) -> std::vector<int> {
            return {i, i * 2};
        });
        const std::vector<int> expected{1, 2, 2, 4, 3, 6, 4, 8};
        QCOMPARE(trans, expected);
    }
    {
        // QList -> QList appending container
        const QList<int> v({1, 2, 3, 4});
        const auto trans = Utils::transform<QList<int>>(v, [](int i) -> QList<int> {
            return {i, i * 2};
        });
        const QList<int> expected{1, 2, 2, 4, 3, 6, 4, 8};
        QCOMPARE(trans, expected);
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
    QCOMPARE(Utils::sorted(s5, &QString::size), QList<QString *>({&arr1[2], &arr1[1], &arr1[0]}));
    // member with pointers
    QList<Struct> arr2({4, 1, 3});
    QList<Struct *> s6({&arr2[0], &arr2[1], &arr2[2]});
    QCOMPARE(Utils::sorted(s6, &Struct::member), QList<Struct *>({&arr2[1], &arr2[2], &arr2[0]}));
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
}

void tst_Algorithm::findOr()
{
    std::vector<int> v1{1, 2, 3, 4};
    QCOMPARE(Utils::findOr(v1, 10, [](int i) { return i == 2; }), 2);
    QCOMPARE(Utils::findOr(v1, 10, [](int i) { return i == 5; }), 10);
}

void tst_Algorithm::findOrDefault()
{
    {
        std::vector<int> v1{1, 2, 3, 4};
        QCOMPARE(Utils::findOrDefault(v1, [](int i) { return i == 2; }), 2);
        QCOMPARE(Utils::findOrDefault(v1, [](int i) { return i == 5; }), 0);
    }
    {
        std::vector<std::shared_ptr<Struct>> v4;
        v4.emplace_back(std::make_shared<Struct>(1));
        v4.emplace_back(std::make_shared<Struct>(3));
        v4.emplace_back(std::make_shared<Struct>(5));
        v4.emplace_back(std::make_shared<Struct>(7));
        QCOMPARE(Utils::findOrDefault(v4, &Struct::isOdd), v4.at(0));
        QCOMPARE(Utils::findOrDefault(v4, &Struct::isEven), std::shared_ptr<Struct>());
    }
}

void tst_Algorithm::toReferences()
{
    // toReference
    {
        // std::vector -> std::vector
        std::vector<Struct> v;
        const std::vector<std::reference_wrapper<Struct>> x = Utils::toReferences(v);
    }
    {
        // QList -> std::vector
        QList<Struct> v;
        const std::vector<std::reference_wrapper<Struct>> x = Utils::toReferences<std::vector>(v);
    }
    {
        // std::vector -> QList
        std::vector<Struct> v;
        const QList<std::reference_wrapper<Struct>> x = Utils::toReferences<QList>(v);
    }
    {
        // std::vector -> std::list
        std::vector<Struct> v;
        const std::list<std::reference_wrapper<Struct>> x = Utils::toReferences<std::list>(v);
    }
    // toConstReference
    {
        // std::vector -> std::vector
        const std::vector<Struct> v;
        const std::vector<std::reference_wrapper<const Struct>> x = Utils::toConstReferences(v);
    }
    {
        // QList -> std::vector
        const QList<Struct> v;
        const std::vector<std::reference_wrapper<const Struct>> x
            = Utils::toConstReferences<std::vector>(v);
    }
    {
        // std::vector -> QList
        const std::vector<Struct> v;
        const QList<std::reference_wrapper<const Struct>> x = Utils::toConstReferences<QList>(v);
    }
    {
        // std::vector -> std::list
        const std::vector<Struct> v;
        const std::list<std::reference_wrapper<const Struct>> x
            = Utils::toConstReferences<std::list>(v);
    }
}

void tst_Algorithm::take()
{
    {
        QList<Struct> v {1, 3, 5, 6, 7, 8, 9, 11, 13, 15, 13, 16, 17};
        std::optional<Struct> r1 = Utils::take(v, [](const Struct &s) { return s.member == 13; });
        QVERIFY(static_cast<bool>(r1));
        QCOMPARE(r1.value().member, 13);
        std::optional<Struct> r2 = Utils::take(v, [](const Struct &s) { return s.member == 13; });
        QVERIFY(static_cast<bool>(r2));
        QCOMPARE(r2.value().member, 13);
        std::optional<Struct> r3 = Utils::take(v, [](const Struct &s) { return s.member == 13; });
        QVERIFY(!static_cast<bool>(r3));

        std::optional<Struct> r4 = Utils::take(v, &Struct::isEven);
        QVERIFY(static_cast<bool>(r4));
        QCOMPARE(r4.value().member, 6);
    }
    {
        QList<Struct> v {0, 0, 0, 0, 0, 0, 1, 2, 3};
        std::optional<Struct> r1 = Utils::take(v, &Struct::member);
        QVERIFY(static_cast<bool>(r1));
        QCOMPARE(r1.value().member, 1);
    }
}

void tst_Algorithm::sorted()
{
    const QList<int> vOrig{4, 3, 6, 5, 8};
    const QList<int> vExpected{3, 4, 5, 6, 8};

    // plain
    {
        // non-const lvalue
        QList<int> vncl = vOrig;
        const QList<int> rncl = Utils::sorted(vncl);
        QCOMPARE(rncl, vExpected);
        QCOMPARE(vncl, vOrig); // was not modified

        // const lvalue
        const QList<int> rcl = Utils::sorted(vOrig);
        QCOMPARE(rcl, vExpected);

        // non-const rvalue
        const auto vncr = [vOrig]() -> QList<int> { return vOrig; };
        const QList<int> rncr = Utils::sorted(vncr());
        QCOMPARE(rncr, vExpected);

        // const rvalue
        const auto vcr = [vOrig]() -> const QList<int> { return vOrig; };
        const QList<int> rcr = Utils::sorted(vcr());
        QCOMPARE(rcr, vExpected);
    }

    // predicate
    {
        // non-const lvalue
        QList<int> vncl = vOrig;
        const QList<int> rncl = Utils::sorted(vncl, [](int a, int b) { return a < b; });
        QCOMPARE(rncl, vExpected);
        QCOMPARE(vncl, vOrig); // was not modified

        // const lvalue
        const QList<int> rcl = Utils::sorted(vOrig, [](int a, int b) { return a < b; });
        QCOMPARE(rcl, vExpected);

        // non-const rvalue
        const auto vncr = [vOrig]() -> QList<int> { return vOrig; };
        const QList<int> rncr = Utils::sorted(vncr(), [](int a, int b) { return a < b; });
        QCOMPARE(rncr, vExpected);

        // const rvalue
        const auto vcr = [vOrig]() -> const QList<int> { return vOrig; };
        const QList<int> rcr = Utils::sorted(vcr(), [](int a, int b) { return a < b; });
        QCOMPARE(rcr, vExpected);
    }

    const QList<Struct> mvOrig({4, 3, 2, 1});
    const QList<Struct> mvExpected({1, 2, 3, 4});
    // member
    {
        // non-const lvalue
        QList<Struct> mvncl = mvOrig;
        const QList<Struct> rncl = Utils::sorted(mvncl, &Struct::member);
        QCOMPARE(rncl, mvExpected);
        QCOMPARE(mvncl, mvOrig); // was not modified

        // const lvalue
        const QList<Struct> rcl = Utils::sorted(mvOrig, &Struct::member);
        QCOMPARE(rcl, mvExpected);

        // non-const rvalue
        const auto vncr = [mvOrig]() -> QList<Struct> { return mvOrig; };
        const QList<Struct> rncr = Utils::sorted(vncr(), &Struct::member);
        QCOMPARE(rncr, mvExpected);

        // const rvalue
        const auto vcr = [mvOrig]() -> const QList<Struct> { return mvOrig; };
        const QList<Struct> rcr = Utils::sorted(vcr(), &Struct::member);
        QCOMPARE(rcr, mvExpected);
    }

    // member function
    {
        // non-const lvalue
        QList<Struct> mvncl = mvOrig;
        const QList<Struct> rncl = Utils::sorted(mvncl, &Struct::getMember);
        QCOMPARE(rncl, mvExpected);
        QCOMPARE(mvncl, mvOrig); // was not modified

        // const lvalue
        const QList<Struct> rcl = Utils::sorted(mvOrig, &Struct::getMember);
        QCOMPARE(rcl, mvExpected);

        // non-const rvalue
        const auto vncr = [mvOrig]() -> QList<Struct> { return mvOrig; };
        const QList<Struct> rncr = Utils::sorted(vncr(), &Struct::getMember);
        QCOMPARE(rncr, mvExpected);

        // const rvalue
        const auto vcr = [mvOrig]() -> const QList<Struct> { return mvOrig; };
        const QList<Struct> rcr = Utils::sorted(vcr(), &Struct::getMember);
        QCOMPARE(rcr, mvExpected);
    }
}

QTEST_GUILESS_MAIN(tst_Algorithm)

#include "tst_algorithm.moc"
