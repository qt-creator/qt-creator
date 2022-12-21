// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/memoryusagemodel.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

class MemoryUsageModelTest : public QObject
{
    Q_OBJECT
public:
    MemoryUsageModelTest(QObject *parent = 0);

private slots:
    void initTestCase();
    void testRowMaxValue();
    void testTypeId();
    void testColor();
    void testLabels();
    void testDetails();
    void testExpandedRow();
    void testCollapsedRow();
    void testLocation();
    void testRelativeHeight();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    MemoryUsageModel model;

    int heapPageTypeId = -1;
    int smallItemTypeId = -1;
    int largeItemTypeId = -1;
    int rangeTypeId = -1;
};

} // namespace Internal
} // namespace QmlProfiler
