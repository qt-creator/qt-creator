// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/flamegraphmodel.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>

#include <tracing/timelinemodelaggregator.h>

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class FlameGraphModelTest : public QObject
{
    Q_OBJECT
public:
    FlameGraphModelTest(QObject *parent = nullptr);
    static int generateData(QmlProfilerModelManager *manager,
                            Timeline::TimelineModelAggregator *aggregator);

private slots:
    void initTestCase();
    void testIndex();
    void testCounts();
    void testData();
    void testRoleNames();
    void testNotes();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    FlameGraphModel model;
    int rangeModelId = -1;
};

} // namespace Internal
} // namespace QmlProfiler
