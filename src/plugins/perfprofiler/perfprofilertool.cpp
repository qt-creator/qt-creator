/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfconfigwidget.h"
#include "perfloaddialog.h"
#include "perfprofilerplugin.h"
#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"
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
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QHBoxLayout>
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
    options->menu()->setTitle(tr("Performance Analyzer Options"));
    menu->addMenu(options, Debugger::Constants::G_ANALYZER_OPTIONS);
    options->menu()->setEnabled(true);

    const Core::Context globalContext(Core::Constants::C_GLOBAL);
    m_loadPerfData = new QAction(tr("Load perf.data File"), options);
    Core::Command *command = Core::ActionManager::registerAction(
                m_loadPerfData, Constants::PerfProfilerTaskLoadPerf, globalContext);
    connect(m_loadPerfData, &QAction::triggered, this, &PerfProfilerTool::showLoadPerfDialog);
    options->addAction(command);

    m_loadTrace = new QAction(tr("Load Trace File"), options);
    command = Core::ActionManager::registerAction(m_loadTrace, Constants::PerfProfilerTaskLoadTrace,
                                                  globalContext);
    connect(m_loadTrace, &QAction::triggered, this, &PerfProfilerTool::showLoadTraceDialog);
    options->addAction(command);

    m_saveTrace = new QAction(tr("Save Trace File"), options);
    command = Core::ActionManager::registerAction(m_saveTrace, Constants::PerfProfilerTaskSaveTrace,
                                                  globalContext);
    connect(m_saveTrace, &QAction::triggered, this, &PerfProfilerTool::showSaveTraceDialog);
    options->addAction(command);

    m_limitToRange = new QAction(tr("Limit to Range Selected in Timeline"), options);
    command = Core::ActionManager::registerAction(m_limitToRange, Constants::PerfProfilerTaskLimit,
                                                                 globalContext);
    connect(m_limitToRange, &QAction::triggered, this, [this]() {
        m_traceManager->restrictByFilter(m_traceManager->rangeAndThreadFilter(
                                             m_zoomControl->selectionStart(),
                                             m_zoomControl->selectionEnd()));
    });
    options->addAction(command);

    m_showFullRange = new QAction(tr("Show Full Range"), options);
    command = Core::ActionManager::registerAction(m_showFullRange,
                                                  Constants::PerfProfilerTaskFullRange,
                                                  globalContext);
    connect(m_showFullRange, &QAction::triggered, this, [this]() {
        m_traceManager->restrictByFilter(m_traceManager->rangeAndThreadFilter(-1, -1));
    });
    options->addAction(command);

    QAction *tracePointsAction = new QAction(tr("Create Memory Trace Points"), options);
    tracePointsAction->setIcon(Debugger::Icons::TRACEPOINT_TOOLBAR.icon());
    tracePointsAction->setIconVisibleInMenu(false);
    tracePointsAction->setToolTip(tr("Create trace points for memory profiling on the target "
                                       "device."));
    command = Core::ActionManager::registerAction(tracePointsAction,
                                                  Constants::PerfProfilerTaskTracePoints,
                                                  globalContext);
    connect(tracePointsAction, &QAction::triggered, this, &PerfProfilerTool::createTracePoints);
    options->addAction(command);

    m_tracePointsButton = new QToolButton;
    m_tracePointsButton->setDefaultAction(tracePointsAction);
    m_objectsToDelete << m_tracePointsButton;

    auto action = new QAction(tr("Performance Analyzer"), this);
    action->setToolTip(tr("Finds performance bottlenecks."));
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
    m_clearButton = new QToolButton;
    m_filterButton = new QToolButton;
    m_filterMenu = new QMenu(m_filterButton);
    m_aggregateButton = new QToolButton;
    m_recordedLabel = new QLabel;
    m_recordedLabel->setProperty("panelwidget", true);
    m_delayLabel = new QLabel;
    m_delayLabel->setProperty("panelwidget", true);
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
    m_traceView->setWindowTitle(tr("Timeline"));
    connect(m_traceView, &PerfProfilerTraceView::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);

    m_statisticsView = new PerfProfilerStatisticsView(nullptr, this);
    m_statisticsView->setWindowTitle(tr("Statistics"));

    m_flameGraphView = new PerfProfilerFlameGraphView(nullptr, this);
    m_flameGraphView->setWindowTitle(tr("Flame Graph"));

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
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") +
                         QLatin1String(Constants::PerfProfilerPerspectiveId));
    if (!settings->contains(m_statisticsView->objectName())
            || !settings->contains(m_flameGraphView->objectName())) {
        settings->remove(QString());
    }
    settings->endGroup();

    m_recordButton->setCheckable(true);

    QMenu *recordMenu = new QMenu(m_recordButton);
    connect(recordMenu, &QMenu::aboutToShow, recordMenu, [recordMenu] {
        recordMenu->hide();
        PerfSettings *settings = nullptr;
        Target *target = SessionManager::startupTarget();
        if (target) {
            if (auto runConfig = target->activeRunConfiguration())
                settings = runConfig->currentSettings<PerfSettings>(Constants::PerfSettingsId);
        }

        PerfConfigWidget *widget = new PerfConfigWidget(
                    settings ? settings : PerfProfilerPlugin::globalSettings(),
                    Core::ICore::dialogParent());
        widget->setTracePointsButtonVisible(true);
        widget->setTarget(target);
        widget->setWindowFlags(Qt::Dialog);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        widget->show();
    }, Qt::QueuedConnection);
    m_recordButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_recordButton->setMenu(recordMenu);
    setRecording(true);
    connect(m_recordButton, &QAbstractButton::clicked, this, &PerfProfilerTool::setRecording);

    m_clearButton->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    m_clearButton->setToolTip(tr("Discard data."));
    connect(m_clearButton, &QAbstractButton::clicked, this, &PerfProfilerTool::clear);

    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterButton->setProperty("noArrow", true);
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
        errorDialog->setWindowTitle(tr("Performance Analyzer"));
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
    connect(menu1->addAction(tr("Limit to Selected Range")), &QAction::triggered,
            m_limitToRange, &QAction::trigger);
    menu1->addAction(m_showFullRange);
    connect(menu1->addAction(tr("Reset Zoom")), &QAction::triggered, this, [this](){
        m_zoomControl->setRange(m_zoomControl->traceStart(), m_zoomControl->traceEnd());
    });

    m_traceView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_traceView, &QWidget::customContextMenuRequested,
            menu1, [menu1, this](const QPoint &pos) {
        menu1->exec(m_traceView->mapToGlobal(pos));
    });

    menu1 = new QMenu(m_statisticsView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(tr("Limit to Range Selected in Timeline")), &QAction::triggered,
            m_limitToRange, &QAction::trigger);
    connect(menu1->addAction(tr("Show Full Range")), &QAction::triggered,
            m_showFullRange, &QAction::trigger);
    connect(menu1->addAction(tr("Copy Table")), &QAction::triggered,
            m_statisticsView, &PerfProfilerStatisticsView::copyFocusedTableToClipboard);
    QAction *copySelection = menu1->addAction(tr("Copy Row"));
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
    connect(menu1->addAction(tr("Limit to Range Selected in Timeline")), &QAction::triggered,
            m_limitToRange, &QAction::trigger);
    connect(menu1->addAction(tr("Show Full Range")), &QAction::triggered,
            m_showFullRange, &QAction::trigger);
    QAction *resetAction = menu1->addAction(tr("Reset Flame Graph"));
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
                             tr("No Data Loaded"),
                             tr("The profiler did not produce any samples. "
                                "Make sure that you are running a recent Linux kernel and that "
                                "the \"perf\" utility is available and generates useful call "
                                "graphs.\nYou might find further explanations in the Application "
                                "Output view."));
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
        m_startAction->setToolTip(tr("A performance analysis is still in progress."));
        m_loadPerfData->setEnabled(false);
        m_loadTrace->setEnabled(false);
    } else {
        QString whyNot = tr("Start a performance analysis.");
        bool canRun = ProjectExplorerPlugin::canRunStartupProject(
                    ProjectExplorer::Constants::PERFPROFILER_RUN_MODE, &whyNot);
        m_startAction->setToolTip(whyNot);
        m_startAction->setEnabled(canRun);
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

    QAction *enableAll = m_filterMenu->addAction(tr("Enable All"));
    QAction *disableAll = m_filterMenu->addAction(tr("Disable All"));
    m_filterMenu->addSeparator();

    QList<PerfProfilerTraceManager::Thread> threads = m_traceManager->threads().values();
    std::sort(threads.begin(), threads.end());

    for (const PerfProfilerTraceManager::Thread &thread : qAsConst(threads)) {
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
        fi.setFile(m_fileFinder.findFile(filePath).first().toString());
        if (!fi.exists() || !fi.isReadable())
            return;
    }

    // The text editors count columns starting with 0, but the ASTs store the
    // location starting with 1, therefore the -1.
    EditorManager::openEditorAt(fi.filePath(), lineNumber, columnNumber - 1, Utils::Id(),
                                EditorManager::DoNotSwitchToDesignMode
                                | EditorManager::DoNotSwitchToEditMode);

}

static Utils::FilePaths collectQtIncludePaths(const ProjectExplorer::Kit *kit)
{
    QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit);
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

    const QList<Project *> projects = SessionManager::projects();
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
    m_traceManager->loadFromPerfData(dlg.traceFilePath(), dlg.executableDirPath(), kit);
}

void PerfProfilerTool::showLoadTraceDialog()
{
    m_perspective.select();

    QString filename = QFileDialog::getOpenFileName(
                ICore::dialogParent(), tr("Load Trace File"),
                "", tr("Trace File (*.ptq)"));
    if (filename.isEmpty())
        return;

    startLoading();

    const Project *currentProject = SessionManager::startupProject();
    const Target *target = currentProject ?  currentProject->activeTarget() : nullptr;
    const Kit *kit = target ? target->kit() : nullptr;
    populateFileFinder(currentProject, kit);

    m_traceManager->loadFromTraceFile(filename);
}

void PerfProfilerTool::showSaveTraceDialog()
{
    m_perspective.select();

    QString filename = QFileDialog::getSaveFileName(
                ICore::dialogParent(), tr("Save Trace File"),
                "", tr("Trace File (*.ptq)"));
    if (filename.isEmpty())
        return;
    if (!filename.endsWith(".ptq"))
        filename += ".ptq";

    setToolActionsEnabled(false);
    m_traceManager->saveToTraceFile(filename);
}

void PerfProfilerTool::setAggregated(bool aggregated)
{
    m_aggregateButton->setChecked(aggregated);
    m_aggregateButton->setToolTip(aggregated ? tr("Show all addresses.")
                                             : tr("Aggregate by functions."));
    emit aggregatedChanged(aggregated);
}

void PerfProfilerTool::setRecording(bool recording)
{
    const static QIcon recordOn = Debugger::Icons::RECORD_ON.icon();
    const static QIcon recordOff = Debugger::Icons::RECORD_OFF.icon();

    m_recordButton->setIcon(recording ? recordOn : recordOff);
    m_recordButton->setChecked(recording);
    m_recordButton->setToolTip(recording ? tr("Stop collecting profile data.") :
                                           tr("Collect profile data."));
    emit recordingChanged(recording);
}

void PerfProfilerTool::updateTime(qint64 duration, qint64 delay)
{
    qint64 e9 = 1e9, e8 = 1e8, ten = 10; // compiler would cast to double
    if (duration > 0)
        m_recordedLabel->setText(tr("Recorded: %1.%2s").arg(duration / e9)
                                .arg(qAbs(duration / e8) % ten));
    else if (duration == 0)
        m_recordedLabel->clear();

    if (delay > 0)
        m_delayLabel->setText(tr("Processing delay: %1.%2s").arg(delay / e9)
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
