// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerruncontrol.h"

#include "qmlprofilerclientmanager.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertool.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>

#include <solutions/tasking/barrier.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace QmlProfiler::Internal {

Group qmlProfilerRecipe(RunControl *runControl)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    const auto onSetup = [runControl](Barrier &barrier) {
        QmlProfilerTool::instance()->finalizeRunControl(runControl);
        QmlProfilerClientManager *clientManager = QmlProfilerTool::instance()->clientManager();
        RunInterface *iface = runStorage().activeStorage();
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionFailed,
                         &barrier, [barrier = &barrier] { barrier->stopWithResult(DoneResult::Error); });
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionClosed,
                         &barrier, &Barrier::advance);
        QObject::connect(iface, &RunInterface::canceled, &barrier, [barrier = &barrier] {
            if (QmlProfilerTool::instance() == nullptr) {
                barrier->stopWithResult(DoneResult::Error);
                return;
            }
            QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
            if (stateManager) {
                if (stateManager->currentState() == QmlProfilerStateManager::AppRunning)
                    stateManager->setCurrentState(QmlProfilerStateManager::AppStopRequested);
                QObject::connect(stateManager, &QmlProfilerStateManager::stateChanged,
                                 barrier, [stateManager, barrier] {
                    if (stateManager->currentState() == QmlProfilerStateManager::Idle) {
                        QmlProfilerTool::instance()->handleStop();
                        barrier->stopWithResult(DoneResult::Error);
                    }
                });
            }
        });
        clientManager->setServer(runControl->qmlChannel());
        clientManager->connectToServer();
        emit iface->started();
    };
    const auto onDone = [] {
        if (QmlProfilerTool::instance() == nullptr)
            return;
        QmlProfilerTool::instance()->handleStop();
        QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
        if (stateManager && stateManager->currentState() == QmlProfilerStateManager::AppRunning)
            stateManager->setCurrentState(QmlProfilerStateManager::AppStopRequested);
    };
    return { BarrierTask(onSetup, onDone) };
}

RunWorker *createLocalQmlProfilerWorker(RunControl *runControl)
{
    auto profiler = new RunWorker(runControl, qmlProfilerRecipe(runControl));
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

    auto worker = createProcessWorker(runControl, modifier);
    worker->addStopDependency(profiler);
    // We need to open the local server before the application tries to connect.
    // In the TCP case, it doesn't hurt either to start the profiler before.
    worker->addStartDependency(profiler);
    return worker;
}

// Factories

// The bits plugged in in remote setups.
class QmlProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    QmlProfilerRunWorkerFactory()
    {
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
        setProducer(&createLocalQmlProfilerWorker);
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

} // QmlProfiler::Internal
