// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlprofilerfindingsmodel.h"
#include "../qmlprofilermodelmanager.h"

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerFindingsModelTest : public QObject
{
    Q_OBJECT

public:
    QmlProfilerFindingsModelTest();

private slots:
    void initTestCase();
    void testSlowCompileReported();
    void testFastCompileIgnored();
    void testPixmapLoadErrorsAggregated();
    void testSyncViewLoadAttributedToHandler();
    void testPeriodicHandlerReported();
    void testOversizedPixmapReported();
    void testPerFrameCostReported();
    void testLocationWithoutLine();
    void testSeverityOrdering();
    void testJsonExport();
    void testClear();

private:
    QmlProfilerModelManager manager;
    QmlProfilerFindingsModel model;

    int slowCompileTypeId = -1;
    int fastCompileTypeId = -1;
    int pixmapErrorTypeId = -1;
    int pixmapSizeTypeId = -1;
    int handlerTypeId = -1;
    int createdTypeId = -1;
    int timerHandlerTypeId = -1;
    int bindingTypeId = -1;
    int animationTypeId = -1;
};

} // namespace Profiler::Internal
