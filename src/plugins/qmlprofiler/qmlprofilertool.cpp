/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilertool.h"
#include "qmlprofilerengine.h"
#include "qmlprofilerplugin.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerattachdialog.h"
#include "qmlprofilereventview.h"

#include "tracewindow.h"
#include "timelineview.h"

#include <qmljsdebugclient/qmlprofilereventlist.h>
#include <qmljsdebugclient/qdeclarativedebugclient.h>

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerconstants.h>
#include <analyzerbase/analyzerruncontrol.h>

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <utils/fancymainwindow.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/helpmanager.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt-s60/s60deployconfiguration.h>

#include <QtCore/QFile>

#include <QtGui/QApplication>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QMessageBox>
#include <QtGui/QDockWidget>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>

using namespace Analyzer;
using namespace QmlProfiler::Internal;
using namespace QmlJsDebugClient;
using namespace ProjectExplorer;

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerToolPrivate(QmlProfilerTool *qq) : q(qq) {}
    ~QmlProfilerToolPrivate() {}

    QmlProfilerTool *q;

    QDeclarativeDebugConnection *m_client;
    QTimer m_connectionTimer;
    int m_connectionAttempts;
    TraceWindow *m_traceWindow;
    QmlProfilerEventsView *m_eventsView;
    QmlProfilerEventsView *m_calleeView;
    QmlProfilerEventsView *m_callerView;
    Project *m_project;
    Utils::FileInProjectFinder m_projectFinder;
    RunConfiguration *m_runConfiguration;
    bool m_isAttached;
    QToolButton *m_recordButton;
    QToolButton *m_clearButton;
    bool m_recordingEnabled;
    bool m_appIsRunning;

    enum ConnectMode {
        TcpConnection, OstConnection
    };

    ConnectMode m_connectMode;
    QString m_tcpHost;
    quint64 m_tcpPort;
    QString m_ostDevice;
};

QmlProfilerTool::QmlProfilerTool(QObject *parent)
    : IAnalyzerTool(parent), d(new QmlProfilerToolPrivate(this))
{
    setObjectName("QmlProfilerTool");
    d->m_client = 0;
    d->m_connectionAttempts = 0;
    d->m_traceWindow = 0;
    d->m_project = 0;
    d->m_runConfiguration = 0;
    d->m_isAttached = false;
    d->m_recordingEnabled = true;
    d->m_appIsRunning = false;

    d->m_connectionTimer.setInterval(200);
    connect(&d->m_connectionTimer, SIGNAL(timeout()), SLOT(tryToConnect()));

    qmlRegisterType<Canvas>("Monitor", 1, 0, "Canvas");
    qmlRegisterType<TiledCanvas>("Monitor", 1, 0, "TiledCanvas");
    qmlRegisterType<Context2D>();
    qmlRegisterType<CanvasImage>();
    qmlRegisterType<CanvasGradient>();
    qmlRegisterType<TimelineView>("Monitor", 1, 0,"TimelineView");
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d->m_client;
    delete d;
}

QByteArray QmlProfilerTool::id() const
{
    return "QmlProfiler";
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

void QmlProfilerTool::showContextMenu(const QPoint &position)
{
    QmlProfilerEventsView *senderView = qobject_cast<QmlProfilerEventsView *>(sender());

    QMenu menu;
    QAction *loadAction = menu.addAction(tr("Load QML Trace"));
    QAction *saveAction = menu.addAction(tr("Save QML Trace"));
    QAction *copyRowAction;
    QAction *copyTableAction;
    if (senderView) {
        if (senderView->selectedItem().isValid())
            copyRowAction = menu.addAction(tr("Copy Row"));
        copyTableAction = menu.addAction(tr("Copy Table"));
    }

    QAction *selectedAction = menu.exec(position);
    if (selectedAction == loadAction)
        showLoadDialog();
    if (selectedAction == saveAction)
        showSaveDialog();
    if (selectedAction == copyRowAction)
        senderView->copyRowToClipboard();
    if (selectedAction == copyTableAction)
        senderView->copyTableToClipboard();
}

IAnalyzerEngine *QmlProfilerTool::createEngine(const AnalyzerStartParameters &sp,
    RunConfiguration *runConfiguration)
{
    QmlProfilerEngine *engine = new QmlProfilerEngine(this, sp, runConfiguration);

    d->m_connectMode = QmlProfilerToolPrivate::TcpConnection;

    if (runConfiguration) {
        // Check minimum Qt Version. We cannot really be sure what the Qt version
        // at runtime is, but guess that the active build configuraiton has been used.
        QtSupport::QtVersionNumber minimumVersion(4, 7, 4);
        if (Qt4ProjectManager::Qt4BuildConfiguration *qt4Config
                = qobject_cast<Qt4ProjectManager::Qt4BuildConfiguration*>(
                    runConfiguration->target()->activeBuildConfiguration())) {
            if (qt4Config->qtVersion()->isValid() && qt4Config->qtVersion()->qtVersion() < minimumVersion) {
                int result = QMessageBox::warning(QApplication::activeWindow(), tr("QML Profiler"),
                     tr("The QML profiler requires Qt 4.7.4 or newer.\n"
                     "The Qt version configured in your active build configuration is too old.\n"
                     "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (result == QMessageBox::No)
                    return 0;
            }
        }

        // Check whether we should use OST instead of TCP
        if (Qt4ProjectManager::S60DeployConfiguration *deployConfig
                = qobject_cast<Qt4ProjectManager::S60DeployConfiguration*>(
                    runConfiguration->target()->activeDeployConfiguration())) {
            if (deployConfig->communicationChannel()
                    == Qt4ProjectManager::S60DeployConfiguration::CommunicationCodaSerialConnection) {
                d->m_connectMode = QmlProfilerToolPrivate::OstConnection;
                d->m_ostDevice = deployConfig->serialPortName();
            }
        }
    }

    // FIXME: Check that there's something sensible in sp.connParams
    if (d->m_connectMode == QmlProfilerToolPrivate::TcpConnection) {
        d->m_tcpHost = sp.connParams.host;
        d->m_tcpPort = sp.connParams.port;
    }

    d->m_runConfiguration = runConfiguration;

    if (runConfiguration)
        d->m_project = runConfiguration->target()->project();
    else
        d->m_project = ProjectExplorerPlugin::instance()->currentProject();

    if (d->m_project) {
        d->m_projectFinder.setProjectDirectory(d->m_project->projectDirectory());
        updateProjectFileList();
        connect(d->m_project, SIGNAL(fileListChanged()), this, SLOT(updateProjectFileList()));
    }

    connect(engine, SIGNAL(processRunning(int)), this, SLOT(connectClient(int)));
    connect(engine, SIGNAL(finished()), this, SLOT(disconnectClient()));
    connect(engine, SIGNAL(finished()), this, SLOT(correctTimer()));
    connect(engine, SIGNAL(stopRecording()), this, SLOT(stopRecording()));
    connect(d->m_traceWindow, SIGNAL(viewUpdated()), engine, SLOT(dataReceived()));
    connect(this, SIGNAL(connectionFailed()), engine, SLOT(finishProcess()));
    connect(this, SIGNAL(fetchingData(bool)), engine, SLOT(setFetchingData(bool)));
    connect(engine, SIGNAL(starting(const Analyzer::IAnalyzerEngine*)), this, SLOT(setAppIsRunning()));
    connect(engine, SIGNAL(finished()), this, SLOT(setAppIsStopped()));
    connect(this, SIGNAL(cancelRun()), engine, SLOT(finishProcess()));
    emit fetchingData(d->m_recordButton->isChecked());

    return engine;
}

QWidget *QmlProfilerTool::createWidgets()
{
    QTC_ASSERT(!d->m_traceWindow, return 0);

    //
    // DockWidgets
    //

    Utils::FancyMainWindow *mw = AnalyzerManager::mainWindow();

    d->m_traceWindow = new TraceWindow(mw);
    d->m_traceWindow->reset(d->m_client);

    connect(d->m_traceWindow, SIGNAL(gotoSourceLocation(QString,int)),this, SLOT(gotoSourceLocation(QString,int)));
    connect(d->m_traceWindow, SIGNAL(timeChanged(qreal)), this, SLOT(updateTimer(qreal)));
    connect(d->m_traceWindow, SIGNAL(contextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(d->m_traceWindow->getEventList(), SIGNAL(error(QString)), this, SLOT(showErrorDialog(QString)));

    d->m_eventsView = new QmlProfilerEventsView(mw, d->m_traceWindow->getEventList());
    d->m_eventsView->setViewType(QmlProfilerEventsView::EventsView);

    connect(d->m_eventsView, SIGNAL(gotoSourceLocation(QString,int)),
            this, SLOT(gotoSourceLocation(QString,int)));
    connect(d->m_eventsView, SIGNAL(contextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    d->m_calleeView = new QmlProfilerEventsView(mw, d->m_traceWindow->getEventList());
    d->m_calleeView->setViewType(QmlProfilerEventsView::CalleesView);
    connect(d->m_calleeView, SIGNAL(gotoSourceLocation(QString,int)),
            this, SLOT(gotoSourceLocation(QString,int)));
    connect(d->m_calleeView, SIGNAL(contextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    d->m_callerView = new QmlProfilerEventsView(mw, d->m_traceWindow->getEventList());
    d->m_callerView->setViewType(QmlProfilerEventsView::CallersView);
    connect(d->m_callerView, SIGNAL(gotoSourceLocation(QString,int)),
            this, SLOT(gotoSourceLocation(QString,int)));
    connect(d->m_callerView, SIGNAL(contextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    QDockWidget *eventsDock = AnalyzerManager::createDockWidget
            (this, tr("Events"), d->m_eventsView, Qt::BottomDockWidgetArea);
    QDockWidget *timelineDock = AnalyzerManager::createDockWidget
            (this, tr("Timeline"), d->m_traceWindow, Qt::BottomDockWidgetArea);
    QDockWidget *calleeDock = AnalyzerManager::createDockWidget
            (this, tr("Callees"), d->m_calleeView, Qt::BottomDockWidgetArea);
    QDockWidget *callerDock = AnalyzerManager::createDockWidget
            (this, tr("Callers"), d->m_callerView, Qt::BottomDockWidgetArea);

    eventsDock->show();
    timelineDock->show();
    calleeDock->show();
    callerDock->show();

    mw->splitDockWidget(mw->toolBarDockWidget(), eventsDock, Qt::Vertical);
    mw->tabifyDockWidget(eventsDock, timelineDock);
    mw->tabifyDockWidget(timelineDock, calleeDock);
    mw->tabifyDockWidget(calleeDock, callerDock);

    //
    // Toolbar
    //
    QWidget *toolbarWidget = new QWidget;
    toolbarWidget->setObjectName(QLatin1String("QmlProfilerToolBarWidget"));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    d->m_recordButton = new QToolButton(toolbarWidget);
    // icon and tooltip set in setRecording(), called later
    d->m_recordButton->setCheckable(true);

    connect(d->m_recordButton,SIGNAL(toggled(bool)), this, SLOT(setRecording(bool)));
    d->m_recordButton->setChecked(true);
    layout->addWidget(d->m_recordButton);

    d->m_clearButton = new QToolButton(toolbarWidget);
    d->m_clearButton->setIcon(QIcon(QLatin1String(":/qmlprofiler/clean_pane_small.png")));
    d->m_clearButton->setToolTip(tr("Discard data"));
    connect(d->m_clearButton,SIGNAL(clicked()), this, SLOT(clearDisplay()));
    layout->addWidget(d->m_clearButton);

    QLabel *timeLabel = new QLabel(tr("Elapsed:      0 s"));
    QPalette palette = timeLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    timeLabel->setPalette(palette);
    timeLabel->setIndent(10);
    connect(d->m_traceWindow, SIGNAL(viewUpdated()), this, SLOT(correctTimer()));
    connect(this, SIGNAL(setTimeLabel(QString)), timeLabel, SLOT(setText(QString)));
    correctTimer();
    layout->addWidget(timeLabel);

    toolbarWidget->setLayout(layout);

    return toolbarWidget;
}

void QmlProfilerTool::connectClient(int port)
{
    QTC_ASSERT(!d->m_client, return;)
    d->m_client = new QDeclarativeDebugConnection;
    d->m_traceWindow->reset(d->m_client);
    connect(d->m_client, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(connectionStateChanged()));
    d->m_connectionTimer.start();
    d->m_tcpPort = port;
}

void QmlProfilerTool::connectToClient()
{
    if (!d->m_client || d->m_client->state() != QAbstractSocket::UnconnectedState)
        return;

    if (d->m_connectMode == QmlProfilerToolPrivate::TcpConnection) {
        logStatus(QString("QML Profiler: Connecting to %1:%2 ...").arg(d->m_tcpHost, QString::number(d->m_tcpPort)));
        d->m_client->connectToHost(d->m_tcpHost, d->m_tcpPort);
    } else {
        logStatus(QString("QML Profiler: Connecting to %1 ...").arg(d->m_tcpHost));
        d->m_client->connectToOst(d->m_ostDevice);
    }
}

void QmlProfilerTool::disconnectClient()
{
    // this might be actually be called indirectly by QDDConnectionPrivate::readyRead(), therefore allow
    // method to complete before deleting object
    if (d->m_client) {
        d->m_client->deleteLater();
        d->m_client = 0;
    }
}

void QmlProfilerTool::startRecording()
{
    if (d->m_client && d->m_client->isConnected()) {
        clearDisplay();
        d->m_traceWindow->setRecording(true);
    }
    emit fetchingData(true);
}

void QmlProfilerTool::stopRecording()
{
    d->m_traceWindow->setRecording(false);
    emit fetchingData(false);

    // manage early stop
    if (d->m_client && !d->m_client->isConnected() && d->m_appIsRunning)
        emit cancelRun();
}

void QmlProfilerTool::setRecording(bool recording)
{
    d->m_recordingEnabled = recording;

    // update record button
    d->m_recordButton->setToolTip( d->m_recordingEnabled ? tr("Disable profiling") : tr("Enable profiling"));
    d->m_recordButton->setIcon(QIcon(d->m_recordingEnabled ? QLatin1String(":/qmlprofiler/recordOn.png") :
                                                             QLatin1String(":/qmlprofiler/recordOff.png")));

    if (recording)
        startRecording();
    else
        stopRecording();
}

void QmlProfilerTool::setAppIsRunning()
{
    d->m_appIsRunning = true;
}

void QmlProfilerTool::setAppIsStopped()
{
    d->m_appIsRunning = false;
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileUrl, int lineNumber)
{
    if (lineNumber < 0 || fileUrl.isEmpty())
        return;

    const QString projectFileName = d->m_projectFinder.findFile(fileUrl);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(projectFileName);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(lineNumber);
        textEditor->widget()->setFocus();
    }
}

void QmlProfilerTool::correctTimer() {
    if (d->m_traceWindow->getEventList()->count() == 0)
        updateTimer(0);
}

void QmlProfilerTool::updateTimer(qreal elapsedSeconds)
{
    QString timeString = QString::number(elapsedSeconds,'f',1);
    timeString = QString("      ").left(6-timeString.length()) + timeString;
    emit setTimeLabel(tr("Elapsed: %1 s").arg(timeString));
}

void QmlProfilerTool::updateProjectFileList()
{
    d->m_projectFinder.setProjectFiles(
                d->m_project->files(Project::ExcludeGeneratedFiles));
}

void QmlProfilerTool::clearDisplay()
{
    d->m_traceWindow->clearDisplay();
    d->m_eventsView->clear();
    d->m_calleeView->clear();
    d->m_callerView->clear();
}

static void startRemoteTool(IAnalyzerTool *tool, StartMode mode)
{
    Q_UNUSED(tool);
    QmlProfilerAttachDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;

    AnalyzerStartParameters sp;
    sp.toolId = tool->id();
    sp.startMode = mode;
    sp.connParams.host = dialog.address();
    sp.connParams.port = dialog.port();

    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, 0);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));

    ProjectExplorerPlugin::instance()->startRunControl(rc, tool->id());
}

void QmlProfilerTool::tryToConnect()
{
    ++d->m_connectionAttempts;

    if (d->m_client && d->m_client->isConnected()) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
    } else if (d->m_connectionAttempts == 50) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;

        Core::ICore * const core = Core::ICore::instance();
        QMessageBox *infoBox = new QMessageBox(core->mainWindow());
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(tr("Qt Creator"));
        infoBox->setText(tr("Could not connect to the in-process QML profiler.\n"
                            "Do you want to retry?"));
        infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Retry);
        infoBox->setModal(true);

        connect(infoBox, SIGNAL(finished(int)),
                this, SLOT(retryMessageBoxFinished(int)));

        infoBox->show();
    } else {
        connectToClient();
    }
}

void QmlProfilerTool::connectionStateChanged()
{
    if (!d->m_client)
        return;
    switch (d->m_client->state()) {
    case QAbstractSocket::UnconnectedState:
    {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: disconnected");
        break;
    }
    case QAbstractSocket::HostLookupState:
        break;
    case QAbstractSocket::ConnectingState: {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: Connecting to debug server ...");
        break;
    }
    case QAbstractSocket::ConnectedState:
    {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: connected and running");
        updateRecordingState();
        break;
    }
    case QAbstractSocket::ClosingState:
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: closing ...");
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
}

void QmlProfilerTool::updateRecordingState()
{
    if (d->m_client->isConnected()) {
        d->m_traceWindow->setRecording(d->m_recordingEnabled);
    } else {
        d->m_traceWindow->setRecording(false);
    }

    if (d->m_traceWindow->isRecording())
        clearDisplay();
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
        pe->runProject(pro, id());
    } else if (mode == StartRemote) {
        startRemoteTool(this, mode);
    }
}

void QmlProfilerTool::logStatus(const QString &msg)
{
    Core::MessageManager *messageManager = Core::MessageManager::instance();
    messageManager->printToOutputPane(msg, false);
}

void QmlProfilerTool::logError(const QString &msg)
{
    // TODO: Rather show errors in the application ouput
    Core::MessageManager *messageManager = Core::MessageManager::instance();
    messageManager->printToOutputPane(msg, true);
}

void QmlProfilerTool::showSaveDialog()
{
    Core::ICore *core = Core::ICore::instance();
    QString filename = QFileDialog::getSaveFileName(core->mainWindow(), tr("Save QML Trace"), QString(), tr("QML traces (%1)").arg(TraceFileExtension));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QLatin1String(TraceFileExtension)))
            filename += QLatin1String(TraceFileExtension);
        d->m_traceWindow->getEventList()->save(filename);
    }
}

void QmlProfilerTool::showLoadDialog()
{
    Core::ICore *core = Core::ICore::instance();
    QString filename = QFileDialog::getOpenFileName(core->mainWindow(), tr("Load QML Trace"), QString(), tr("QML traces (%1)").arg(TraceFileExtension));

    if (!filename.isEmpty()) {
        // delayed load (prevent graphical artifacts due to long load time)
        d->m_traceWindow->getEventList()->setFilename(filename);
        QTimer::singleShot(100, d->m_traceWindow->getEventList(), SLOT(load()));
    }
}

void QmlProfilerTool::showErrorDialog(const QString &error)
{
    Core::ICore *core = Core::ICore::instance();
    QMessageBox *errorDialog = new QMessageBox(core->mainWindow());
    errorDialog->setIcon(QMessageBox::Warning);
    errorDialog->setWindowTitle(tr("QML Profiler"));
    errorDialog->setText(error);
    errorDialog->setStandardButtons(QMessageBox::Ok);
    errorDialog->setDefaultButton(QMessageBox::Ok);
    errorDialog->setModal(false);
    errorDialog->show();
}

void QmlProfilerTool::retryMessageBoxFinished(int result)
{
    switch (result) {
    case QMessageBox::Retry: {
        d->m_connectionAttempts = 0;
        d->m_connectionTimer.start();
        break;
    }
    case QMessageBox::Help: {
        Core::HelpManager *helpManager = Core::HelpManager::instance();
        helpManager->handleHelpRequest("qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html");
        // fall through
    }
    default: {
        if (d->m_client) {
            logStatus("QML Profiler: Failed to connect! " + d->m_client->errorString());
        } else {
            logStatus("QML Profiler: Failed to connect!");
        }

        emit connectionFailed();
        break;
    }
    }
}
