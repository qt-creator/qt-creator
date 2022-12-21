// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerstatisticsview.h"
#include "qmlprofilertraceview.h"
#include "quick3dframeview.h"
#include "flamegraphview.h"

namespace Utils { class Perspective; }

namespace QmlProfiler {
namespace Internal {

class QmlProfilerViewManager : public QObject
{
    Q_OBJECT

public:
    QmlProfilerViewManager(QObject *parent,
                           QmlProfilerModelManager *modelManager,
                           QmlProfilerStateManager *profilerState);
    ~QmlProfilerViewManager() override;

    QmlProfilerTraceView *traceView() const { return m_traceView; }
    QmlProfilerStatisticsView *statisticsView() const { return m_statisticsView; }
    FlameGraphView *flameGraphView() const { return m_flameGraphView; }
    Quick3DFrameView *quick3dView() const { return m_quick3dView; }
    Utils::Perspective *perspective() const { return m_perspective; }

    void clear();

signals:
    void typeSelected(int typeId);
    void gotoSourceLocation(QString,int,int);
    void viewsCreated();

private:
    void createViews();

    QmlProfilerTraceView *m_traceView = nullptr;
    QmlProfilerStatisticsView *m_statisticsView = nullptr;
    FlameGraphView *m_flameGraphView = nullptr;
    Quick3DFrameView *m_quick3dView = nullptr;
    QmlProfilerStateManager *m_profilerState = nullptr;
    QmlProfilerModelManager *m_profilerModelManager = nullptr;
    Utils::Perspective *m_perspective = nullptr;
};


} // namespace Internal
} // namespace QmlProfiler
