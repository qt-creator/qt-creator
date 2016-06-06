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
#include "qmlevent_test.h"

#include <QtTest>
#include <QList>
#include <QQueue>

namespace QmlProfiler {
namespace Internal {

QmlEventTest::QmlEventTest(QObject *parent) : QObject(parent)
{
}

void QmlEventTest::testCtors()
{
    {
        QmlEvent event;
        QCOMPARE(event.timestamp(), -1);
        QCOMPARE(event.typeIndex(), -1);
        QVERIFY(!event.isValid());
        auto numbers = event.numbers<QList<qint32>, qint32>();
        QVERIFY(numbers.isEmpty());
    }

    {
        QmlEvent event(12, 10, {34, 35, 25});
        QCOMPARE(event.timestamp(), 12);
        QCOMPARE(event.typeIndex(), 10);
        QVERIFY(event.isValid());
        auto numbers = event.numbers<QList<qint8>, qint8>();
        QCOMPARE(numbers.length(), 3);
        QCOMPARE(numbers[0], static_cast<qint8>(34));
        QCOMPARE(numbers[1], static_cast<qint8>(35));
        QCOMPARE(numbers[2], static_cast<qint8>(25));
    }

    {
        QmlEvent event(11, 9, QString("blah"));
        QCOMPARE(event.timestamp(), 11);
        QCOMPARE(event.typeIndex(), 9);
        QCOMPARE(event.string(), QString("blah"));
    }

    {
        QmlEvent event(20, 30, QVector<qint64>({600, 700, 800, 900}));
        QCOMPARE(event.timestamp(), 20);
        QCOMPARE(event.typeIndex(), 30);
        QCOMPARE(event.numbers<QVector<qint32>>(), QVector<qint32>({600, 700, 800, 900}));

        QmlEvent event2(event);
        QCOMPARE(event2, event);

        QmlEvent event3;
        event3 = event;
        QCOMPARE(event3, event);

        QmlEvent event4(std::move(event2));
        QCOMPARE(event4, event);

        QmlEvent event5;
        event5 = std::move(event3);
        QCOMPARE(event5, event);
    }
}

void QmlEventTest::testNumbers()
{
    QmlEvent event(1, 2, {3});
    QCOMPARE(event.number<qint32>(0), 3);
    QCOMPARE(event.number<qint32>(1), 0);
    event.setNumber<qint8>(1, 20);
    QCOMPARE(event.number<qint32>(0), 3);
    QCOMPARE(event.number<qint32>(1), 20);
    QCOMPARE(event.number<qint32>(2), 0);
    event.setNumber<qint16>(2, 800);
    QCOMPARE(event.number<qint32>(0), 3);
    QCOMPARE(event.number<qint32>(1), 20);
    QCOMPARE(event.number<qint32>(2), 800);
    QCOMPARE(event.number<qint32>(3), 0);
    event.setNumber<qint32>(3, 0xffffff);
    QCOMPARE(event.number<qint32>(0), 3);
    QCOMPARE(event.number<qint32>(1), 20);
    QCOMPARE(event.number<qint32>(2), 800);
    QCOMPARE(event.number<qint32>(3), 0xffffff);
    QCOMPARE(event.number<qint32>(4), 0);
    event.setNumber<qint64>(4, std::numeric_limits<qint64>::max());
    QCOMPARE(event.number<qint32>(0), 3);
    QCOMPARE(event.number<qint32>(1), 20);
    QCOMPARE(event.number<qint32>(2), 800);
    QCOMPARE(event.number<qint32>(3), 0xffffff);
    QCOMPARE(event.number<qint64>(4), std::numeric_limits<qint64>::max());
    QCOMPARE(event.number<qint64>(5), 0LL);
}

void QmlEventTest::testStreamOps()
{
    QQueue<QmlEvent> sentEvents;

    QmlEvent event(12, 13, QString("semmelsemmel"));

    QBuffer wbuffer;
    wbuffer.open(QIODevice::WriteOnly);
    QDataStream wstream(&wbuffer);
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(9000);
    event.setNumber(0, 700);
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(90000);
    event.setNumber(0, 70000);
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(11000000000LL);
    event.setNumber(0, 5000000000LL);
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(-1);
    event.setNumbers({2});
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(-1000);
    event.setNumber(0, 800);
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(-100000000);
    event.setNumber(0, 90000);
    wstream << event;
    sentEvents.enqueue(event);

    event.setTimestamp(0);
    event.setNumber(0, 6000000000LL);
    wstream << event;
    sentEvents.enqueue(event);

    QmlEvent event2(20, 30, QString("naeh"));
    QVERIFY(event != event2);

    QBuffer rbuffer;
    rbuffer.setData(wbuffer.data());
    rbuffer.open(QIODevice::ReadOnly);
    QDataStream rstream(&rbuffer);

    while (!sentEvents.isEmpty()) {
        rstream >> event2;
        QCOMPARE(event2, sentEvents.dequeue());
    }

    QVERIFY(rstream.atEnd());
}

} // namespace Internal
} // namespace QmlProfiler
