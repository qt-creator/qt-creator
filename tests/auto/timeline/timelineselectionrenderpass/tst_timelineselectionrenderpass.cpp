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

#include <timeline/runscenegraphtest.h>
#include <timeline/timelineselectionrenderpass.h>
#include <timeline/timelinerenderstate.h>
#include <timeline/timelineabstractrenderer_p.h>
#include <timeline/timelineitemsrenderpass.h>

#include <QtTest>
#include <QSGMaterialShader>
#include <QSGSimpleRectNode>

using namespace Timeline;

class DummyModel : public TimelineModel {
public:
    DummyModel(int id = 12);
    void loadData();
    float relativeHeight(int index) const;
};

class tst_TimelineSelectionRenderPass : public QObject
{
    Q_OBJECT

private slots:
    void instance();
    void update();
};

DummyModel::DummyModel(int id) : TimelineModel(id)
{
}

void DummyModel::loadData()
{
    for (int i = 0; i < 10; ++i)
        insert(i, 1, 1);
    insert(10, 200, 200);
    insert(11, 200, 200);
}

float DummyModel::relativeHeight(int index) const
{
    if (index == 10)
        return 0.002f;
    return 1.0f;
}

void tst_TimelineSelectionRenderPass::instance()
{
    const TimelineSelectionRenderPass *inst = TimelineSelectionRenderPass::instance();
    const TimelineSelectionRenderPass *inst2 = TimelineSelectionRenderPass::instance();
    QCOMPARE(inst, inst2);
    QVERIFY(inst != 0);
}

void compareSelectionNode(QSGNode *node, const QRectF &rect, int selectionId)
{
    QSGGeometryNode *geometryNode = static_cast<QSGGeometryNode *>(node);
    QSGGeometry *geometry = geometryNode->geometry();
    QCOMPARE(geometry->vertexCount(), 4);
    QCOMPARE(geometry->drawingMode(), (GLenum)GL_TRIANGLE_STRIP);
    OpaqueColoredPoint2DWithSize *data =
            static_cast<OpaqueColoredPoint2DWithSize *>(geometry->vertexData());
    float *lowerLeft = reinterpret_cast<float *>(data);
    float *lowerRight = reinterpret_cast<float *>(++data);
    float *upperLeft = reinterpret_cast<float *>(++data);
    float *upperRight = reinterpret_cast<float *>(++data);

    QCOMPARE(QRectF(QPointF(upperLeft[0], upperLeft[1]), QPointF(lowerRight[0], lowerRight[1])),
            rect);
    QCOMPARE(lowerRight[0], upperRight[0]);
    QCOMPARE(lowerRight[1], lowerLeft[1]);
    QCOMPARE(upperLeft[0], lowerLeft[0]);
    QCOMPARE(upperLeft[1], upperRight[1]);

    QCOMPARE(int(lowerLeft[4]), selectionId);
    QCOMPARE(int(lowerRight[4]), selectionId);
    QCOMPARE(int(upperLeft[4]), selectionId);
    QCOMPARE(int(upperRight[4]), selectionId);

    TimelineItemsMaterial *material = static_cast<TimelineItemsMaterial *>(
                geometryNode->material());
    QVERIFY(!(material->flags() & QSGMaterial::Blending));
}

void tst_TimelineSelectionRenderPass::update()
{
    const TimelineSelectionRenderPass *inst = TimelineSelectionRenderPass::instance();
    TimelineAbstractRenderer renderer;
    TimelineRenderState parentState(0, 400, 1, 1);
    TimelineRenderPass::State *nullState = 0;
    QSGNode *nullNode = 0;
    TimelineRenderPass::State *result = inst->update(&renderer, &parentState, 0, 0, 10, true, 1);
    QCOMPARE(result, nullState);

    DummyModel model;

    result = inst->update(&renderer, &parentState, 0, 0, 10, true, 1);
    QCOMPARE(result, nullState);

    renderer.setModel(&model);
    result = inst->update(&renderer, &parentState, 0, 0, 10, true, 1);
    QCOMPARE(result, nullState);

    model.loadData();
    result = inst->update(&renderer, &parentState, 0, 0, 10, true, 1);
    QVERIFY(result != nullState);

    renderer.setSelectedItem(0);
    result = inst->update(&renderer, &parentState, 0, 0, 10, true, 1);
    QVERIFY(result != nullState);

    // The selection renderer creates a overlays.
    QVERIFY(result->collapsedOverlay() != nullNode);
    QVERIFY(result->expandedOverlay() != nullNode);
    QCOMPARE(result->expandedRows().count(), 0);
    QCOMPARE(result->collapsedRows().count(), 0);

    TimelineRenderPass::State *result2 = inst->update(&renderer, &parentState, result, 0, 10, false,
                                                      1);
    QCOMPARE(result2, result);

    renderer.setSelectedItem(1);
    result = inst->update(&renderer, &parentState, result, 0, 10, false, 1);
    QVERIFY(result != nullState);
    compareSelectionNode(result->collapsedOverlay(), QRectF(1, 0, 1, 30), model.selectionId(1));

    model.setExpanded(true);
    result = inst->update(&renderer, &parentState, result, 0, 10, false, 1);
    QVERIFY(result != nullState);
    compareSelectionNode(result->expandedOverlay(), QRectF(1, 0, 1, 30), model.selectionId(1));

    renderer.setSelectedItem(10);
    result = inst->update(&renderer, &parentState, result, 0, 11, false, 1);
    QVERIFY(result != nullState);
    float top = 30 * (1.0 - model.relativeHeight(10));
    compareSelectionNode(result->expandedOverlay(), QRectF(10, top, 200, 30 - top),
                         model.selectionId(10));

    renderer.setSelectedItem(11);
    result = inst->update(&renderer, &parentState, result, 0, 12, false, 1);
    QVERIFY(result != nullState);
    compareSelectionNode(result->expandedOverlay(), QRectF(11, 0, 200, 30), model.selectionId(11));

    parentState.setPassState(0, result);
    parentState.assembleNodeTree(&model, 1, 1);

    runSceneGraphTest(parentState.collapsedOverlayRoot());
    runSceneGraphTest(parentState.expandedOverlayRoot());
}

QTEST_MAIN(tst_TimelineSelectionRenderPass)

#include "tst_timelineselectionrenderpass.moc"

