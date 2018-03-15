/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <valarray>

// must get included after the containers above or gcc4.9 will have a problem using
// initializer_list related code on the templates inside algorithm.h
#include <utils/pointeralgorithm.h>

class tst_PointerAlgorithm : public QObject
{
    Q_OBJECT

private slots:
    void anyOf();
    void count();
    void contains();
    void findOr();
    void findOrDefault();
    void toRawPointer();
    void toReferences();
    void take();
    void takeOrDefault();
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

void tst_PointerAlgorithm::anyOf()
{
    {
        std::vector<std::unique_ptr<int>> vector;
        vector.emplace_back(std::make_unique<int>(5));
        vector.emplace_back(std::make_unique<int>(6));
        vector.emplace_back(std::make_unique<int>(7));
        vector.emplace_back(std::unique_ptr<int>());
        std::vector<int *> ptrVector = Utils::toRawPointer(vector);

        QVERIFY(Utils::anyOf(vector, ptrVector.at(0)));
        int foo = 42;
        QVERIFY(!Utils::anyOf(vector, &foo));
        QVERIFY(Utils::anyOf(vector, nullptr));
    }
    {
        std::vector<std::unique_ptr<int>> vector;
        vector.emplace_back(std::make_unique<int>(5));
        vector.emplace_back(std::make_unique<int>(6));
        vector.emplace_back(std::make_unique<int>(7));
        std::vector<int *> ptrVector = Utils::toRawPointer(vector);

        QVERIFY(!Utils::anyOf(vector, nullptr));
    }
}

void tst_PointerAlgorithm::count()
{
    std::vector<std::unique_ptr<int>> vector;
    vector.emplace_back(std::make_unique<int>(5));
    vector.emplace_back(std::unique_ptr<int>());
    vector.emplace_back(std::make_unique<int>(6));
    vector.emplace_back(std::make_unique<int>(7));
    vector.emplace_back(std::unique_ptr<int>());
    std::vector<int *> ptrVector = Utils::toRawPointer(vector);

    QCOMPARE(Utils::count(vector, ptrVector.at(0)), 1);
    int foo = 42;
    QCOMPARE(Utils::count(vector, &foo), 0);
    QCOMPARE(Utils::count(vector, nullptr), 2);
}

void tst_PointerAlgorithm::contains()
{
    std::vector<std::unique_ptr<int>> vector;
    vector.emplace_back(std::make_unique<int>(5));
    vector.emplace_back(std::make_unique<int>(6));
    vector.emplace_back(std::make_unique<int>(7));
    vector.emplace_back(std::unique_ptr<int>());
    std::vector<int *> ptrVector = Utils::toRawPointer(vector);

    QVERIFY(Utils::contains(vector, ptrVector.at(0)));
    int foo = 42;
    QVERIFY(!Utils::contains(vector, &foo));
    QVERIFY(Utils::contains(vector, nullptr));
}

void tst_PointerAlgorithm::findOr()
{
    {
        std::vector<std::unique_ptr<int>> vector;
        vector.emplace_back(std::make_unique<int>(5));
        vector.emplace_back(std::make_unique<int>(2));
        vector.emplace_back(std::make_unique<int>(6));
        vector.emplace_back(std::make_unique<int>(7));
        vector.emplace_back(std::unique_ptr<int>());
        std::vector<int *> ptrVector = Utils::toRawPointer(vector);

        int foo = 42;
        int bar = 23;
        QVERIFY(Utils::findOr(vector, &foo, ptrVector.at(0)) == ptrVector.at(0));
        QVERIFY(Utils::findOr(vector, nullptr, &foo) == nullptr);
        QVERIFY(Utils::findOr(vector, &foo, nullptr) == nullptr);
        QVERIFY(Utils::findOr(vector, &foo, &bar) == &foo);

        QCOMPARE(Utils::findOr(vector, &foo,
                               [](const std::unique_ptr<int> &ip) { return ip && *ip == 2; }),
                 ptrVector.at(1));
        QCOMPARE(Utils::findOr(vector, &foo,
                               [](const std::unique_ptr<int> &ip) { return ip && *ip == 43; }),
                 &foo);
    }
    {
        std::vector<std::unique_ptr<Struct>> v3;
        v3.emplace_back(std::make_unique<Struct>(1));
        v3.emplace_back(std::make_unique<Struct>(3));
        v3.emplace_back(std::make_unique<Struct>(5));
        v3.emplace_back(std::make_unique<Struct>(7));
        Struct defS(6);
        QCOMPARE(Utils::findOr(v3, &defS, &Struct::isOdd), v3.at(0).get());
        QCOMPARE(Utils::findOr(v3, &defS, &Struct::isEven), &defS);
    }
    {
        std::vector<std::shared_ptr<Struct>> v4;
        v4.emplace_back(std::make_shared<Struct>(1));
        v4.emplace_back(std::make_shared<Struct>(3));
        v4.emplace_back(std::make_shared<Struct>(5));
        v4.emplace_back(std::make_shared<Struct>(7));
        std::shared_ptr<Struct> sharedDefS = std::make_shared<Struct>(6);
        QCOMPARE(Utils::findOr(v4, sharedDefS, &Struct::isOdd), v4.at(0));
        QCOMPARE(Utils::findOr(v4, sharedDefS, &Struct::isEven), sharedDefS);
    }
}

void tst_PointerAlgorithm::findOrDefault()
{
    {
        std::vector<std::unique_ptr<int>> vector;
        vector.emplace_back(std::make_unique<int>(5));
        vector.emplace_back(std::make_unique<int>(6));
        vector.emplace_back(std::make_unique<int>(7));
        vector.emplace_back(std::unique_ptr<int>());
        std::vector<int *> ptrVector = Utils::toRawPointer(vector);

        int foo = 42;
        QVERIFY(Utils::findOrDefault(vector, ptrVector.at(0)) == ptrVector.at(0));
        QVERIFY(Utils::findOrDefault(vector, &foo) == nullptr);
    }
    {
        std::vector<std::unique_ptr<int>> v2;
        v2.emplace_back(std::make_unique<int>(1));
        v2.emplace_back(std::make_unique<int>(2));
        v2.emplace_back(std::make_unique<int>(3));
        v2.emplace_back(std::make_unique<int>(4));
        QCOMPARE(Utils::findOrDefault(v2, [](const std::unique_ptr<int> &ip) { return *ip == 2; }), v2.at(1).get());
        QCOMPARE(Utils::findOrDefault(v2, [](const std::unique_ptr<int> &ip) { return *ip == 5; }), static_cast<int*>(nullptr));
    }
    {
        std::vector<std::unique_ptr<Struct>> v3;
        v3.emplace_back(std::make_unique<Struct>(1));
        v3.emplace_back(std::make_unique<Struct>(3));
        v3.emplace_back(std::make_unique<Struct>(5));
        v3.emplace_back(std::make_unique<Struct>(7));
        QCOMPARE(Utils::findOrDefault(v3, &Struct::isOdd), v3.at(0).get());
        QCOMPARE(Utils::findOrDefault(v3, &Struct::isEven), static_cast<Struct*>(nullptr));
    }
}

void tst_PointerAlgorithm::toRawPointer()
{
    const std::vector<std::unique_ptr<Struct>> v;

    // same result container
    const std::vector<Struct *> x1 = Utils::toRawPointer(v);
    // different result container
    const std::vector<Struct *> x2 = Utils::toRawPointer<std::vector>(v);
    const QVector<Struct *> x3 = Utils::toRawPointer<QVector>(v);
    const std::list<Struct *> x4 = Utils::toRawPointer<std::list>(v);
    // different fully specified result container
    const std::vector<BaseStruct *> x5 = Utils::toRawPointer<std::vector<BaseStruct *>>(v);
    const QVector<BaseStruct *> x6 = Utils::toRawPointer<QVector<BaseStruct *>>(v);
}

void tst_PointerAlgorithm::toReferences()
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

void tst_PointerAlgorithm::take()
{
    {
        std::vector<std::unique_ptr<int>> vector;
        vector.emplace_back(std::make_unique<int>(5));
        vector.emplace_back(std::make_unique<int>(2));
        vector.emplace_back(std::make_unique<int>(6));
        vector.emplace_back(std::make_unique<int>(7));
        vector.emplace_back(std::unique_ptr<int>());
        std::vector<int *> ptrVector = Utils::toRawPointer(vector);
        int foo = 42;

        QVERIFY(Utils::take(vector, ptrVector.at(0)).value().get() == ptrVector.at(0));
        QVERIFY(Utils::take(vector, ptrVector.at(0)) == Utils::nullopt);
        QVERIFY(Utils::take(vector, &foo) == Utils::nullopt);
        QVERIFY(Utils::take(vector, nullptr).value().get() == nullptr);
    }
}

void tst_PointerAlgorithm::takeOrDefault()
{
    {
        std::vector<std::unique_ptr<int>> vector;
        vector.emplace_back(std::make_unique<int>(5));
        vector.emplace_back(std::make_unique<int>(2));
        vector.emplace_back(std::make_unique<int>(6));
        vector.emplace_back(std::make_unique<int>(7));
        vector.emplace_back(std::unique_ptr<int>());
        std::vector<int *> ptrVector = Utils::toRawPointer(vector);
        int foo = 42;

        QVERIFY(Utils::takeOrDefault(vector, ptrVector.at(0)).get() == ptrVector.at(0));
        QVERIFY(Utils::takeOrDefault(vector, ptrVector.at(0)).get() == nullptr);
        QVERIFY(Utils::takeOrDefault(vector, &foo).get() == nullptr);
        QVERIFY(Utils::takeOrDefault(vector, nullptr).get() == nullptr);
    }
}

QTEST_MAIN(tst_PointerAlgorithm)

#include "tst_pointeralgorithm.moc"
