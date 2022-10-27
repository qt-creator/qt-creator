// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "utils/asynctask.h"

#include <QtTest>

using namespace Utils;

class tst_AsyncTask : public QObject
{
    Q_OBJECT

private slots:
    void runAsync();
    void crefFunction();
    void futureSynchonizer();
};

void report3(QFutureInterface<int> &fi)
{
    fi.reportResults({0, 2, 1});
}

void reportN(QFutureInterface<double> &fi, int n)
{
    fi.reportResults(QVector<double>(n, 0));
}

void reportString1(QFutureInterface<QString> &fi, const QString &s)
{
    fi.reportResult(s);
}

void reportString2(QFutureInterface<QString> &fi, QString s)
{
    fi.reportResult(s);
}

class Callable {
public:
    void operator()(QFutureInterface<double> &fi, int n) const
    {
        fi.reportResults(QVector<double>(n, 0));
    }
};

class MyObject {
public:
    static void staticMember0(QFutureInterface<double> &fi)
    {
        fi.reportResults({0, 2, 1});
    }

    static void staticMember1(QFutureInterface<double> &fi, int n)
    {
        fi.reportResults(QVector<double>(n, 0));
    }

    void member0(QFutureInterface<double> &fi) const
    {
        fi.reportResults({0, 2, 1});
    }

    void member1(QFutureInterface<double> &fi, int n) const
    {
        fi.reportResults(QVector<double>(n, 0));
    }

    void memberString1(QFutureInterface<QString> &fi, const QString &s) const
    {
        fi.reportResult(s);
    }

    void memberString2(QFutureInterface<QString> &fi, QString s) const
    {
        fi.reportResult(s);
    }

    void nonConstMember(QFutureInterface<double> &fi)
    {
        fi.reportResults({0, 2, 1});
    }
};

template <typename Function, typename ...Args,
          typename ResultType = typename Internal::resultType<Function>::type>
std::shared_ptr<AsyncTask<ResultType>> createAsyncTask(const Function &function, const Args &...args)
{
    auto asyncTask = std::make_shared<AsyncTask<ResultType>>();
    asyncTask->setAsyncCallData(function, args...);
    asyncTask->start();
    return asyncTask;
}

void tst_AsyncTask::runAsync()
{
    // free function pointer
    QCOMPARE(createAsyncTask(&report3)->results(),
             QList<int>({0, 2, 1}));
    QCOMPARE(createAsyncTask(report3)->results(),
             QList<int>({0, 2, 1}));

    QCOMPARE(createAsyncTask(reportN, 4)->results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(createAsyncTask(reportN, 2)->results(),
             QList<double>({0, 0}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(createAsyncTask(reportString1, s)->results(),
             QList<QString>({s}));
    QCOMPARE(createAsyncTask(reportString1, crs)->results(),
             QList<QString>({crs}));
    QCOMPARE(createAsyncTask(reportString1, cs)->results(),
             QList<QString>({cs}));
    QCOMPARE(createAsyncTask(reportString1, QString(QLatin1String("rvalue")))->results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    QCOMPARE(createAsyncTask(reportString2, s)->results(),
             QList<QString>({s}));
    QCOMPARE(createAsyncTask(reportString2, crs)->results(),
             QList<QString>({crs}));
    QCOMPARE(createAsyncTask(reportString2, cs)->results(),
             QList<QString>({cs}));
    QCOMPARE(createAsyncTask(reportString2, QString(QLatin1String("rvalue")))->results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(createAsyncTask([](QFutureInterface<double> &fi, int n) {
                 fi.reportResults(QVector<double>(n, 0));
             }, 3)->results(),
             QList<double>({0, 0, 0}));

    // std::function
    const std::function<void(QFutureInterface<double>&,int)> fun = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(createAsyncTask(fun, 2)->results(),
             QList<double>({0, 0}));

    // operator()
    QCOMPARE(createAsyncTask(Callable(), 3)->results(),
             QList<double>({0, 0, 0}));
    const Callable c{};
    QCOMPARE(createAsyncTask(c, 2)->results(),
             QList<double>({0, 0}));

    // static member functions
    QCOMPARE(createAsyncTask(&MyObject::staticMember0)->results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(createAsyncTask(&MyObject::staticMember1, 2)->results(),
             QList<double>({0, 0}));

    // member functions
    const MyObject obj{};
    QCOMPARE(createAsyncTask(&MyObject::member0, &obj)->results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(createAsyncTask(&MyObject::member1, &obj, 4)->results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(createAsyncTask(&MyObject::memberString1, &obj, s)->results(),
             QList<QString>({s}));
    QCOMPARE(createAsyncTask(&MyObject::memberString1, &obj, crs)->results(),
             QList<QString>({crs}));
    QCOMPARE(createAsyncTask(&MyObject::memberString1, &obj, cs)->results(),
             QList<QString>({cs}));
    QCOMPARE(createAsyncTask(&MyObject::memberString1, &obj, QString(QLatin1String("rvalue")))->results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(createAsyncTask(&MyObject::memberString2, &obj, s)->results(),
             QList<QString>({s}));
    QCOMPARE(createAsyncTask(&MyObject::memberString2, &obj, crs)->results(),
             QList<QString>({crs}));
    QCOMPARE(createAsyncTask(&MyObject::memberString2, &obj, cs)->results(),
             QList<QString>({cs}));
    QCOMPARE(createAsyncTask(&MyObject::memberString2, &obj, QString(QLatin1String("rvalue")))->results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    MyObject nonConstObj{};
    QCOMPARE(createAsyncTask(&MyObject::nonConstMember, &nonConstObj)->results(),
             QList<double>({0, 2, 1}));
}

void tst_AsyncTask::crefFunction()
{
    // free function pointer with future interface
    auto fun = &report3;
    QCOMPARE(createAsyncTask(std::cref(fun))->results(),
             QList<int>({0, 2, 1}));

    // lambda with future interface
    auto lambda = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(createAsyncTask(std::cref(lambda), 3)->results(),
             QList<double>({0, 0, 0}));

    // std::function with future interface
    const std::function<void(QFutureInterface<double>&,int)> funObj = [](QFutureInterface<double> &fi, int n) {
        fi.reportResults(QVector<double>(n, 0));
    };
    QCOMPARE(createAsyncTask(std::cref(funObj), 2)->results(),
             QList<double>({0, 0}));

    // callable with future interface
    const Callable c{};
    QCOMPARE(createAsyncTask(std::cref(c), 2)->results(),
             QList<double>({0, 0}));

    // member functions with future interface
    auto member = &MyObject::member0;
    const MyObject obj{};
    QCOMPARE(createAsyncTask(std::cref(member), &obj)->results(),
             QList<double>({0, 2, 1}));
}

template <typename Function, typename ...Args,
          typename ResultType = typename Internal::resultType<Function>::type>
typename AsyncTask<ResultType>::StartHandler startHandler(const Function &function, const Args &...args)
{
    return [=] { return Utils::runAsync(function, args...); };
}

void tst_AsyncTask::futureSynchonizer()
{
    auto lambda = [](QFutureInterface<int> &fi) {
        while (true) {
            if (fi.isCanceled()) {
                fi.reportCanceled();
                fi.reportFinished();
                return;
            }
            QThread::msleep(100);
        }
    };

    FutureSynchronizer synchronizer;
    {
        AsyncTask<int> task;
        task.setAsyncCallData(lambda);
        task.setFutureSynchronizer(&synchronizer);
        task.start();
        QThread::msleep(10);
        // We assume here that worker thread will still work for about 90 ms.
        QVERIFY(!task.isCanceled());
        QVERIFY(!task.isDone());
    }
    synchronizer.flushFinishedFutures();
    QVERIFY(!synchronizer.isEmpty());
    // The destructor of synchronizer should wait for about 90 ms for worker thread to be canceled
}

QTEST_GUILESS_MAIN(tst_AsyncTask)

#include "tst_asynctask.moc"
