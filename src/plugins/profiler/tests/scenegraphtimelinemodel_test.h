// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/scenegraphtimelinemodel.h>
#include <profiler/qmlprofilermodelmanager.h>

#include <QObject>

namespace Profiler::Internal {

class SceneGraphTimelineModelTest : public QObject
{
    Q_OBJECT

public:
    SceneGraphTimelineModelTest();

private slots:
    void initTestCase();
    void testCollapsedRows();

private:
    int indexOfStage(SceneGraphTimelineModel::SceneGraphStage stage) const;

    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    SceneGraphTimelineModel model;
};

} // namespace Profiler::Internal
