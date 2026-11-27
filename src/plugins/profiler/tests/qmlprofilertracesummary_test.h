// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/qmlprofilermodelmanager.h>

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerTraceSummaryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testEvents();
    void testSortAndLimit_data();
    void testSortAndLimit();
    void testRange();
    void testFrames();
    void testStages();

private:
    QmlProfilerModelManager manager;
};

} // namespace Profiler::Internal
