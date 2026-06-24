// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertool.h"

#include "perfprofilerconstants.h"
#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerflamegraphview.h"
#include "perfloaddialog.h"
#include "perfprofilerstatisticsview.h"
#include "perfprofilertr.h"
#include "perfprofilertracemanager.h"
#include "perftimelinemodelmanager.h"
#include "perfsettings.h"
#include "perftracepointdialog.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/perspective.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/action.h>
#include <utils/fancymainwindow.h>
#include <utils/fileinprojectfinder.h>
#include <utils/fileutils.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QCoreApplication>
#include <QDirIterator>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Profiler::Constants;
using namespace Utils;
using namespace std::placeholders;

namespace Profiler::Internal {

class PerfProfilerToolPrivate
{
public:
    Core::Perspective m_perspective{
        Constants::PerfProfilerPerspectiveId,
        QCoreApplication::translate("QtC::PerfProfiler", "Performance Analyzer")
    };

    QAction m_startAction;
    QAction m_stopAction;
    Utils::Action m_loadPerfData;
    Utils::Action m_loadTrace;
    Utils::Action m_saveTrace;
    Utils::Action m_limitToRange;
    Utils::Action m_showFullRange;
    QToolButton m_clearButton;
    QToolButton m_recordButton;
    QLabel m_recordedLabel;
    QLabel m_delayLabel;
    QMenu m_filterMenu;
    QToolButton m_filterButton;
    QToolButton m_aggregateButton;
    QToolButton m_tracePointsButton;

    Timeline::TimelineZoomControl m_zoomControl;
    Timeline::TimelineWidget m_traceView{&modelManager(), &m_zoomControl};
    PerfProfilerStatisticsView m_statisticsView;
    PerfProfilerFlameGraphModel m_flameGraphModel{&traceManager()};
    PerfProfilerFlameGraphView m_flameGraphView{&m_flameGraphModel};
    Utils::FileInProjectFinder m_fileFinder;
    bool m_readerRunning = false;
    bool m_processRunning = false;
};

static PerfProfilerTool *s_instance;

PerfProfilerTool::PerfProfilerTool()
    : d(new PerfProfilerToolPrivate)
{
    s_instance = this;
    traceManager().registerFeatures(PerfEventType::allFeatures(),
                                     std::bind(&PerfProfilerTool::initialize, this),
                                     std::bind(&PerfProfilerTool::finalize, this),
                                     std::bind(&PerfProfilerTool::clearUi, this));

    const Id subMenu = "Analyzer.Menu.PerfOptions";
    ActionContainer *options = ActionManager::createMenu(subMenu);
    options->menu()->setTitle(Tr::tr("Performance Analyzer Options"));
    options->menu()->setEnabled(true);

    ActionContainer *menu = ActionManager::actionContainer(Core::Constants::M_DEBUG_ANALYZER);
    menu->addMenu(options, Core::Constants::G_ANALYZER_OPTIONS);

    ActionBuilder(options, Constants::PerfProfilerTaskLoadPerf)
        .adopt(&d->m_loadPerfData)
        .setText(Tr::tr("Load perf.data File"))
        .addToContainer(subMenu)
        .addOnTriggered(this, &PerfProfilerTool::showLoadPerfDialog);

    ActionBuilder(options, Constants::PerfProfilerTaskLoadTrace)
        .adopt(&d->m_loadTrace)
        .setText(Tr::tr("Load Trace File"))
        .addToContainer(subMenu)
        .addOnTriggered(this, &PerfProfilerTool::showLoadTraceDialog);

    ActionBuilder(options, Constants::PerfProfilerTaskSaveTrace)
        .adopt(&d->m_saveTrace)
        .setText(Tr::tr("Save Trace File"))
        .addToContainer(subMenu)
        .addOnTriggered(this, &PerfProfilerTool::showSaveTraceDialog);

    ActionBuilder(options, Constants::PerfProfilerTaskLimit)
        .adopt(&d->m_limitToRange)
        .setText(Tr::tr("Limit to Range Selected in Timeline"))
        .addToContainer(subMenu)
        .addOnTriggered(this, [this] {
            traceManager().restrictByFilter(traceManager().rangeAndThreadFilter(
                d->m_zoomControl.selectionStart(),
                d->m_zoomControl.selectionEnd()));
        });

    ActionBuilder(options, Constants::PerfProfilerTaskFullRange)
        .adopt(&d->m_showFullRange)
        .setText(Tr::tr("Show Full Range"))
        .addToContainer(subMenu)
        .addOnTriggered(this, [] {
            traceManager().restrictByFilter(traceManager().rangeAndThreadFilter(-1, -1));
        });

    QAction *tracePointsAction = nullptr;
    ActionBuilder(options, Constants::PerfProfilerTaskTracePoints)
        .setText(Tr::tr("Create Memory Trace Points"))
        .bindContextAction(&tracePointsAction)
        .setIcon(ProjectExplorer::Icons::TRACEPOINT_TOOLBAR.icon())
        .setIconVisibleInMenu(false)
        .setToolTip(Tr::tr("Create trace points for memory profiling on the target device."))
        .addToContainer(subMenu)
        .addOnTriggered(this, &PerfProfilerTool::createTracePoints);

    StyleHelper::setPanelWidget(&d->m_tracePointsButton);
    d->m_tracePointsButton.setDefaultAction(tracePointsAction);

    QAction *action = nullptr;
    ActionBuilder(this, Constants::PerfProfilerLocalActionId)
        .setText(Tr::tr("Performance Analyzer"))
        .bindContextAction(&action)
        .setToolTip(Tr::tr("Finds performance bottlenecks."))
        .addToContainer(Core::Constants::M_DEBUG_ANALYZER, Core::Constants::G_ANALYZER_TOOLS)
        .addOnTriggered(this, [this] {
            d->m_perspective.select();
            ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
        });

    d->m_startAction.setText(Tr::tr("Start"));
    d->m_startAction.setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR.icon());

    d->m_stopAction.setText(Tr::tr("Stop"));
    d->m_stopAction.setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());

    QObject::connect(&d->m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(&d->m_startAction, &QAction::changed, action, [action, tracePointsAction, this] {
        action->setEnabled(d->m_startAction.isEnabled());
        tracePointsAction->setEnabled(d->m_startAction.isEnabled());
    });

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &PerfProfilerTool::updateRunActions);

    StyleHelper::setPanelWidget(&d->m_recordButton);
    StyleHelper::setPanelWidget(&d->m_clearButton);
    StyleHelper::setPanelWidget(&d->m_filterButton);
    StyleHelper::setPanelWidget(&d->m_aggregateButton);
    StyleHelper::setPanelWidget(&d->m_recordedLabel);
    StyleHelper::setPanelWidget(&d->m_delayLabel);

    d->m_traceView.setObjectName(QLatin1String("PerfProfilerTraceView"));
    d->m_traceView.setWindowTitle(Tr::tr("Timeline"));
    connect(&d->m_traceView, &Timeline::TimelineWidget::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);

    d->m_statisticsView.setWindowTitle(Tr::tr("Statistics"));
    d->m_flameGraphView.setWindowTitle(Tr::tr("Flame Graph"));

    connect(&d->m_statisticsView, &PerfProfilerStatisticsView::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);
    connect(&d->m_flameGraphView, &PerfProfilerFlameGraphView::gotoSourceLocation,
            this, &PerfProfilerTool::gotoSourceLocation);

    d->m_perspective.addWindow(&d->m_traceView, Perspective::SplitVertical, nullptr);
    // Split the details off before tabbing other views onto the trace view; otherwise
    // QMainWindow::splitDockWidget() would just add it as a tab (the anchor is tabbed).
    d->m_perspective.addWindow(d->m_traceView.rangeDetailsWidget(), Perspective::SplitHorizontal,
                               &d->m_traceView);
    d->m_perspective.addWindow(&d->m_flameGraphView, Perspective::AddToTab, &d->m_traceView);
    d->m_perspective.addWindow(&d->m_statisticsView, Perspective::AddToTab, &d->m_flameGraphView);

    connect(&d->m_statisticsView, &PerfProfilerStatisticsView::typeSelected,
            &d->m_traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(&d->m_flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            &d->m_traceView, &Timeline::TimelineWidget::selectByTypeId);

    // Route the flame graph's details into the shared range details view.
    connect(&d->m_flameGraphView, &Timeline::FlameGraphWidget::detailsChanged,
            d->m_traceView.rangeDetailsWidget(), &Timeline::RangeDetailsWidget::setData);
    connect(&d->m_flameGraphView, &Timeline::FlameGraphWidget::detailsCleared,
            d->m_traceView.rangeDetailsWidget(), &Timeline::RangeDetailsWidget::clear);

    connect(&d->m_traceView, &Timeline::TimelineWidget::typeSelected,
            &d->m_statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(&d->m_flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            &d->m_statisticsView, &PerfProfilerStatisticsView::selectByTypeId);

    connect(&d->m_traceView, &Timeline::TimelineWidget::typeSelected,
            &d->m_flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);
    connect(&d->m_statisticsView, &PerfProfilerStatisticsView::typeSelected,
            &d->m_flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);

    // Clear settings if the statistics or flamegraph view isn't there yet.
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Key("AnalyzerViewSettings_") + Constants::PerfProfilerPerspectiveId);
    if (!settings->contains(keyFromString(d->m_statisticsView.objectName()))
            || !settings->contains(keyFromString(d->m_flameGraphView.objectName()))) {
        settings->remove(Key());
    }
    settings->endGroup();

    d->m_recordButton.setCheckable(true);

    QMenu *recordMenu = new QMenu(&d->m_recordButton);
    connect(recordMenu, &QMenu::aboutToShow, recordMenu, [recordMenu] {
        recordMenu->hide();
        PerfSettings *settings = nullptr;
        Target *target = ProjectManager::startupTarget();
        if (target) {
            if (auto runConfig = activeRunConfigForActiveProject())
                settings = runConfig->currentSettings<PerfSettings>(Constants::PerfSettingsId);
        }

        QWidget *widget = settings ? settings->createPerfConfigWidget(target)
                                   : globalSettings().createPerfConfigWidget(target);
        widget->setWindowFlags(Qt::Dialog);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        widget->show();
    }, Qt::QueuedConnection);
    d->m_recordButton.setPopupMode(QToolButton::MenuButtonPopup);
    d->m_recordButton.setMenu(recordMenu);
    setRecording(true);
    connect(&d->m_recordButton, &QAbstractButton::clicked, this, &PerfProfilerTool::setRecording);

    d->m_clearButton.setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    d->m_clearButton.setToolTip(Tr::tr("Discard data."));
    connect(&d->m_clearButton, &QAbstractButton::clicked, this, &PerfProfilerTool::clear);

    d->m_filterButton.setIcon(Utils::Icons::FILTER.icon());
    d->m_filterButton.setPopupMode(QToolButton::InstantPopup);
    d->m_filterButton.setProperty(StyleHelper::C_NO_ARROW, true);
    d->m_filterButton.setMenu(&d->m_filterMenu);

    d->m_aggregateButton.setIcon(Utils::Icons::EXPAND_ALL_TOOLBAR.icon());
    d->m_aggregateButton.setCheckable(true);
    setAggregated(false);
    connect(&d->m_aggregateButton, &QAbstractButton::toggled, this, &PerfProfilerTool::setAggregated);

    d->m_recordedLabel.setIndent(10);
    connect(&d->m_clearButton, &QAbstractButton::clicked, &d->m_recordedLabel, &QLabel::clear);

    d->m_delayLabel.setIndent(10);

    connect(&traceManager(), &PerfProfilerTraceManager::error, this, [](const QString &message) {
        QMessageBox *errorDialog = new QMessageBox(ICore::dialogParent());
        errorDialog->setIcon(QMessageBox::Warning);
        errorDialog->setWindowTitle(Tr::tr("Performance Analyzer"));
        errorDialog->setText(message);
        errorDialog->setStandardButtons(QMessageBox::Ok);
        errorDialog->setDefaultButton(QMessageBox::Ok);
        errorDialog->setModal(false);
        errorDialog->show();
    });

    connect(&traceManager(), &PerfProfilerTraceManager::finishedEmpty, this, [this] {
        d->m_readerRunning = false;
        Core::MessageManager::writeDisrupting(
            Tr::tr("The profiler did not produce any samples. "
                   "Make sure that you are running a recent Linux kernel and that "
                   "the \"perf\" utility is available and generates useful call "
                   "graphs.\nYou might find further explanations in the "
                   "Application Output view."));
        clear();
    });

    connect(&traceManager(), &PerfProfilerTraceManager::saveFinished, this, [this] {
        setToolActionsEnabled(true);
    });

    connect(this, &PerfProfilerTool::aggregatedChanged,
            &traceManager(), &PerfProfilerTraceManager::setAggregateAddresses);

    QMenu *menu1 = new QMenu(&d->m_traceView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(Tr::tr("Limit to Selected Range")), &QAction::triggered,
            &d->m_limitToRange, &QAction::trigger);
    menu1->addAction(&d->m_showFullRange);
    connect(menu1->addAction(Tr::tr("Reset Zoom")), &QAction::triggered, this, [this](){
        d->m_zoomControl.setRange(d->m_zoomControl.traceStart(), d->m_zoomControl.traceEnd());
    });

    d->m_traceView.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&d->m_traceView, &QWidget::customContextMenuRequested,
            menu1, [menu1, this](const QPoint &pos) {
        menu1->exec(d->m_traceView.mapToGlobal(pos));
    });

    menu1 = new QMenu(&d->m_statisticsView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(Tr::tr("Limit to Range Selected in Timeline")), &QAction::triggered,
            &d->m_limitToRange, &QAction::trigger);
    connect(menu1->addAction(Tr::tr("Show Full Range")), &QAction::triggered,
            &d->m_showFullRange, &QAction::trigger);
    connect(menu1->addAction(Tr::tr("Copy Table")), &QAction::triggered,
            &d->m_statisticsView, &PerfProfilerStatisticsView::copyFocusedTableToClipboard);
    QAction *copySelection = menu1->addAction(Tr::tr("Copy Row"));
    connect(copySelection, &QAction::triggered,
            &d->m_statisticsView, &PerfProfilerStatisticsView::copyFocusedSelectionToClipboard);
    d->m_statisticsView.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&d->m_statisticsView, &QWidget::customContextMenuRequested,
            menu1, [this, menu1, copySelection](const QPoint &pos) {
        copySelection->setEnabled(d->m_statisticsView.focusedTableHasValidSelection());
        menu1->exec(d->m_statisticsView.mapToGlobal(pos));
    });

    menu1 = new QMenu(&d->m_flameGraphView);
    addLoadSaveActionsToMenu(menu1);
    connect(menu1->addAction(Tr::tr("Limit to Range Selected in Timeline")), &QAction::triggered,
            &d->m_limitToRange, &QAction::trigger);
    connect(menu1->addAction(Tr::tr("Show Full Range")), &QAction::triggered,
            &d->m_showFullRange, &QAction::trigger);
    menu1->addAction(d->m_flameGraphView.resetAction());
    d->m_flameGraphView.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&d->m_flameGraphView, &QWidget::customContextMenuRequested,
            menu1, [this, menu1](const QPoint &pos) {
        menu1->exec(d->m_flameGraphView.mapToGlobal(pos));
    });

    d->m_perspective.addToolBarAction(&d->m_startAction);
    d->m_perspective.addToolBarAction(&d->m_stopAction);
    d->m_perspective.addToolBarWidget(&d->m_recordButton);
    d->m_perspective.addToolBarWidget(&d->m_clearButton);
    d->m_perspective.addToolBarWidget(&d->m_filterButton);
    d->m_perspective.addToolBarWidget(&d->m_aggregateButton);
    d->m_perspective.addToolBarWidget(&d->m_recordedLabel);
    d->m_perspective.addToolBarWidget(&d->m_delayLabel);
    d->m_perspective.addToolBarWidget(&d->m_tracePointsButton);

    updateRunActions();
}

PerfProfilerTool::~PerfProfilerTool()
{
    delete d;
}

const QAction *PerfProfilerTool::stopAction() const
{
    return &d->m_stopAction;
}

PerfProfilerTool *PerfProfilerTool::instance()
{
    return s_instance;
}

void PerfProfilerTool::addLoadSaveActionsToMenu(QMenu *menu)
{
    menu->addAction(&d->m_loadPerfData);
    menu->addAction(&d->m_loadTrace);
    menu->addAction(&d->m_saveTrace);
}

void PerfProfilerTool::createTracePoints()
{
    PerfTracePointDialog dialog;
    dialog.exec();
}

void PerfProfilerTool::initialize()
{
    d->m_readerRunning = true;
    clearUi();
    setToolActionsEnabled(false);
    updateRunActions();
}

void PerfProfilerTool::finalize()
{
    d->m_readerRunning = false;
    const qint64 startTime = traceManager().traceStart();
    const qint64 endTime = traceManager().traceEnd();
    QTC_ASSERT(endTime >= startTime, return);
    d->m_zoomControl.setTrace(startTime, endTime);
    d->m_zoomControl.setRange(startTime, startTime + (endTime - startTime) / 10);
    updateTime(d->m_zoomControl.traceDuration(), -1);
    updateFilterMenu();
    updateRunActions();
    setToolActionsEnabled(true);
}

void PerfProfilerTool::onRunControlStarted()
{
    d->m_processRunning = true;
    updateRunActions();
}

void PerfProfilerTool::onRunControlFinished()
{
    d->m_processRunning = false;
    updateRunActions();
}

void PerfProfilerTool::onWorkerCreation(RunControl *runControl)
{
    populateFileFinder(runControl->project(), runControl->kit());
}

void PerfProfilerTool::updateRunActions()
{
    d->m_stopAction.setEnabled(d->m_processRunning);
    if (d->m_readerRunning || d->m_processRunning) {
        d->m_startAction.setEnabled(false);
        d->m_startAction.setToolTip(Tr::tr("A performance analysis is still in progress."));
        d->m_loadPerfData.setEnabled(false);
        d->m_loadTrace.setEnabled(false);
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(
            ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
        d->m_startAction.setToolTip(canRun ? Tr::tr("Start a performance analysis.") : canRun.error());
        d->m_startAction.setEnabled(canRun.has_value());
        d->m_loadPerfData.setEnabled(true);
        d->m_loadTrace.setEnabled(true);
    }
    d->m_saveTrace.setEnabled(!traceManager().isEmpty());
}

void PerfProfilerTool::setToolActionsEnabled(bool on)
{
    d->m_limitToRange.setEnabled(on);
    d->m_showFullRange.setEnabled(on);
    d->m_clearButton.setEnabled(on);
    d->m_filterButton.setEnabled(on);
    d->m_aggregateButton.setEnabled(on);
    d->m_filterMenu.setEnabled(on);
    d->m_traceView.setEnabled(on);
    d->m_statisticsView.setEnabled(on);
    d->m_flameGraphView.setEnabled(on);
}

bool PerfProfilerTool::isRecording() const
{
    return d->m_recordButton.isChecked();
}

static bool operator<(const PerfProfilerTraceManager::Thread &a,
                      const PerfProfilerTraceManager::Thread &b)
{
    return a.tid < b.tid;
}

void PerfProfilerTool::updateFilterMenu()
{
    d->m_filterMenu.clear();

    QAction *enableAll = d->m_filterMenu.addAction(Tr::tr("Enable All"));
    QAction *disableAll = d->m_filterMenu.addAction(Tr::tr("Disable All"));
    d->m_filterMenu.addSeparator();

    QList<PerfProfilerTraceManager::Thread> threads = traceManager().threads().values();
    std::sort(threads.begin(), threads.end());

    for (const PerfProfilerTraceManager::Thread &thread : std::as_const(threads)) {
        QAction *action = d->m_filterMenu.addAction(
                    QString::fromLatin1("%1 (%2)")
                    .arg(QString::fromUtf8(traceManager().string(thread.name)))
                    .arg(thread.tid));
        action->setCheckable(true);
        action->setData(thread.tid);
        action->setChecked(thread.enabled);
        if (thread.tid == 0) {
            action->setEnabled(false);
        } else {
            connect(action, &QAction::toggled, this, [action](bool checked) {
                traceManager().setThreadEnabled(action->data().toUInt(), checked);
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
        fi.setFile(d->m_fileFinder.findFile(filePath).constFirst().toUrlishString());
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
    QDirIterator dit(paths.first().toUrlishString(), QStringList(), QDir::Dirs | QDir::NoDotAndDotDot,
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
    d->m_perspective.select();

    PerfLoadDialog dlg(Core::ICore::dialogParent());
    if (dlg.exec() != PerfLoadDialog::Accepted)
        return;

    Kit *kit = dlg.kit();
    d->m_fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
    d->m_fileFinder.setSysroot(sysroot(kit));
    d->m_fileFinder.setProjectFiles(sourceFiles());
    traceManager().loadFromPerfData(FilePath::fromUserInput(dlg.traceFilePath()), dlg.executableDirPath(), kit);
}

void PerfProfilerTool::loadTraceFile(const FilePath &filePath)
{
    d->m_perspective.select();

    const Project *activeProject = ProjectManager::startupProject();
    const Kit *kit = activeKit(activeProject);
    populateFileFinder(activeProject, kit);

    traceManager().loadFromTraceFile(filePath);
}

void PerfProfilerTool::showLoadTraceDialog()
{
    const FilePath filePath = FileUtils::getOpenFilePath(Tr::tr("Load Trace File"),
                                                         {}, Tr::tr("Trace File (*.ptq)"));
    if (filePath.isEmpty())
        return;

    loadTraceFile(filePath);
}

void PerfProfilerTool::showSaveTraceDialog()
{
    d->m_perspective.select();

    FilePath filePath = FileUtils::getSaveFilePath(Tr::tr("Save Trace File"),
                                                   {}, Tr::tr("Trace File (*.ptq)"));
    if (filePath.isEmpty())
        return;
    if (!filePath.endsWith(".ptq"))
        filePath = filePath.stringAppended(".ptq");

    setToolActionsEnabled(false);
    traceManager().saveToTraceFile(filePath);
}

void PerfProfilerTool::setAggregated(bool aggregated)
{
    d->m_aggregateButton.setChecked(aggregated);
    d->m_aggregateButton.setToolTip(aggregated ? Tr::tr("Show all addresses.")
                                               : Tr::tr("Aggregate by functions."));
    emit aggregatedChanged(aggregated);
}

void PerfProfilerTool::setRecording(bool recording)
{
    const static QIcon recordOn = ProjectExplorer::Icons::RECORD_ON.icon();
    const static QIcon recordOff = ProjectExplorer::Icons::RECORD_OFF.icon();

    d->m_recordButton.setIcon(recording ? recordOn : recordOff);
    d->m_recordButton.setChecked(recording);
    d->m_recordButton.setToolTip(recording ? Tr::tr("Stop collecting profile data.") :
                                             Tr::tr("Collect profile data."));
    emit recordingChanged(recording);
}

void PerfProfilerTool::updateTime(qint64 duration, qint64 delay)
{
    qint64 e9 = 1e9, e8 = 1e8, ten = 10; // compiler would cast to double
    if (duration > 0)
        d->m_recordedLabel.setText(Tr::tr("Recorded: %1.%2s").arg(duration / e9)
                                   .arg(qAbs(duration / e8) % ten));
    else if (duration == 0)
        d->m_recordedLabel.clear();

    if (delay > 0)
        d->m_delayLabel.setText(Tr::tr("Processing delay: %1.%2s").arg(delay / e9)
                                .arg(qAbs(delay / e8) % ten));
    else if (delay == 0)
        d->m_delayLabel.clear();
}

void PerfProfilerTool::populateFileFinder(const Project *project, const Kit *kit)
{
    d->m_fileFinder.setProjectFiles(sourceFiles(project));

    if (project)
        d->m_fileFinder.setProjectDirectory(project->projectDirectory());

    if (kit) {
        d->m_fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
        d->m_fileFinder.setSysroot(sysroot(kit));
    }
}

void PerfProfilerTool::clear()
{
    clearData();
    clearUi();
}

void PerfProfilerTool::clearData()
{
    traceManager().clearAll();
    traceManager().setAggregateAddresses(d->m_aggregateButton.isChecked());
    d->m_zoomControl.clear();
}

void PerfProfilerTool::clearUi()
{
    d->m_traceView.clear();
    updateTime(0, 0);
    updateFilterMenu();
    updateRunActions();
}

void setupPerfProfilerTool()
{
    (void) new PerfProfilerTool;
}

void destroyPerfProfilerTool()
{
    delete s_instance;
}

} // Profiler::Internal
