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
#include <timeline/timelinenotesrenderpass.h>
#include <timeline/timelinerenderstate.h>
#include <timeline/timelineabstractrenderer_p.h>

#include <QtTest>
#include <QSGMaterialShader>

using namespace Timeline;

class DummyModel : public TimelineModel {
public:
    DummyModel(int id = 12);
    void loadData();
};

class tst_TimelineNotesRenderPass : public QObject
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
    TimelineRenderState parentState(0, 8, 1, 1);
    TimelineRenderPass::State *nullState = 0;
    QSGNode *nullNode = 0;
    TimelineRenderPass::State *result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    DummyModel model;
    DummyModel otherModel(13);

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

    notes.add(12, 0, QLatin1String("x"));
    notes.add(12, 9, QLatin1String("xx"));
    notes.add(13, 0, QLatin1String("y"));
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
    QSGMaterialShader *shader1 = material1->createShader();
    QVERIFY(shader1 != 0);
    QSGMaterialShader *shader2 = material2->createShader();
    QVERIFY(shader2 != 0);
    QCOMPARE(shader1->attributeNames(), shader2->attributeNames());

    delete shader1;
    delete shader2;

    parentState.setPassState(0, result);
    parentState.assembleNodeTree(&model, 1, 1);

    runSceneGraphTest(parentState.collapsedOverlayRoot());
    runSceneGraphTest(parentState.expandedRowRoot());
}

QTEST_MAIN(tst_TimelineNotesRenderPass)

#include "tst_timelinenotesrenderpass.moc"

