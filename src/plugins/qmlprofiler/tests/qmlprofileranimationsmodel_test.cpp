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

#include "qmlprofileranimationsmodel_test.h"
#include <timeline/timelineformattime.h>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlProfilerAnimationsModelTest::QmlProfilerAnimationsModelTest(QObject *parent) :
    QObject(parent), manager(nullptr), model(&manager)
{
}

static int frameRate(int i)
{
    return i * 7 - 2;
}

void QmlProfilerAnimationsModelTest::initTestCase()
{
    manager.startAcquiring();

    QmlEventType type(Event, MaximumRangeType, AnimationFrame);
    QmlEvent event;
    event.setTypeIndex(manager.numLoadedEventTypes());
    manager.addEventType(type);

    for (int i = 0; i < 10; ++i) {
        event.setTimestamp(i);
        event.setNumbers<int>({frameRate(i), 20 - i, (i % 2) ? RenderThread : GuiThread});
        manager.addEvent(event);
    }
    manager.acquiringDone();
    QCOMPARE(manager.state(), QmlProfilerModelManager::Done);
}

void QmlProfilerAnimationsModelTest::testAccepted()
{
    QVERIFY(!model.accepted(QmlEventType()));
    QVERIFY(!model.accepted(QmlEventType(Event)));
    QVERIFY(!model.accepted(QmlEventType(Event, MaximumRangeType)));
    QVERIFY(model.accepted(QmlEventType(Event, MaximumRangeType, AnimationFrame)));
}

void QmlProfilerAnimationsModelTest::testRowMaxValue()
{
    QCOMPARE(model.rowMaxValue(0), 0);
    QCOMPARE(model.rowMaxValue(1), 20);
    QCOMPARE(model.rowMaxValue(2), 19);
}

void QmlProfilerAnimationsModelTest::testRowNumbers()
{
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(model.expandedRow(i), (i % 2) ? 2 : 1);
        QCOMPARE(model.collapsedRow(i), model.expandedRow(i));
    }
}

void QmlProfilerAnimationsModelTest::testTypeId()
{
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.typeId(i), 0);
}

void QmlProfilerAnimationsModelTest::testColor()
{
    QColor last = QColor::fromHsl(0, 0, 0);
    for (int i = 0; i < 10; ++i) {
        QColor next = model.color(i);
        QVERIFY(next.hue() > last.hue());
        last = next;
    }
}

void QmlProfilerAnimationsModelTest::testRelativeHeight()
{
    float last = 1;
    for (int i = 0; i < 10; ++i) {
        float next = model.relativeHeight(i);
        QVERIFY(next <= last);
        last = next;
    }
}

void QmlProfilerAnimationsModelTest::testLabels()
{
    QVariantList labels = model.labels();
    QCOMPARE(labels.length(), 2);

    QVariantMap label0 = labels[0].toMap();
    QCOMPARE(label0["displayName"].toString(), QmlProfilerAnimationsModel::tr("Animations"));
    QCOMPARE(label0["description"].toString(), QmlProfilerAnimationsModel::tr("GUI Thread"));
    QCOMPARE(label0["id"].toInt(), static_cast<int>(GuiThread));

    QVariantMap label1 = labels[1].toMap();
    QCOMPARE(label1["displayName"].toString(), QmlProfilerAnimationsModel::tr("Animations"));
    QCOMPARE(label1["description"].toString(), QmlProfilerAnimationsModel::tr("Render Thread"));
    QCOMPARE(label1["id"].toInt(), static_cast<int>(RenderThread));
}

void QmlProfilerAnimationsModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        QVariantMap details = model.details(i);
        QCOMPARE(details["displayName"].toString(), model.displayName());
        QCOMPARE(details[QmlProfilerAnimationsModel::tr("Duration")].toString(),
                Timeline::formatTime(1));
        QCOMPARE(details[QmlProfilerAnimationsModel::tr("Framerate")].toString(),
                QString::fromLatin1("%1 FPS").arg(frameRate(i)));
        QCOMPARE(details[QmlProfilerAnimationsModel::tr("Animations")].toString(),
                QString::number(20 - i));
        QCOMPARE(details[QmlProfilerAnimationsModel::tr("Context")].toString(), i % 2 ?
                    QmlProfilerAnimationsModel::tr("Render Thread") :
                    QmlProfilerAnimationsModel::tr("GUI Thread"));
    }
}

void QmlProfilerAnimationsModelTest::cleanupTestCase()
{
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.expandedRowCount(), 1);
    QCOMPARE(model.collapsedRowCount(), 1);
}

} // namespace Internal
} // namespace QmlProfiler
