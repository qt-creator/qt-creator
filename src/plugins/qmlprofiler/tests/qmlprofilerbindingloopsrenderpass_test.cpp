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


#include "qmlprofilerbindingloopsrenderpass_test.h"

#include <qmlprofiler/qmlprofilerbindingloopsrenderpass.h>
#include <qmlprofiler/qmlprofilerrangemodel.h>
#include <timeline/timelineabstractrenderer.h>
#include <timeline/runscenegraphtest.h>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

class DummyModel : public QmlProfilerRangeModel {
public:
    DummyModel(QmlProfilerModelManager *manager);
    void loadData();
};

DummyModel::DummyModel(QmlProfilerModelManager *manager) :
    QmlProfilerRangeModel(manager, Binding)
{
}

void DummyModel::loadData()
{
    QmlEventType type(MaximumMessage, Binding);
    modelManager()->addEventType(type);

    for (int i = 0; i < 10; ++i) {
        QmlEvent event(i, 0, {});
        event.setRangeStage(RangeStart);
        loadEvent(event, type);
    }

    for (int i = 10; i < 20; ++i) {
        QmlEvent event(i, 0, {});
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
    QVERIFY(inst != 0);
}

void QmlProfilerBindingLoopsRenderPassTest::testUpdate()
{
    const QmlProfilerBindingLoopsRenderPass *inst = QmlProfilerBindingLoopsRenderPass::instance();
    Timeline::TimelineAbstractRenderer renderer;
    Timeline::TimelineRenderState parentState(0, 8, 1, 1);
    Timeline::TimelineRenderPass::State *nullState = 0;
    QSGNode *nullNode = 0;
    Timeline::TimelineRenderPass::State *result =
            inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    QmlProfilerModelManager manager(nullptr);
    DummyModel model(&manager);
    renderer.setModel(&model);
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    model.loadData();
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    result = inst->update(&renderer, &parentState, 0, 2, 9, true, 1);
    QVERIFY(result != nullState);
    QCOMPARE(result->expandedOverlay(), nullNode);
    QVERIFY(result->collapsedOverlay() != nullNode);
    QCOMPARE(result->expandedRows().count(), 2);  // all the loops are in one row
    QCOMPARE(result->collapsedRows().count(), 0); // it's an overlay
    QCOMPARE(result->expandedRows()[1]->childCount(), 1);
    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(result->expandedRows()[1]->firstChild());
    QSGMaterial *material1 = node->material();
    QVERIFY(material1 != 0);
    QCOMPARE(node->geometry()->vertexCount(), 7 * 4);
    node = static_cast<QSGGeometryNode *>(result->collapsedOverlay()->firstChild());
    QSGMaterial *material2 = node->material();
    QCOMPARE(node->geometry()->vertexCount(), 7 * 18);
    QVERIFY(material2 != 0);
    QCOMPARE(material1->type(), material2->type());
    QSGMaterialShader *shader1 = material1->createShader();
    QVERIFY(shader1 != 0);
    QSGMaterialShader *shader2 = material2->createShader();
    QVERIFY(shader2 != 0);
    QCOMPARE(shader1->attributeNames(), shader2->attributeNames());

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

    Timeline::runSceneGraphTest(parentState.collapsedOverlayRoot());
    Timeline::runSceneGraphTest(parentState.expandedRowRoot());
}

} // namespace Internal
} // namespace QmlProfiler
