// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tasking/concurrentcall.h>

#include <QtTest>

using namespace Tasking;

using namespace std::chrono;
using namespace std::chrono_literals;

class MyObject
{
public:
    static void staticMember(QPromise<double> &promise, int n)
    {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    }

    void member(QPromise<double> &promise, int n) const
    {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    }
};

class tst_ConcurrentCall : public QObject
{
    Q_OBJECT

private slots:
    // void futureSynchonizer();
    void taskTree_data();
    void taskTree();

private:
    QThreadPool m_threadPool;
    MyObject m_myObject;
};

struct TestData
{
    TreeStorage<bool> storage;
    Group root;
};

void report3(QPromise<int> &promise)
{
    promise.addResult(0);
    promise.addResult(2);
    promise.addResult(1);
}

static void staticReport3(QPromise<int> &promise)
{
    promise.addResult(0);
    promise.addResult(2);
    promise.addResult(1);
}

void reportN(QPromise<double> &promise, int n)
{
    for (int i = 0; i < n; ++i)
        promise.addResult(0);
}

static void staticReportN(QPromise<double> &promise, int n)
{
    for (int i = 0; i < n; ++i)
        promise.addResult(0);
}

class Functor {
public:
    void operator()(QPromise<double> &promise, int n) const
    {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    }
};

// void tst_ConcurrentCall::futureSynchonizer()
// {
//     auto lambda = [](QPromise<int> &promise) {
//         while (true) {
//             if (promise.isCanceled()) {
//                 promise.future().cancel();
//                 promise.finish();
//                 return;
//             }
//             QThread::msleep(100);
//         }
//     };

//     FutureSynchronizer synchronizer;
//     synchronizer.setCancelOnWait(false);
//     {
//         Async<int> task;
//         task.setConcurrentCallData(lambda);
//         task.setFutureSynchronizer(&synchronizer);
//         task.start();
//         QThread::msleep(10);
//         // We assume here that worker thread will still work for about 90 ms.
//         QVERIFY(!task.isCanceled());
//         QVERIFY(!task.isDone());
//     }
//     synchronizer.flushFinishedFutures();
//     QVERIFY(!synchronizer.isEmpty());
//     // The destructor of synchronizer should wait for about 90 ms for worker thread to be canceled
// }

void multiplyBy2(QPromise<int> &promise, int input) { promise.addResult(input * 2); }

template <typename...>
struct FutureArgType;

template <typename Arg>
struct FutureArgType<QFuture<Arg>>
{
    using Type = Arg;
};

template <typename...>
struct ConcurrentResultType;

template<typename Function, typename ...Args>
struct ConcurrentResultType<Function, Args...>
{
    using Type = typename FutureArgType<decltype(QtConcurrent::run(
        std::declval<Function>(), std::declval<Args>()...))>::Type;
};

template <typename Function, typename ...Args,
          typename ResultType = typename ConcurrentResultType<Function, Args...>::Type>
TestData createTestData(const QList<ResultType> &expectedResults, Function &&function, Args &&...args)
{
    TreeStorage<bool> storage;

    const auto onSetup = [=](ConcurrentCall<ResultType> &task) {
        task.setConcurrentCallData(function, args...);
    };
    const auto onDone = [storage, expectedResults](const ConcurrentCall<ResultType> &task) {
        *storage = task.results() == expectedResults;
    };

    const Group root {
        storage,
        ConcurrentCallTask<ResultType>(onSetup, onDone)
    };

    return TestData{storage, root};
}

void tst_ConcurrentCall::taskTree_data()
{
    QTest::addColumn<TestData>("testData");

    const QList<int> report3Result{0, 2, 1};
    const QList<double> reportNResult{0, 0};

    auto lambda = [](QPromise<double> &promise, int n) {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    };
    const std::function<void(QPromise<double> &, int)> fun = [](QPromise<double> &promise, int n) {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    };

    QTest::newRow("RefGlobalNoArgs")
        << createTestData(report3Result, &report3);
    QTest::newRow("GlobalNoArgs")
        << createTestData(report3Result, report3);
    QTest::newRow("RefStaticNoArgs")
        << createTestData(report3Result, &staticReport3);
    QTest::newRow("StaticNoArgs")
        << createTestData(report3Result, staticReport3);
    QTest::newRow("RefGlobalIntArg")
        << createTestData(reportNResult, &reportN, 2);
    QTest::newRow("GlobalIntArg")
        << createTestData(reportNResult, reportN, 2);
    QTest::newRow("RefStaticIntArg")
        << createTestData(reportNResult, &staticReportN, 2);
    QTest::newRow("StaticIntArg")
        << createTestData(reportNResult, staticReportN, 2);
    QTest::newRow("Lambda")
        << createTestData(reportNResult, lambda, 2);
    QTest::newRow("Function")
        << createTestData(reportNResult, fun, 2);
    QTest::newRow("Functor")
        << createTestData(reportNResult, Functor(), 2);
    QTest::newRow("StaticMemberFunction")
        << createTestData(reportNResult, &MyObject::staticMember, 2);
    QTest::newRow("MemberFunction")
        << createTestData(reportNResult, &MyObject::member, &m_myObject, 2);

    {
        TreeStorage<bool> storage;
        TreeStorage<int> internalStorage;

        const auto onSetup = [internalStorage](ConcurrentCall<int> &task) {
            task.setConcurrentCallData(multiplyBy2, *internalStorage);
        };
        const auto onDone = [internalStorage](const ConcurrentCall<int> &task) {
            *internalStorage = task.result();
        };

        const Group root {
            storage,
            internalStorage,
            onGroupSetup([internalStorage] { *internalStorage = 1; }),
            ConcurrentCallTask<int>(onSetup, onDone, CallDoneIf::Success),
            ConcurrentCallTask<int>(onSetup, onDone, CallDoneIf::Success),
            ConcurrentCallTask<int>(onSetup, onDone, CallDoneIf::Success),
            ConcurrentCallTask<int>(onSetup, onDone, CallDoneIf::Success),
            onGroupDone([storage, internalStorage] { *storage = *internalStorage == 16; })
        };

        QTest::newRow("Sequential") << TestData{storage, root};
    }
}

void tst_ConcurrentCall::taskTree()
{
    QFETCH(TestData, testData);

    TaskTree taskTree({testData.root.withTimeout(1000ms)});
    bool actualResult = false;
    const auto collectResult = [&actualResult](const bool &storage) {
        actualResult = storage;
    };
    taskTree.onStorageDone(testData.storage, collectResult);
    const DoneWith result = taskTree.runBlocking();
    QCOMPARE(taskTree.isRunning(), false);
    QCOMPARE(result, DoneWith::Success);
    QVERIFY(actualResult);
}

QTEST_GUILESS_MAIN(tst_ConcurrentCall)

#include "tst_concurrentcall.moc"
