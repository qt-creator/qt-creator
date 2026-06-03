// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QScrollBar>
#include <QTest>

#include <tracing/timelinescrollsync.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/timeruler.h>
#include <tracing/tracklabels.h>
#include <tracing/trackpainter.h>

using namespace Timeline;

class tst_TimelineScrollSync : public QObject
{
    Q_OBJECT
private slots:
    void rulerSyncedOnRegister();
    void rulerGetsRangeChange();
    void painterSyncedOnRegister();
    void painterGetsRangeChange();
    void labelsScrollBarPropagates();
    void labelsRegisteredAfterScrollBar();
};

void tst_TimelineScrollSync::rulerSyncedOnRegister()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0, 100000);
    zoom.setRange(1000, 5000);

    TimelineScrollSync sync(&zoom);
    TimeRuler ruler;
    sync.registerRuler(&ruler);

    QCOMPARE(ruler.rangeStart(), qint64(1000));
    QCOMPARE(ruler.rangeEnd(), qint64(5000));
}

void tst_TimelineScrollSync::rulerGetsRangeChange()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0, 100000);
    zoom.setRange(1000, 5000);

    TimelineScrollSync sync(&zoom);
    TimeRuler ruler;
    sync.registerRuler(&ruler);

    zoom.setRange(2000, 8000);

    QCOMPARE(ruler.rangeStart(), qint64(2000));
    QCOMPARE(ruler.rangeEnd(), qint64(8000));
}

void tst_TimelineScrollSync::painterSyncedOnRegister()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0, 100000);
    zoom.setRange(3000, 7000);

    TimelineScrollSync sync(&zoom);
    TrackPainter painter;
    sync.registerContent(&painter);

    QCOMPARE(painter.rangeStart(), qint64(3000));
    QCOMPARE(painter.rangeEnd(), qint64(7000));
}

void tst_TimelineScrollSync::painterGetsRangeChange()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0, 100000);
    zoom.setRange(3000, 7000);

    TimelineScrollSync sync(&zoom);
    TrackPainter painter;
    sync.registerContent(&painter);

    zoom.setRange(0, 100000);

    QCOMPARE(painter.rangeStart(), qint64(0));
    QCOMPARE(painter.rangeEnd(), qint64(100000));
}

void tst_TimelineScrollSync::labelsScrollBarPropagates()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0, 100000);

    TimelineScrollSync sync(&zoom);
    TrackLabels labels;
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 500);

    sync.registerLabels(&labels);
    sync.setVerticalScrollBar(&bar);

    bar.setValue(100);
    QCOMPARE(labels.scrollOffset(), 100);

    bar.setValue(300);
    QCOMPARE(labels.scrollOffset(), 300);
}

void tst_TimelineScrollSync::labelsRegisteredAfterScrollBar()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0, 100000);

    TimelineScrollSync sync(&zoom);
    TrackLabels labels;
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 500);

    sync.setVerticalScrollBar(&bar);
    sync.registerLabels(&labels);

    bar.setValue(200);
    QCOMPARE(labels.scrollOffset(), 200);
}

QTEST_MAIN(tst_TimelineScrollSync)

#include "tst_timelinescrollsync.moc"
