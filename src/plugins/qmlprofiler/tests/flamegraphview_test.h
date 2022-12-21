// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/flamegraphview.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>

#include <tracing/timelinemodelaggregator.h>

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class FlameGraphViewTest : public QObject
{
    Q_OBJECT
public:
    FlameGraphViewTest(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void testSelection();
    void testContextMenu();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    FlameGraphView view;
};

} // namespace Internal
} // namespace QmlProfiler
