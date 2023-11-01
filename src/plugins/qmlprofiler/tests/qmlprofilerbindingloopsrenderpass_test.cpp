// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include "qmlprofilerbindingloopsrenderpass_test.h"

#include <tracing/timelineabstractrenderer.h>
#include <qmlprofiler/qmlprofilerbindingloopsrenderpass.h>
#include <qmlprofiler/qmlprofilerrangemodel.h>

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

class DummyModel : public QmlProfilerRangeModel {
public:
    DummyModel(QmlProfilerModelManager *manager, Timeline::TimelineModelAggregator *aggregator);
    void loadData();
};

DummyModel::DummyModel(QmlProfilerModelManager *manager,
                       Timeline::TimelineModelAggregator *aggregator) :
    QmlProfilerRangeModel(manager, Binding, aggregator)
{
}

void DummyModel::loadData()
{
    QmlEventType type(UndefinedMessage, Binding);
    const int typeIndex = modelManager()->appendEventType(QmlEventType(type));
    QCOMPARE(typeIndex, 0);

    for (int i = 0; i < 10; ++i) {
        QmlEvent event(i, typeIndex, {});
        event.setRangeStage(RangeStart);
        loadEvent(event, type);
    }

    for (int i = 10; i < 20; ++i) {
        QmlEvent event(i, typeIndex, {});
        event.setRangeStage(RangeEnd);
        loadEvent(event, type);
    }

    finalize();
}

QmlProfilerBindingLoopsRenderPassTest::QmlProfilerBindingLoopsRenderPassTest(QObject *parent) :
    QObject(parent)
{
}

void QmlProfilerBindingLoopsRenderPassTest::testInstance()
{
    const QmlProfilerBindingLoopsRenderPass *inst = QmlProfilerBindingLoopsRenderPass::instance();
    const QmlProfilerBindingLoopsRenderPass *inst2 = QmlProfilerBindingLoopsRenderPass::instance();
    QCOMPARE(inst, inst2);
    QVERIFY(inst != nullptr);
}

void QmlProfilerBindingLoopsRenderPassTest::testUpdate()
{
    const QmlProfilerBindingLoopsRenderPass *inst = QmlProfilerBindingLoopsRenderPass::instance();
    Timeline::TimelineAbstractRenderer renderer;
    Timeline::TimelineRenderState parentState(0, 8, 1, 1);
    Timeline::TimelineRenderPass::State *nullState = nullptr;
    QSGNode *nullNode = nullptr;
    Timeline::TimelineRenderPass::State *result =
            inst->update(&renderer, &parentState, nullptr, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    DummyModel model(&manager, &aggregator);
    renderer.setModel(&model);
    result = inst->update(&renderer, &parentState, nullptr, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    model.loadData();
    result = inst->update(&renderer, &parentState, nullptr, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    result = inst->update(&renderer, &parentState, nullptr, 2, 9, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QVERIFY(result->collapsedOverlay() != nullNode);
    QCOMPARE(result->expandedRows().count(), 2);  // all the loops are in one row
    QCOMPARE(result->collapsedRows().count(), 0); // it's an overlay
    QCOMPARE(result->expandedRows()[1]->childCount(), 1);
    auto node = static_cast<const QSGGeometryNode *>(result->expandedRows()[1]->firstChild());
    QSGMaterial *material1 = node->material();
    QVERIFY(material1 != nullptr);
    QCOMPARE(node->geometry()->vertexCount(), 7 * 4);
    node = static_cast<QSGGeometryNode *>(result->collapsedOverlay()->firstChild());
    QSGMaterial *material2 = node->material();
    QCOMPARE(node->geometry()->vertexCount(), 7 * 18);
    QVERIFY(material2 != nullptr);
    QCOMPARE(material1->type(), material2->type());
    QSGMaterialShader *shader1 = material1->createShader(QSGRendererInterface::RenderMode2D);
    QVERIFY(shader1 != 0);
    QSGMaterialShader *shader2 = material2->createShader(QSGRendererInterface::RenderMode2D);
    QVERIFY(shader2 != 0);

    delete shader1;
    delete shader2;

    result = inst->update(&renderer, &parentState, result, 0, 10, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QVERIFY(result->collapsedOverlay() != nullNode);
    QCOMPARE(result->expandedRows().count(), 2);
    QCOMPARE(result->collapsedRows().count(), 0);

    QCOMPARE(result->expandedRows()[1]->childCount(), 2);
    QCOMPARE(result->collapsedOverlay()->childCount(), 2);

    node = static_cast<QSGGeometryNode *>(result->expandedRows()[1]->childAtIndex(1));
    QCOMPARE(node->geometry()->vertexCount(), 4);
    node = static_cast<QSGGeometryNode *>(result->collapsedOverlay()->childAtIndex(1));
    QCOMPARE(node->geometry()->vertexCount(), 18);

    model.setExpanded(true);
    result = inst->update(&renderer, &parentState, result, 0, 10, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QVERIFY(result->collapsedOverlay() != nullNode);
    QCOMPARE(result->expandedRows().count(), 2);
    QCOMPARE(result->collapsedRows().count(), 0);

    parentState.setPassState(0, result);
    parentState.assembleNodeTree(&model, 1, 1);

    QVERIFY(parentState.collapsedOverlayRoot());
    QVERIFY(parentState.expandedRowRoot());
}

} // namespace Internal
} // namespace QmlProfiler
