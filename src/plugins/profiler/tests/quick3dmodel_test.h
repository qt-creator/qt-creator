// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/quick3dmodel.h>

#include <QObject>

namespace Profiler::Internal {

class Quick3DModelTest : public QObject
{
    Q_OBJECT

public:
    Quick3DModelTest();

private slots:
    void initTestCase();
    void testRowMaxValue();
    void testFormatRowValue();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    Quick3DModel model;
};

} // namespace Profiler::Internal
