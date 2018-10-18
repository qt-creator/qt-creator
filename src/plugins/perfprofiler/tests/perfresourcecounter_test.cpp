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

#include "perfresourcecounter_test.h"
#include <perfprofiler/perfresourcecounter.h>
#include <QtTest>

namespace PerfProfiler {
namespace Internal {

struct SizePayload : public NoPayload
{
    SizePayload(qint64 *size = nullptr) : size(size) {}

    void adjust(qint64 diff)
    {
        if (size)
            *size += diff;
    }

    qint64 *size;
};

typedef PerfResourceCounter<SizePayload> SizeCounter;
typedef PerfResourceCounter<NoPayload> NoPayloadCounter;

template<typename Container>
static qint64 sum(const Container &container)
{
    qint64 amount = 0;
    for (auto it = container.begin(), end = container.end(); it != end; ++it) {
        const qint64 size = it->second.size();
        Q_ASSERT(size >= 0);
        amount += size;
    }
    return amount;
}

template<typename Counter>
static void fill(Counter &counter, const typename Counter::Container &container, bool fragmented)
{
    qint64 allocated = 0;
    QHash<void *, int> allocations;
    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
            int amount = fragmented ? j : i;
            allocated += amount;
            counter.request(amount);
            void *alloc = malloc(amount);
            allocations.insertMulti(alloc, amount);
            counter.obtain(reinterpret_cast<quintptr>(alloc));
            QCOMPARE(counter.currentTotal(), allocated);
        }
    }

    QCOMPARE(allocated, 99 * 50 * 100);
    QCOMPARE(counter.currentTotal(), allocated);
    QCOMPARE(sum(container), allocated);

    for (auto it = allocations.begin(), end = allocations.end(); it != end; ++it) {
        free(it.key());
        counter.release(reinterpret_cast<quintptr>(it.key()));
        allocated -= it.value();
        QCOMPARE(counter.currentTotal(), allocated);
    }

    allocations.clear();

    QCOMPARE(allocated, 0);
    QCOMPARE(counter.currentTotal(), 0);
    QCOMPARE(sum(container), 0);
}

void PerfResourceCounterTest::testMallocFree()
{
    SizeCounter::Container intContainer;
    SizeCounter intCounter(&intContainer);
    QCOMPARE(static_cast<int>(intContainer.size()), 0);

    fill(intCounter, intContainer, true);
    fill(intCounter, intContainer, false);

    NoPayloadCounter::Container nothingContainer;
    NoPayloadCounter nothingCounter(&nothingContainer);
    QCOMPARE(static_cast<int>(nothingContainer.size()), 0);

    fill(nothingCounter, nothingContainer, true);
    fill(nothingCounter, nothingContainer, false);
}

void PerfResourceCounterTest::testRandomFill()
{
    for (int i = 0; i < 100; ++i) {
        SizeCounter::Container container;
        SizeCounter counter(&container);
        for (int i = 0; i < 10000; ++i) {
            const int amount = qrand();
            counter.request(amount, i);
            counter.obtain(qrand());
            if (sum(container) != counter.currentTotal())
                qDebug() << "ouch";
            QCOMPARE(sum(container), counter.currentTotal());
        }
        for (int i = 0; i < 10000; ++i) {
            counter.release(qrand());
            QCOMPARE(sum(container), counter.currentTotal());
        }

        // We cannot check counter.currentTotal() because the release guessing only works on
        // averageReleased() which needs some observed releases to work.
    }
}

void PerfResourceCounterTest::testUnitSized()
{
    NoPayloadCounter::Container container;
    NoPayloadCounter counter(&container);
    QList<int> ids;
    for (int i = 0; i < 10000; ++i) {
        counter.request(1);
        int id = qrand();
        counter.obtain(id);
        ids.append(id);
        QCOMPARE(counter.currentTotal(), ids.length());
    }
    QCOMPARE(sum(container), counter.currentTotal());
    while (!ids.isEmpty()) {
        counter.release(ids.takeLast());
        QCOMPARE(counter.currentTotal(), ids.length());
    }
    QCOMPARE(sum(container), 0);
}

void PerfResourceCounterTest::testRandomAlternate()
{
    NoPayloadCounter::Container container;
    NoPayloadCounter counter(&container);
    for (int i = 0; i < 1000; ++i) {
        for (int i = 0; i < 100; ++i) {
            counter.request(qrand());
            counter.obtain(qrand());
            counter.release(qrand());
        }
        QCOMPARE(sum(container), counter.currentTotal());
    }
}

void PerfResourceCounterTest::testGuesses()
{
    SizeCounter::Container container;
    SizeCounter counter(&container);
    for (int i = 0; i < 1000; i += 20) {
        QCOMPARE(counter.currentTotal(), i / 2);
        counter.request(10, i);
        counter.obtain(i + 1);
    }

    QCOMPARE(static_cast<int>(container.size()), 50);
    QCOMPARE(sum(container), counter.currentTotal());
    QCOMPARE(counter.currentTotal(), 500ll);

    for (int i = 10; i < 1000; i += 20) {
        QCOMPARE(counter.currentTotal(), 505 - i / 2);
        counter.release(i);
    }

    QCOMPARE(static_cast<int>(container.size()), 0);

    QCOMPARE(sum(container), counter.currentTotal());
    QCOMPARE(counter.currentTotal(), 0ll);
}

void PerfResourceCounterTest::testNegative()
{
    NoPayloadCounter::Container container;
    NoPayloadCounter counter(&container);
    NoPayloadCounter counter2(&container);

    for (quint64 i = 0; i < 100; i += 10) {
        counter.request(10);
        counter.obtain(i + 1);
        counter2.request(10);
        counter2.obtain(100 + i + 1);
    }

    for (quint64 i = 0; i < 100; i += 10) {
        counter.release(i + 1);
        counter.release(100 + i + 1);
    }

    QCOMPARE(counter.maxTotal(), 100ll);
    QCOMPARE(counter.minTotal(), -100ll);
}

void PerfResourceCounterTest::testInvalidId()
{
    NoPayloadCounter::Container container;
    NoPayloadCounter counter(&container);

    counter.request(1000);
    counter.obtain(0);

    QVERIFY(counter.isEmpty());
    QCOMPARE(counter.currentTotal(), 0ll);

    counter.request(500);
    counter.obtain(1);

    QCOMPARE(container.size(), static_cast<std::size_t>(1));
    QCOMPARE(counter.currentTotal(), 500ll);

    counter.release(0);

    QCOMPARE(container.size(), static_cast<std::size_t>(1));
    QCOMPARE(counter.currentTotal(), 500ll);

    counter.release(1);

    QCOMPARE(counter.currentTotal(), 0ll);
}

void PerfResourceCounterTest::testMultiCounter()
{
    NoPayloadCounter::Container container;
    NoPayloadCounter counter1(&container);
    NoPayloadCounter counter2(&container);

    QCOMPARE(counter1.currentTotal(), 0ll);
    QCOMPARE(counter2.currentTotal(), 0ll);

    for (quint64 i = 0; i < 100; i += 10) {
        counter1.request(10);
        counter1.obtain(i + 1);
    }

    QCOMPARE(counter1.currentTotal(), 100ll);
    QCOMPARE(counter2.currentTotal(), 0ll);

    for (quint64 i = 100; i < 200; i += 10) {
        counter2.request(10);
        counter2.obtain(i + 1);
    }

    QCOMPARE(counter1.currentTotal(), 100ll);
    QCOMPARE(counter2.currentTotal(), 100ll);

    for (quint64 i = 0; i < 100; i += 10)
        counter2.release(i + 1);

    QCOMPARE(counter1.currentTotal(), 100ll);
    QCOMPARE(counter2.currentTotal(), 0ll);

    for (quint64 i = 100; i < 200; i += 10)
        counter1.release(i + 1);

    QCOMPARE(counter1.currentTotal(), 0ll);
    QCOMPARE(counter2.currentTotal(), 0ll);
}

void PerfResourceCounterTest::testMove()
{
    SizeCounter::Container container;
    SizeCounter counter(&container);

    qint64 origRequestSize = 0;
    qint64 origObtainSize = 0;
    counter.request(50, SizePayload(&origRequestSize));
    counter.obtain(50, SizePayload(&origObtainSize));
    QCOMPARE(origRequestSize, 50ll);
    QCOMPARE(origObtainSize, 0ll);
    QCOMPARE(counter.currentTotal(), 50ll);

    // Simple move
    qint64 moveRequestSize = 0;
    qint64 moveObtainSize = 0;
    counter.request(100, 50, SizePayload(&moveRequestSize));
    counter.move(100, SizePayload(&moveObtainSize));
    QCOMPARE(origRequestSize, 0ll);
    QCOMPARE(moveRequestSize, 100ll);
    QCOMPARE(moveObtainSize, 0ll);
    QCOMPARE(counter.currentTotal(), 100ll);

    // Move, reusing the same ID
    qint64 reuseRequestSize = 0;
    qint64 reuseObtainSize = 0;
    counter.request(200, 100, SizePayload(&reuseRequestSize));
    counter.move(100, SizePayload(&reuseObtainSize));
    QCOMPARE(moveRequestSize, 0ll);
    QCOMPARE(reuseRequestSize, 200ll);
    QCOMPARE(reuseObtainSize, 0ll);
    QCOMPARE(counter.currentTotal(), 200ll);

    // Move, implemented with request/obtain/release
    qint64 outerRequestSize = 0;
    counter.request(500, 100, &outerRequestSize);
    qint64 innerRequestSize = 0;
    counter.request(500, 50, &innerRequestSize);
    qint64 innerObtainSize = 0;
    counter.obtain(1000, &innerObtainSize);
    QCOMPARE(innerObtainSize, 0ll);
    QCOMPARE(innerRequestSize, 500ll);
    QCOMPARE(counter.currentTotal(), 700ll);
    qint64 innerReleaseSize = 0;
    counter.release(100, &innerReleaseSize);
    QCOMPARE(innerReleaseSize, 0ll);
    QCOMPARE(reuseRequestSize, 0ll);
    QCOMPARE(counter.currentTotal(), 500ll);
    qint64 outerMoveSize = 0;
    counter.move(1000, &outerMoveSize);
    QCOMPARE(outerMoveSize, 0ll);
    QCOMPARE(innerRequestSize, 0ll);
    QCOMPARE(outerRequestSize, 500ll);
    QCOMPARE(counter.currentTotal(), 500ll);

    // Failed simple move
    qint64 failedRequestSize = 0;
    counter.request(1000, 2000, &failedRequestSize);
    qint64 failedMoveSize = 0;
    counter.move(0, &failedMoveSize);
    QCOMPARE(failedRequestSize, 0ll);
    QCOMPARE(failedMoveSize, 0ll);
    QCOMPARE(outerRequestSize, 500ll);
    QCOMPARE(counter.currentTotal(), 500ll);

    // Failed move, using request/obtain/(not) release
    qint64 failedOuterRequestSize = 0;
    counter.request(1000, 2000, &failedOuterRequestSize);
    qint64 failedInnerRequestSize = 0;
    counter.request(2000, &failedInnerRequestSize);
    qint64 failedInnerObtainSize = 0;
    counter.obtain(0, &failedInnerObtainSize);
    QCOMPARE(failedInnerObtainSize, 0ll);
    QCOMPARE(failedInnerRequestSize, 0ll);
    QCOMPARE(outerRequestSize, 500ll);
    QCOMPARE(counter.currentTotal(), 500ll);
    qint64 failedOuterMoveSize = 0;
    counter.move(0, &failedOuterMoveSize);
    QCOMPARE(failedOuterRequestSize, 0ll);
    QCOMPARE(failedOuterMoveSize, 0ll);
    QCOMPARE(outerRequestSize, 500ll);
    QCOMPARE(counter.currentTotal(), 500ll);
}

} // namespace Internal
} // namespace PerfProfiler

