// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerruncontrol.h"

#include "qmlprofilertool.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/url.h>

#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;

namespace QmlProfiler::Internal {

const char QmlServerUrl[] = "QmlServerUrl";

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

void QmlProfilerRunner::notifyRemoteFinished()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        break;
    case QmlProfilerStateManager::Idle:
        break;
    default:
        const QString message = QString::fromLatin1("Process died unexpectedly from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
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

void QmlProfilerRunner::setServerUrl(const QUrl &serverUrl)
{
    recordData(QmlServerUrl, serverUrl);
}

QUrl QmlProfilerRunner::serverUrl() const
{
    QVariant recordedServer = recordedData(QmlServerUrl);
    return recordedServer.toUrl();
}

//
// LocalQmlProfilerSupport
//

static QUrl localServerUrl(RunControl *runControl)
{
    QUrl serverUrl;
    Kit *kit = runControl->kit();
    const QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);
    if (version) {
        if (version->qtVersion() >= QVersionNumber(5, 6, 0))
            serverUrl = Utils::urlFromLocalSocket();
        else
            serverUrl = Utils::urlFromLocalHostAndFreePort();
    } else {
        qWarning("Running QML profiler on Kit without Qt version?");
        serverUrl = Utils::urlFromLocalHostAndFreePort();
    }
    return serverUrl;
}

LocalQmlProfilerSupport::LocalQmlProfilerSupport(RunControl *runControl)
    : LocalQmlProfilerSupport(runControl, localServerUrl(runControl))
{
}

LocalQmlProfilerSupport::LocalQmlProfilerSupport(RunControl *runControl, const QUrl &serverUrl)
    : SimpleTargetRunner(runControl)
{
    setId("LocalQmlProfilerSupport");

    auto profiler = new QmlProfilerRunner(runControl);
    profiler->setServerUrl(serverUrl);

    addStopDependency(profiler);
    // We need to open the local server before the application tries to connect.
    // In the TCP case, it doesn't hurt either to start the profiler before.
    addStartDependency(profiler);

    setStartModifier([this, profiler, serverUrl] {

        QUrl serverUrl = profiler->serverUrl();
        QString code;
        if (serverUrl.scheme() == Utils::urlSocketScheme())
            code = QString("file:%1").arg(serverUrl.path());
        else if (serverUrl.scheme() == Utils::urlTcpScheme())
            code = QString("port:%1").arg(serverUrl.port());
        else
            QTC_CHECK(false);

        QString arguments = Utils::ProcessArgs::quoteArg(
                                QmlDebug::qmlDebugCommandLineArguments(QmlDebug::QmlProfilerServices, code, true));

        Utils::CommandLine cmd = commandLine();
        const QString oldArgs = cmd.arguments();
        cmd.setArguments(arguments);
        cmd.addArgs(oldArgs, Utils::CommandLine::Raw);
        setCommandLine(cmd);

        forceRunOnHost();
    });
}

// Factories

// The bits plugged in in remote setups.
QmlProfilerRunWorkerFactory::QmlProfilerRunWorkerFactory()
{
    setProduct<QmlProfilerRunner>();
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
}

// The full local profiler.
LocalQmlProfilerRunWorkerFactory::LocalQmlProfilerRunWorkerFactory()
{
    setProduct<LocalQmlProfilerSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // QmlProfiler::Internal
