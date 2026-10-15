// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scenegraphtimelinemodel_test.h"

#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

SceneGraphTimelineModelTest::SceneGraphTimelineModelTest()
    : model(&manager, &aggregator)
{}

// Polish (GUI thread) 10..20, Render (render thread) 15..30 and
// RenderPreprocess (render details) 15..25 overlap in time.
void SceneGraphTimelineModelTest::initTestCase()
{
    manager.initialize();

    const int polishType = manager.appendEventType(
        QmlEventType(SceneGraphFrame, UndefinedRangeType, SceneGraphPolishFrame));
    const int renderLoopType = manager.appendEventType(
        QmlEventType(SceneGraphFrame, UndefinedRangeType, SceneGraphRenderLoopFrame));
    const int rendererType = manager.appendEventType(
        QmlEventType(SceneGraphFrame, UndefinedRangeType, SceneGraphRendererFrame));

    QmlEvent event;
    event.setTypeIndex(polishType);
    event.setTimestamp(20);
    event.setNumbers({10});
    manager.appendEvent(QmlEvent(event));

    event.setTypeIndex(renderLoopType);
    event.setTimestamp(30);
    event.setNumbers({0, 15, 0});
    manager.appendEvent(QmlEvent(event));

    event.setTypeIndex(rendererType);
    event.setTimestamp(25);
    event.setNumbers({10, 0, 0, 0});
    manager.appendEvent(QmlEvent(event));

    manager.finalize();
}

int SceneGraphTimelineModelTest::indexOfStage(SceneGraphTimelineModel::SceneGraphStage stage) const
{
    for (int i = 0; i < model.count(); ++i) {
        if (model.selectionId(i) == stage)
            return i;
    }
    return -1;
}

void SceneGraphTimelineModelTest::testCollapsedRows()
{
    QCOMPARE(model.count(), 3);

    const int polishIndex = indexOfStage(SceneGraphTimelineModel::Polish);
    const int renderIndex = indexOfStage(SceneGraphTimelineModel::Render);
    const int preprocessIndex = indexOfStage(SceneGraphTimelineModel::RenderPreprocess);
    QVERIFY(polishIndex != -1);
    QVERIFY(renderIndex != -1);
    QVERIFY(preprocessIndex != -1);

    QCOMPARE(model.collapsedRow(polishIndex), 0);
    QCOMPARE(model.collapsedRow(renderIndex), 1);
    QVERIFY(model.collapsedRow(preprocessIndex) >= 2);
    QCOMPARE(model.collapsedRowCount(), model.collapsedRow(preprocessIndex) + 1);
}

} // namespace Profiler::Internal
