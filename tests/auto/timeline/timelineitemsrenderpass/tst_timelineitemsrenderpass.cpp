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
#include <QtTest>
#include <QSGMaterialShader>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QSGEngine>
#include <QSGAbstractRenderer>
#include <timeline/timelineitemsrenderpass.h>
#include <timeline/timelinerenderstate.h>

using namespace Timeline;

void renderMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type > QtDebugMsg)
        QTest::qFail(message.toLatin1().constData(), context.file, context.line);
    else
        QTest::qWarn(message.toLatin1().constData(), context.file, context.line);
}

void runSceneGraph(QSGNode *node)
{
    QSurfaceFormat format;
    format.setStencilBufferSize(8);
    format.setDepthBufferSize(24);

    QOpenGLContext context;
    context.setFormat(format);
    QVERIFY(context.create());

    QOffscreenSurface surface;
    surface.setFormat(format);
    surface.create();

    QVERIFY(context.makeCurrent(&surface));

    QSGEngine engine;
    QSGRootNode root;
    root.appendChildNode(node);
    engine.initialize(&context);

    QSGAbstractRenderer *renderer = engine.createRenderer();
    QVERIFY(renderer != 0);
    renderer->setRootNode(&root);
    QtMessageHandler originalHandler = qInstallMessageHandler(renderMessageHandler);
    renderer->renderScene();
    qInstallMessageHandler(originalHandler);
    delete renderer;

    // Unfortunately we cannot check the results of the rendering. But at least we know the shaders
    // have not crashed here.

    context.doneCurrent();
}

class DummyModel : public TimelineModel {
public:
    DummyModel();
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

DummyModel::DummyModel() : TimelineModel(12, QLatin1String("dings"))
{
}

void DummyModel::loadData()
{
    for (int i = 0; i < 10; ++i)
        insert(i, 1, 1);
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
    TimelineRenderState parentState(0, 8, 1, 1);
    TimelineRenderPass::State *nullState = 0;
    QSGNode *nullNode = 0;
    TimelineRenderPass::State *result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    DummyModel model;
    renderer.setModel(&model);
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    model.loadData();
    result = inst->update(&renderer, &parentState, 0, 0, 0, true, 1);
    QCOMPARE(result, nullState);

    result = inst->update(&renderer, &parentState, 0, 2, 8, true, 1);
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
    QCOMPARE(node->geometry()->vertexCount(), 26);
    node = static_cast<QSGGeometryNode *>(result->collapsedRows()[0]->firstChild());
    QSGMaterial *material2 = node->material();
    QCOMPARE(node->geometry()->vertexCount(), 26);
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
    QCOMPARE(result->expandedOverlay(), nullNode);
    QCOMPARE(result->expandedRows().count(), 1);
    QCOMPARE(result->collapsedRows().count(), 1);
    QCOMPARE(result->expandedRows()[0]->childCount(), 2);
    QCOMPARE(result->collapsedRows()[0]->childCount(), 2);
    node = static_cast<QSGGeometryNode *>(result->expandedRows()[0]->lastChild());
    QCOMPARE(node->geometry()->vertexCount(), 8);
    node = static_cast<QSGGeometryNode *>(result->collapsedRows()[0]->lastChild());
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

    runSceneGraph(parentState.collapsedRowRoot());
    runSceneGraph(parentState.expandedRowRoot());
}

QTEST_MAIN(tst_TimelineItemsRenderPass)

#include "tst_timelineitemsrenderpass.moc"

