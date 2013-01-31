/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
#include "qmlprofilerdatamodel.h"
#include "qmlprofilerdetailsrewriter.h"
#include "timelinerenderer.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>

#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qmlprofilercanvas.h"

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <utils/fancymainwindow.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/localapplicationrunconfiguration.h>

#include <remotelinux/remotelinuxrunconfiguration.h>
#include <remotelinux/linuxdevice.h>

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
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QTimer>
#include <QTime>

using namespace Core;
using namespace Core::Constants;
using namespace Analyzer;
using namespace Analyzer::Constants;
using namespace QmlProfiler::Internal;
using namespace QmlProfiler::Constants;
using namespace QmlDebug;
using namespace ProjectExplorer;
using namespace QmlProjectManager;
using namespace RemoteLinux;

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerToolPrivate(QmlProfilerTool *qq) : q(qq) {}
    ~QmlProfilerToolPrivate() {}

    QmlProfilerTool *q;

    QmlProfilerStateManager *m_profilerState;
    QmlProfilerClientManager *m_profilerConnections;
    QmlProfilerDataModel *m_profilerDataModel;
    QmlProfilerDetailsRewriter *m_detailsRewriter;

    QmlProfilerViewManager *m_viewContainer;
    Utils::FileInProjectFinder m_projectFinder;
    RunConfiguration *m_runConfiguration;
    QToolButton *m_recordButton;
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
    : IAnalyzerTool(parent), d(new QmlProfilerToolPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerTool"));

    d->m_profilerState = 0;
    d->m_viewContainer = 0;
    d->m_runConfiguration = 0;

    qmlRegisterType<QmlProfilerCanvas>("Monitor", 1, 0, "Canvas2D");
    qmlRegisterType<Context2D>();
    qmlRegisterType<CanvasGradient>();
    qmlRegisterType<TimelineRenderer>("Monitor", 1, 0,"TimelineRenderer");

    d->m_profilerState = new QmlProfilerStateManager(this);
    connect(d->m_profilerState, SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));
    connect(d->m_profilerState, SIGNAL(clientRecordingChanged()), this, SLOT(clientRecordingChanged()));
    connect(d->m_profilerState, SIGNAL(serverRecordingChanged()), this, SLOT(serverRecordingChanged()));

    d->m_profilerConnections = new QmlProfilerClientManager(this);
    d->m_profilerConnections->registerProfilerStateManager(d->m_profilerState);
    connect(d->m_profilerConnections, SIGNAL(connectionClosed()), this, SLOT(clientsDisconnected()));

    d->m_profilerDataModel = new QmlProfilerDataModel(this);
    connect(d->m_profilerDataModel, SIGNAL(stateChanged()), this, SLOT(profilerDataModelStateChanged()));
    connect(d->m_profilerDataModel, SIGNAL(error(QString)), this, SLOT(showErrorDialog(QString)));
    connect(d->m_profilerConnections,
            SIGNAL(addRangedEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation)),
            d->m_profilerDataModel,
            SLOT(addRangedEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation)));
    connect(d->m_profilerConnections,
            SIGNAL(addV8Event(int,QString,QString,int,double,double)),
            d->m_profilerDataModel,
            SLOT(addV8Event(int,QString,QString,int,double,double)));
    connect(d->m_profilerConnections, SIGNAL(addFrameEvent(qint64,int,int)), d->m_profilerDataModel, SLOT(addFrameEvent(qint64,int,int)));
    connect(d->m_profilerConnections, SIGNAL(traceStarted(qint64)), d->m_profilerDataModel, SLOT(setTraceStartTime(qint64)));
    connect(d->m_profilerConnections, SIGNAL(traceFinished(qint64)), d->m_profilerDataModel, SLOT(setTraceEndTime(qint64)));
    connect(d->m_profilerConnections, SIGNAL(dataReadyForProcessing()), d->m_profilerDataModel, SLOT(complete()));


    d->m_detailsRewriter = new QmlProfilerDetailsRewriter(this, &d->m_projectFinder);
    connect(d->m_profilerDataModel, SIGNAL(requestDetailsForLocation(int,QmlDebug::QmlEventLocation)),
            d->m_detailsRewriter, SLOT(requestDetailsForLocation(int,QmlDebug::QmlEventLocation)));
    connect(d->m_detailsRewriter, SIGNAL(rewriteDetailsString(int,QmlDebug::QmlEventLocation,QString)),
            d->m_profilerDataModel, SLOT(rewriteDetailsString(int,QmlDebug::QmlEventLocation,QString)));
    connect(d->m_detailsRewriter, SIGNAL(eventDetailsChanged()), d->m_profilerDataModel, SLOT(finishedRewritingDetails()));
    connect(d->m_profilerDataModel, SIGNAL(reloadDocumentsForDetails()), d->m_detailsRewriter, SLOT(reloadDocuments()));

    Command *command = 0;
    const Context globalContext(C_GLOBAL);

    ActionContainer *menu = Core::ActionManager::actionContainer(M_DEBUG_ANALYZER);
    ActionContainer *options = Core::ActionManager::createMenu(M_DEBUG_ANALYZER_QML_OPTIONS);
    options->menu()->setTitle(tr("QML Profiler Options"));
    menu->addMenu(options, G_ANALYZER_OPTIONS);
    options->menu()->setEnabled(true);

    QAction *act = d->m_loadQmlTrace = new QAction(tr("Load QML Trace"), options);
    command = Core::ActionManager::registerAction(act, "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.LoadQMLTrace", globalContext);
    connect(act, SIGNAL(triggered()), this, SLOT(showLoadDialog()));
    options->addAction(command);

    act = d->m_saveQmlTrace = new QAction(tr("Save QML Trace"), options);
    d->m_saveQmlTrace->setEnabled(false);
    command = Core::ActionManager::registerAction(act, "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.SaveQMLTrace", globalContext);
    connect(act, SIGNAL(triggered()), this, SLOT(showSaveDialog()));
    options->addAction(command);

    d->m_recordingTimer.setInterval(100);
    connect(&d->m_recordingTimer, SIGNAL(timeout()), this, SLOT(updateTimeDisplay()));
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d;
}

Core::Id QmlProfilerTool::id() const
{
    return Core::Id("QmlProfiler");
}

RunMode QmlProfilerTool::runMode() const
{
    return QmlProfilerRunMode;
}

QString QmlProfilerTool::displayName() const
{
    return tr("QML Profiler");
}

QString QmlProfilerTool::description() const
{
    return tr("The QML Profiler can be used to find performance bottlenecks in "
              "applications using QML.");
}

IAnalyzerTool::ToolMode QmlProfilerTool::toolMode() const
{
    return AnyMode;
}

IAnalyzerEngine *QmlProfilerTool::createEngine(const AnalyzerStartParameters &sp,
    RunConfiguration *runConfiguration)
{
    QmlProfilerEngine *engine = new QmlProfilerEngine(this, sp, runConfiguration);

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
        d->m_profilerConnections->setTcpConnection(sp.connParams.host, sp.connParams.port);

    d->m_runConfiguration = runConfiguration;

    //
    // Initialize m_projectFinder
    //

    QString projectDirectory;
    if (d->m_runConfiguration) {
        Project *project = d->m_runConfiguration->target()->project();
        projectDirectory = project->projectDirectory();
    }

    populateFileFinder(projectDirectory, sp.sysroot);

    connect(engine, SIGNAL(processRunning(quint16)), d->m_profilerConnections, SLOT(connectClient(quint16)));
    connect(d->m_profilerConnections, SIGNAL(connectionFailed()), engine, SLOT(cancelProcess()));

    return engine;
}

bool QmlProfilerTool::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (qobject_cast<QmlProjectRunConfiguration *>(runConfiguration)
            || qobject_cast<RemoteLinuxRunConfiguration *>(runConfiguration)
            || qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration))
        return mode == runMode();
    return false;
}

static QString sysroot(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return QString());
    ProjectExplorer::Kit *k = runConfig->target()->kit();
    if (k && ProjectExplorer::SysRootKitInformation::hasSysRoot(k))
        return ProjectExplorer::SysRootKitInformation::sysRoot(runConfig->target()->kit()).toString();
    return QString();
}

AnalyzerStartParameters QmlProfilerTool::createStartParameters(RunConfiguration *runConfiguration, RunMode mode) const
{
    Q_UNUSED(mode);

    AnalyzerStartParameters sp;
    sp.startMode = StartQml; // FIXME: The parameter struct is not needed/not used.

    // FIXME: This is only used to communicate the connParams settings.
    if (QmlProjectRunConfiguration *rc1 =
            qobject_cast<QmlProjectRunConfiguration *>(runConfiguration)) {
        // This is a "plain" .qmlproject.
        sp.environment = rc1->environment();
        sp.workingDirectory = rc1->workingDirectory();
        sp.debuggee = rc1->observerPath();
        sp.debuggeeArgs = rc1->viewerArguments();
        sp.displayName = rc1->displayName();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc1->debuggerAspect()->qmlDebugServerPort();
    } else if (LocalApplicationRunConfiguration *rc2 =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration)) {
        sp.environment = rc2->environment();
        sp.workingDirectory = rc2->workingDirectory();
        sp.debuggee = rc2->executable();
        sp.debuggeeArgs = rc2->commandLineArguments();
        sp.displayName = rc2->displayName();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc2->debuggerAspect()->qmlDebugServerPort();
    } else if (RemoteLinux::RemoteLinuxRunConfiguration *rc3 =
            qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp.debuggee = rc3->remoteExecutableFilePath();
        sp.debuggeeArgs = rc3->arguments();
        sp.connParams = ProjectExplorer::DeviceKitInformation::device(rc3->target()->kit())->sshParameters();
        sp.analyzerCmdPrefix = rc3->commandPrefix();
        sp.displayName = rc3->displayName();
        sp.sysroot = sysroot(rc3);
    } else {
        // What could that be?
        QTC_ASSERT(false, return sp);
    }
    return sp;
}

QWidget *QmlProfilerTool::createWidgets()
{
    QTC_ASSERT(!d->m_viewContainer, return 0);


    d->m_viewContainer = new QmlProfilerViewManager(this,
                                                    this,
                                                    d->m_profilerDataModel,
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
    setRecording(d->m_profilerState->clientRecording());
    layout->addWidget(d->m_recordButton);

    d->m_clearButton = new QToolButton(toolbarWidget);
    d->m_clearButton->setIcon(QIcon(QLatin1String(":/qmlprofiler/clean_pane_small.png")));
    d->m_clearButton->setToolTip(tr("Discard data"));

    connect(d->m_clearButton,SIGNAL(clicked()), this, SLOT(clearData()));

    layout->addWidget(d->m_clearButton);

    d->m_timeLabel = new QLabel();
    QPalette palette = d->m_timeLabel->palette();
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
    SessionManager *sessionManager = ProjectExplorerPlugin::instance()->session();
    QList<Project *> projects = sessionManager->projects();
    if (Project *startupProject = ProjectExplorerPlugin::instance()->startupProject()) {
        // startup project first
        projects.removeOne(ProjectExplorerPlugin::instance()->startupProject());
        projects.insert(0, startupProject);
    }
    foreach (Project *project, projects)
        sourceFiles << project->files(Project::ExcludeGeneratedFiles);

    if (!projects.isEmpty()) {
        if (projectDirectory.isEmpty())
            projectDirectory = projects.first()->projectDirectory();

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
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        EditorManager::instance()->addCurrentPositionToNavigationHistory();
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
    } else if (d->m_profilerDataModel->currentState() != QmlProfilerDataModel::Empty ) {
        seconds = (d->m_profilerDataModel->traceEndTime() - d->m_profilerDataModel->traceStartTime()) / 1.0e9;
    }
    QString timeString = QString::number(seconds,'f',1);
    QString profilerTimeStr = QmlProfilerTool::tr("%1 s").arg(timeString, 6);
    d->m_timeLabel->setText(tr("Elapsed: %1").arg(profilerTimeStr));
}

void QmlProfilerTool::clearData()
{
    d->m_profilerDataModel->clear();
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
    Q_UNUSED(tool);

    QString host;
    quint16 port;
    QString sysroot;

    {
        QSettings *settings = ICore::settings();

        host = settings->value(QLatin1String("AnalyzerQmlAttachDialog/host"), QLatin1String("localhost")).toString();
        port = settings->value(QLatin1String("AnalyzerQmlAttachDialog/port"), 3768).toInt();
        sysroot = settings->value(QLatin1String("AnalyzerQmlAttachDialog/sysroot")).toString();

        QmlProfilerAttachDialog dialog;

        dialog.setAddress(host);
        dialog.setPort(port);
        dialog.setSysroot(sysroot);

        if (dialog.exec() != QDialog::Accepted)
            return;

        host = dialog.address();
        port = dialog.port();
        sysroot = dialog.sysroot();

        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/host"), host);
        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/port"), port);
        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/sysroot"), sysroot);
    }

    AnalyzerStartParameters sp;
    sp.toolId = tool->id();
    sp.startMode = mode;
    sp.connParams.host = host;
    sp.connParams.port = port;
    sp.sysroot = sysroot;

    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, 0);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));

    ProjectExplorerPlugin::instance()->startRunControl(rc, tool->runMode());
}

void QmlProfilerTool::startTool(StartMode mode)
{
    using namespace ProjectExplorer;

    // Make sure mode is shown.
    AnalyzerManager::showMode();

    if (mode == StartLocal) {
        ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
        // ### not sure if we're supposed to check if the RunConFiguration isEnabled
        Project *pro = pe->startupProject();
        pe->runProject(pro, runMode());
    } else if (mode == StartRemote) {
        startRemoteTool(this, mode);
    }
}

void QmlProfilerTool::logStatus(const QString &msg)
{
    MessageManager *messageManager = MessageManager::instance();
    messageManager->printToOutputPane(msg, false);
}

void QmlProfilerTool::logError(const QString &msg)
{
    MessageManager *messageManager = MessageManager::instance();
    messageManager->printToOutputPane(msg, true);
}

void QmlProfilerTool::showErrorDialog(const QString &error)
{
    QMessageBox *errorDialog = new QMessageBox(Core::ICore::mainWindow());
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
    d->m_saveQmlTrace->setEnabled(!d->m_profilerDataModel->isEmpty());
}

void QmlProfilerTool::showSaveDialog()
{
    QString filename = QFileDialog::getSaveFileName(Core::ICore::mainWindow(), tr("Save QML Trace"), QString(),
                                                    tr("QML traces (*%1)").arg(QLatin1String(TraceFileExtension)));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QLatin1String(TraceFileExtension)))
            filename += QLatin1String(TraceFileExtension);
        d->m_profilerDataModel->save(filename);
    }
}

void QmlProfilerTool::showLoadDialog()
{
    if (ModeManager::currentMode()->id() != MODE_ANALYZE)
        AnalyzerManager::showMode();

    if (AnalyzerManager::currentSelectedTool() != this)
        AnalyzerManager::selectTool(this, StartRemote);

    QString filename = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Load QML Trace"), QString(),
                                                    tr("QML traces (*%1)").arg(QLatin1String(TraceFileExtension)));

    if (!filename.isEmpty()) {
        // delayed load (prevent graphical artifacts due to long load time)
        d->m_profilerDataModel->setFilename(filename);
        QTimer::singleShot(100, d->m_profilerDataModel, SLOT(load()));
    }
}

void QmlProfilerTool::clientsDisconnected()
{
    // If the application stopped by itself, check if we have all the data
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppDying) {
        if (d->m_profilerDataModel->currentState() == QmlProfilerDataModel::AcquiringData)
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppKilled);
        else
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopped);

        // ... and return to the "base" state
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
    }

    // If the connection is closed while the app is still running, no special action is needed
}

void QmlProfilerTool::profilerDataModelStateChanged()
{
    switch (d->m_profilerDataModel->currentState()) {
    case QmlProfilerDataModel::Empty :
        clearDisplay();
        break;
    case QmlProfilerDataModel::AcquiringData :
    case QmlProfilerDataModel::ProcessingData :
        // nothing to be done for these two
        break;
    case QmlProfilerDataModel::Done :
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
    QMessageBox *noExecWarning = new QMessageBox(Core::ICore::mainWindow());
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
    return new QMessageBox(Core::ICore::mainWindow());
}

void QmlProfilerTool::handleHelpRequest(const QString &link)
{
    HelpManager *helpManager = HelpManager::instance();
    helpManager->handleHelpRequest(link);
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
        d->m_profilerDataModel->clear();
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
            clearData();
            d->m_profilerDataModel->prepareForWriting();
        }
    }
}

