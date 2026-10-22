// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerDashboardTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testBreakdownSumsToScore_data();
    void testBreakdownSumsToScore();
    void testFlaggedFactors();
    void testShowSlowestFrame();

private:
    int m_originalTargetFps = 0;
};

} // namespace Profiler::Internal
