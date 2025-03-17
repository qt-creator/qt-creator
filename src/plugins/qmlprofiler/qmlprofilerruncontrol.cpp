// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerruncontrol.h"

#include "qmlprofilerclientmanager.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilertr.h"

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

#include <QGuiApplication>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;

namespace QmlProfiler::Internal {

QmlProfilerRunner::QmlProfilerRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("QmlProfilerRunner");
    runControl->requestQmlChannel();
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
}

void QmlProfilerRunner::start()
{
    QmlProfilerTool::instance()->finalizeRunControl(runControl());
    connect(this, &QmlProfilerRunner::stopped,
            QmlProfilerTool::instance(), &QmlProfilerTool::handleStop);
    QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
    QTC_ASSERT(stateManager, return);

    connect(stateManager, &QmlProfilerStateManager::stateChanged, this, [this, stateManager] {
        if (stateManager->currentState() == QmlProfilerStateManager::Idle)
            reportStopped();
    });

    QmlProfilerClientManager *clientManager = QmlProfilerTool::instance()->clientManager();
    connect(clientManager, &QmlProfilerClientManager::connectionFailed, this, [this, clientManager, stateManager] {
        auto infoBox = new QMessageBox(ICore::dialogParent());
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(QGuiApplication::applicationDisplayName());

        const int interval = clientManager->retryInterval();
        const int retries = clientManager->maximumRetries();

        infoBox->setText(Tr::tr("Could not connect to the in-process QML profiler "
                                "within %1 s.\n"
                                "Do you want to retry and wait %2 s?")
                             .arg(interval * retries / 1000.0)
                             .arg(interval * 2 * retries / 1000.0));
        infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Retry);
        infoBox->setModal(true);

        connect(infoBox, &QDialog::finished, this, [this, clientManager, stateManager, interval](int result) {
            const auto cancelProcess = [this, stateManager] {
                switch (stateManager->currentState()) {
                case QmlProfilerStateManager::Idle:
                    break;
                case QmlProfilerStateManager::AppRunning:
                    stateManager->setCurrentState(QmlProfilerStateManager::AppDying);
                    break;
                default: {
                    const QString message = QString::fromLatin1("Unexpected process termination requested with state %1 in %2:%3")
                    .arg(stateManager->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
                    qWarning("%s", qPrintable(message));
                    return;
                }
                }
                runControl()->initiateStop();
            };

            switch (result) {
            case QMessageBox::Retry:
                clientManager->setRetryInterval(interval * 2);
                clientManager->retryConnect();
                break;
            case QMessageBox::Help:
                HelpManager::showHelpUrl(
                    "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html");
                Q_FALLTHROUGH();
            case QMessageBox::Cancel:
                // The actual error message has already been logged.
                QmlProfilerTool::logState(Tr::tr("Failed to connect."));
                cancelProcess();
                break;
            }
        });

        infoBox->show();
    }, Qt::QueuedConnection); // Queue any connection failures after reportStarted()
    clientManager->setServer(runControl()->qmlChannel());
    clientManager->connectToServer();

    reportStarted();
}

void QmlProfilerRunner::stop()
{
    QTC_ASSERT(QmlProfilerTool::instance(), return);
    QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
    if (!stateManager) {
        reportStopped();
        return;
    }

    switch (stateManager->currentState()) {
    case QmlProfilerStateManager::AppRunning:
        stateManager->setCurrentState(QmlProfilerStateManager::AppStopRequested);
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Pressed "stop" a second time. Kill the application without collecting data
        stateManager->setCurrentState(QmlProfilerStateManager::Idle);
        reportStopped();
        break;
    case QmlProfilerStateManager::Idle:
    case QmlProfilerStateManager::AppDying:
        // valid, but no further action is needed
        break;
    default: {
        const QString message = QString::fromLatin1("Unexpected engine stop from state %1 in %2:%3")
            .arg(stateManager->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
    }
        break;
    }
}

RunWorker *createLocalQmlProfilerWorker(RunControl *runControl)
{
    auto worker = new ProcessRunner(runControl);

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
        worker->setCommandLine(cmd.toLocal());
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
