// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertool.h"

#include "perfloaddialog.h"
#include "perfprofilertr.h"
#include "perfsettings.h"
#include "perftracepointdialog.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>

#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/debuggericons.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace Debugger;
using namespace PerfProfiler::Constants;
using namespace Utils;
using namespace std::placeholders;

namespace PerfProfiler {
namespace Internal {

static PerfProfilerTool *s_instance;

PerfProfilerTool::PerfProfilerTool()
{
    s_instance = this;
    m_traceManager = new PerfProfilerTraceManager(this);
    m_traceManager->registerFeatures(PerfEventType::allFeatures(),
                                     nullptr,
                                     std::bind(&PerfProfilerTool::initialize, this),
                                     std::bind(&PerfProfilerTool::finalize, this),
                                     std::bind(&PerfProfilerTool::clearUi, this));

    m_modelManager = new PerfTimelineModelManager(m_traceManager);
    m_zoomControl = new Timeline::TimelineZoomControl(this);
    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(Constants::PerfOptionsMenuId);
    options->menu()->setTitle(Tr::tr("Performance Analyzer Options"));
    menu->addMenu(options, Debugger::Constants::G_ANALYZER_OPTIONS);
    options->menu()->setEnabled(true);

    const Core::Context globalContext(Core::Constants::C_GLOBAL);
    m_loadPerfData = new QAction(Tr::tr("Load perf.data File"), options);
    Core::Command *command = Core::ActionManager::registerAction(
                m_loadPerfData, Constants::PerfProfilerTaskLoadPerf, globalContext);
    connect(m_loadPerfData, &QAction::triggered, this, &PerfProfilerTool::showLoadPerfDialog);
    options->addAction(command);

    m_loadTrace = new QAction(Tr::tr("Load Trace File"), options);
    command = Core::ActionManager::registerAction(m_loadTrace, Constants::PerfProfilerTaskLoadTrace,
                                                  globalContext);
    connect(m_loadTrace, &QAction::triggered, this, &PerfProfilerTool::showLoadTraceDialog);
    options->addAction(command);

    m_saveTrace = new QAction(Tr::tr("Save Trace File"), options);
    command = Core::ActionManager::registerAction(m_saveTrace, Constants::PerfProfilerTaskSaveTrace,
                                                  globalContext);
    connect(m_saveTrace, &QAction::triggered, this, &PerfProfilerTool::showSaveTraceDialog);
    options->addAction(command);

    m_limitToRange = new QAction(Tr::tr("Limit to Range Selected in Timeline"), options);
    command = Core::ActionManager::registerAction(m_limitToRange, Constants::PerfProfilerTaskLimit,
                                                                 globalContext);
    connect(m_limitToRange, &QAction::triggered, this, [this]() {
        m_traceManager->restrictByFilter(m_traceManager->rangeAndThreadFilter(
                                             m_zoomControl->selectionStart(),
                                             m_zoomControl->selectionEnd()));
    });
    options->addAction(command);

    m_showFullRange = new QAction(Tr::tr("Show Full Range"), options);
    command = Core::ActionManager::registerAction(m_showFullRange,
                                                  Constants::PerfProfilerTaskFullRange,
                                                  globalContext);
    connect(m_showFullRange, &QAction::triggered, this, [this]() {
        m_traceManager->restrictByFilter(m_traceManager->rangeAndThreadFilter(-1, -1));
    });
    options->addAction(command);

    QAction *tracePointsAction = new QAction(Tr::tr("Create Memory Trace Points"), options);
    tracePointsAction->setIcon(Debugger::Icons::TRACEPOINT_TOOLBAR.icon());
    tracePointsAction->setIconVisibleInMenu(false);
    tracePointsAction->setToolTip(Tr::tr("Create trace points for memory profiling on the target "
                                         "device."));
    command = Core::ActionManager::registerAction(tracePointsAction,
                                                  Constants::PerfProfilerTaskTracePoints,
                                                  globalContext);
    connect(tracePointsAction, &QAction::triggered, this, &PerfProfilerTool::createTracePoints);
    options->addAction(command);

    m_tracePointsButton = new QToolButton;
    StyleHelper::setPanelWidget(m_tracePointsButton);
    m_tracePointsButton->setDefaultAction(tracePointsAction);
    m_objectsToDelete << m_tracePointsButton;

    auto action = new QAction(Tr::tr("Performance Analyzer"), this);
    action->setToolTip(Tr::tr("Finds performance bottlenecks."));
    menu->addAction(ActionManager::registerAction(action, Constants::PerfProfilerLocalActionId),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this] {
        m_perspective.select();
        ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
    });

    m_startAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();
    m_objectsToDelete << m_startAction << m_stopAction;

    QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(m_startAction, &QAction::changed, action, [action, tracePointsAction, this] {
        action->setEnabled(m_startAction->isEnabled());
        tracePointsAction->setEnabled(m_startAction->isEnabled());
    });

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &PerfProfilerTool::updateRunActions);

    m_recordButton = new QToolButton;
    StyleHelper::setPanelWidget(m_recordButton);
    m_clearButton = new QToolButton;
    StyleHelper::setPanelWidget(m_clearButton);
    m_filterButton = new QToolButton;
    StyleHelper::setPanelWidget(m_filterButton);
    m_filterMenu = new QMenu(m_filterButton);
    m_aggregateButton = new QToolButton;
    StyleHelper::setPanelWidget(m_aggregateButton);
    m_recordedLabel = new QLabel;
    StyleHelper::setPanelWidget(m_recordedLabel);
    m_delayLabel = new QLabel;
    StyleHelper::setPanelWidget(m_delayLabel);
    m_objectsToDelete << m_recordButton << m_clearButton << m_filterButton << m_aggregateButton
                      << m_recordedLabel << m_delayLabel;

    m_perspective.setAboutToActivateCallback([this]() { createViews(); });
    updateRunActions();
}

PerfProfilerTool::~PerfProfilerTool()
{
    qDeleteAll(m_objectsToDelete);
}

void PerfProfilerTool::createViews()
{
    m_objectsToDelete.clear();
    m_traceView = new PerfProfilerTraceView(nullptr, this);
    m_traceView->setWindowTitle(Tr::tr("Timeline"));
    connect(m_traceView, &PerfProfilerTraceView::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);

    m_statisticsView = new PerfProfilerStatisticsView(nullptr, this);
    m_statisticsView->setWindowTitle(Tr::tr("Statistics"));

    m_flameGraphView = new PerfProfilerFlameGraphView(nullptr, this);
    m_flameGraphView->setWindowTitle(Tr::tr("Flame Graph"));

    connect(m_statisticsView, &PerfProfilerStatisticsView::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);
    connect(m_flameGraphView, &PerfProfilerFlameGraphView::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);

    if (m_traceView->isUsable()) {
        m_perspective.addWindow(m_traceView, Perspective::SplitVertical, nullptr);
        m_perspective.addWindow(m_flameGraphView, Perspective::AddToTab, m_traceView);
    } else {
        m_perspective.addWindow(m_flameGraphView, Perspective::SplitVertical, nullptr);
    }
    m_perspective.addWindow(m_statisticsView, Perspective::AddToTab, m_flameGraphView);

    connect(m_statisticsView, &PerfProfilerStatisticsView::typeSelected,
            m_traceView, &PerfProfilerTraceView::selectByTypeId);
    connect(m_flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            m_traceView, &PerfProfilerTraceView::selectByTypeId);

    connect(m_traceView, &PerfProfilerTraceView::typeSelected,
            m_statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(m_flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            m_statisticsView, &PerfProfilerStatisticsView::selectByTypeId);

    connect(m_traceView, &PerfProfilerTraceView::typeSelected,
            m_flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);
    connect(m_statisticsView, &PerfProfilerStatisticsView::typeSelected,
            m_flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);

    // Clear settings if the statistics or flamegraph view isn't there yet.
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Key("AnalyzerViewSettings_") + Constants::PerfProfilerPerspectiveId);
    if (!settings->contains(keyFromString(m_statisticsView->objectName()))
            || !settings->contains(keyFromString(m_flameGraphView->objectName()))) {
        settings->remove(Key());
    }
    settings->endGroup();

    m_recordButton->setCheckable(true);

    QMenu *recordMenu = new QMenu(m_recordButton);
    connect(recordMenu, &QMenu::aboutToShow, recordMenu, [recordMenu] {
        recordMenu->hide();
        PerfSettings *settings = nullptr;
        Target *target = ProjectManager::startupTarget();
        if (target) {
            if (auto runConfig = target->activeRunConfiguration())
                settings = runConfig->currentSettings<PerfSettings>(Constants::PerfSettingsId);
        }

        QWidget *widget = settings ? settings->createPerfConfigWidget(target)
                                   : globalSettings().createPerfConfigWidget(target);
        widget->setWindowFlags(Qt::Dialog);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        widget->show();
    }, Qt::QueuedConnection);
    m_recordButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_recordButton->setMenu(recordMenu);
    setRecording(true);
    connect(m_recordButton, &QAbstractButton::clicked, this, &PerfProfilerTool::setRecording);

    m_clearButton->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    m_clearButton->setToolTip(Tr::tr("Discard data."));
    connect(m_clearButton, &QAbstractButton::clicked, this, &PerfProfilerTool::clear);

    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterButton->setProperty(StyleHelper::C_NO_ARROW, true);
    m_filterButton->setMenu(m_filterMenu);

    m_aggregateButton->setIcon(Utils::Icons::EXPAND_ALL_TOOLBAR.icon());
    m_aggregateButton->setCheckable(true);
    setAggregated(false);
    connect(m_aggregateButton, &QAbstractButton::toggled, this, &PerfProfilerTool::setAggregated);

    m_recordedLabel->setIndent(10);
    connect(m_clearButton, &QAbstractButton::clicked, m_recordedLabel, &QLabel::clear);

    m_delayLabel->setIndent(10);

    connect(m_traceManager, &PerfProfilerTraceManager::error, this, [](const QString &message) {
        QMessageBox *errorDialog = new QMessageBox(ICore::dialogParent());
        errorDialog->setIcon(QMessageBox::Warning);
        errorDialog->setWindowTitle(Tr::tr("Performance Analyzer"));
        errorDialog->setText(message);
        errorDialog->setStandardButtons(QMessageBox::Ok);
        errorDialog->setDefaultButton(QMessageBox::Ok);
        errorDialog->setModal(false);
        errorDialog->show();
    });

    connect(m_traceManager, &PerfProfilerTraceManager::loadFinished, this, [this]() {
        m_readerRunning = false;
        updateRunActions();
    });

    connect(m_traceManager, &PerfProfilerTraceManager::saveFinished, this, [this]() {
        setToolActionsEnabled(true);
    });

    connect(this, &PerfProfilerTool::aggregatedChanged,
            m_traceManager, &PerfProfilerTraceManager::setAggregateAddresses);

    QMenu *menu1 = new QMenu(m_traceView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(Tr::tr("Limit to Selected Range")), &QAction::triggered,
            m_limitToRange, &QAction::trigger);
    menu1->addAction(m_showFullRange);
    connect(menu1->addAction(Tr::tr("Reset Zoom")), &QAction::triggered, this, [this](){
        m_zoomControl->setRange(m_zoomControl->traceStart(), m_zoomControl->traceEnd());
    });

    m_traceView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_traceView, &QWidget::customContextMenuRequested,
            menu1, [menu1, this](const QPoint &pos) {
        menu1->exec(m_traceView->mapToGlobal(pos));
    });

    menu1 = new QMenu(m_statisticsView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(Tr::tr("Limit to Range Selected in Timeline")), &QAction::triggered,
            m_limitToRange, &QAction::trigger);
    connect(menu1->addAction(Tr::tr("Show Full Range")), &QAction::triggered,
            m_showFullRange, &QAction::trigger);
    connect(menu1->addAction(Tr::tr("Copy Table")), &QAction::triggered,
            m_statisticsView, &PerfProfilerStatisticsView::copyFocusedTableToClipboard);
    QAction *copySelection = menu1->addAction(Tr::tr("Copy Row"));
    connect(copySelection, &QAction::triggered,
            m_statisticsView, &PerfProfilerStatisticsView::copyFocusedSelectionToClipboard);
    m_statisticsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_statisticsView, &QWidget::customContextMenuRequested,
            menu1, [this, menu1, copySelection](const QPoint &pos) {
        copySelection->setEnabled(m_statisticsView->focusedTableHasValidSelection());
        menu1->exec(m_statisticsView->mapToGlobal(pos));
    });

    menu1 = new QMenu(m_flameGraphView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(Tr::tr("Limit to Range Selected in Timeline")), &QAction::triggered,
            m_limitToRange, &QAction::trigger);
    connect(menu1->addAction(Tr::tr("Show Full Range")), &QAction::triggered,
            m_showFullRange, &QAction::trigger);
    QAction *resetAction = menu1->addAction(Tr::tr("Reset Flame Graph"));
    connect(resetAction, &QAction::triggered,
            m_flameGraphView, &PerfProfilerFlameGraphView::resetRoot);
    m_flameGraphView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_flameGraphView, &QWidget::customContextMenuRequested,
            menu1, [this, menu1, resetAction](const QPoint &pos) {
        resetAction->setEnabled(m_flameGraphView->isZoomed());
        menu1->exec(m_flameGraphView->mapToGlobal(pos));
    });

    m_perspective.addToolBarAction(m_startAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarWidget(m_recordButton);
    m_perspective.addToolBarWidget(m_clearButton);
    m_perspective.addToolBarWidget(m_filterButton);
    m_perspective.addToolBarWidget(m_aggregateButton);
    m_perspective.addToolBarWidget(m_recordedLabel);
    m_perspective.addToolBarWidget(m_delayLabel);
    m_perspective.addToolBarWidget(m_tracePointsButton);

    m_perspective.setAboutToActivateCallback(Perspective::Callback());
}

PerfProfilerTool *PerfProfilerTool::instance()
{
    return s_instance;
}

PerfProfilerTraceManager *PerfProfilerTool::traceManager() const
{
    return m_traceManager;
}

void PerfProfilerTool::addLoadSaveActionsToMenu(QMenu *menu)
{
    menu->addAction(m_loadPerfData);
    menu->addAction(m_loadTrace);
    menu->addAction(m_saveTrace);
}

void PerfProfilerTool::createTracePoints()
{
    PerfTracePointDialog dialog;
    dialog.exec();
}

void PerfProfilerTool::initialize()
{
    clearUi();
    setToolActionsEnabled(false);
}

void PerfProfilerTool::finalize()
{
    const qint64 startTime = m_traceManager->traceStart();
    const qint64 endTime = m_traceManager->traceEnd();
    QTC_ASSERT(endTime >= startTime, return);
    m_zoomControl->setTrace(startTime, endTime);
    m_zoomControl->setRange(startTime, startTime + (endTime - startTime) / 10);
    updateTime(m_zoomControl->traceDuration(), -1);
    updateFilterMenu();
    updateRunActions();
    setToolActionsEnabled(true);
}

void PerfProfilerTool::startLoading()
{
    m_readerRunning = true;
}

void PerfProfilerTool::onReaderFinished()
{
    m_readerRunning = false;
    if (m_traceManager->traceDuration() <= 0) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("No Data Loaded"),
                             Tr::tr("The profiler did not produce any samples. "
                                    "Make sure that you are running a recent Linux kernel and that "
                                    "the \"perf\" utility is available and generates useful call "
                                    "graphs.\nYou might find further explanations in the "
                                    "Application Output view."));
        clear();
    } else {
        m_traceManager->finalize();
    }
}

void PerfProfilerTool::onRunControlStarted()
{
    m_processRunning = true;
    updateRunActions();
}

void PerfProfilerTool::onRunControlFinished()
{
    m_processRunning = false;
    updateRunActions();
}

void PerfProfilerTool::onReaderStarted()
{
    m_traceManager->initialize();
}

void PerfProfilerTool::onWorkerCreation(RunControl *runControl)
{
    populateFileFinder(runControl->project(), runControl->kit());
}

void PerfProfilerTool::updateRunActions()
{
    m_stopAction->setEnabled(m_processRunning);
    if (m_readerRunning || m_processRunning) {
        m_startAction->setEnabled(false);
        m_startAction->setToolTip(Tr::tr("A performance analysis is still in progress."));
        m_loadPerfData->setEnabled(false);
        m_loadTrace->setEnabled(false);
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(
            ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
        m_startAction->setToolTip(canRun ? Tr::tr("Start a performance analysis.") : canRun.error());
        m_startAction->setEnabled(bool(canRun));
        m_loadPerfData->setEnabled(true);
        m_loadTrace->setEnabled(true);
    }
    m_saveTrace->setEnabled(!m_traceManager->isEmpty());
}

void PerfProfilerTool::setToolActionsEnabled(bool on)
{
    m_limitToRange->setEnabled(on);
    m_showFullRange->setEnabled(on);
    m_clearButton->setEnabled(on);
    m_filterButton->setEnabled(on);
    m_aggregateButton->setEnabled(on);
    m_filterMenu->setEnabled(on);
    if (m_traceView)
        m_traceView->setEnabled(on);
    if (m_statisticsView)
        m_statisticsView->setEnabled(on);
    if (m_flameGraphView)
        m_flameGraphView->setEnabled(on);
}

PerfTimelineModelManager *PerfProfilerTool::modelManager() const
{
    return m_modelManager;
}

Timeline::TimelineZoomControl *PerfProfilerTool::zoomControl() const
{
    return m_zoomControl;
}

bool PerfProfilerTool::isRecording() const
{
    return m_recordButton->isChecked();
}

static bool operator<(const PerfProfilerTraceManager::Thread &a,
                      const PerfProfilerTraceManager::Thread &b)
{
    return a.tid < b.tid;
}

void PerfProfilerTool::updateFilterMenu()
{
    m_filterMenu->clear();

    QAction *enableAll = m_filterMenu->addAction(Tr::tr("Enable All"));
    QAction *disableAll = m_filterMenu->addAction(Tr::tr("Disable All"));
    m_filterMenu->addSeparator();

    QList<PerfProfilerTraceManager::Thread> threads = m_traceManager->threads().values();
    std::sort(threads.begin(), threads.end());

    for (const PerfProfilerTraceManager::Thread &thread : std::as_const(threads)) {
        QAction *action = m_filterMenu->addAction(
                    QString::fromLatin1("%1 (%2)")
                    .arg(QString::fromUtf8(m_traceManager->string(thread.name)))
                    .arg(thread.tid));
        action->setCheckable(true);
        action->setData(thread.tid);
        action->setChecked(thread.enabled);
        if (thread.tid == 0) {
            action->setEnabled(false);
        } else {
            connect(action, &QAction::toggled, this, [this, action](bool checked) {
                m_traceManager->setThreadEnabled(action->data().toUInt(), checked);
            });
            connect(enableAll, &QAction::triggered,
                    action, [action]() { action->setChecked(true); });
            connect(disableAll, &QAction::triggered,
                    action, [action]() { action->setChecked(false); });
        }
    }
}

void PerfProfilerTool::gotoSourceLocation(QString filePath, int lineNumber, int columnNumber)
{
    if (lineNumber < 0 || filePath.isEmpty())
        return;

    QFileInfo fi(filePath);
    if (!fi.isAbsolute() || !fi.exists() || !fi.isReadable()) {
        fi.setFile(m_fileFinder.findFile(filePath).constFirst().toString());
        if (!fi.exists() || !fi.isReadable())
            return;
    }

    // The text editors count columns starting with 0, but the ASTs store the
    // location starting with 1, therefore the -1.
    EditorManager::openEditorAt({FilePath::fromFileInfo(fi), lineNumber, columnNumber - 1},
                                Utils::Id(),
                                EditorManager::DoNotSwitchToDesignMode
                                    | EditorManager::DoNotSwitchToEditMode);
}

static Utils::FilePaths collectQtIncludePaths(const ProjectExplorer::Kit *kit)
{
    QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit);
    if (qt == nullptr)
        return Utils::FilePaths();
    Utils::FilePaths paths{qt->headerPath()};
    QDirIterator dit(paths.first().toString(), QStringList(), QDir::Dirs | QDir::NoDotAndDotDot,
                     QDirIterator::Subdirectories);
    while (dit.hasNext()) {
        dit.next();
        paths << Utils::FilePath::fromString(dit.filePath());
    }
    return paths;
}

static Utils::FilePath sysroot(const Kit *kit)
{
    return SysRootKitAspect::sysRoot(kit);
}

static Utils::FilePaths sourceFiles(const Project *currentProject = nullptr)
{
    Utils::FilePaths sourceFiles;

    // Have the current project first.
    if (currentProject)
        sourceFiles.append(currentProject->files(Project::SourceFiles));

    const QList<Project *> projects = ProjectManager::projects();
    for (const Project *project : projects) {
        if (project != currentProject)
            sourceFiles.append(project->files(Project::SourceFiles));
    }

    return sourceFiles;
}

void PerfProfilerTool::showLoadPerfDialog()
{
    m_perspective.select();

    PerfLoadDialog dlg(Core::ICore::dialogParent());
    if (dlg.exec() != PerfLoadDialog::Accepted)
        return;

    startLoading();

    Kit *kit = dlg.kit();
    m_fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
    m_fileFinder.setSysroot(sysroot(kit));
    m_fileFinder.setProjectFiles(sourceFiles());
    m_traceManager->loadFromPerfData(FilePath::fromUserInput(dlg.traceFilePath()), dlg.executableDirPath(), kit);
}

void PerfProfilerTool::showLoadTraceDialog()
{
    m_perspective.select();

    FilePath filePath = FileUtils::getOpenFilePath(nullptr, Tr::tr("Load Trace File"),
                                                   {}, Tr::tr("Trace File (*.ptq)"));
    if (filePath.isEmpty())
        return;

    startLoading();

    const Project *currentProject = ProjectManager::startupProject();
    const Target *target = currentProject ?  currentProject->activeTarget() : nullptr;
    const Kit *kit = target ? target->kit() : nullptr;
    populateFileFinder(currentProject, kit);

    m_traceManager->loadFromTraceFile(filePath);
}

void PerfProfilerTool::showSaveTraceDialog()
{
    m_perspective.select();

    FilePath filePath = FileUtils::getSaveFilePath(nullptr, Tr::tr("Save Trace File"),
                                                   {}, Tr::tr("Trace File (*.ptq)"));
    if (filePath.isEmpty())
        return;
    if (!filePath.endsWith(".ptq"))
        filePath = filePath.stringAppended(".ptq");

    setToolActionsEnabled(false);
    m_traceManager->saveToTraceFile(filePath);
}

void PerfProfilerTool::setAggregated(bool aggregated)
{
    m_aggregateButton->setChecked(aggregated);
    m_aggregateButton->setToolTip(aggregated ? Tr::tr("Show all addresses.")
                                             : Tr::tr("Aggregate by functions."));
    emit aggregatedChanged(aggregated);
}

void PerfProfilerTool::setRecording(bool recording)
{
    const static QIcon recordOn = Debugger::Icons::RECORD_ON.icon();
    const static QIcon recordOff = Debugger::Icons::RECORD_OFF.icon();

    m_recordButton->setIcon(recording ? recordOn : recordOff);
    m_recordButton->setChecked(recording);
    m_recordButton->setToolTip(recording ? Tr::tr("Stop collecting profile data.") :
                                           Tr::tr("Collect profile data."));
    emit recordingChanged(recording);
}

void PerfProfilerTool::updateTime(qint64 duration, qint64 delay)
{
    qint64 e9 = 1e9, e8 = 1e8, ten = 10; // compiler would cast to double
    if (duration > 0)
        m_recordedLabel->setText(Tr::tr("Recorded: %1.%2s").arg(duration / e9)
                                 .arg(qAbs(duration / e8) % ten));
    else if (duration == 0)
        m_recordedLabel->clear();

    if (delay > 0)
        m_delayLabel->setText(Tr::tr("Processing delay: %1.%2s").arg(delay / e9)
                              .arg(qAbs(delay / e8) % ten));
    else if (delay == 0)
        m_delayLabel->clear();
}

void PerfProfilerTool::populateFileFinder(const Project *project, const Kit *kit)
{
    m_fileFinder.setProjectFiles(sourceFiles(project));

    if (project)
        m_fileFinder.setProjectDirectory(project->projectDirectory());

    if (kit) {
        m_fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
        m_fileFinder.setSysroot(sysroot(kit));
    }
}

void PerfProfilerTool::clear()
{
    clearData();
    clearUi();
}

void PerfProfilerTool::clearData()
{
    m_traceManager->clearAll();
    m_traceManager->setAggregateAddresses(m_aggregateButton->isChecked());
    m_zoomControl->clear();
}

void PerfProfilerTool::clearUi()
{
    if (m_traceView)
        m_traceView->clear();
    updateTime(0, 0);
    updateFilterMenu();
    updateRunActions();
}

} // namespace Internal
} // namespace PerfProfiler
