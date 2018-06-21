/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmlprofilerstatisticsview.h"
#include "qmlprofilertraceview.h"
#include "flamegraphview.h"

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool;

class QmlProfilerViewManager : public QObject
{
    Q_OBJECT

public:
    QmlProfilerViewManager(QObject *parent,
                           QmlProfilerModelManager *modelManager,
                           QmlProfilerStateManager *profilerState);

    QmlProfilerTraceView *traceView() const { return m_traceView; }
    QmlProfilerStatisticsView *statisticsView() const { return m_statisticsView; }
    FlameGraphView *flameGraphView() const { return m_flameGraphView; }

    void clear();

signals:
    void typeSelected(int typeId);
    void gotoSourceLocation(QString,int,int);

private:
    QmlProfilerTraceView *m_traceView = nullptr;
    QmlProfilerStatisticsView *m_statisticsView = nullptr;
    FlameGraphView *m_flameGraphView = nullptr;
    QmlProfilerStateManager *m_profilerState = nullptr;
    QmlProfilerModelManager *m_profilerModelManager = nullptr;
};


} // namespace Internal
} // namespace QmlProfiler
