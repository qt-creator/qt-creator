// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerruncontrol.h"

#include "qmlprofilerclientmanager.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilertracebackend.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>

#include <QtTaskTree/QBarrier>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Profiler::Internal {

Group qmlProfilerRecipe(RunControl *runControl)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    // The backend this run records into, bound at setup: by the time a handler
    // fires, liveBackend() may already be another run's, and this run's
    // document may have been closed.
    const Storage<QPointer<QmlProfilerTraceBackend>> backendStorage;

    const auto onSetup = [runControl, backendStorage](QBarrier &barrier) {
        QmlProfilerTool::instance()->finalizeRunControl(runControl);
        QmlProfilerTraceBackend *backend = QmlProfilerTool::instance()->liveBackend();
        QTC_ASSERT(backend, barrier.stopWithResult(DoneResult::Error); return);
        *backendStorage = backend;
        QmlProfilerClientManager *clientManager = backend->clientManager();
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionFailed,
                         &barrier, [barrier = &barrier] { barrier->stopWithResult(DoneResult::Error); });
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionClosed,
                         &barrier, &QBarrier::advance);
        QObject::connect(runControl, &RunControl::canceled, &barrier,
                         [barrier = &barrier, backend = QPointer(backend)] {
            if (!backend) {
                barrier->stopWithResult(DoneResult::Error);
                return;
            }
            QmlProfilerStateManager *stateManager = backend->stateManager();
            if (stateManager->currentState() == QmlProfilerStateManager::AppRunning)
                stateManager->setCurrentState(QmlProfilerStateManager::AppStopRequested);
            QObject::connect(stateManager, &QmlProfilerStateManager::stateChanged,
                             barrier, [stateManager, barrier, backend] {
                if (stateManager->currentState() == QmlProfilerStateManager::Idle) {
                    QmlProfilerTool::instance()->handleStop(backend);
                    barrier->stopWithResult(DoneResult::Error);
                }
            });
        });
        clientManager->setServer(runControl->qmlChannel());
        clientManager->connectToServer();
        runControl->reportStarted();
    };
    const auto onDone = [backendStorage] {
        QmlProfilerTool *tool = QmlProfilerTool::instance();
        if (!tool)
            return;
        QmlProfilerTraceBackend *backend = *backendStorage;
        tool->handleStop(backend);
        // Reaching onDone still in AppRunning means the application went away on its
        // own: the user-cancel path has already advanced the state past AppRunning.
        // Route that through AppDying rather than AppStopRequested - there is no live
        // connection left to ask the server to stop recording.
        if (backend
            && backend->stateManager()->currentState() == QmlProfilerStateManager::AppRunning) {
            backend->stateManager()->setCurrentState(QmlProfilerStateManager::AppDying);
        }
    };
    return { backendStorage, QBarrierTask(onSetup, onDone) };
}

Group localQmlProfilerRecipe(RunControl *runControl)
{
    runControl->requestQmlChannel();

    const auto modifier = [runControl](Process &process) {
        const QUrl serverUrl = runControl->qmlChannel();
        QString code;
        if (serverUrl.scheme() == Utils::urlSocketScheme())
            code = QString("file:%1").arg(serverUrl.path());
        else if (serverUrl.scheme() == Utils::urlTcpScheme())
            code = QString("port:%1").arg(serverUrl.port());
        else
            QTC_CHECK(false);

        const QString arguments = ProcessArgs::quoteArg(
            qmlDebugCommandLineArguments(QmlProfilerServices, code, true));

        CommandLine cmd = runControl->commandLine();
        cmd.prependArgs(arguments, CommandLine::Raw);
        process.setCommand(cmd.toLocal());
    };

    const ProcessTask processTask = runControl->processTaskWithModifier(modifier,
                                                                        {.setupCanceler = false});

    return {
        When (processTask, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
            qmlProfilerRecipe(runControl)
        }
    };
}

// Factories

// The bits plugged in in remote setups.
class QmlProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    QmlProfilerRunWorkerFactory()
    {
        setId("QmlProfilerRunWorkerFactory");
        setRecipeProducer(qmlProfilerRecipe);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
    }
};

// The full local profiler.
class LocalQmlProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    LocalQmlProfilerRunWorkerFactory()
    {
        setId(ProjectExplorer::Constants::QML_PROFILER_RUN_FACTORY);
        setRecipeProducer(&localQmlProfilerRecipe);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

        addSupportForLocalRunConfigs();
    }
};

void setupQmlProfilerRunning()
{
    static QmlProfilerRunWorkerFactory theQmlProfilerRunWorkerFactory;
    static LocalQmlProfilerRunWorkerFactory theLocalQmlProfilerRunWorkerFactory;
}

} // namespace Profiler::Internal
