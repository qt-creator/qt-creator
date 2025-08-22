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
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionFailed,
                         &barrier, [barrier = &barrier] { barrier->stopWithResult(DoneResult::Error); });
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionClosed,
                         &barrier, &Barrier::advance);
        QObject::connect(runControl, &RunControl::canceled, &barrier, [barrier = &barrier] {
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
        runControl->reportStarted();
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

        QmlProfilerTool::instance()->finalizeRunControl(runControl);
        QmlProfilerClientManager *clientManager = QmlProfilerTool::instance()->clientManager();

        const auto handleDone = [runControl, process = &process] {
            if (QmlProfilerTool::instance()) {
                QmlProfilerTool::instance()->handleStop();
                QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
                if (stateManager && stateManager->currentState() == QmlProfilerStateManager::AppRunning)
                    stateManager->setCurrentState(QmlProfilerStateManager::AppStopRequested);
            }
            runControl->handleProcessCancellation(process);
        };

        QObject::connect(clientManager, &QmlProfilerClientManager::connectionFailed,
                         &process, [handleDone] { handleDone(); });
        QObject::connect(clientManager, &QmlProfilerClientManager::connectionClosed,
                         &process, [handleDone] { handleDone(); });
        QObject::connect(runControl, &RunControl::canceled, &process, [handleDone, process = &process] {
            QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
            if (QmlProfilerTool::instance() == nullptr || stateManager == nullptr) {
                handleDone();
                return;
            }
            if (stateManager->currentState() == QmlProfilerStateManager::AppRunning)
                stateManager->setCurrentState(QmlProfilerStateManager::AppStopRequested);
            QObject::connect(stateManager, &QmlProfilerStateManager::stateChanged,
                             process, [handleDone, stateManager] {
                if (stateManager->currentState() == QmlProfilerStateManager::Idle) {
                    QmlProfilerTool::instance()->handleStop();
                    handleDone();
                }
            });
        });
        clientManager->setServer(runControl->qmlChannel());
        clientManager->connectToServer();
    };

    return { runControl->processRecipe(modifier, {false, false}) };
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

} // QmlProfiler::Internal
