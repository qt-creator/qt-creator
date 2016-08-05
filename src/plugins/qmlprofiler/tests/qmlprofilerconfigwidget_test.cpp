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
#include "qmlprofilerconfigwidget_test.h"
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlProfilerConfigWidgetTest::QmlProfilerConfigWidgetTest(QObject *parent) :
    QObject(parent), widget(&settings)
{
    widget.show();
    flushEnabled = widget.findChild<QCheckBox*>("flushEnabled");
    QVERIFY(flushEnabled);
    flushInterval = widget.findChild<QSpinBox*>("flushInterval");
    QVERIFY(flushInterval);
    aggregateTraces = widget.findChild<QCheckBox*>("aggregateTraces");
    QVERIFY(aggregateTraces);
}

void QmlProfilerConfigWidgetTest::testUpdateFromSettings()
{
    QSignalSpy flushes(flushEnabled, SIGNAL(stateChanged(int)));
    QCOMPARE(flushEnabled->checkState(), Qt::Unchecked);
    settings.setFlushEnabled(true);
    QCOMPARE(flushEnabled->checkState(), Qt::Checked);
    settings.setFlushEnabled(false);
    QCOMPARE(flushEnabled->checkState(), Qt::Unchecked);
    QCOMPARE(flushes.count(), 2);


    QSignalSpy intervals(flushInterval, SIGNAL(valueChanged(int)));
    QCOMPARE(flushInterval->value(), 1000);
    settings.setFlushInterval(200);
    QCOMPARE(flushInterval->value(), 200);
    settings.setFlushInterval(1000);
    QCOMPARE(flushInterval->value(), 1000);
    QCOMPARE(intervals.count(), 2);

    QSignalSpy aggregates(aggregateTraces, SIGNAL(stateChanged(int)));
    QCOMPARE(aggregateTraces->checkState(), Qt::Unchecked);
    settings.setAggregateTraces(true);
    QCOMPARE(aggregateTraces->checkState(), Qt::Checked);
    settings.setAggregateTraces(false);
    QCOMPARE(aggregateTraces->checkState(), Qt::Unchecked);
    QCOMPARE(aggregates.count(), 2);
}

void QmlProfilerConfigWidgetTest::testChangeWidget()
{
    QCOMPARE(flushEnabled->checkState(), Qt::Unchecked);
    QVERIFY(!settings.flushEnabled());
    flushEnabled->setCheckState(Qt::Checked);
    QVERIFY(settings.flushEnabled());
    flushEnabled->setCheckState(Qt::Unchecked);
    QVERIFY(!settings.flushEnabled());

    QCOMPARE(flushInterval->value(), 1000);
    QCOMPARE(settings.flushInterval(), 1000u);
    flushInterval->setValue(200);
    QCOMPARE(settings.flushInterval(), 200u);
    flushInterval->setValue(1000);
    QCOMPARE(settings.flushInterval(), 1000u);

    QCOMPARE(aggregateTraces->checkState(), Qt::Unchecked);
    QVERIFY(!settings.aggregateTraces());
    aggregateTraces->setCheckState(Qt::Checked);
    QVERIFY(settings.aggregateTraces());
    aggregateTraces->setCheckState(Qt::Unchecked);
    QVERIFY(!settings.aggregateTraces());
}

} // namespace Internal
} // namespace QmlProfiler
