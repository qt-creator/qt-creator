// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/ctfstatisticsmodel.h>
#include <profiler/ctftracemanager.h>

#include <tracing/timelinemodelaggregator.h>

#include <QObject>

namespace Profiler::Internal {

class CtfTimelineModel;

class CtfTimelineModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCommandLocation();
    void testOneTypeInSeveralPlaces();
    void testEventsWithoutLocation();

private:
    Timeline::TimelineModelAggregator m_aggregator;
    CtfStatisticsModel m_statistics{nullptr};
    CtfTraceManager m_manager{nullptr, &m_aggregator, &m_statistics};
    CtfTimelineModel *m_model = nullptr; // Owned by the aggregator.
};

} // namespace Profiler::Internal
