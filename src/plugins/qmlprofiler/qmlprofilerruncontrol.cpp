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

#include "qmlprofilerclientmanager.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilertool.h"
#include "qmlprofilerplugin.h"

#include <debugger/analyzer/analyzermanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <qmldebug/qmloutputparser.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/qtcassert.h>

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

using namespace Debugger;
using namespace Core;
using namespace ProjectExplorer;
using namespace QmlProfiler::Internal;

namespace QmlProfiler {

static QString QmlServerUrl = "QmlServerUrl";

//
// QmlProfilerRunControlPrivate
//

class QmlProfilerRunner::QmlProfilerRunnerPrivate
{
public:
    QmlProfilerStateManager *m_profilerState = 0;
    QTimer m_noDebugOutputTimer;
};

//
// QmlProfilerRunControl
//

QmlProfilerRunner::QmlProfilerRunner(RunControl *runControl)
    : RunWorker(runControl)
    , d(new QmlProfilerRunnerPrivate)
{
    setDisplayName("QmlProfilerRunner");
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    runControl->setSupportsReRunning(false);

    // Only wait 4 seconds for the 'Waiting for connection' on application output, then just try to connect
    // (application output might be redirected / blocked)
    d->m_noDebugOutputTimer.setSingleShot(true);
    d->m_noDebugOutputTimer.setInterval(4000);
    connect(&d->m_noDebugOutputTimer, &QTimer::timeout, this, [this]() {
        notifyRemoteSetupDone(Utils::Port());
    });
}

QmlProfilerRunner::~QmlProfilerRunner()
{
    if (runControl()->isRunning() && d->m_profilerState)
        runControl()->stop();
    delete d;
}

void QmlProfilerRunner::start()
{
    Internal::QmlProfilerTool::instance()->finalizeRunControl(this);
    QTC_ASSERT(d->m_profilerState, return);

    QUrl serverUrl = this->serverUrl();

    if (serverUrl.port() != -1) {
        auto clientManager = Internal::QmlProfilerTool::clientManager();
        clientManager->setServerUrl(serverUrl);
        clientManager->connectToTcpServer();
    }
    else if (serverUrl.path().isEmpty())
        d->m_noDebugOutputTimer.start();

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppRunning);

    reportStarted();
}

void QmlProfilerRunner::stop()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning:
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopRequested);
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Pressed "stop" a second time. Kill the application without collecting data
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
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

void QmlProfilerRunner::notifyRemoteSetupFailed(const QString &errorMessage)
{
    QMessageBox *infoBox = new QMessageBox(ICore::mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(tr("Qt Creator"));
    //: %1 is detailed error message
    infoBox->setText(tr("Could not connect to the in-process QML debugger:\n%1")
                     .arg(errorMessage));
    infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Ok);
    infoBox->setModal(true);

    connect(infoBox, &QDialog::finished,
            this, &QmlProfilerRunner::wrongSetupMessageBoxFinished);

    infoBox->show();

    // KILL
    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
    d->m_noDebugOutputTimer.stop();
}

void QmlProfilerRunner::wrongSetupMessageBoxFinished(int button)
{
    if (button == QMessageBox::Help) {
        HelpManager::handleHelpRequest(QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
                               "#setting-up-qml-debugging"));
    }
}

void QmlProfilerRunner::notifyRemoteSetupDone(Utils::Port port)
{
    d->m_noDebugOutputTimer.stop();

    QUrl serverUrl = this->serverUrl();
    if (!port.isValid())
        port = Utils::Port(serverUrl.port());

    if (port.isValid()) {
        serverUrl.setPort(port.number());
        auto clientManager = Internal::QmlProfilerTool::clientManager();
        clientManager->setServerUrl(serverUrl);
        clientManager->connectToTcpServer();
    }
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
        d->m_noDebugOutputTimer.stop();
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
    if (recordedServer.isValid())
        return recordedServer.toUrl();
    QTC_ASSERT(connection().is<UrlConnection>(), return QUrl());
    return connection().as<UrlConnection>();
}

//
// LocalQmlProfilerSupport
//

static QUrl localServerUrl(RunControl *runControl)
{
    QUrl serverUrl;
    RunConfiguration *runConfiguration = runControl->runConfiguration();
    Kit *kit = runConfiguration->target()->kit();
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    if (version) {
        if (version->qtVersion() >= QtSupport::QtVersionNumber(5, 6, 0))
            serverUrl = UrlConnection::localSocket();
        else
            serverUrl = UrlConnection::localHostAndFreePort();
    } else {
        qWarning("Running QML profiler on Kit without Qt version?");
        serverUrl = UrlConnection::localHostAndFreePort();
    }
    return serverUrl;
}

LocalQmlProfilerSupport::LocalQmlProfilerSupport(RunControl *runControl)
    : LocalQmlProfilerSupport(runControl, localServerUrl(runControl))
{
}

LocalQmlProfilerSupport::LocalQmlProfilerSupport(RunControl *runControl, const QUrl &serverUrl)
    : RunWorker(runControl)
{
    m_profiler = new QmlProfilerRunner(runControl);
    m_profiler->setServerUrl(serverUrl);
    m_profiler->addDependency(this);

    StandardRunnable debuggee = runnable().as<StandardRunnable>();
    QString arguments = QmlDebug::qmlDebugArguments(QmlDebug::QmlProfilerServices, serverUrl);

    if (!debuggee.commandLineArguments.isEmpty())
        arguments += ' ' + debuggee.commandLineArguments;

    debuggee.commandLineArguments = arguments;
    debuggee.runMode = ApplicationLauncher::Gui;

    m_profilee = new SimpleTargetRunner(runControl);
    m_profilee->setRunnable(debuggee);
    addDependency(m_profilee);
}

void LocalQmlProfilerSupport::start()
{
    reportStarted();
    emit localRunnerStarted();
}

void LocalQmlProfilerSupport::stop()
{
    reportStopped();
    emit localRunnerStopped();
}

} // namespace QmlProfiler
