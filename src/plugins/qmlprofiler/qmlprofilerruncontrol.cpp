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
#include <debugger/analyzer/analyzerstartparameters.h>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtsupportconstants.h>

#include <qmldebug/qmloutputparser.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTcpServer>
#include <QTemporaryFile>
#include <QTimer>

using namespace Debugger;
using namespace Core;
using namespace ProjectExplorer;
using namespace QmlProfiler::Internal;

namespace QmlProfiler {

//
// QmlProfilerRunControlPrivate
//

class QmlProfilerRunner::QmlProfilerRunnerPrivate
{
public:
    QmlProfilerStateManager *m_profilerState = 0;
    QTimer m_noDebugOutputTimer;
    bool m_isLocal = false;
    QmlProfilerRunner::Configuration m_configuration;
    ProjectExplorer::ApplicationLauncher m_launcher;
    QmlDebug::QmlOutputParser m_outputParser;
};

//
// QmlProfilerRunControl
//

QmlProfilerRunner::QmlProfilerRunner(RunControl *runControl)
    : RunWorker(runControl)
    , d(new QmlProfilerRunnerPrivate)
{
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

    QTC_ASSERT(connection().is<AnalyzerConnection>(), return);
    auto conn = connection().as<AnalyzerConnection>();

    if (conn.analyzerPort.isValid()) {
        auto clientManager = Internal::QmlProfilerTool::clientManager();
        clientManager->setTcpConnection(conn.analyzerHost, conn.analyzerPort);
        clientManager->connectToTcpServer();
    }
    else if (conn.analyzerSocket.isEmpty())
        d->m_noDebugOutputTimer.start();

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppRunning);

    if (d->m_isLocal) {
        QTC_ASSERT(!d->m_configuration.socket.isEmpty() || d->m_configuration.port.isValid(), return);

        StandardRunnable debuggee = runnable().as<StandardRunnable>();
        QString arguments = d->m_configuration.socket.isEmpty() ?
                    QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices,
                                                   d->m_configuration.port) :
                    QmlDebug::qmlDebugLocalArguments(QmlDebug::QmlProfilerServices,
                                                     d->m_configuration.socket);


        if (!debuggee.commandLineArguments.isEmpty())
            arguments += ' ' + debuggee.commandLineArguments;

        debuggee.commandLineArguments = arguments;
        debuggee.runMode = ApplicationLauncher::Gui;

        // queue this, as the process can already die in the call to start().
        // We want the started() signal to be emitted before the stopped() signal.
        connect(&d->m_launcher, &ApplicationLauncher::processExited,
                this, &QmlProfilerRunner::spontaneousStop,
                Qt::QueuedConnection);
        d->m_launcher.start(debuggee);

        emit localRunnerStarted();
    }
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

    if (!port.isValid()) {
        QTC_ASSERT(connection().is<AnalyzerConnection>(), return);
        port = connection().as<AnalyzerConnection>().analyzerPort;
    }
    if (port.isValid()) {
        auto clientManager = Internal::QmlProfilerTool::clientManager();
        clientManager->setTcpConnection(connection().as<AnalyzerConnection>().analyzerHost, port);
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

QString QmlProfilerRunner::findFreeSocket()
{
    Utils::TemporaryFile file("qmlprofiler-freesocket");
    if (file.open()) {
        return file.fileName();
    } else {
        qWarning() << "Could not open a temporary file to find a debug socket.";
        return QString();
    }
}

Utils::Port QmlProfilerRunner::findFreePort(QString &host)
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)
            && !server.listen(QHostAddress::LocalHostIPv6)) {
        qWarning() << "Cannot open port on host for QML profiling.";
        return Utils::Port();
    }
    host = server.serverAddress().toString();
    return Utils::Port(server.serverPort());
}

void QmlProfilerRunner::setLocalConfiguration(const Configuration &configuration)
{
    d->m_isLocal = true;
    d->m_configuration = configuration;
    connect(&d->m_launcher, &ApplicationLauncher::appendMessage,
            this, &QmlProfilerRunner::appendMessage);
    connect(this, &QmlProfilerRunner::localRunnerStopped,
            this, &QmlProfilerRunner::notifyRemoteFinished);
    connect(runControl(), &RunControl::finished,
            this, &QmlProfilerRunner::stopLocalRunner);

    d->m_outputParser.setNoOutputText(ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput());

    connect(runControl(), &RunControl::appendMessageRequested,
            this, [this](RunControl *, const QString &msg, Utils::OutputFormat) {
        d->m_outputParser.processOutput(msg);
    });

    connect(&d->m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, [this](Utils::Port port) {
        notifyRemoteSetupDone(port);
    });

    connect(&d->m_outputParser, &QmlDebug::QmlOutputParser::noOutputMessage,
            this, [this] {
        notifyRemoteSetupDone(Utils::Port());
    });

    connect(&d->m_outputParser, &QmlDebug::QmlOutputParser::connectingToSocketMessage,
            this, [this] {
        notifyRemoteSetupDone(Utils::Port());
    });

    connect(&d->m_outputParser, &QmlDebug::QmlOutputParser::errorMessage,
            this, [this](const QString &message) {
        notifyRemoteSetupFailed(message);
    });
}

void QmlProfilerRunner::spontaneousStop(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    disconnect(&d->m_launcher, &ApplicationLauncher::processExited,
               this, &QmlProfilerRunner::spontaneousStop);

    emit localRunnerStopped();
}

void QmlProfilerRunner::stopLocalRunner()
{
    if (d->m_launcher.isRunning())
        d->m_launcher.stop();
}

} // namespace QmlProfiler
