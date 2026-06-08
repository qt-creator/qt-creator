// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "flamegraphview.h"
#include "qmlprofilerstatisticsview.h"
#include "qmlprofilertraceview.h"
#include "quick3dframeview.h"

#include <coreplugin/perspective.h>

namespace QmlProfiler::Internal {

class QmlProfilerStateManager;

class QmlProfilerViewManager : public QObject
{
    Q_OBJECT

public:
    QmlProfilerViewManager(QmlProfilerModelManager *modelManager,
                           QmlProfilerStateManager *profilerState);

    QmlProfilerTraceView *traceView() { return &m_traceView; }
    QmlProfilerStatisticsView *statisticsView() { return &m_statisticsView; }
    FlameGraphView *flameGraphView() { return &m_flameGraphView; }
    Quick3DFrameView *quick3dView() { return &m_quick3dView; }
    Core::Perspective *perspective() { return &m_perspective; }

    void clear();

signals:
    void typeSelected(int typeId);
    void gotoSourceLocation(QString,int,int);

private:
    QmlProfilerModelManager *m_profilerModelManager = nullptr;
    QmlProfilerStateManager *m_profilerState = nullptr;
    QmlProfilerTraceView m_traceView;
    QmlProfilerStatisticsView m_statisticsView;
    FlameGraphView m_flameGraphView;
    Quick3DFrameView m_quick3dView;
    Core::Perspective m_perspective;
};

} // namespace QmlProfiler::Internal
