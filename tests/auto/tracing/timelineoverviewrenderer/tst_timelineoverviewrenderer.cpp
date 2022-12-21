// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineoverviewrenderer_p.h>

using namespace Timeline;

class DummyRenderer : public TimelineOverviewRenderer {
    friend class tst_TimelineOverviewRenderer;
};

class DummyModel : public TimelineModel {
public:
    DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        setCollapsedRowCount(3);
        setExpandedRowCount(3);
        for (int i = 0; i < 10; ++i)
            insert(i, i, i);
        emit contentChanged();
    }
};

class tst_TimelineOverviewRenderer : public QObject
{
    Q_OBJECT

private slots:
    void updatePaintNode();
};

void tst_TimelineOverviewRenderer::updatePaintNode()
{
    DummyRenderer renderer;
    TimelineModelAggregator aggregator;
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    DummyModel model(&aggregator);
    renderer.setModel(&model);
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    model.loadData();
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    TimelineZoomControl zoomer;
    renderer.setZoomer(&zoomer);
    QCOMPARE(renderer.updatePaintNode(0, 0), static_cast<QSGNode *>(0));
    zoomer.setTrace(0, 10);
    QSGNode *node = renderer.updatePaintNode(0, 0);
    QVERIFY(node != 0);
    QCOMPARE(renderer.updatePaintNode(node, 0), node);
    delete node;
}

QTEST_GUILESS_MAIN(tst_TimelineOverviewRenderer)

#include "tst_timelineoverviewrenderer.moc"
