// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../trackpainterbackends.h"

#include <QScrollBar>
#include <QTest>

#include <tracing/timelinescrollsync.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/timeruler.h>
#include <tracing/tracklabels.h>

using namespace std::chrono;
using namespace Timeline;

class tst_TimelineScrollSync : public QObject
{
    Q_OBJECT
private slots:
    void rulerSyncedOnRegister();
    void rulerGetsRangeChange();
    void painterSyncedOnRegister_data();
    void painterSyncedOnRegister();
    void painterGetsRangeChange_data();
    void painterGetsRangeChange();
    void labelsScrollBarPropagates();
    void labelsRegisteredAfterScrollBar();
};

void tst_TimelineScrollSync::rulerSyncedOnRegister()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0ns, 100000ns);
    zoom.setRange(1000ns, 5000ns);

    TimelineScrollSync sync(&zoom);
    TimeRuler ruler;
    sync.registerRuler(&ruler);

    QCOMPARE(ruler.rangeStart(), 1000ns);
    QCOMPARE(ruler.rangeEnd(), 5000ns);
}

void tst_TimelineScrollSync::rulerGetsRangeChange()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0ns, 100000ns);
    zoom.setRange(1000ns, 5000ns);

    TimelineScrollSync sync(&zoom);
    TimeRuler ruler;
    sync.registerRuler(&ruler);

    zoom.setRange(2000ns, 8000ns);

    QCOMPARE(ruler.rangeStart(), 2000ns);
    QCOMPARE(ruler.rangeEnd(), 8000ns);
}

void tst_TimelineScrollSync::painterSyncedOnRegister_data()
{
    addTrackPainterBackends();
}

void tst_TimelineScrollSync::painterSyncedOnRegister()
{
    QFETCH(TrackBackend, backend);

    withTrackPainter(backend, []<typename TrackPainter>() {
        TimelineZoomControl zoom;
        zoom.setTrace(0ns, 100000ns);
        zoom.setRange(3000ns, 7000ns);

        TimelineScrollSync sync(&zoom);
        TrackPainter painter;
        sync.registerContent(&painter);

        QCOMPARE(painter.rangeStart(), 3000ns);
        QCOMPARE(painter.rangeEnd(), 7000ns);
    });
}

void tst_TimelineScrollSync::painterGetsRangeChange_data()
{
    addTrackPainterBackends();
}

void tst_TimelineScrollSync::painterGetsRangeChange()
{
    QFETCH(TrackBackend, backend);

    withTrackPainter(backend, []<typename TrackPainter>() {
        TimelineZoomControl zoom;
        zoom.setTrace(0ns, 100000ns);
        zoom.setRange(3000ns, 7000ns);

        TimelineScrollSync sync(&zoom);
        TrackPainter painter;
        sync.registerContent(&painter);

        zoom.setRange(0ns, 100000ns);

        QCOMPARE(painter.rangeStart(), 0ns);
        QCOMPARE(painter.rangeEnd(), 100000ns);
    });
}

void tst_TimelineScrollSync::labelsScrollBarPropagates()
{
    TimelineZoomControl zoom;
    zoom.setTrace(0ns, 100000ns);

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
    zoom.setTrace(0ns, 100000ns);

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
