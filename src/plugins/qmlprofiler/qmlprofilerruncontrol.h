// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerstatemanager.h"

#include <projectexplorer/runcontrol.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/outputformat.h>
#include <utils/port.h>

#include <qmldebug/qmloutputparser.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    QmlProfilerRunner(ProjectExplorer::RunControl *runControl);
    ~QmlProfilerRunner() override;

    void registerProfilerStateManager( QmlProfilerStateManager *profilerState );
    void cancelProcess();

private:
    void start() override;
    void stop() override;

    void profilerStateChanged();

    class QmlProfilerRunnerPrivate;
    QmlProfilerRunnerPrivate *d;
};

ProjectExplorer::RunWorker *createLocalQmlProfilerWorker(ProjectExplorer::RunControl *runControl);
void setupQmlProfilerRunning();

} // QmlProfiler::Internal

class SimpleQmlProfilerRunnerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit SimpleQmlProfilerRunnerFactory(const QList<Utils::Id> &runConfigs, const QList<Utils::Id> &extraRunModes = {})
    {
        cloneProduct(ProjectExplorer::Constants::QML_PROFILER_RUN_FACTORY);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        for (const Utils::Id &id : extraRunModes)
            addSupportedRunMode(id);
        setSupportedRunConfigs(runConfigs);
    }
};

} // QmlProfiler
