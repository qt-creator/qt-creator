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

#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerattachdialog.h"
#include "qmlprofilerviewmanager.h"
#include "qmlprofilerclientmanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdetailsrewriter.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilersettings.h"
#include "qmlprofilerplugin.h"

#include <debugger/debuggericons.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerruncontrol.h>
#include <debugger/analyzer/analyzerstartparameters.h>

#include <utils/fancymainwindow.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <texteditor/texteditor.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <qtsupport/qtkitinformation.h>

#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTcpServer>
#include <QTime>
#include <QTimer>
#include <QToolButton>

using namespace Core;
using namespace Core::Constants;
using namespace Debugger;
using namespace Debugger::Constants;
using namespace QmlProfiler::Constants;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerStateManager *m_profilerState = 0;
    QmlProfilerClientManager *m_profilerConnections = 0;
    QmlProfilerModelManager *m_profilerModelManager = 0;

    QmlProfilerViewManager *m_viewContainer = 0;
    QToolButton *m_recordButton = 0;
    QMenu *m_recordFeaturesMenu = 0;

    QAction *m_startAction = 0;
    QAction *m_stopAction = 0;
    QToolButton *m_clearButton = 0;

    // elapsed time display
    QTimer m_recordingTimer;
    QTime m_recordingElapsedTime;
    QLabel *m_timeLabel = 0;

    // open search
    QToolButton *m_searchButton = 0;

    // hide and show categories
    QToolButton *m_displayFeaturesButton = 0;
    QMenu *m_displayFeaturesMenu = 0;

    // save and load actions
    QAction *m_saveQmlTrace = 0;
    QAction *m_loadQmlTrace = 0;

    bool m_toolBusy = false;
};

QmlProfilerTool::QmlProfilerTool(QObject *parent)
    : QObject(parent), d(new QmlProfilerToolPrivate)
{
    setObjectName(QLatin1String("QmlProfilerTool"));

    d->m_profilerState = new QmlProfilerStateManager(this);
    connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerTool::profilerStateChanged);
    connect(d->m_profilerState, &QmlProfilerStateManager::clientRecordingChanged,
            this, &QmlProfilerTool::clientRecordingChanged);
    connect(d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerTool::serverRecordingChanged);
    connect(d->m_profilerState, &QmlProfilerStateManager::recordedFeaturesChanged,
            this, &QmlProfilerTool::setRecordedFeatures);

    d->m_profilerConnections = new QmlProfilerClientManager(this);
    d->m_profilerConnections->setProfilerStateManager(d->m_profilerState);
    connect(d->m_profilerConnections, &QmlProfilerClientManager::connectionClosed,
            this, &QmlProfilerTool::clientsDisconnected);

    d->m_profilerModelManager = new QmlProfilerModelManager(this);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerTool::profilerDataModelStateChanged);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::error,
            this, &QmlProfilerTool::showErrorDialog);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::availableFeaturesChanged,
            this, &QmlProfilerTool::setAvailableFeatures);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::saveFinished,
            this, &QmlProfilerTool::onLoadSaveFinished);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::loadFinished,
            this, &QmlProfilerTool::onLoadSaveFinished);

    d->m_profilerConnections->setModelManager(d->m_profilerModelManager);
    Command *command = 0;

    ActionContainer *menu = ActionManager::actionContainer(M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(M_DEBUG_ANALYZER_QML_OPTIONS);
    options->menu()->setTitle(tr("QML Profiler Options"));
    menu->addMenu(options, G_ANALYZER_OPTIONS);
    options->menu()->setEnabled(true);

    QAction *act = d->m_loadQmlTrace = new QAction(tr("Load QML Trace"), options);
    command = ActionManager::registerAction(act, Constants::QmlProfilerLoadActionId);
    connect(act, &QAction::triggered, this, &QmlProfilerTool::showLoadDialog, Qt::QueuedConnection);
    options->addAction(command);

    act = d->m_saveQmlTrace = new QAction(tr("Save QML Trace"), options);
    d->m_saveQmlTrace->setEnabled(false);
    command = ActionManager::registerAction(act, Constants::QmlProfilerSaveActionId);
    connect(act, &QAction::triggered, this, &QmlProfilerTool::showSaveDialog, Qt::QueuedConnection);
    options->addAction(command);

    d->m_recordingTimer.setInterval(100);
    connect(&d->m_recordingTimer, &QTimer::timeout, this, &QmlProfilerTool::updateTimeDisplay);
    d->m_viewContainer = new QmlProfilerViewManager(this,
                                                    d->m_profilerModelManager,
                                                    d->m_profilerState);
    connect(d->m_viewContainer, &QmlProfilerViewManager::gotoSourceLocation,
            this, &QmlProfilerTool::gotoSourceLocation);

    //
    // Toolbar
    //
    d->m_recordButton = new QToolButton;
    d->m_recordButton->setCheckable(true);

    connect(d->m_recordButton,&QAbstractButton::clicked,
            this, &QmlProfilerTool::recordingButtonChanged);
    d->m_recordButton->setChecked(true);
    d->m_recordFeaturesMenu = new QMenu(d->m_recordButton);
    d->m_recordButton->setMenu(d->m_recordFeaturesMenu);
    d->m_recordButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(d->m_recordFeaturesMenu, &QMenu::triggered,
            this, &QmlProfilerTool::toggleRequestedFeature);

    setRecording(d->m_profilerState->clientRecording());

    d->m_clearButton = new QToolButton;
    d->m_clearButton->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    d->m_clearButton->setToolTip(tr("Discard data"));

    connect(d->m_clearButton, &QAbstractButton::clicked, [this](){
        if (checkForUnsavedNotes())
            clearData();
    });

    d->m_searchButton = new QToolButton;
    d->m_searchButton->setIcon(Utils::Icons::ZOOM_TOOLBAR.icon());
    d->m_searchButton->setToolTip(tr("Search timeline event notes."));

    connect(d->m_searchButton, &QToolButton::clicked, this, &QmlProfilerTool::showTimeLineSearch);
    d->m_searchButton->setEnabled(d->m_viewContainer->traceView()->isUsable());

    d->m_displayFeaturesButton = new QToolButton;
    d->m_displayFeaturesButton->setIcon(Utils::Icons::FILTER.icon());
    d->m_displayFeaturesButton->setToolTip(tr("Hide or show event categories."));
    d->m_displayFeaturesButton->setPopupMode(QToolButton::InstantPopup);
    d->m_displayFeaturesButton->setProperty("noArrow", true);
    d->m_displayFeaturesMenu = new QMenu(d->m_displayFeaturesButton);
    d->m_displayFeaturesButton->setMenu(d->m_displayFeaturesMenu);
    connect(d->m_displayFeaturesMenu, &QMenu::triggered,
            this, &QmlProfilerTool::toggleVisibleFeature);

    d->m_timeLabel = new QLabel();
    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::white);
    d->m_timeLabel->setPalette(palette);
    d->m_timeLabel->setIndent(10);
    updateTimeDisplay();

    setAvailableFeatures(d->m_profilerModelManager->availableFeatures());
    setRecordedFeatures(0);

    // When the widgets are requested we assume that the session data
    // is available, then we can populate the file finder
    d->m_profilerModelManager->qmlModel()->populateFileFinder();

    auto runControlCreator = [this](RunConfiguration *runConfiguration, Core::Id) {
        return createRunControl(runConfiguration);
    };

    QString description = tr("The QML Profiler can be used to find performance "
                             "bottlenecks in applications using QML.");

    d->m_startAction = Debugger::createStartAction();
    d->m_stopAction = Debugger::createStopAction();

    ActionDescription desc;
    desc.setText(tr("QML Profiler"));
    desc.setToolTip(description);
    desc.setPerspectiveId(Constants::QmlProfilerPerspectiveId);
    desc.setRunControlCreator(runControlCreator);
    desc.setToolPreparer([this] { return prepareTool(); });
    desc.setRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    desc.setMenuGroup(Debugger::Constants::G_ANALYZER_TOOLS);
    Debugger::registerAction(Constants::QmlProfilerLocalActionId, desc, d->m_startAction);

    desc.setText(tr("QML Profiler (External)"));
    desc.setToolTip(description);
    desc.setPerspectiveId(Constants::QmlProfilerPerspectiveId);
    desc.setRunControlCreator(runControlCreator);
    desc.setCustomToolStarter([this](RunConfiguration *rc) { startRemoteTool(rc); });
    desc.setToolPreparer([this] { return prepareTool(); });
    desc.setRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    desc.setMenuGroup(Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    Debugger::registerAction(Constants::QmlProfilerRemoteActionId, desc);

    Utils::ToolbarDescription toolbar;
    toolbar.addAction(d->m_startAction);
    toolbar.addAction(d->m_stopAction);
    toolbar.addWidget(d->m_recordButton);
    toolbar.addWidget(d->m_clearButton);
    toolbar.addWidget(d->m_searchButton);
    toolbar.addWidget(d->m_displayFeaturesButton);
    toolbar.addWidget(d->m_timeLabel);
    Debugger::registerToolbar(Constants::QmlProfilerPerspectiveId, toolbar);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &QmlProfilerTool::updateRunActions);
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d;
}

void QmlProfilerTool::updateRunActions()
{
    if (d->m_toolBusy) {
        d->m_startAction->setEnabled(false);
        d->m_startAction->setToolTip(tr("A QML Profiler analysis is still in progress."));
        d->m_stopAction->setEnabled(true);
    } else {
        QString whyNot = tr("Start QML Profiler analysis.");
        bool canRun = ProjectExplorerPlugin::canRunStartupProject
                (ProjectExplorer::Constants::QML_PROFILER_RUN_MODE, &whyNot);
        d->m_startAction->setToolTip(whyNot);
        d->m_startAction->setEnabled(canRun);
        d->m_stopAction->setEnabled(false);
    }
}

AnalyzerRunControl *QmlProfilerTool::createRunControl(RunConfiguration *runConfiguration)
{
    d->m_toolBusy = true;
    if (runConfiguration) {
        QmlProfilerRunConfigurationAspect *aspect = static_cast<QmlProfilerRunConfigurationAspect *>(
                    runConfiguration->extraAspect(Constants::SETTINGS));
        if (aspect) {
            if (QmlProfilerSettings *settings = static_cast<QmlProfilerSettings *>(aspect->currentSettings())) {
                d->m_profilerConnections->setFlushInterval(settings->flushEnabled() ?
                                                               settings->flushInterval() : 0);
                d->m_profilerModelManager->setAggregateTraces(settings->aggregateTraces());
            }
        }
    }

    auto runControl = new QmlProfilerRunControl(runConfiguration, this);
    connect(runControl, &RunControl::finished, this, [this, runControl] {
        d->m_toolBusy = false;
        updateRunActions();
        disconnect(d->m_stopAction, &QAction::triggered, runControl, &QmlProfilerRunControl::stop);
    });

    connect(d->m_stopAction, &QAction::triggered, runControl, &QmlProfilerRunControl::stop);

    updateRunActions();
    return runControl;
}

void QmlProfilerTool::finalizeRunControl(QmlProfilerRunControl *runControl)
{
    runControl->registerProfilerStateManager(d->m_profilerState);
    QmlProfilerClientManager *clientManager = d->m_profilerConnections;

    QTC_ASSERT(runControl->connection().is<AnalyzerConnection>(), return);
    // FIXME: Check that there's something sensible in sp.connParams
    auto connection = runControl->connection().as<AnalyzerConnection>();
    if (!connection.analyzerSocket.isEmpty()) {
        clientManager->setLocalSocket(connection.analyzerSocket);
        // We open the server and the application connects to it, so let's do that right away.
        clientManager->startLocalServer();
    } else {
        clientManager->setTcpConnection(connection.analyzerHost, connection.analyzerPort);
    }

    //
    // Initialize m_projectFinder
    //

    RunConfiguration *runConfiguration = runControl->runConfiguration();
    if (runConfiguration) {
        d->m_profilerModelManager->qmlModel()->populateFileFinder(runConfiguration);
    }

    if (connection.analyzerSocket.isEmpty()) {
        QString host = connection.analyzerHost;
        connect(runControl, &QmlProfilerRunControl::processRunning,
                clientManager, [clientManager, host](Utils::Port port) {
            clientManager->setTcpConnection(host, port);
            clientManager->connectToTcpServer();
        });
    }
    connect(clientManager, &QmlProfilerClientManager::connectionFailed,
            runControl, [this, clientManager, runControl]() {
        QMessageBox *infoBox = new QMessageBox(ICore::mainWindow());
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(tr("Qt Creator"));
        infoBox->setText(tr("Could not connect to the in-process QML profiler.\n"
                            "Do you want to retry?"));
        infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Retry);
        infoBox->setModal(true);

        connect(infoBox, &QDialog::finished, runControl, [clientManager, runControl](int result) {
            switch (result) {
            case QMessageBox::Retry:
                clientManager->retryConnect();
                break;
            case QMessageBox::Help:
                HelpManager::handleHelpRequest(
                            "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html");
            case QMessageBox::Cancel:
                // The actual error message has already been logged.
                logState(tr("Failed to connect."));
                runControl->cancelProcess();
                break;
            }
        });

        infoBox->show();
    });
}

void QmlProfilerTool::recordingButtonChanged(bool recording)
{
    // clientRecording is our intention for new sessions. That may differ from the state of the
    // current session, as indicated by the button. To synchronize it, toggle once.

    if (recording && d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        if (checkForUnsavedNotes()) {
            if (!d->m_profilerModelManager->aggregateTraces() ||
                    d->m_profilerModelManager->state() == QmlProfilerModelManager::Done)
                clearData(); // clear before the recording starts, unless we aggregate recordings
            if (d->m_profilerState->clientRecording())
                d->m_profilerState->setClientRecording(false);
            d->m_profilerState->setClientRecording(true);
        } else {
            d->m_recordButton->setChecked(false);
        }
    } else {
        if (d->m_profilerState->clientRecording() == recording)
            d->m_profilerState->setClientRecording(!recording);
        d->m_profilerState->setClientRecording(recording);
    }
}

void QmlProfilerTool::setRecording(bool recording)
{
    const static QIcon recordOn = Debugger::Icons::RECORD_ON.icon();
    const static QIcon recordOff = Debugger::Icons::RECORD_OFF.icon();

    // update display
    d->m_recordButton->setToolTip( recording ? tr("Disable Profiling") : tr("Enable Profiling"));
    d->m_recordButton->setIcon(recording ? recordOn : recordOff);

    d->m_recordButton->setChecked(recording);

    // manage timer
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        if (recording) {
            d->m_recordingTimer.start();
            d->m_recordingElapsedTime.start();
        } else {
            d->m_recordingTimer.stop();
        }
    }
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber)
{
    if (lineNumber < 0 || fileUrl.isEmpty())
        return;

    const QString projectFileName = d->m_profilerModelManager->qmlModel()->findLocalFile(fileUrl);

    QFileInfo fileInfo(projectFileName);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return;

    // The text editors count columns starting with 0, but the ASTs store the
    // location starting with 1, therefore the -1.
    EditorManager::openEditorAt(
                projectFileName, lineNumber == 0 ? 1 : lineNumber, columnNumber - 1, Id(),
                EditorManager::DoNotSwitchToDesignMode | EditorManager::DoNotSwitchToEditMode);
}

void QmlProfilerTool::updateTimeDisplay()
{
    double seconds = 0;
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppStopRequested:
    case QmlProfilerStateManager::AppDying:
        return; // Transitional state: don't update the display.
    case QmlProfilerStateManager::AppRunning:
        if (d->m_profilerState->serverRecording()) {
            seconds = d->m_recordingElapsedTime.elapsed() / 1000.0;
            break;
        } // else fall through
    case QmlProfilerStateManager::Idle:
        if (d->m_profilerModelManager->state() != QmlProfilerModelManager::Empty &&
               d->m_profilerModelManager->state() != QmlProfilerModelManager::ClearingData)
            seconds = d->m_profilerModelManager->traceTime()->duration() / 1.0e9;
        break;
    }
    QString timeString = QString::number(seconds,'f',1);
    QString profilerTimeStr = QmlProfilerTool::tr("%1 s").arg(timeString, 6);
    d->m_timeLabel->setText(tr("Elapsed: %1").arg(profilerTimeStr));
}

void QmlProfilerTool::showTimeLineSearch()
{
    QmlProfilerTraceView *traceView = d->m_viewContainer->traceView();
    QTC_ASSERT(qobject_cast<QDockWidget *>(traceView->parentWidget()), return);
    traceView->parentWidget()->raise();
    traceView->setFocus();
    Core::Find::openFindToolBar(Core::Find::FindForwardDirection);
}

void QmlProfilerTool::clearData()
{
    d->m_profilerModelManager->clear();
    d->m_profilerConnections->clearBufferedData();
    setRecordedFeatures(0);
}

void QmlProfilerTool::clearDisplay()
{
    d->m_profilerConnections->clearBufferedData();
    d->m_viewContainer->clear();
    updateTimeDisplay();
}

void QmlProfilerTool::setButtonsEnabled(bool enable)
{
    d->m_clearButton->setEnabled(enable);
    d->m_displayFeaturesButton->setEnabled(enable);
    d->m_searchButton->setEnabled(d->m_viewContainer->traceView()->isUsable() && enable);
    d->m_recordFeaturesMenu->setEnabled(enable);
}

bool QmlProfilerTool::prepareTool()
{
    if (d->m_recordButton->isChecked()) {
        if (checkForUnsavedNotes()) {
            clearData(); // clear right away to suppress second warning on server recording change
            return true;
        } else {
            return false;
        }
    }
    return true;
}

void QmlProfilerTool::startRemoteTool(ProjectExplorer::RunConfiguration *rc)
{
    Id kitId;
    quint16 port;
    Kit *kit = 0;

    {
        QSettings *settings = ICore::settings();

        kitId = Id::fromSetting(settings->value(QLatin1String("AnalyzerQmlAttachDialog/kitId")));
        port = settings->value(QLatin1String("AnalyzerQmlAttachDialog/port"), 3768).toUInt();

        QmlProfilerAttachDialog dialog;

        dialog.setKitId(kitId);
        dialog.setPort(port);

        if (dialog.exec() != QDialog::Accepted)
            return;

        kit = dialog.kit();
        port = dialog.port();

        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/kitId"), kit->id().toSetting());
        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/port"), port);
    }

    AnalyzerConnection connection;

    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    if (device) {
        Connection toolControl = device->toolControlChannel(IDevice::QmlControlChannel);
        QTC_ASSERT(toolControl.is<HostName>(), return);
        connection.analyzerHost = toolControl.as<HostName>().host();
        connection.connParams = device->sshParameters();
    }
    connection.analyzerPort = Utils::Port(port);

    auto runControl = qobject_cast<QmlProfilerRunControl *>(createRunControl(rc));
    runControl->setConnection(connection);

    ProjectExplorerPlugin::startRunControl(runControl, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
}

void QmlProfilerTool::logState(const QString &msg)
{
    MessageManager::write(msg, MessageManager::Flash);
}

void QmlProfilerTool::logError(const QString &msg)
{
    MessageManager::write(msg);
}

void QmlProfilerTool::showErrorDialog(const QString &error)
{
    QMessageBox *errorDialog = new QMessageBox(ICore::mainWindow());
    errorDialog->setIcon(QMessageBox::Warning);
    errorDialog->setWindowTitle(tr("QML Profiler"));
    errorDialog->setText(error);
    errorDialog->setStandardButtons(QMessageBox::Ok);
    errorDialog->setDefaultButton(QMessageBox::Ok);
    errorDialog->setModal(false);
    errorDialog->show();
}

void QmlProfilerTool::showLoadOption()
{
    d->m_loadQmlTrace->setEnabled(!d->m_profilerState->serverRecording());
}

void QmlProfilerTool::showSaveOption()
{
    d->m_saveQmlTrace->setEnabled(!d->m_profilerModelManager->isEmpty());
}

void saveLastTraceFile(const QString &filename)
{
    QmlProfilerSettings *settings = QmlProfilerPlugin::globalSettings();
    if (filename != settings->lastTraceFile()) {
        settings->setLastTraceFile(filename);
        settings->writeGlobalSettings();
    }
}

void QmlProfilerTool::showSaveDialog()
{
    QLatin1String tFile(QtdFileExtension);
    QLatin1String zFile(QztFileExtension);
    QString filename = QFileDialog::getSaveFileName(
                ICore::mainWindow(), tr("Save QML Trace"),
                QmlProfilerPlugin::globalSettings()->lastTraceFile(),
                tr("QML traces (*%1 *%2)").arg(zFile).arg(tFile));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(zFile) && !filename.endsWith(tFile))
            filename += zFile;
        saveLastTraceFile(filename);
        Debugger::enableMainWindow(false);
        d->m_profilerModelManager->save(filename);
    }
}

void QmlProfilerTool::showLoadDialog()
{
    if (!checkForUnsavedNotes())
        return;

    Debugger::selectPerspective(QmlProfilerPerspectiveId);

    QLatin1String tFile(QtdFileExtension);
    QLatin1String zFile(QztFileExtension);
    QString filename = QFileDialog::getOpenFileName(
                ICore::mainWindow(), tr("Load QML Trace"),
                QmlProfilerPlugin::globalSettings()->lastTraceFile(),
                tr("QML traces (*%1 *%2)").arg(zFile).arg(tFile));

    if (!filename.isEmpty()) {
        saveLastTraceFile(filename);
        Debugger::enableMainWindow(false);
        connect(d->m_profilerModelManager, &QmlProfilerModelManager::recordedFeaturesChanged,
                this, &QmlProfilerTool::setRecordedFeatures);
        d->m_profilerModelManager->qmlModel()->populateFileFinder();
        d->m_profilerModelManager->load(filename);
    }
}

void QmlProfilerTool::onLoadSaveFinished()
{
    disconnect(d->m_profilerModelManager, &QmlProfilerModelManager::recordedFeaturesChanged,
               this, &QmlProfilerTool::setRecordedFeatures);
    Debugger::enableMainWindow(true);
}

/*!
    Checks if we have unsaved notes. If so, shows a warning dialog. Returns true if we can continue
    with a potentially destructive operation and discard the warnings, or false if not. We don't
    want to show a save/discard dialog here because that will often result in a confusing series of
    different dialogs: first "save" and then immediately "load" or "connect".
 */
bool QmlProfilerTool::checkForUnsavedNotes()
{
    if (!d->m_profilerModelManager->notesModel()->isModified())
        return true;
    return QMessageBox::warning(QApplication::activeWindow(), tr("QML Profiler"),
                                tr("You are about to discard the profiling data, including unsaved "
                                   "notes. Do you want to continue?"),
                                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}

void QmlProfilerTool::restoreFeatureVisibility()
{
    // Restore the shown/hidden state of features to what the user selected. When clearing data the
    // the model manager sets its features to 0, and models get automatically shown, for the mockup.
    quint64 features = 0;
    foreach (const QAction *action, d->m_displayFeaturesMenu->actions()) {
        if (action->isChecked())
            features |= (1ULL << action->data().toUInt());
    }
    d->m_profilerModelManager->setVisibleFeatures(features);
}

void QmlProfilerTool::clientsDisconnected()
{
    if (d->m_profilerModelManager->state() == QmlProfilerModelManager::AcquiringData) {
        if (d->m_profilerModelManager->aggregateTraces()) {
            d->m_profilerModelManager->acquiringDone();
        } else {
            // If the application stopped by itself, check if we have all the data
            if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppDying ||
                    d->m_profilerState->currentState() == QmlProfilerStateManager::Idle) {
                showNonmodalWarning(tr("Application finished before loading profiled data.\n"
                                       "Please use the stop button instead."));
                d->m_profilerModelManager->clear();
            }
        }
    }

    // ... and return to the "base" state
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppDying)
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
}

void addFeatureToMenu(QMenu *menu, ProfileFeature feature, quint64 enabledFeatures)
{
    QAction *action =
            menu->addAction(QmlProfilerTool::tr(QmlProfilerModelManager::featureName(feature)));
    action->setCheckable(true);
    action->setData(static_cast<uint>(feature));
    action->setChecked(enabledFeatures & (1ULL << (feature)));
}

template<ProfileFeature feature>
void QmlProfilerTool::updateFeatures(quint64 features)
{
    if (features & (1ULL << (feature))) {
        addFeatureToMenu(d->m_recordFeaturesMenu, feature,
                         d->m_profilerState->requestedFeatures());
        addFeatureToMenu(d->m_displayFeaturesMenu, feature,
                         d->m_profilerModelManager->visibleFeatures());
    }
    updateFeatures<static_cast<ProfileFeature>(feature + 1)>(features);
}

template<>
void QmlProfilerTool::updateFeatures<MaximumProfileFeature>(quint64 features)
{
    Q_UNUSED(features);
    return;
}

void QmlProfilerTool::setAvailableFeatures(quint64 features)
{
    if (features != d->m_profilerState->requestedFeatures())
        d->m_profilerState->setRequestedFeatures(features); // by default, enable them all.
    if (d->m_recordFeaturesMenu && d->m_displayFeaturesMenu) {
        d->m_recordFeaturesMenu->clear();
        d->m_displayFeaturesMenu->clear();
        updateFeatures<static_cast<ProfileFeature>(0)>(features);
    }
}

void QmlProfilerTool::setRecordedFeatures(quint64 features)
{
    foreach (QAction *action, d->m_displayFeaturesMenu->actions())
        action->setEnabled(features & (1ULL << action->data().toUInt()));
}

void QmlProfilerTool::profilerDataModelStateChanged()
{
    switch (d->m_profilerModelManager->state()) {
    case QmlProfilerModelManager::Empty :
        d->m_recordButton->setEnabled(true);
        setButtonsEnabled(true);
        break;
    case QmlProfilerModelManager::ClearingData :
        d->m_recordButton->setEnabled(false);
        setButtonsEnabled(false);
        clearDisplay();
        break;
    case QmlProfilerModelManager::AcquiringData :
        restoreFeatureVisibility();
        d->m_recordButton->setEnabled(true); // Press recording button to stop recording
        setButtonsEnabled(false);            // Other buttons disabled
        break;
    case QmlProfilerModelManager::ProcessingData :
        d->m_recordButton->setEnabled(false);
        setButtonsEnabled(false);
        break;
    case QmlProfilerModelManager::Done :
        showSaveOption();
        updateTimeDisplay();
        d->m_recordButton->setEnabled(true);
        setButtonsEnabled(true);
    break;
    default:
        break;
    }
}

QList <QAction *> QmlProfilerTool::profilerContextMenuActions()
{
    QList <QAction *> commonActions;
    ActionManager *manager = ActionManager::instance();
    if (manager) {
        Command *command = manager->command(Constants::QmlProfilerLoadActionId);
        if (command)
            commonActions << command->action();
        command = manager->command(Constants::QmlProfilerSaveActionId);
        if (command)
            commonActions << command->action();
    }
    return commonActions;
}

void QmlProfilerTool::showNonmodalWarning(const QString &warningMsg)
{
    QMessageBox *noExecWarning = new QMessageBox(ICore::mainWindow());
    noExecWarning->setIcon(QMessageBox::Warning);
    noExecWarning->setWindowTitle(tr("QML Profiler"));
    noExecWarning->setText(warningMsg);
    noExecWarning->setStandardButtons(QMessageBox::Ok);
    noExecWarning->setDefaultButton(QMessageBox::Ok);
    noExecWarning->setModal(false);
    noExecWarning->show();
}

void QmlProfilerTool::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppDying : {
        // If already disconnected when dying, check again that all data was read
        if (!d->m_profilerConnections->isConnected())
            clientsDisconnected();
        break;
    }
    case QmlProfilerStateManager::Idle :
        // when the app finishes, set recording display to client status
        setRecording(d->m_profilerState->clientRecording());
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Don't allow toggling the recording while data is loaded when application quits
        if (d->m_profilerState->serverRecording()) {
            d->m_recordButton->setEnabled(false);
            // Turn off recording and wait for remaining data
            d->m_profilerConnections->stopRecording();
        } else {
            // Directly transition to idle
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        }
        break;
    default:
        // no special action needed for other states
        break;
    }
}

void QmlProfilerTool::clientRecordingChanged()
{
    // if application is running, display server record changes
    // if application is stopped, display client record changes
    if (d->m_profilerState->currentState() != QmlProfilerStateManager::AppRunning)
        setRecording(d->m_profilerState->clientRecording());
}

void QmlProfilerTool::serverRecordingChanged()
{
    showLoadOption();
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        // clear the old data each time we start a new profiling session
        if (d->m_profilerState->serverRecording()) {
            // We cannot stop it here, so we cannot give the usual yes/no dialog. Show a dialog
            // offering to immediately save the data instead.
            if (d->m_profilerModelManager->notesModel()->isModified() &&
                    QMessageBox::warning(QApplication::activeWindow(), tr("QML Profiler"),
                                         tr("Starting a new profiling session will discard the "
                                            "previous data, including unsaved notes.\nDo you want "
                                            "to save the data first?"),
                                         QMessageBox::Save, QMessageBox::Discard) ==
                    QMessageBox::Save)
                showSaveDialog();

            setRecording(true);
            if (!d->m_profilerModelManager->aggregateTraces() ||
                    d->m_profilerModelManager->state() == QmlProfilerModelManager::Done)
                clearData();
            d->m_profilerModelManager->startAcquiring();
        } else {
            setRecording(false);

            // changes back once loading is finished, see profilerDataModelStateChanged()
            if (!d->m_profilerModelManager->aggregateTraces()) {
                d->m_recordButton->setEnabled(false);
                d->m_profilerModelManager->acquiringDone();
            }
        }
    } else if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppStopRequested) {
        d->m_profilerModelManager->acquiringDone();
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
    }
}

void QmlProfilerTool::toggleRequestedFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerState->setRequestedFeatures(
                    d->m_profilerState->requestedFeatures() | (1ULL << feature));
    else
        d->m_profilerState->setRequestedFeatures(
                    d->m_profilerState->requestedFeatures() & (~(1ULL << feature)));
}

void QmlProfilerTool::toggleVisibleFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerModelManager->setVisibleFeatures(
                    d->m_profilerModelManager->visibleFeatures() | (1ULL << feature));
    else
        d->m_profilerModelManager->setVisibleFeatures(
                    d->m_profilerModelManager->visibleFeatures() & (~(1ULL << feature)));
}

} // namespace Internal
} // namespace QmlProfiler
