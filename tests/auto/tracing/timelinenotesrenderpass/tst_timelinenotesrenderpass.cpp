// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinenotesrenderpass.h>
#include <tracing/timelinerenderstate.h>
#include <tracing/timelineabstractrenderer_p.h>

#include <QtTest>
#include <QSGMaterialShader>

using namespace Timeline;

class DummyModel : public TimelineModel {
public:
    DummyModel(TimelineModelAggregator *parent);
    void loadData();
};

class tst_TimelineNotesRenderPass : public QObject
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
}

void tst_TimelineNotesRenderPass::instance()
{
    const TimelineNotesRenderPass *inst = TimelineNotesRenderPass::instance();
    const TimelineNotesRenderPass *inst2 = TimelineNotesRenderPass::instance();
    QCOMPARE(inst, inst2);
    QVERIFY(inst != 0);
}

void tst_TimelineNotesRenderPass::update()
{
    const TimelineNotesRenderPass *inst = TimelineNotesRenderPass::instance();
    TimelineAbstractRenderer renderer;
    TimelineModelAggregator aggregator;
    TimelineRenderState parentState(0, 8, 1, 1);
    TimelineRenderPass::State *nullState = 0;
    QSGNode *nullNode = 0;
    TimelineRenderPass::State *result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    DummyModel model(&aggregator);
    DummyModel otherModel(&aggregator);

    TimelineNotesModel notes;
    notes.addTimelineModel(&model);
    notes.addTimelineModel(&otherModel);
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    renderer.setModel(&model);
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    renderer.setNotes(&notes);
    model.loadData();
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QVERIFY(result != nullState);

    // The notes renderer creates a collapsed overlay and expanded nodes per row.
    QVERIFY(result->collapsedOverlay() != nullNode);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedRows().count(), 1);
    QCOMPARE(result->collapsedRows().count(), 0);

    TimelineRenderPass::State *result2 = inst->update(&renderer, &parentState, result, 0, 0, false,
                                                      1);
    QCOMPARE(result2, result);

    notes.add(model.modelId(), 0, QLatin1String("x"));
    notes.add(model.modelId(), 9, QLatin1String("xx"));
    notes.add(otherModel.modelId(), 0, QLatin1String("y"));
    QVERIFY(renderer.notesDirty());
    result = inst->update(&renderer, &parentState, result, 0, 0, true, 1);
    QVERIFY(result != nullState);
    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(result->expandedRows()[0]);
    QSGMaterial *material1 = node->material();
    QVERIFY(material1 != 0);
    QCOMPARE(node->geometry()->vertexCount(), 2);
    node = static_cast<QSGGeometryNode *>(result->collapsedOverlay());
    QSGMaterial *material2 = node->material();
    QCOMPARE(node->geometry()->vertexCount(), 2);
    QVERIFY(material2 != 0);
    QCOMPARE(material1->type(), material2->type());
    QSGMaterialShader *shader1 = material1->createShader(QSGRendererInterface::RenderMode2D);
    QVERIFY(shader1 != 0);
    QSGMaterialShader *shader2 = material2->createShader(QSGRendererInterface::RenderMode2D);
    QVERIFY(shader2 != 0);

    delete shader1;
    delete shader2;

    parentState.setPassState(0, result);
    parentState.assembleNodeTree(&model, 1, 1);

    QVERIFY(parentState.collapsedOverlayRoot());
    QVERIFY(parentState.expandedRowRoot());
}

QTEST_MAIN(tst_TimelineNotesRenderPass)

#include "tst_timelinenotesrenderpass.moc"

