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
#include <utils/mapreduce.h>

#include <QtTest>

#if !defined(Q_CC_MSVC) || _MSC_VER >= 1900 // MSVC2015
#define SUPPORTS_MOVE
#endif

class tst_MapReduce : public QObject
{
    Q_OBJECT

private slots:
    void mapReduce();
    void mapReduceRvalueContainer();
    void map();
    void orderedMapReduce();
#ifdef SUPPORTS_MOVE
    void moveOnlyType();
#endif
};

static int returnxx(int x)
{
    return x*x;
}

static void returnxxThroughFutureInterface(QFutureInterface<int> &fi, int x)
{
    fi.reportResult(x*x);
}

void tst_MapReduce::mapReduce()
{
    const auto initWithFutureInterface = [](QFutureInterface<double> &fi) -> double {
        fi.reportResult(0.);
        return 0.;
    };
    const auto reduceWithFutureInterface = [](QFutureInterface<double> &fi, double &state, int value) {
        state += value;
        fi.reportResult(value);
    };
    const auto reduceWithReturn = [](double &state, int value) -> double {
        state += value;
        return value;
    };
    const auto cleanupWithFutureInterface = [](QFutureInterface<double> &fi, double &state) {
        state /= 2.;
        fi.reportResult(state);
    };

    {
        QList<double> results = Utils::mapReduce(QList<int>({1, 2, 3, 4, 5}),
                                                 initWithFutureInterface, returnxx,
                                                 reduceWithFutureInterface, cleanupWithFutureInterface)
                .results();
        Utils::sort(results); // mapping order is undefined
        QCOMPARE(results, QList<double>({0., 1., 4., 9., 16., 25., 27.5}));
    }
    {
        QList<double> results = Utils::mapReduce(QList<int>({1, 2, 3, 4, 5}),
                                                 initWithFutureInterface, returnxxThroughFutureInterface,
                                                 reduceWithFutureInterface, cleanupWithFutureInterface)
                .results();
        Utils::sort(results); // mapping order is undefined
        QCOMPARE(results, QList<double>({0., 1., 4., 9., 16., 25., 27.5}));
    }
    {
        QList<double> results = Utils::mapReduce(QList<int>({1, 2, 3, 4, 5}),
                                                 initWithFutureInterface, returnxx,
                                                 reduceWithReturn, cleanupWithFutureInterface)
                .results();
        Utils::sort(results); // mapping order is undefined
        QCOMPARE(results, QList<double>({0., 1., 4., 9., 16., 25., 27.5}));
    }
    {
        // lvalue ref container
        QList<int> container({1, 2, 3, 4, 5});
        QList<double> results = Utils::mapReduce(container,
                                                 initWithFutureInterface, returnxx,
                                                 reduceWithReturn, cleanupWithFutureInterface)
                .results();
        Utils::sort(results); // mapping order is undefined
        QCOMPARE(results, QList<double>({0., 1., 4., 9., 16., 25., 27.5}));
    }
    {
        // init and cleanup without future interface
        QCOMPARE(Utils::mapReduce(QList<int>({1, 2, 3}),
                                  []() { return 0.; },
                                  [](int v) { return v*2; },
                                  [](double &state, int v) { return state += v/4.; },
                                  [](double &) { },
                                  Utils::MapReduceOption::Ordered).results(),
                 QList<double>({.5, 1.5, 3.}));
    }
}

void tst_MapReduce::mapReduceRvalueContainer()
{
    {
        QFuture<int> future = Utils::mapReduce(QList<int>({1, 2, 3, 4, 5}),
                                                 []() { return 0; },
                                                 [](int value) { return value; },
                                                 [](QFutureInterface<int> &, int &state, int value) { state += value; },
                                                 [](QFutureInterface<int> &fi, int &state) { fi.reportResult(state); });
        // here, lifetime of the QList temporary ends
        QCOMPARE(future.results(), QList<int>({15}));
    }
}

void tst_MapReduce::map()
{
    QCOMPARE(Utils::map(QList<int>({2, 5, 1}), [](int x) { return x*2.5; }).results(),
             QList<double>({5., 12.5, 2.5}));
    {
        // void result
        QList<int> results;
        QMutex mutex;
        Utils::map(
                    // container
                    QList<int>({2, 5, 1}),
                    // map
                    [&mutex, &results](int x) { QMutexLocker l(&mutex); results.append(x); }
                    ).waitForFinished();
        // Utils::map is "ordered" by default, but that means that result reporting is ordered,
        // the map function is still called out-of-order
        qSort(results);
        QCOMPARE(results, QList<int>({1, 2, 5}));
    }
    {
        // inplace editing
        QList<int> container({2, 5, 1});
        Utils::map(std::ref(container), [](int &x) { x *= 2; }).waitForFinished();
        QCOMPARE(container, QList<int>({4, 10, 2}));
    }
}

void tst_MapReduce::orderedMapReduce()
{
    QCOMPARE(Utils::mapReduce(QList<int>({1, 2, 3, 4}),
                              []() { return 0; },
                              [](int i) { return i*2; },
                              [](int &state, int val) { state += val; return state; },
                              [](int &) { },
                              Utils::MapReduceOption::Ordered).results(),
             QList<int>({2, 6, 12, 20}));
}

#ifdef SUPPORTS_MOVE

class MoveOnlyType
{
public:
    MoveOnlyType() noexcept {} // <- with GCC 5 the defaulted one is noexcept(false)
    MoveOnlyType(const MoveOnlyType &) = delete;
    MoveOnlyType(MoveOnlyType &&) = default;
    MoveOnlyType &operator=(const MoveOnlyType &) = delete;
    MoveOnlyType &operator=(MoveOnlyType &&) = default;
};

class MoveOnlyState : public MoveOnlyType
{
public:
    int count = 0;
};

class MoveOnlyInit : public MoveOnlyType
{
public:
    MoveOnlyState operator()(QFutureInterface<int> &) const { return MoveOnlyState(); }
};

class MoveOnlyMap : public MoveOnlyType
{
public:
    int operator()(const MoveOnlyType &) const { return 1; }
};

class MoveOnlyReduce : public MoveOnlyType
{
public:
    void operator()(QFutureInterface<int> &, MoveOnlyState &state, int) { ++state.count; }
};

class MoveOnlyList : public std::vector<MoveOnlyType>
{
public:
    MoveOnlyList() { emplace_back(MoveOnlyType()); emplace_back(MoveOnlyType()); }
    MoveOnlyList(const MoveOnlyList &) = delete;
    MoveOnlyList(MoveOnlyList &&) = default;
    MoveOnlyList &operator=(const MoveOnlyList &) = delete;
    MoveOnlyList &operator=(MoveOnlyList &&) = default;
};

void tst_MapReduce::moveOnlyType()
{
    QCOMPARE(Utils::mapReduce(MoveOnlyList(),
                              MoveOnlyInit(),
                              MoveOnlyMap(),
                              MoveOnlyReduce(),
                              [](QFutureInterface<int> &fi, MoveOnlyState &state) { fi.reportResult(state.count); }
                ).results(),
             QList<int>({2}));
}

#endif

QTEST_MAIN(tst_MapReduce)

#include "tst_mapreduce.moc"
