// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QSemaphore>
#include <QTest>
#include <QThread>

#include <projectexplorer/gcctoolchain.h>

#include <atomic>
#include <memory>
#include <vector>

//////////////// the actual autotest

class tst_ToolchainCache : public QObject
{
    Q_OBJECT

private slots:
    void insertOne();
    void insertOneOne();
    void insertOneTwo();
    void insertOneTwoThree();
    void insertOneTwoOneThree();
    void valueOfACachedKey();
    void valueOfAKeyAskedForAtTheSameTime();
    void valueAfterInvalidate();
};


void tst_ToolchainCache::insertOne()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(*cache.check(key1), value1);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolchainCache::insertOneOne()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key1, value1);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(*cache.check(key1), value1);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolchainCache::insertOneTwo()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    const QStringList key2 = {"two"};
    const QString value2 = "value2";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key2, value2);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(*cache.check(key1), value1);
    QVERIFY(bool(cache.check(key2)));
    QCOMPARE(*cache.check(key2), value2);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolchainCache::insertOneTwoThree()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    const QStringList key2 = {"two"};
    const QString value2 = "value2";
    const QStringList key3 = {"three"};
    const QString value3 = "value3";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key2, value2);
    cache.insert(key3, value3);

    QVERIFY(!cache.check(key1)); // key1 was evicted
    QVERIFY(bool(cache.check(key2)));
    QCOMPARE(*cache.check(key2), value2);
    QVERIFY(bool(cache.check(key3)));
    QCOMPARE(*cache.check(key3), value3);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolchainCache::insertOneTwoOneThree()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    const QStringList key2 = {"two"};
    const QString value2 = "value2";
    const QStringList key3 = {"three"};
    const QString value3 = "value3";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key2, value2);
    cache.insert(key1, value1);
    cache.insert(key3, value3);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(*cache.check(key1), value1);
    QVERIFY(!cache.check(key2)); // key2 was evicted
    QVERIFY(bool(cache.check(key3)));
    QCOMPARE(*cache.check(key3), value3);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolchainCache::valueOfACachedKey()
{
    const QStringList key1 = {"one"};
    ProjectExplorer::Cache<QStringList, QString, 2> cache;
    int computed = 0;
    const auto compute = [&computed] {
        ++computed;
        return QString("value1");
    };

    QCOMPARE(cache.value(key1, compute), "value1");
    QCOMPARE(cache.value(key1, compute), "value1");

    QCOMPARE(computed, 1);
    QCOMPARE(*cache.check(key1), "value1");
}

void tst_ToolchainCache::valueOfAKeyAskedForAtTheSameTime()
{
    const QStringList key1 = {"one"};
    ProjectExplorer::Cache<QStringList, QString, 2> cache;
    std::atomic_int computed = 0;
    const auto compute = [&computed] {
        ++computed;
        // Long enough for the other threads to ask while this one computes.
        QThread::msleep(200);
        return QString("value1");
    };

    const int threadCount = 4;
    std::vector<QString> results(threadCount);
    std::vector<std::unique_ptr<QThread>> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(QThread::create([&cache, &key1, &compute, &results, i] {
            results[i] = cache.value(key1, compute);
        }));
        threads.back()->start();
    }
    for (const std::unique_ptr<QThread> &thread : threads)
        QVERIFY(thread->wait(10000));

    QCOMPARE(computed.load(), 1);
    for (const QString &result : results)
        QCOMPARE(result, "value1");
}

// What an older computation gives is not for those that ask after the cache was invalidated.
void tst_ToolchainCache::valueAfterInvalidate()
{
    const QStringList key = {"key"};
    ProjectExplorer::Cache<QStringList, QString, 2> cache;
    QSemaphore started;
    QSemaphore release;

    QString first;
    std::unique_ptr<QThread> thread(QThread::create([&] {
        first = cache.value(key, [&] {
            started.release();
            release.acquire();
            return QString("old");
        });
    }));
    thread->start();
    started.acquire();

    cache.invalidate();
    QCOMPARE(cache.value(key, [] { return QString("new"); }), QString("new"));

    release.release();
    thread->wait();
    QCOMPARE(first, QString("old"));
    QVERIFY(cache.check(key));
    QCOMPARE(*cache.check(key), QString("new"));
}

QTEST_GUILESS_MAIN(tst_ToolchainCache)
#include "tst_toolchaincache.moc"
