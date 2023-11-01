// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelineitemsrenderpass.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinerenderstate.h>

#include <QtTest>
#include <QSGMaterialShader>

using namespace Timeline;

class DummyModel : public TimelineModel {
public:
    DummyModel(TimelineModelAggregator *parent);
    void loadData();
    float relativeHeight(int index) const;
};

class tst_TimelineItemsRenderPass : public QObject
{
    Q_OBJECT

private slots:
    void instance();
    void update();
};

DummyModel::DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent)
{
}

void DummyModel::loadData()
{
    for (int i = 0; i < 10; ++i)
        insert(i, 1, 1);

    insert(5, 0, 10);
}

float DummyModel::relativeHeight(int index) const
{
    return 1.0 / static_cast<float>(index / 3 + 1);
}

void tst_TimelineItemsRenderPass::instance()
{
    const TimelineItemsRenderPass *inst = TimelineItemsRenderPass::instance();
    const TimelineItemsRenderPass *inst2 = TimelineItemsRenderPass::instance();
    QCOMPARE(inst, inst2);
    QVERIFY(inst != 0);
}

void tst_TimelineItemsRenderPass::update()
{
    const TimelineItemsRenderPass *inst = TimelineItemsRenderPass::instance();
    TimelineAbstractRenderer renderer;
    TimelineModelAggregator aggregator;
    TimelineRenderState parentState(0, 8, 1, 1);
    TimelineRenderPass::State *nullState = 0;
    QSGNode *nullNode = 0;
    TimelineRenderPass::State *result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    DummyModel model(&aggregator);
    renderer.setModel(&model);
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    model.loadData();
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    result = inst->update(&renderer, &parentState, 0, 2, 9, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedRows().count(), 1);
    QCOMPARE(result->collapsedRows().count(), 1);
    QCOMPARE(result->expandedRows()[0]->childCount(), 1);
    QCOMPARE(result->collapsedRows()[0]->childCount(), 1);
    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(result->expandedRows()[0]->firstChild());
    QSGMaterial *material1 = node->material();
    QVERIFY(material1 != 0);
    QCOMPARE(node->geometry()->vertexCount(), 30);
    node = static_cast<QSGGeometryNode *>(result->collapsedRows()[0]->firstChild());
    QSGMaterial *material2 = node->material();
    QCOMPARE(node->geometry()->vertexCount(), 30);
    QVERIFY(material2 != 0);
    QCOMPARE(material1->type(), material2->type());
    QSGMaterialShader *shader1 = material1->createShader(QSGRendererInterface::RenderMode2D);
    QVERIFY(shader1 != 0);
    QSGMaterialShader *shader2 = material2->createShader(QSGRendererInterface::RenderMode2D);
    QVERIFY(shader2 != 0);

    delete shader1;
    delete shader2;

    result = inst->update(&renderer, &parentState, result, 0, 11, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedRows().count(), 1);
    QCOMPARE(result->collapsedRows().count(), 1);

    // 0-sized node starting at 8 may also be added. We don't test for this one.
    QVERIFY(result->expandedRows()[0]->childCount() > 1);
    QVERIFY(result->collapsedRows()[0]->childCount() > 1);

    node = static_cast<QSGGeometryNode *>(result->expandedRows()[0]->childAtIndex(1));
    QCOMPARE(node->geometry()->vertexCount(), 8);
    node = static_cast<QSGGeometryNode *>(result->collapsedRows()[0]->childAtIndex(1));
    QCOMPARE(node->geometry()->vertexCount(), 8);

    model.setExpanded(true);
    result = inst->update(&renderer, &parentState, result, 0, 10, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedRows().count(), 1);
    QCOMPARE(result->collapsedRows().count(), 1);

    parentState.setPassState(0, result);
    parentState.assembleNodeTree(&model, 1, 1);

    QVERIFY(parentState.collapsedRowRoot());
    QVERIFY(parentState.expandedRowRoot());
}

QTEST_MAIN(tst_TimelineItemsRenderPass)

#include "tst_timelineitemsrenderpass.moc"

