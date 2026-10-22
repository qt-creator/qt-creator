// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlprofilerdashboardstats.h"
#include "../qmlprofilermodelmanager.h"

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerDashboardStatsTest : public QObject
{
    Q_OBJECT

public:
    QmlProfilerDashboardStatsTest();

private slots:
    void initTestCase();
    void testRatedAgainstTarget_data();
    void testRatedAgainstTarget();
    void testDashboardFollowsSetting();
    void testBreakdownRatedAgainstTarget();

private:
    QmlProfilerModelManager manager;
    QmlProfilerDashboardStats stats;
};

} // namespace Profiler::Internal
