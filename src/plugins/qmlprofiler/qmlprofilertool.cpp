/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilerengine.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerattachdialog.h"
#include "qmlprofilerviewmanager.h"
#include "qmlprofilerclientmanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdetailsrewriter.h"
#include "timelinerenderer.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>

#include <utils/fancymainwindow.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <texteditor/texteditor.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
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
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QTcpServer>
#include <QTime>
#include <QTimer>
#include <QToolButton>

using namespace Core;
using namespace Core::Constants;
using namespace Analyzer;
using namespace Analyzer::Constants;
using namespace QmlProfiler::Constants;
using namespace QmlDebug;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerStateManager *m_profilerState;
    QmlProfilerClientManager *m_profilerConnections;
    QmlProfilerModelManager *m_profilerModelManager;

    QmlProfilerViewManager *m_viewContainer;
    Utils::FileInProjectFinder m_projectFinder;
    QToolButton *m_recordButton;
    QMenu *m_featuresMenu;

    QToolButton *m_clearButton;

    // elapsed time display
    QTimer m_recordingTimer;
    QTime m_recordingElapsedTime;
    QLabel *m_timeLabel;

    // save and load actions
    QAction *m_saveQmlTrace;
    QAction *m_loadQmlTrace;
};

QmlProfilerTool::QmlProfilerTool(QObject *parent)
    : IAnalyzerTool(parent), d(new QmlProfilerToolPrivate)
{
    setObjectName(QLatin1String("QmlProfilerTool"));
    setRunMode(QmlProfilerRunMode);
    setToolMode(AnyMode);

    d->m_profilerState = 0;
    d->m_viewContainer = 0;
    d->m_recordButton = 0;
    d->m_featuresMenu = 0;
    d->m_clearButton = 0;
    d->m_timeLabel = 0;

    qmlRegisterType<TimelineRenderer>("Monitor", 1, 0,"TimelineRenderer");

    d->m_profilerState = new QmlProfilerStateManager(this);
    connect(d->m_profilerState, SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));
    connect(d->m_profilerState, SIGNAL(clientRecordingChanged()), this, SLOT(clientRecordingChanged()));
    connect(d->m_profilerState, SIGNAL(serverRecordingChanged()), this, SLOT(serverRecordingChanged()));

    d->m_profilerConnections = new QmlProfilerClientManager(this);
    d->m_profilerConnections->registerProfilerStateManager(d->m_profilerState);
    connect(d->m_profilerConnections, SIGNAL(connectionClosed()), this, SLOT(clientsDisconnected()));

    d->m_profilerModelManager = new QmlProfilerModelManager(&d->m_projectFinder, this);
    connect(d->m_profilerModelManager, SIGNAL(stateChanged()), this, SLOT(profilerDataModelStateChanged()));
    connect(d->m_profilerModelManager, SIGNAL(error(QString)), this, SLOT(showErrorDialog(QString)));
    connect(d->m_profilerModelManager, SIGNAL(availableFeaturesChanged(quint64)),
            this, SLOT(setAvailableFeatures(quint64)));

    d->m_profilerConnections->setModelManager(d->m_profilerModelManager);
    Command *command = 0;
    const Context globalContext(C_GLOBAL);

    ActionContainer *menu = ActionManager::actionContainer(M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(M_DEBUG_ANALYZER_QML_OPTIONS);
    options->menu()->setTitle(tr("QML Profiler Options"));
    menu->addMenu(options, G_ANALYZER_OPTIONS);
    options->menu()->setEnabled(true);

    QAction *act = d->m_loadQmlTrace = new QAction(tr("Load QML Trace"), options);
    command = ActionManager::registerAction(act, "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.LoadQMLTrace", globalContext);
    connect(act, SIGNAL(triggered()), this, SLOT(showLoadDialog()));
    options->addAction(command);

    act = d->m_saveQmlTrace = new QAction(tr("Save QML Trace"), options);
    d->m_saveQmlTrace->setEnabled(false);
    command = ActionManager::registerAction(act, "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.SaveQMLTrace", globalContext);
    connect(act, SIGNAL(triggered()), this, SLOT(showSaveDialog()));
    options->addAction(command);

    d->m_recordingTimer.setInterval(100);
    connect(&d->m_recordingTimer, SIGNAL(timeout()), this, SLOT(updateTimeDisplay()));
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d;
}

AnalyzerRunControl *QmlProfilerTool::createRunControl(const AnalyzerStartParameters &sp,
    RunConfiguration *runConfiguration)
{
    QmlProfilerRunControl *engine = new QmlProfilerRunControl(sp, runConfiguration);

    engine->registerProfilerStateManager(d->m_profilerState);

    bool isTcpConnection = true;

    if (runConfiguration) {
        // Check minimum Qt Version. We cannot really be sure what the Qt version
        // at runtime is, but guess that the active build configuraiton has been used.
        QtSupport::QtVersionNumber minimumVersion(4, 7, 4);
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(runConfiguration->target()->kit());
        if (version) {
            if (version->isValid() && version->qtVersion() < minimumVersion) {
                int result = QMessageBox::warning(QApplication::activeWindow(), tr("QML Profiler"),
                     tr("The QML profiler requires Qt 4.7.4 or newer.\n"
                     "The Qt version configured in your active build configuration is too old.\n"
                     "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (result == QMessageBox::No)
                    return 0;
            }
        }
    }

    // FIXME: Check that there's something sensible in sp.connParams
    if (isTcpConnection)
        d->m_profilerConnections->setTcpConnection(sp.analyzerHost, sp.analyzerPort);

    //
    // Initialize m_projectFinder
    //

    QString projectDirectory;
    if (runConfiguration) {
        Project *project = runConfiguration->target()->project();
        projectDirectory = project->projectDirectory().toString();
    }

    populateFileFinder(projectDirectory, sp.sysroot);

    connect(engine, SIGNAL(processRunning(quint16)), d->m_profilerConnections, SLOT(connectClient(quint16)));
    connect(d->m_profilerConnections, SIGNAL(connectionFailed()), engine, SLOT(cancelProcess()));

    return engine;
}

static QString sysroot(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return QString());
    Kit *k = runConfig->target()->kit();
    if (k && SysRootKitInformation::hasSysRoot(k))
        return SysRootKitInformation::sysRoot(runConfig->target()->kit()).toString();
    return QString();
}

QWidget *QmlProfilerTool::createWidgets()
{
    QTC_ASSERT(!d->m_viewContainer, return 0);


    d->m_viewContainer = new QmlProfilerViewManager(this,
                                                    this,
                                                    d->m_profilerModelManager,
                                                    d->m_profilerState);
    connect(d->m_viewContainer, SIGNAL(gotoSourceLocation(QString,int,int)),
            this, SLOT(gotoSourceLocation(QString,int,int)));

    //
    // Toolbar
    //
    QWidget *toolbarWidget = new QWidget;
    toolbarWidget->setObjectName(QLatin1String("QmlProfilerToolBarWidget"));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    d->m_recordButton = new QToolButton(toolbarWidget);
    d->m_recordButton->setCheckable(true);

    connect(d->m_recordButton,SIGNAL(clicked(bool)), this, SLOT(recordingButtonChanged(bool)));
    d->m_recordButton->setChecked(true);
    d->m_featuresMenu = new QMenu(d->m_recordButton);
    d->m_recordButton->setMenu(d->m_featuresMenu);
    d->m_recordButton->setPopupMode(QToolButton::MenuButtonPopup);
    setAvailableFeatures(d->m_profilerModelManager->availableFeatures());
    connect(d->m_featuresMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(toggleRecordingFeature(QAction*)));

    setRecording(d->m_profilerState->clientRecording());
    layout->addWidget(d->m_recordButton);

    d->m_clearButton = new QToolButton(toolbarWidget);
    d->m_clearButton->setIcon(QIcon(QLatin1String(":/qmlprofiler/clean_pane_small.png")));
    d->m_clearButton->setToolTip(tr("Discard data"));

    connect(d->m_clearButton,SIGNAL(clicked()), this, SLOT(clearData()));

    layout->addWidget(d->m_clearButton);

    d->m_timeLabel = new QLabel();
    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::white);
    d->m_timeLabel->setPalette(palette);
    d->m_timeLabel->setIndent(10);
    updateTimeDisplay();
    layout->addWidget(d->m_timeLabel);

    toolbarWidget->setLayout(layout);

    // When the widgets are requested we assume that the session data
    // is available, then we can populate the file finder
    populateFileFinder();

    return toolbarWidget;
}

void QmlProfilerTool::populateFileFinder(QString projectDirectory, QString activeSysroot)
{
    // Initialize filefinder with some sensible default
    QStringList sourceFiles;
    QList<Project *> projects = SessionManager::projects();
    if (Project *startupProject = SessionManager::startupProject()) {
        // startup project first
        projects.removeOne(startupProject);
        projects.insert(0, startupProject);
    }
    foreach (Project *project, projects)
        sourceFiles << project->files(Project::ExcludeGeneratedFiles);

    if (!projects.isEmpty()) {
        if (projectDirectory.isEmpty())
            projectDirectory = projects.first()->projectDirectory().toString();

        if (activeSysroot.isEmpty()) {
            if (Target *target = projects.first()->activeTarget())
                if (RunConfiguration *rc = target->activeRunConfiguration())
                    activeSysroot = sysroot(rc);
        }
    }

    d->m_projectFinder.setProjectDirectory(projectDirectory);
    d->m_projectFinder.setProjectFiles(sourceFiles);
    d->m_projectFinder.setSysroot(activeSysroot);
}

void QmlProfilerTool::recordingButtonChanged(bool recording)
{
    d->m_profilerState->setClientRecording(recording);
}

void QmlProfilerTool::setRecording(bool recording)
{
    // update display
    d->m_recordButton->setToolTip( recording ? tr("Disable profiling") : tr("Enable profiling"));
    d->m_recordButton->setIcon(QIcon(recording ? QLatin1String(":/qmlprofiler/recordOn.png") :
                                                 QLatin1String(":/qmlprofiler/recordOff.png")));

    d->m_recordButton->setChecked(recording);

    // manage timer
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        if (recording) {
            d->m_recordingTimer.start();
            d->m_recordingElapsedTime.start();
        } else {
            d->m_recordingTimer.stop();
        }
        d->m_recordButton->menu()->setEnabled(!recording);
    } else {
        d->m_recordButton->menu()->setEnabled(true);
    }
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber)
{
    if (lineNumber < 0 || fileUrl.isEmpty())
        return;

    const QString projectFileName = d->m_projectFinder.findFile(fileUrl);

    QFileInfo fileInfo(projectFileName);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return;

    IEditor *editor = EditorManager::openEditor(projectFileName);
    TextEditor::BaseTextEditor *textEditor = qobject_cast<TextEditor::BaseTextEditor*>(editor);

    if (textEditor) {
        EditorManager::addCurrentPositionToNavigationHistory();
        // textEditor counts columns starting with 0, but the ASTs store the
        // location starting with 1, therefore the -1 in the call to gotoLine
        textEditor->gotoLine(lineNumber, columnNumber - 1);
        textEditor->widget()->setFocus();
    }
}

void QmlProfilerTool::updateTimeDisplay()
{
    double seconds = 0;
    if (d->m_profilerState->serverRecording() &&
        d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
            seconds = d->m_recordingElapsedTime.elapsed() / 1000.0;
    } else if (d->m_profilerModelManager->state() != QmlProfilerDataState::Empty &&
               d->m_profilerModelManager->state() != QmlProfilerDataState::ClearingData) {
        seconds = d->m_profilerModelManager->traceTime()->duration() / 1.0e9;
    }
    QString timeString = QString::number(seconds,'f',1);
    QString profilerTimeStr = QmlProfilerTool::tr("%1 s").arg(timeString, 6);
    d->m_timeLabel->setText(tr("Elapsed: %1").arg(profilerTimeStr));
}

void QmlProfilerTool::clearData()
{
    d->m_profilerModelManager->clear();
    d->m_profilerConnections->discardPendingData();
}

void QmlProfilerTool::clearDisplay()
{
    d->m_profilerConnections->clearBufferedData();
    d->m_viewContainer->clear();
    updateTimeDisplay();
}

static void startRemoteTool(IAnalyzerTool *tool, StartMode mode)
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

    AnalyzerStartParameters sp;
    sp.startMode = mode;

    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    if (device) {
        sp.connParams = device->sshParameters();
        sp.analyzerHost = device->qmlProfilerHost();
    }
    sp.sysroot = SysRootKitInformation::sysRoot(kit).toString();
    sp.analyzerPort = port;

    AnalyzerRunControl *rc = tool->createRunControl(sp, 0);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));

    ProjectExplorerPlugin::startRunControl(rc, tool->runMode());
}

void QmlProfilerTool::startTool(StartMode mode)
{
    // Make sure mode is shown.
    AnalyzerManager::showMode();

    if (mode == StartLocal) {
        // ### not sure if we're supposed to check if the RunConFiguration isEnabled
        Project *pro = SessionManager::startupProject();
        ProjectExplorerPlugin::instance()->runProject(pro, runMode());
    } else if (mode == StartRemote) {
        startRemoteTool(this, mode);
    }
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

void QmlProfilerTool::showSaveOption()
{
    d->m_saveQmlTrace->setEnabled(!d->m_profilerModelManager->isEmpty());
}

void QmlProfilerTool::showSaveDialog()
{
    QString filename = QFileDialog::getSaveFileName(ICore::mainWindow(), tr("Save QML Trace"), QString(),
                                                    tr("QML traces (*%1)").arg(QLatin1String(TraceFileExtension)));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QLatin1String(TraceFileExtension)))
            filename += QLatin1String(TraceFileExtension);
        d->m_profilerModelManager->save(filename);
    }
}

void QmlProfilerTool::showLoadDialog()
{
    if (ModeManager::currentMode()->id() != MODE_ANALYZE)
        AnalyzerManager::showMode();

    AnalyzerManager::selectTool(this, StartRemote);

    QString filename = QFileDialog::getOpenFileName(ICore::mainWindow(), tr("Load QML Trace"), QString(),
                                                    tr("QML traces (*%1)").arg(QLatin1String(TraceFileExtension)));

    if (!filename.isEmpty()) {
        // delayed load (prevent graphical artifacts due to long load time)
        d->m_profilerModelManager->setFilename(filename);
        QTimer::singleShot(100, d->m_profilerModelManager, SLOT(load()));
    }
}

void QmlProfilerTool::clientsDisconnected()
{
    // If the application stopped by itself, check if we have all the data
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppDying) {
        if (d->m_profilerModelManager->state() == QmlProfilerDataState::AcquiringData)
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppKilled);
        else
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopped);

        // ... and return to the "base" state
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
    }
    // If the connection is closed while the app is still running, no special action is needed
}

template<QmlDebug::ProfileFeature feature>
void QmlProfilerTool::updateFeaturesMenu(quint64 features)
{
    if (features & (1ULL << (feature))) {
        QAction *action = d->m_featuresMenu->addAction(tr(QmlProfilerModelManager::featureName(
                                               static_cast<QmlDebug::ProfileFeature>(feature))));
        action->setCheckable(true);
        action->setData(static_cast<uint>(feature));
        action->setChecked(d->m_profilerState->recordingFeatures() & (1ULL << (feature)));
    }
    updateFeaturesMenu<static_cast<QmlDebug::ProfileFeature>(feature + 1)>(features);
}

template<>
void QmlProfilerTool::updateFeaturesMenu<QmlDebug::MaximumProfileFeature>(quint64 features)
{
    Q_UNUSED(features);
    return;
}

void QmlProfilerTool::setAvailableFeatures(quint64 features)
{
    if (features != d->m_profilerState->recordingFeatures())
        d->m_profilerState->setRecordingFeatures(features); // by default, enable them all.
    if (d->m_featuresMenu) {
        d->m_featuresMenu->clear();
        updateFeaturesMenu<static_cast<QmlDebug::ProfileFeature>(0)>(features);
    }
}

void QmlProfilerTool::profilerDataModelStateChanged()
{
    switch (d->m_profilerModelManager->state()) {
    case QmlProfilerDataState::Empty :
        break;
    case QmlProfilerDataState::ClearingData :
        clearDisplay();
        break;
    case QmlProfilerDataState::AcquiringData :
    case QmlProfilerDataState::ProcessingData :
        // nothing to be done for these two
        break;
    case QmlProfilerDataState::Done :
        if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppStopRequested)
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppReadyToStop);
        showSaveOption();
        updateTimeDisplay();
    break;
    default:
        break;
    }
}

QList <QAction *> QmlProfilerTool::profilerContextMenuActions() const
{
    QList <QAction *> commonActions;
    commonActions << d->m_loadQmlTrace << d->m_saveQmlTrace;
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

QMessageBox *QmlProfilerTool::requestMessageBox()
{
    return new QMessageBox(ICore::mainWindow());
}

void QmlProfilerTool::handleHelpRequest(const QString &link)
{
    HelpManager::handleHelpRequest(link);
}

void QmlProfilerTool::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppDying : {
        // If already disconnected when dying, check again that all data was read
        if (!d->m_profilerConnections->isConnected())
            QTimer::singleShot(0, this, SLOT(clientsDisconnected()));
        break;
    }
    case QmlProfilerStateManager::AppKilled : {
        showNonmodalWarning(tr("Application finished before loading profiled data.\nPlease use the stop button instead."));
        d->m_profilerModelManager->clear();
        break;
    }
    case QmlProfilerStateManager::Idle :
        // when the app finishes, set recording display to client status
        setRecording(d->m_profilerState->clientRecording());
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
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        setRecording(d->m_profilerState->serverRecording());
        // clear the old data each time we start a new profiling session
        if (d->m_profilerState->serverRecording()) {
            d->m_clearButton->setEnabled(false);
            clearData();
            d->m_profilerModelManager->prepareForWriting();
        } else {
            d->m_clearButton->setEnabled(true);
        }
    } else {
        d->m_clearButton->setEnabled(true);
    }
}

void QmlProfilerTool::toggleRecordingFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerState->setRecordingFeatures(
                    d->m_profilerState->recordingFeatures() | (1ULL << feature));
    else
        d->m_profilerState->setRecordingFeatures(
                    d->m_profilerState->recordingFeatures() & (~(1ULL << feature)));

    // Keep the menu open to allow for more features to be toggled
    d->m_recordButton->showMenu();
}

}
}
