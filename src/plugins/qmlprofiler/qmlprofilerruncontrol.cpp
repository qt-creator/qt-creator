// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerruncontrol.h"

#include "qmlprofilertool.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/url.h>

#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;

namespace QmlProfiler::Internal {

//
// QmlProfilerRunControlPrivate
//

class QmlProfilerRunner::QmlProfilerRunnerPrivate
{
public:
    QPointer<QmlProfilerStateManager> m_profilerState;
};

//
// QmlProfilerRunControl
//

QmlProfilerRunner::QmlProfilerRunner(RunControl *runControl)
    : RunWorker(runControl)
    , d(new QmlProfilerRunnerPrivate)
{
    setId("QmlProfilerRunner");
    runControl->requestQmlChannel();
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    setSupportsReRunning(false);
}

QmlProfilerRunner::~QmlProfilerRunner()
{
    delete d;
}

void QmlProfilerRunner::start()
{
    if (!d->m_profilerState)
        QmlProfilerTool::instance()->finalizeRunControl(this);
    QTC_ASSERT(d->m_profilerState, return);
    reportStarted();
}

void QmlProfilerRunner::stop()
{
    if (!d->m_profilerState) {
        reportStopped();
        return;
    }

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopRequested);
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Pressed "stop" a second time. Kill the application without collecting data
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        reportStopped();
        break;
    case QmlProfilerStateManager::Idle:
    case QmlProfilerStateManager::AppDying:
        // valid, but no further action is needed
        break;
    default: {
        const QString message = QString::fromLatin1("Unexpected engine stop from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
    }
        break;
    }
}

void QmlProfilerRunner::cancelProcess()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::Idle:
        break;
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        break;
    default: {
        const QString message = QString::fromLatin1("Unexpected process termination requested with state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
        return;
    }
    }
    runControl()->initiateStop();
}

void QmlProfilerRunner::registerProfilerStateManager( QmlProfilerStateManager *profilerState )
{
    // disconnect old
    if (d->m_profilerState)
        disconnect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
                   this, &QmlProfilerRunner::profilerStateChanged);

    d->m_profilerState = profilerState;

    // connect
    if (d->m_profilerState)
        connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
                this, &QmlProfilerRunner::profilerStateChanged);
}

void QmlProfilerRunner::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::Idle:
        reportStopped();
        break;
    default:
        break;
    }
}

RunWorker *createLocalQmlProfilerWorker(RunControl *runControl)
{
    auto worker = new SimpleTargetRunner(runControl);

    worker->setId("LocalQmlProfilerSupport");

    auto profiler = new QmlProfilerRunner(runControl);

    worker->addStopDependency(profiler);
    // We need to open the local server before the application tries to connect.
    // In the TCP case, it doesn't hurt either to start the profiler before.
    worker->addStartDependency(profiler);

    worker->setStartModifier([worker, runControl] {

        QUrl serverUrl = runControl->qmlChannel();
        QString code;
        if (serverUrl.scheme() == Utils::urlSocketScheme())
            code = QString("file:%1").arg(serverUrl.path());
        else if (serverUrl.scheme() == Utils::urlTcpScheme())
            code = QString("port:%1").arg(serverUrl.port());
        else
            QTC_CHECK(false);

        QString arguments = Utils::ProcessArgs::quoteArg(
            qmlDebugCommandLineArguments(QmlProfilerServices, code, true));

        Utils::CommandLine cmd = worker->commandLine();
        const QString oldArgs = cmd.arguments();
        cmd.setArguments(arguments);
        cmd.addArgs(oldArgs, Utils::CommandLine::Raw);
        worker->setCommandLine(cmd);
        worker->forceRunOnHost();
    });

    return worker;
}

// Factories

// The bits plugged in in remote setups.
class QmlProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    QmlProfilerRunWorkerFactory()
    {
        setProduct<QmlProfilerRunner>();
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
