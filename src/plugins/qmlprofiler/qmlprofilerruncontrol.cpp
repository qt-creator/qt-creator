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

#include "qmlprofilerruncontrol.h"

#include "qmlprofilerclientmanager.h"
#include "qmlprofilertool.h"

#include <app/app_version.h>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/qtcassert.h>
#include <utils/qtcfallthrough.h>
#include <utils/url.h>

#include <QMessageBox>

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
    QPointer<QmlProfilerStateManager> m_profilerState;
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
    setSupportsReRunning(false);
}

QmlProfilerRunner::~QmlProfilerRunner()
{
    if (runControl()->isRunning() && d->m_profilerState)
        runControl()->initiateStop();
    delete d;
}

void QmlProfilerRunner::start()
{
    Internal::QmlProfilerTool::instance()->finalizeRunControl(this);
    QTC_ASSERT(d->m_profilerState, return);

    QUrl serverUrl = this->serverUrl();

    QmlProfilerClientManager *clientManager = Internal::QmlProfilerTool::clientManager();

    connect(clientManager, &QmlProfilerClientManager::connectionFailed,
            this, [this, clientManager] {
        QMessageBox *infoBox = new QMessageBox(ICore::mainWindow());
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(Core::Constants::IDE_DISPLAY_NAME);
        infoBox->setText(QmlProfilerTool::tr("Could not connect to the in-process QML profiler.\n"
                                             "Do you want to retry?"));
        infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Retry);
        infoBox->setModal(true);

        connect(infoBox, &QDialog::finished, this, [clientManager, this](int result) {
            switch (result) {
            case QMessageBox::Retry:
                clientManager->retryConnect();
                break;
            case QMessageBox::Help:
                HelpManager::handleHelpRequest(
                            "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html");
                Q_FALLTHROUGH();
            case QMessageBox::Cancel:
                // The actual error message has already been logged.
                QmlProfilerTool::logState(QmlProfilerTool::tr("Failed to connect."));
                cancelProcess();
                break;
            }
        });

        infoBox->show();
    }, Qt::QueuedConnection); // Queue any connection failures after reportStarted()

    clientManager->connectToServer(serverUrl);
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
    RunConfiguration *runConfiguration = runControl->runConfiguration();
    Kit *kit = runConfiguration->target()->kit();
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    if (version) {
        if (version->qtVersion() >= QtSupport::QtVersionNumber(5, 6, 0))
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
    setDisplayName("LocalQmlProfilerSupport");

    m_profiler = new QmlProfilerRunner(runControl);
    m_profiler->setServerUrl(serverUrl);

    addStopDependency(m_profiler);
    // We need to open the local server before the application tries to connect.
    // In the TCP case, it doesn't hurt either to start the profiler before.
    addStartDependency(m_profiler);

    StandardRunnable debuggee = runnable().as<StandardRunnable>();

    QString code;
    if (serverUrl.scheme() == Utils::urlSocketScheme())
        code = QString("file:%1").arg(serverUrl.path());
    else if (serverUrl.scheme() == Utils::urlTcpScheme())
        code = QString("port:%1").arg(serverUrl.port());
    else
        QTC_CHECK(false);

    QString arguments = QmlDebug::qmlDebugCommandLineArguments(QmlDebug::QmlProfilerServices,
                                                               code, true);

    if (!debuggee.commandLineArguments.isEmpty())
        arguments += ' ' + debuggee.commandLineArguments;

    debuggee.commandLineArguments = arguments;
    debuggee.runMode = ApplicationLauncher::Gui;

    setRunnable(debuggee);
}

} // namespace QmlProfiler
