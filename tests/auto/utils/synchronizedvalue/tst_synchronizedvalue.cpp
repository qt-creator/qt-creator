// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <QString>

#include <utils/synchronizedvalue.h>

namespace Utils {

struct TestData
{
    TestData() = default;
    TestData(int iValue)
        : i(iValue)
    {}

    bool operator==(const TestData &other) const { return i == other.i && str == other.str; }
    bool operator!=(const TestData &other) const { return !(*this == other); }
    QString str;
    int i{20};
};

class tst_synchronizedvalue : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    void read()
    {
        SynchronizedValue<TestData> data;

        data.read([](const auto &d) {
            QCOMPARE(d.str, QString());
            QCOMPARE(d.i, 20);
        });
    }

    void ctor()
    {
        SynchronizedValue<TestData> data(200);
        QCOMPARE(data.get<int>([](const auto &d) { return d.i; }), 200);
    }

    void initializerCtor()
    {
        SynchronizedValue<QList<int>> data({1, 2, 3});
        QCOMPARE(data.get<QList<int>>([](const auto &d) { return d; }), QList<int>({1, 2, 3}));
    }

    void get()
    {
        SynchronizedValue<TestData> data(200);
        QCOMPARE(data.get<int>([](const auto &d) { return d.i; }), 200);
    }

    void constLock()
    {
        SynchronizedValue<TestData> data(200);
        QCOMPARE(data.readLocked()->i, 200);
        QCOMPARE(data.get().i, 200);

        auto lk = data.readLocked();
        QCOMPARE(lk->i, 200);

        // This should fail to compile:
        // data.readLocked()->i = 200;
    }

    void lock()
    {
        SynchronizedValue<TestData> data(123);
        data.writeLocked()->i = 200;
        QCOMPARE(data.get<int>([](const auto &d) { return d.i; }), 200);

        {
            auto wlk = data.writeLocked();
            wlk->str = "Write locked";
        }

        QCOMPARE(data.readLocked()->str, "Write locked");

        SynchronizedValue<QString> lockedStr("Hello World");
        QCOMPARE(*lockedStr.readLocked(), "Hello World");
        *lockedStr.writeLocked() = "Hello World 2";
        QCOMPARE(*lockedStr.readLocked(), "Hello World 2");
    }

    void trivialGet()
    {
        SynchronizedValue<TestData> data(200);
        TestData d = data.get();
        QCOMPARE(data, d);
        QCOMPARE(d.i, 200);
    }

    void equalop()
    {
        SynchronizedValue<TestData> data(200);
        SynchronizedValue<TestData> data2(300);

        data = data2;

        QCOMPARE(data.get<int>([](const auto &d) { return d.i; }), 300);
        QCOMPARE(data2.get<int>([](const auto &d) { return d.i; }), 300);

        TestData dataNoLock(1337);
        data = dataNoLock;

        QCOMPARE(data.get<int>([](const auto &d) { return d.i; }), 1337);
    }

    void compareEq()
    {
        SynchronizedValue<TestData> data(200);
        SynchronizedValue<TestData> data2(300);

        QVERIFY(data != data2);
        QVERIFY(!(data == data2));

        data2.write([](auto &d) { d.i = 200; });

        QVERIFY(data == data2);
        QVERIFY(!(data != data2));
    }

    void operators()
    {
        SynchronizedValue<int> data(200);
        SynchronizedValue<int> data2(300);

        QVERIFY(data < data2);
        QVERIFY(data <= data2);
        QVERIFY(data2 > data);
        QVERIFY(data2 >= data);
        QVERIFY(data2 != data);
        QVERIFY(!(data2 == data));

        QVERIFY(data > 100);
        QVERIFY(data >= 100);
        QVERIFY(data >= 200);
        QVERIFY(data < 300);
        QVERIFY(data <= 300);

        QVERIFY(100 < data);
        QVERIFY(200 <= data);
        QVERIFY(199 <= data);
        QVERIFY(300 > data);
        QVERIFY(200 >= data);
        QVERIFY(201 >= data);
        QVERIFY(200 == data);
        QVERIFY(200 != data2);
    }

    void copyCtor()
    {
        SynchronizedValue<TestData> data(123);
        SynchronizedValue<TestData> data2(data);

        QCOMPARE(data.get<int>([](const auto &d) { return d.i; }), 123);
        QCOMPARE(data2.get<int>([](const auto &d) { return d.i; }), 123);

        TestData dataNoLock(1337);
        SynchronizedValue<TestData> data3(dataNoLock);

        QCOMPARE(data3.get<int>([](const auto &d) { return d.i; }), 1337);
    }

    void multilock()
    {
        SynchronizedValue<int> value(100);

        // Multiple reader, no writer
        {
            QCOMPARE(value.get(), 100);
            auto firstLock = value.readLocked();
            QVERIFY(firstLock.ownsLock());
            auto secondLock = value.readLocked();
            QVERIFY(secondLock.ownsLock());

            QCOMPARE(*firstLock, 100);
            QCOMPARE(*secondLock, 100);

            auto thirdLock = value.writeLocked(std::try_to_lock);
            QVERIFY(!thirdLock.ownsLock());
        }

        // Single writer, no readers
        {
            auto firstLock = value.writeLocked();
            QVERIFY(firstLock.ownsLock());
            auto secondLock = value.writeLocked(std::try_to_lock);
            QVERIFY(!secondLock.ownsLock());

            auto readLock = value.readLocked(std::try_to_lock);
            QVERIFY(!readLock.ownsLock());
        }
    }

    void synchronizeMultiple()
    {
        SynchronizedValue<int> sv1;
        SynchronizedValue<QString> sv2;

        auto [lk1, lk2] = synchronize(sv1, sv2);

        QVERIFY(lk1.ownsLock());
        QVERIFY(lk2.ownsLock());
    }
};

} // namespace Utils

QTEST_GUILESS_MAIN(Utils::tst_synchronizedvalue)

#include "tst_synchronizedvalue.moc"
