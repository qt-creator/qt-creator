// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerstatemanager.h"

#include <projectexplorer/runcontrol.h>

#include <utils/outputformat.h>
#include <utils/port.h>

#include <qmldebug/qmloutputparser.h>

namespace QmlProfiler::Internal {

class QmlProfilerRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    QmlProfilerRunner(ProjectExplorer::RunControl *runControl);
    ~QmlProfilerRunner() override;

    void setServerUrl(const QUrl &serverUrl);
    QUrl serverUrl() const;

    void registerProfilerStateManager( QmlProfilerStateManager *profilerState );

    void cancelProcess();
    void notifyRemoteFinished();

private:
    void start() override;
    void stop() override;

    void profilerStateChanged();

    class QmlProfilerRunnerPrivate;
    QmlProfilerRunnerPrivate *d;
};

class LocalQmlProfilerSupport : public ProjectExplorer::SimpleTargetRunner
{
    Q_OBJECT

public:
    LocalQmlProfilerSupport(ProjectExplorer::RunControl *runControl);
    LocalQmlProfilerSupport(ProjectExplorer::RunControl *runControl,
                            const QUrl &serverUrl);
};

// The bits plugged in in remote setups.
class QmlProfilerRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    QmlProfilerRunWorkerFactory();
};

// The full local profiler.
class LocalQmlProfilerRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    LocalQmlProfilerRunWorkerFactory();
};

} // QmlProfiler::Internal
