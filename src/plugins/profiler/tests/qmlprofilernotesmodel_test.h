// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/qmlprofilermodelmanager.h>

#include <tracing/timelinemodelaggregator.h>

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerRangeModel;

class QmlProfilerNotesModelTest : public QObject
{
    Q_OBJECT

public:
    QmlProfilerNotesModelTest();

private slots:
    void initTestCase();
    void testStashSavesRowsFromOne();
    void testRestoreReadsRowsFromOne();
    void testRestoreWithoutRow();

private:
    QmlProfilerModelManager m_manager;
    Timeline::TimelineModelAggregator m_aggregator;
    QmlProfilerRangeModel *m_rangeModel = nullptr;
    int m_innerIndex = -1;
};

} // namespace Profiler::Internal
