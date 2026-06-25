// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertool.h"

#include "flamegraphview.h"
#include "profilertr.h"
#include "qmlprofilerattachdialog.h"
#include "qmlprofilerclientmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilersettings.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilerstatewidget.h"
#include "qmlprofilerstatisticsview.h"
#include "qmlprofilertraceview.h"
#include "quick3dframeview.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/perspective.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <texteditor/texteditor.h>

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinenotesmodel.h>

#include <utils/fancymainwindow.h>
#include <utils/shutdownguard.h>
#include <utils/fileinprojectfinder.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFileDialog>
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
using namespace QmlDebug;
using namespace Profiler::Constants;
using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

static QmlProfilerTool *m_instance = nullptr;

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerStateManager m_profilerState;
    QmlProfilerClientManager m_profilerConnections;
    QmlProfilerModelManager m_profilerModelManager;

    Perspective m_perspective{Constants::QmlProfilerPerspectiveId, Tr::tr("QML Profiler")};
    QmlProfilerTraceView m_traceView{&m_profilerModelManager};
    QmlProfilerStatisticsView m_statisticsView{&m_profilerModelManager};
    FlameGraphView m_flameGraphView{&m_profilerModelManager};
    Quick3DFrameView m_quick3dView{&m_profilerModelManager};

    void selectByTypeId(int typeId)
    {
        m_traceView.selectByTypeId(typeId);
        m_statisticsView.selectByTypeId(typeId);
        m_flameGraphView.selectByTypeId(typeId);
        m_quick3dView.selectByTypeId(typeId);
    }

    QToolButton m_recordButton;
    QMenu m_recordFeaturesMenu;

    QAction m_startAction;
    QAction m_stopAction;
    QToolButton m_clearButton;

    // open search
    QToolButton m_searchButton;

    // hide and show categories
    QToolButton m_displayFeaturesButton;
    QMenu m_displayFeaturesMenu;

    // elapsed time display
    QLabel m_timeLabel;
    QTimer m_recordingTimer;
    QElapsedTimer m_recordingElapsedTime;

    Core::ActionContainer *m_options = nullptr;
    QAction m_loadQmlTrace;
    QAction m_saveQmlTrace;
    QAction m_runAction;
    QAction m_attachAction;
};

QmlProfilerTool::QmlProfilerTool()
    : d(new QmlProfilerToolPrivate)
{
    m_instance = this;
    setObjectName(QLatin1String("QmlProfilerTool"));

    connect(&d->m_traceView, &QmlProfilerTraceView::typeSelected,
            this, [this](int typeId) { d->selectByTypeId(typeId); });

    // Route the flame graph's details into the shared range details view.
    connect(&d->m_flameGraphView, &FlameGraphView::detailsChanged,
            d->m_traceView.rangeDetailsWidget(), &Timeline::RangeDetailsWidget::setData);
    connect(&d->m_flameGraphView, &FlameGraphView::detailsCleared,
            d->m_traceView.rangeDetailsWidget(), &Timeline::RangeDetailsWidget::clear);

    new QmlProfilerStateWidget(&d->m_profilerState, &d->m_profilerModelManager,
                               &d->m_traceView);

    auto prepareEventsView = [this](QmlProfilerEventsView *view) {
        connect(view, &QmlProfilerEventsView::typeSelected,
                this, [this](int typeId) { d->selectByTypeId(typeId); });
        connect(&d->m_profilerModelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
                view, &QmlProfilerEventsView::onVisibleFeaturesChanged);
        connect(view, &QmlProfilerEventsView::showFullRange,
                this, [this] { d->m_profilerModelManager.restrictToRange(-1, -1); });
        new QmlProfilerStateWidget(&d->m_profilerState, &d->m_profilerModelManager, view);
    };
    prepareEventsView(&d->m_statisticsView);
    prepareEventsView(&d->m_flameGraphView);
    prepareEventsView(&d->m_quick3dView);

    d->m_perspective.setObjectName("QML Profiler View Manager");
    d->m_perspective.addWindow(&d->m_traceView, Perspective::SplitVertical, nullptr);
    // Split the details off the trace view before tabbing the other views onto it.
    // QMainWindow::splitDockWidget() only splits when the anchor is not yet tabbed;
    // doing this later would just add the details as another tab.
    d->m_perspective.addWindow(d->m_traceView.rangeDetailsWidget(),
                               Perspective::SplitHorizontal, &d->m_traceView);
    d->m_perspective.addWindow(&d->m_flameGraphView, Perspective::AddToTab, &d->m_traceView);
    d->m_perspective.addWindow(&d->m_quick3dView, Perspective::AddToTab, &d->m_flameGraphView);
    d->m_perspective.addWindow(&d->m_statisticsView, Perspective::AddToTab, &d->m_traceView);
    d->m_perspective.addWindow(&d->m_traceView, Perspective::Raise, nullptr);

    connect(&d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerTool::profilerStateChanged);
    connect(&d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerTool::updateRunActions);
    connect(&d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerTool::serverRecordingChanged);
    connect(&d->m_profilerState, &QmlProfilerStateManager::recordedFeaturesChanged,
            this, &QmlProfilerTool::setRecordedFeatures);

    d->m_profilerConnections.setModelManager(&d->m_profilerModelManager);
    d->m_profilerConnections.setProfilerStateManager(&d->m_profilerState);
    connect(&d->m_profilerConnections, &QmlProfilerClientManager::connectionClosed,
            this, &QmlProfilerTool::clientsDisconnected);

    d->m_profilerModelManager.registerFeatures(0,
                                                std::bind(&QmlProfilerTool::initialize, this),
                                                std::bind(&QmlProfilerTool::finalize, this),
                                                std::bind(&QmlProfilerTool::clear, this));

    connect(&d->m_profilerModelManager, &QmlProfilerModelManager::error,
            this, &QmlProfilerTool::showErrorDialog);
    connect(&d->m_profilerModelManager, &QmlProfilerModelManager::availableFeaturesChanged,
            this, &QmlProfilerTool::setAvailableFeatures);
    connect(&d->m_profilerModelManager, &QmlProfilerModelManager::saveFinished,
            this, &QmlProfilerTool::onLoadSaveFinished);
    connect(&d->m_profilerModelManager, &QmlProfilerModelManager::loadFinished,
            this, &QmlProfilerTool::onLoadSaveFinished);

    d->m_recordingTimer.setInterval(100);
    connect(&d->m_recordingTimer, &QTimer::timeout, this, &QmlProfilerTool::updateTimeDisplay);
    connect(&d->m_traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerTool::gotoSourceLocation);
    connect(&d->m_statisticsView, &QmlProfilerEventsView::gotoSourceLocation,
            this, &QmlProfilerTool::gotoSourceLocation);
    connect(&d->m_flameGraphView, &QmlProfilerEventsView::gotoSourceLocation,
            this, &QmlProfilerTool::gotoSourceLocation);
    connect(&d->m_quick3dView, &QmlProfilerEventsView::gotoSourceLocation,
            this, &QmlProfilerTool::gotoSourceLocation);

    //
    // Toolbar
    //
    d->m_recordButton.setCheckable(true);
    connect(&d->m_recordButton, &QAbstractButton::clicked,
            this, &QmlProfilerTool::recordingButtonChanged);
    d->m_recordButton.setChecked(true);
    d->m_recordButton.setMenu(&d->m_recordFeaturesMenu);
    d->m_recordButton.setPopupMode(QToolButton::MenuButtonPopup);
    connect(&d->m_recordFeaturesMenu, &QMenu::triggered,
            this, &QmlProfilerTool::toggleRequestedFeature);

    d->m_clearButton.setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    d->m_clearButton.setToolTip(Tr::tr("Discard data"));
    connect(&d->m_clearButton, &QAbstractButton::clicked, [this] {
        if (checkForUnsavedNotes())
            clearData();
    });

    d->m_searchButton.setIcon(Utils::Icons::ZOOM_TOOLBAR.icon());
    d->m_searchButton.setToolTip(Tr::tr("Search timeline event notes."));
    d->m_searchButton.setEnabled(false);
    connect(&d->m_searchButton, &QToolButton::clicked, this, &QmlProfilerTool::showTimeLineSearch);

    d->m_displayFeaturesButton.setIcon(Utils::Icons::FILTER.icon());
    d->m_displayFeaturesButton.setToolTip(Tr::tr("Hide or show event categories."));
    d->m_displayFeaturesButton.setPopupMode(QToolButton::InstantPopup);
    d->m_displayFeaturesButton.setProperty(StyleHelper::C_NO_ARROW, true);
    d->m_displayFeaturesButton.setMenu(&d->m_displayFeaturesMenu);
    connect(&d->m_displayFeaturesMenu, &QMenu::triggered,
            this, &QmlProfilerTool::toggleVisibleFeature);

    StyleHelper::setPanelWidget(&d->m_timeLabel);
    d->m_timeLabel.setIndent(10);
    updateTimeDisplay();

    setAvailableFeatures(d->m_profilerModelManager.availableFeatures());
    setRecordedFeatures(0);

    // When the widgets are requested we assume that the session data
    // is available, then we can populate the file finder
    d->m_profilerModelManager.populateFileFinder();

    d->m_startAction.setText(Tr::tr("Start"));
    d->m_startAction.setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR.icon());

    d->m_stopAction.setText(Tr::tr("Stop"));
    d->m_stopAction.setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());

    QObject::connect(&d->m_startAction, &QAction::triggered, this, &QmlProfilerTool::profileStartupProject);

    Perspective *perspective = &d->m_perspective;
    perspective->addToolBarAction(&d->m_startAction);
    perspective->addToolBarAction(&d->m_stopAction);
    perspective->addToolBarWidget(&d->m_recordButton);
    perspective->addToolBarWidget(&d->m_clearButton);
    perspective->addToolBarWidget(&d->m_searchButton);
    perspective->addToolBarWidget(&d->m_displayFeaturesButton);
    perspective->addToolBarWidget(&d->m_timeLabel);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &QmlProfilerTool::updateRunActions);

    if (EditorManager *editorManager = EditorManager::instance()) {
        connect(editorManager, &EditorManager::editorCreated,
                this, [this](Core::IEditor *editor, const FilePath &filePath) {
            Q_UNUSED(editor)
            d->m_statisticsView.createMarks(filePath.toUrlishString());
        });
    }

    auto updateRecordButton = [this] {
        const bool recording =
                d->m_profilerState.currentState() != QmlProfilerStateManager::AppRunning
                ? d->m_profilerState.clientRecording() : d->m_profilerState.serverRecording();

        const static QIcon recordOn = ProjectExplorer::Icons::RECORD_ON.icon();
        const static QIcon recordOff = ProjectExplorer::Icons::RECORD_OFF.icon();

        // update display
        d->m_recordButton.setToolTip(recording ? Tr::tr("Disable Profiling") : Tr::tr("Enable Profiling"));
        d->m_recordButton.setIcon(recording ? recordOn : recordOff);
        d->m_recordButton.setChecked(recording);
    };

    connect(&d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            &d->m_recordButton, updateRecordButton);
    connect(&d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            &d->m_recordButton, updateRecordButton);
    connect(&d->m_profilerState, &QmlProfilerStateManager::clientRecordingChanged,
            &d->m_recordButton, updateRecordButton);
    updateRecordButton();


    const QString description = Tr::tr("The QML Profiler can be used to find performance "
                                       "bottlenecks in applications using QML.");

    d->m_runAction.setText(Tr::tr("QML Profiler"));
    d->m_runAction.setToolTip(description);
    QObject::connect(&d->m_runAction, &QAction::triggered,
                     this, &QmlProfilerTool::profileStartupProject);

    QObject::connect(&d->m_startAction, &QAction::changed, this, [this] {
        d->m_runAction.setEnabled(d->m_startAction.isEnabled());
    });

    d->m_attachAction.setText(Tr::tr("QML Profiler (Attach to Waiting Application)"));
    d->m_attachAction.setToolTip(description);
    QObject::connect(&d->m_attachAction, &QAction::triggered,
                     this, &QmlProfilerTool::attachToWaitingApplication);

    d->m_loadQmlTrace.setText(Tr::tr("Load QML Trace"));
    connect(&d->m_loadQmlTrace, &QAction::triggered,
            this, &QmlProfilerTool::showLoadDialog, Qt::QueuedConnection);

    d->m_saveQmlTrace.setText(Tr::tr("Save QML Trace"));
    connect(&d->m_saveQmlTrace, &QAction::triggered,
            this, &QmlProfilerTool::showSaveDialog, Qt::QueuedConnection);

    connect(&d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, [this](bool recording) {
        d->m_loadQmlTrace.setEnabled(!recording);
    });
    d->m_loadQmlTrace.setEnabled(!d->m_profilerState.serverRecording());

    connect(&d->m_profilerModelManager, &QmlProfilerModelManager::traceChanged,
            this, [this] {
        d->m_saveQmlTrace.setEnabled(!d->m_profilerModelManager.isEmpty());
    });
    d->m_saveQmlTrace.setEnabled(!d->m_profilerModelManager.isEmpty());

    d->m_options = ActionManager::createMenu("Analyzer.Menu.QMLOptions");
    d->m_options->menu()->setTitle(Tr::tr("QML Profiler Options"));
    d->m_options->menu()->setEnabled(true);
    ActionContainer *menu = ActionManager::actionContainer(M_DEBUG_ANALYZER);

    menu->addAction(ActionManager::registerAction(&d->m_runAction,
                                                  "QmlProfiler.Internal"),
                    Core::Constants::G_ANALYZER_TOOLS);
    menu->addAction(ActionManager::registerAction(&d->m_attachAction,
                                                  "QmlProfiler.AttachToWaitingApplication"),
                    Core::Constants::G_ANALYZER_REMOTE_TOOLS);

    menu->addMenu(d->m_options, G_ANALYZER_OPTIONS);
    d->m_options->addAction(ActionManager::registerAction(&d->m_loadQmlTrace,
                                                       Constants::QmlProfilerLoadActionId));
    d->m_options->addAction(ActionManager::registerAction(&d->m_saveQmlTrace,
                                                       Constants::QmlProfilerSaveActionId));
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d;
    m_instance = nullptr;
}

QmlProfilerTool *QmlProfilerTool::instance()
{
    return m_instance;
}

void QmlProfilerTool::updateRunActions()
{
    if (d->m_profilerState.currentState() != QmlProfilerStateManager::Idle) {
        d->m_startAction.setEnabled(false);
        d->m_startAction.setToolTip(Tr::tr("A QML Profiler analysis is still in progress."));
        d->m_stopAction.setEnabled(true);
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(
            ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        d->m_startAction.setToolTip(canRun ? Tr::tr("Start QML Profiler analysis.") : canRun.error());
        d->m_startAction.setEnabled(canRun.has_value());
        d->m_stopAction.setEnabled(false);
    }
}

void QmlProfilerTool::finalizeRunControl(RunControl *runControl)
{
    if (auto aspect = runControl->aspectData<QmlProfilerRunConfigurationAspect>()) {
        if (auto settings = static_cast<const QmlProfilerSettings *>(aspect->currentSettings)) {
            d->m_profilerConnections.setFlushInterval(settings->flushEnabled() ?
                                                           settings->flushInterval() : 0);
            d->m_profilerModelManager.setAggregateTraces(settings->aggregateTraces());
        }
    }

    connect(&d->m_stopAction, &QAction::triggered, runControl, &RunControl::initiateStop);

    updateRunActions();

    d->m_profilerModelManager.populateFileFinder(runControl->buildConfiguration());
    d->m_profilerState.setCurrentState(QmlProfilerStateManager::AppRunning);
}

void QmlProfilerTool::handleStop()
{
    updateRunActions();
    disconnect(&d->m_stopAction, &QAction::triggered, nullptr, nullptr);

    // If we're still trying to connect, stop now.
    if (d->m_profilerConnections.isConnecting()) {
        showNonmodalWarning(Tr::tr("The application finished before a connection could be "
                                   "established. No data was loaded."));
    }
    d->m_profilerConnections.disconnectFromServer();
}

void QmlProfilerTool::recordingButtonChanged(bool recording)
{
    // clientRecording is our intention for new sessions. That may differ from the state of the
    // current session, as indicated by the button. To synchronize it, toggle once.
    if (recording && d->m_profilerState.currentState() == QmlProfilerStateManager::AppRunning) {
        if (checkForUnsavedNotes()) {
            if (!d->m_profilerModelManager.aggregateTraces())
                clearEvents(); // clear before the recording starts, unless we aggregate recordings
            if (d->m_profilerState.clientRecording())
                d->m_profilerState.setClientRecording(false);
            d->m_profilerState.setClientRecording(true);
        } else {
            d->m_recordButton.setChecked(false);
        }
    } else {
        if (d->m_profilerState.clientRecording() == recording)
            d->m_profilerState.setClientRecording(!recording);
        d->m_profilerState.setClientRecording(recording);
    }
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber)
{
    if (lineNumber < 0 || fileUrl.isEmpty())
        return;

    const auto projectFileName = d->m_profilerModelManager.findLocalFile(fileUrl);

    if (!projectFileName.exists() || !projectFileName.isReadableFile())
        return;

    // The text editors count columns starting with 0, but the ASTs store the
    // location starting with 1, therefore the -1.
    EditorManager::openEditorAt({projectFileName,
                                 lineNumber == 0 ? 1 : lineNumber,
                                 columnNumber - 1},
                                Id(),
                                EditorManager::DoNotSwitchToDesignMode
                                    | EditorManager::DoNotSwitchToEditMode);
}

void QmlProfilerTool::updateTimeDisplay()
{
    double seconds = 0;
    switch (d->m_profilerState.currentState()) {
    case QmlProfilerStateManager::AppStopRequested:
    case QmlProfilerStateManager::AppDying:
        return; // Transitional state: don't update the display.
    case QmlProfilerStateManager::AppRunning:
        if (d->m_profilerState.serverRecording()) {
            seconds = d->m_recordingElapsedTime.elapsed() / 1000.0;
            break;
        }
        Q_FALLTHROUGH();
    case QmlProfilerStateManager::Idle:
        if (d->m_profilerModelManager.traceDuration() > 0)
            seconds = d->m_profilerModelManager.traceDuration() / 1.0e9;
        break;
    }
    QString timeString = QString::number(seconds,'f',1);
    QString profilerTimeStr = Tr::tr("%1 s").arg(timeString, 6);
    d->m_timeLabel.setText(Tr::tr("Elapsed: %1").arg(profilerTimeStr));
}

void QmlProfilerTool::showTimeLineSearch()
{
    QTC_ASSERT(qobject_cast<QDockWidget *>(d->m_traceView.parentWidget()), return);
    d->m_traceView.parentWidget()->raise();
    d->m_traceView.setFocus();
    Core::Find::openFindToolBar(Core::Find::FindForwardDirection);
}

void QmlProfilerTool::clearEvents()
{
    d->m_profilerModelManager.clear();
    d->m_profilerConnections.clearEvents();
    setRecordedFeatures(0);
}

void QmlProfilerTool::clearData()
{
    d->m_profilerModelManager.clearAll();
    d->m_profilerConnections.clearBufferedData();
    setRecordedFeatures(0);
}

void QmlProfilerTool::clearDisplay()
{
    d->m_profilerConnections.clearBufferedData();
    d->m_traceView.clear();
    updateTimeDisplay();
}

void QmlProfilerTool::setButtonsEnabled(bool enable)
{
    d->m_clearButton.setEnabled(enable);
    d->m_displayFeaturesButton.setEnabled(enable);
    d->m_searchButton.setEnabled(enable);
    d->m_recordFeaturesMenu.setEnabled(enable);
}

void QmlProfilerTool::createInitialTextMarks()
{
    QmlProfilerStatisticsView *statsView = &d->m_statisticsView;
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents)
        statsView->createMarks(document->filePath().toUrlishString());
}

bool QmlProfilerTool::prepareTool()
{
    if (d->m_profilerState.clientRecording()) {
        if (checkForUnsavedNotes()) {
            clearData(); // clear right away to suppress second warning on server recording change
            return true;
        } else {
            return false;
        }
    }
    return true;
}

RunControl *QmlProfilerTool::attachToWaitingApplication()
{
    if (!prepareTool())
        return nullptr;

    Id kitId;
    int port;
    Kit *kit = nullptr;

    {
        QtcSettings *settings = ICore::settings();

        kitId = Id::fromSetting(settings->value("AnalyzerQmlAttachDialog/kitId"));
        port = settings->value("AnalyzerQmlAttachDialog/port", 3768).toInt();

        QmlProfilerAttachDialog dialog;

        dialog.setKitId(kitId);
        dialog.setPort(port);

        if (dialog.exec() != QDialog::Accepted)
            return nullptr;

        kit = dialog.kit();
        port = dialog.port();
        QTC_ASSERT(port >= 0, return nullptr);
        QTC_ASSERT(port <= std::numeric_limits<quint16>::max(), return nullptr);

        settings->setValue("AnalyzerQmlAttachDialog/kitId", kit->id().toSetting());
        settings->setValue("AnalyzerQmlAttachDialog/port", port);
    }

    QUrl serverUrl;

    IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
    QTC_ASSERT(device, return nullptr);
    QUrl toolControl = device->toolControlChannel(IDevice::QmlControlChannel);
    serverUrl.setScheme(Utils::urlTcpScheme());
    serverUrl.setHost(toolControl.host());
    serverUrl.setPort(port);

    d->m_perspective.select();

    auto runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    RunConfiguration *activeRunConfig = activeRunConfigForActiveProject();
    if (activeRunConfig && activeRunConfig->kit() == kit)
        runControl->copyDataFromRunConfiguration(activeRunConfig);
    else
        runControl->setKit(kit);
    runControl->setQmlChannel(serverUrl);
    runControl->setRunRecipe(qmlProfilerRecipe(runControl));

    connect(&d->m_profilerConnections, &QmlProfilerClientManager::connectionClosed,
            runControl, &RunControl::initiateStop);
    runControl->start();
    return runControl;
}

void QmlProfilerTool::logState(const QString &msg)
{
    MessageManager::writeFlashing(msg);
}

void QmlProfilerTool::showErrorDialog(const QString &error)
{
    auto errorDialog = new QMessageBox(ICore::dialogParent());
    errorDialog->setIcon(QMessageBox::Warning);
    errorDialog->setWindowTitle(Tr::tr("QML Profiler"));
    errorDialog->setText(error);
    errorDialog->setStandardButtons(QMessageBox::Ok);
    errorDialog->setDefaultButton(QMessageBox::Ok);
    errorDialog->setModal(false);
    errorDialog->show();
}

static void saveLastTraceFile(const FilePath &filePath)
{
    QmlProfilerSettings &s = globalSettings();
    if (filePath != s.lastTraceFile()) {
        s.lastTraceFile.setValue(filePath);
        s.writeSettings();
    }
}

QString QmlProfilerTool::fileDialogTraceFilesFilter()
{
    QString qmlTraceFiles = Tr::tr("QML traces (*%1 *%2)")
                                .arg(QtdFileExtension).arg(QztFileExtension);
    return qmlTraceFiles.append(";;").append(Core::DocumentManager::allFilesFilterString());
}

void QmlProfilerTool::showSaveDialog()
{
    QLatin1String tFile(QtdFileExtension);
    QLatin1String zFile(QztFileExtension);

    FilePath filePath = FileUtils::getSaveFilePath(
                Tr::tr("Save QML Trace"),
                globalSettings().lastTraceFile(),
                fileDialogTraceFilesFilter());
    if (!filePath.isEmpty()) {
        if (!filePath.endsWith(zFile) && !filePath.endsWith(tFile))
            filePath = filePath.stringAppended(zFile);
        saveLastTraceFile(filePath);
        PerspectivesView::enableMainWindow(false);
        Core::ProgressManager::addTask(d->m_profilerModelManager.save(filePath.toUrlishString()),
                                       Tr::tr("Saving Trace Data"), TASK_SAVE,
                                       Core::ProgressManager::ShowInApplicationIcon);
    }
}

void QmlProfilerTool::loadFile(const FilePath &filePath)
{
    saveLastTraceFile(filePath);
    d->m_perspective.select();
    PerspectivesView::enableMainWindow(false);
    connect(&d->m_profilerModelManager, &QmlProfilerModelManager::recordedFeaturesChanged,
            this, &QmlProfilerTool::setRecordedFeatures);
    d->m_profilerModelManager.populateFileFinder();
    Core::ProgressManager::addTask(d->m_profilerModelManager.load(filePath.toUrlishString()),
                                   Tr::tr("Loading Trace Data"), TASK_LOAD);
}

void QmlProfilerTool::showLoadDialog()
{
    if (!checkForUnsavedNotes())
        return;

    FilePath filePath = FileUtils::getOpenFilePath(
                Tr::tr("Load QML Trace"),
                globalSettings().lastTraceFile(),
                fileDialogTraceFilesFilter());

    if (!filePath.isEmpty())
        loadFile(filePath);
}

void QmlProfilerTool::profileStartupProject()
{
    if (!prepareTool())
        return;
    d->m_perspective.select();
    ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
}

void QmlProfilerTool::onLoadSaveFinished()
{
    disconnect(&d->m_profilerModelManager, &QmlProfilerModelManager::recordedFeaturesChanged,
               this, &QmlProfilerTool::setRecordedFeatures);
    PerspectivesView::enableMainWindow(true);
}

/*!
    Checks if we have unsaved notes. If so, shows a warning dialog. Returns true if we can continue
    with a potentially destructive operation and discard the warnings, or false if not. We don't
    want to show a save/discard dialog here because that will often result in a confusing series of
    different dialogs: first "save" and then immediately "load" or "connect".
 */
bool QmlProfilerTool::checkForUnsavedNotes()
{
    if (!d->m_profilerModelManager.notesModel()->isModified())
        return true;
    return QMessageBox::warning(QApplication::activeWindow(), Tr::tr("QML Profiler"),
                                Tr::tr("You are about to discard the profiling data, including unsaved "
                                   "notes. Do you want to continue?"),
                                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}

void QmlProfilerTool::clientsDisconnected()
{
    if (d->m_profilerState.currentState() != QmlProfilerStateManager::Idle) {
        if (d->m_profilerModelManager.aggregateTraces()) {
            d->m_profilerModelManager.finalize();
        } else if (d->m_profilerState.serverRecording()) {
            // If the application stopped by itself, check if we have all the data
            if (d->m_profilerState.currentState() != QmlProfilerStateManager::AppStopRequested) {
                showNonmodalWarning(Tr::tr("Application finished before loading profiled data.\n"
                                       "Please use the stop button instead."));
                // The application went away while still recording, so no more data is
                // coming. Finalize whatever was received so it gets displayed and the
                // "Profiling application" overlay is dismissed.
                d->m_profilerModelManager.finalize();
            }
        }
    }

    // ... and return to the "base" state
    if (d->m_profilerState.currentState() == QmlProfilerStateManager::AppDying) {
        QTimer::singleShot(0, &d->m_profilerState, [this] {
            d->m_profilerState.setCurrentState(QmlProfilerStateManager::Idle);
        });
    }
}

void addFeatureToMenu(QMenu *menu, ProfileFeature feature, quint64 enabledFeatures)
{
    QAction *action =
            menu->addAction(Tr::tr(QmlProfilerModelManager::featureName(feature)));
    action->setCheckable(true);
    action->setData(static_cast<uint>(feature));
    action->setChecked(enabledFeatures & (1ULL << (feature)));
}

void QmlProfilerTool::setAvailableFeatures(quint64 features)
{
    if (features != d->m_profilerState.requestedFeatures())
        d->m_profilerState.setRequestedFeatures(features); // by default, enable them all.
    d->m_recordFeaturesMenu.clear();
    d->m_displayFeaturesMenu.clear();
    for (int feature = 0; feature < MaximumProfileFeature; ++feature) {
        if (features & (1ULL << feature)) {
            addFeatureToMenu(&d->m_recordFeaturesMenu, ProfileFeature(feature),
                             d->m_profilerState.requestedFeatures());
            addFeatureToMenu(&d->m_displayFeaturesMenu, ProfileFeature(feature),
                             d->m_profilerModelManager.visibleFeatures());
        }
    }
}

void QmlProfilerTool::setRecordedFeatures(quint64 features)
{
    const QList<QAction *> actions = d->m_displayFeaturesMenu.actions();
    for (QAction *action : actions)
        action->setEnabled(features & (1ULL << action->data().toUInt()));
}

void QmlProfilerTool::initialize()
{
    setButtonsEnabled(false);            // Other buttons disabled
}

void QmlProfilerTool::finalize()
{
    updateTimeDisplay();
    createInitialTextMarks();
    setButtonsEnabled(true);
    d->m_recordButton.setEnabled(true);
}

void QmlProfilerTool::clear()
{
    clearDisplay();
    setButtonsEnabled(true);
    d->m_recordButton.setEnabled(true);
}

QList <QAction *> QmlProfilerTool::profilerContextMenuActions()
{
    // In case of QmlProfilerTool being used outside of Qt Creator, like with the stand alone
    // Qt Profiler, ActionManager does not get initialized. Avoid crash.
    if (!ActionManager::instance())
        return {};

    QList <QAction *> commonActions;

    if (Command *command = ActionManager::command(Constants::QmlProfilerLoadActionId))
        commonActions << command->action();
    if (Command *command = ActionManager::command(Constants::QmlProfilerSaveActionId))
        commonActions << command->action();

    return commonActions;
}

void QmlProfilerTool::showNonmodalWarning(const QString &warningMsg)
{
    auto noExecWarning = new QMessageBox(ICore::dialogParent());
    noExecWarning->setIcon(QMessageBox::Warning);
    noExecWarning->setWindowTitle(Tr::tr("QML Profiler"));
    noExecWarning->setText(warningMsg);
    noExecWarning->setStandardButtons(QMessageBox::Ok);
    noExecWarning->setDefaultButton(QMessageBox::Ok);
    noExecWarning->setModal(false);
    noExecWarning->show();
}

QmlProfilerClientManager *QmlProfilerTool::clientManager()
{
    return &d->m_profilerConnections;
}

QmlProfilerModelManager *QmlProfilerTool::modelManager()
{
    return &d->m_profilerModelManager;
}

QmlProfilerStateManager *QmlProfilerTool::stateManager()
{
    return &d->m_profilerState;
}

void QmlProfilerTool::profilerStateChanged()
{
    switch (d->m_profilerState.currentState()) {
    case QmlProfilerStateManager::AppDying : {
        // If already disconnected when dying, check again that all data was read
        if (!d->m_profilerConnections.isConnected())
            clientsDisconnected();
        break;
    }
    case QmlProfilerStateManager::Idle :
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Don't allow toggling the recording while data is loaded when application quits
        if (d->m_profilerState.serverRecording()) {
            // Turn off recording and wait for remaining data
            d->m_profilerConnections.stopRecording();
        } else {
            // Directly transition to idle
            QTimer::singleShot(0, &d->m_profilerState, [this] {
                d->m_profilerState.setCurrentState(QmlProfilerStateManager::Idle);
            });
        }
        break;
    default:
        // no special action needed for other states
        break;
    }
}

void QmlProfilerTool::serverRecordingChanged()
{
    if (d->m_profilerState.currentState() == QmlProfilerStateManager::AppRunning) {
        // clear the old data each time we start a new profiling session
        if (d->m_profilerState.serverRecording()) {
            // We cannot stop it here, so we cannot give the usual yes/no dialog. Show a dialog
            // offering to immediately save the data instead.
            if (d->m_profilerModelManager.notesModel()->isModified() &&
                    QMessageBox::warning(QApplication::activeWindow(), Tr::tr("QML Profiler"),
                                         Tr::tr("Starting a new profiling session will discard the "
                                            "previous data, including unsaved notes.\nDo you want "
                                            "to save the data first?"),
                                         QMessageBox::Save, QMessageBox::Discard) ==
                    QMessageBox::Save)
                showSaveDialog();

            d->m_recordingTimer.start();
            d->m_recordingElapsedTime.start();
            if (!d->m_profilerModelManager.aggregateTraces())
                clearEvents();
            d->m_profilerModelManager.initialize();
        } else {
            d->m_recordingTimer.stop();
            if (!d->m_profilerModelManager.aggregateTraces())
                d->m_profilerModelManager.finalize();
        }
    } else if (d->m_profilerState.currentState() == QmlProfilerStateManager::AppStopRequested) {
        d->m_profilerModelManager.finalize();
        d->m_profilerState.setCurrentState(QmlProfilerStateManager::Idle);
    }
}

void QmlProfilerTool::toggleRequestedFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerState.setRequestedFeatures(
                    d->m_profilerState.requestedFeatures() | (1ULL << feature));
    else
        d->m_profilerState.setRequestedFeatures(
                    d->m_profilerState.requestedFeatures() & (~(1ULL << feature)));
}

void QmlProfilerTool::toggleVisibleFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerModelManager.setVisibleFeatures(
                    d->m_profilerModelManager.visibleFeatures() | (1ULL << feature));
    else
        d->m_profilerModelManager.setVisibleFeatures(
            d->m_profilerModelManager.visibleFeatures() & (~(1ULL << feature)));
}

void setupQmlProfilerTool()
{
    static GuardedObject<QmlProfilerTool> theQmlProfilerTool;
}

} // namespace Profiler::Internal
