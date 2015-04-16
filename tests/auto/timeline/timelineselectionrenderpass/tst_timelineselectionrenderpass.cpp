/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "../shared/runscenegraph.h"

#include <timeline/timelineselectionrenderpass.h>
#include <timeline/timelinerenderstate.h>
#include <timeline/timelineabstractrenderer_p.h>

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

DummyModel::DummyModel(int id) : TimelineModel(id, QLatin1String("dings"))
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
        return 0.002;
    return 1.0;
}

void tst_TimelineSelectionRenderPass::instance()
{
    const TimelineSelectionRenderPass *inst = TimelineSelectionRenderPass::instance();
    const TimelineSelectionRenderPass *inst2 = TimelineSelectionRenderPass::instance();
    QCOMPARE(inst, inst2);
    QVERIFY(inst != 0);
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
    QSGSimpleRectNode *node = static_cast<QSGSimpleRectNode *>(result->collapsedOverlay());
    QCOMPARE(node->rect(), QRectF(0, 0, 3, 30));
    QCOMPARE(node->color(), QColor(96, 0, 255));

    model.setExpanded(true);
    result = inst->update(&renderer, &parentState, result, 0, 10, false, 1);
    node = static_cast<QSGSimpleRectNode *>(result->expandedOverlay());
    QCOMPARE(node->rect(), QRectF(0, 0, 3, 30));
    QCOMPARE(node->color(), QColor(96, 0, 255));

    renderer.setSelectedItem(10);
    result = inst->update(&renderer, &parentState, result, 0, 11, false, 1);
    node = static_cast<QSGSimpleRectNode *>(result->expandedOverlay());
    QCOMPARE(node->rect(), QRectF(10, 27, 200, 3));
    QCOMPARE(node->color(), QColor(96, 0, 255));

    renderer.setSelectedItem(11);
    result = inst->update(&renderer, &parentState, result, 0, 12, false, 1);
    node = static_cast<QSGSimpleRectNode *>(result->expandedOverlay());
    QCOMPARE(node->rect(), QRectF(11, 0, 200, 30));
    QCOMPARE(node->color(), QColor(96, 0, 255));

    parentState.setPassState(0, result);
    parentState.assembleNodeTree(&model, 1, 1);

    runSceneGraph(parentState.collapsedOverlayRoot());
    runSceneGraph(parentState.expandedOverlayRoot());
}

QTEST_MAIN(tst_TimelineSelectionRenderPass)

#include "tst_timelineselectionrenderpass.moc"

