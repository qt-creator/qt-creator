// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertracebackend.h"

#include "cpuflamegraphview.h"
#include "perfprofilerconstants.h"
#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerstatisticsview.h"
#include "perfprofilertracemanager.h"
#include "perfsettings.h"
#include "perftimelinemodelmanager.h"
#include "profilertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

// ProjectExplorer, QtSupport and the tool are not part of the WebAssembly build.
#ifndef QTPROFILER_WASM
#include "perfprofilertool.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/sysrootkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#endif

#include <tracing/timelinedetailswidget.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QDirIterator>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QToolButton>

using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class PerfProfilerTraceBackendPrivate
{
public:
    PerfProfilerTraceManager traceManager;
    PerfTimelineModelManager modelManager{&traceManager};
    Timeline::TimelineZoomControl zoomControl;
    PerfProfilerFlameGraphModel flameGraphModel{&traceManager};
    FileInProjectFinder fileFinder;

    QPointer<Timeline::TimelineDetailsWidget> timelineDetails; // Not owned; shared with
                                                         // the document's other views.
    QPointer<Timeline::TimelineWidget> traceView;
    QPointer<PerfProfilerStatisticsView> statisticsView;
    QPointer<CpuFlameGraphView> flameGraphView;

    QToolButton aggregateButton;
    QToolButton tracePointsButton;
    QLabel recordedLabel;
    QLabel delayLabel;

    QList<QAction *> loadSaveActions; // Owned by the tool; shown in context menus.
    QAction *limitToRange = nullptr;
    QAction *showFullRange = nullptr;

    bool readerRunning = false;
};

#ifndef QTPROFILER_WASM
static FilePaths collectQtIncludePaths(const ProjectExplorer::Kit *kit)
{
    QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(kit);
    if (!qt)
        return {};
    FilePaths paths{qt->headerPath()};
    QDirIterator dit(paths.first().toUrlishString(), QStringList(),
                     QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dit.hasNext()) {
        dit.next();
        paths << FilePath::fromString(dit.filePath());
    }
    return paths;
}

// The sources a recorded path might refer to: the profiled project first, then
// everything else that is open.
static FilePaths sourceFiles(const ProjectExplorer::Project *currentProject = nullptr)
{
    FilePaths sourceFiles;
    if (currentProject)
        sourceFiles.append(currentProject->files(ProjectExplorer::Project::SourceFiles));

    const QList<ProjectExplorer::Project *> projects = ProjectExplorer::ProjectManager::projects();
    for (const ProjectExplorer::Project *project : projects) {
        if (project != currentProject)
            sourceFiles.append(project->files(ProjectExplorer::Project::SourceFiles));
    }
    return sourceFiles;
}
#endif // QTPROFILER_WASM

PerfProfilerTraceBackend::PerfProfilerTraceBackend(Timeline::TimelineDetailsWidget *details,
                                                   QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new PerfProfilerTraceBackendPrivate)
{
    d->timelineDetails = details;
    d->modelManager.persistLayout(Core::ICore::settings(), "Analyzer/PerfTimeline");
    d->traceManager.registerFeatures(PerfEventType::allFeatures(),
                                     [this] { initialize(); },
                                     [this] { finalize(); },
                                     [this] { clearUi(); });

    connect(&d->traceManager, &PerfProfilerTraceManager::error,
            this, &PerfProfilerTraceBackend::error);
    connect(&d->traceManager, &PerfProfilerTraceManager::loadFinished, this, [this] {
        emit loadFinished();
        emit traceChanged();
    });
    connect(&d->traceManager, &PerfProfilerTraceManager::saveFinished, this, [this] {
        setToolActionsEnabled(true);
        emit busyChanged(false);
        emit saved();
    });
    connect(&d->traceManager, &PerfProfilerTraceManager::finishedEmpty, this, [this] {
        d->readerRunning = false;
#ifdef QTPROFILER_WASM
        // The standalone viewer has no Messages pane, and a file that holds no
        // samples is an error there.
        emit error(Tr::tr("The trace does not contain any samples."));
#else
        Core::MessageManager::writeDisrupting(
            Tr::tr("The profiler did not produce any samples. "
                   "Make sure that you are running a recent Linux kernel and that "
                   "the \"perf\" utility is available and generates useful call "
                   "graphs.\nYou might find further explanations in the "
                   "Application Output view."));
#endif
        clear();
    });
    connect(this, &PerfProfilerTraceBackend::aggregatedChanged,
            &d->traceManager, &PerfProfilerTraceManager::setAggregateAddresses);

    setupToolBar();
}

PerfProfilerTraceBackend::~PerfProfilerTraceBackend()
{
    // A load still running writes into the trace manager and the models.
    cancelBackgroundWork();
    // Before the trace manager below goes with d: a run still feeding it holds
    // it by plain pointer, and QObject::destroyed would come too late to say so.
    emit aboutToBeDestroyed();
    delete d->traceView;
    delete d->statisticsView;
    delete d->flameGraphView;
    delete d;
}

QWidgetList PerfProfilerTraceBackend::views(QWidget *parent)
{
    d->traceView = new Timeline::TimelineWidget(&d->modelManager, &d->zoomControl,
                                                d->timelineDetails, parent);
    d->traceView->setObjectName("PerfProfilerTraceView");
    d->traceView->setWindowTitle(Tr::tr("Timeline"));

    d->flameGraphView = new CpuFlameGraphView(&d->flameGraphModel,
                                              CpuFlameGraphView::Measures::SamplesAndResources,
                                              parent);

    d->statisticsView = new PerfProfilerStatisticsView(&d->traceManager);
    d->statisticsView->setParent(parent);
    d->statisticsView->setWindowTitle(Tr::tr("Statistics"));

    const auto goTo = [this](const QString &file, int line, int column) {
        if (line < 0 || file.isEmpty())
            return;
        FilePath path = FilePath::fromUserInput(file);
        if (!path.isAbsolutePath() || !path.isReadableFile()) {
            const FilePaths found = d->fileFinder.findFile(file);
            if (found.isEmpty())
                return;
            path = found.constFirst();
            if (!path.isReadableFile())
                return;
        }
        // Recorded locations count columns from 1, the editor from 0.
        emit gotoSourceLocation({path, line, column - 1});
    };
    connect(d->traceView, &Timeline::TimelineWidget::gotoSourceLocation, this, goTo);
    connect(d->statisticsView, &PerfProfilerStatisticsView::gotoSourceLocation, this, goTo);
    connect(d->flameGraphView, &CpuFlameGraphView::gotoSourceLocation, this, goTo);

    connect(d->statisticsView, &PerfProfilerStatisticsView::typeSelected,
            d->traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(d->flameGraphView, &CpuFlameGraphView::typeSelected,
            d->traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(d->traceView, &Timeline::TimelineWidget::typeSelected,
            d->statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(d->flameGraphView, &CpuFlameGraphView::typeSelected,
            d->statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(d->traceView, &Timeline::TimelineWidget::typeSelected,
            d->flameGraphView, &CpuFlameGraphView::selectByTypeId);
    connect(d->statisticsView, &PerfProfilerStatisticsView::typeSelected,
            d->flameGraphView, &CpuFlameGraphView::selectByTypeId);

    // Route the flame graph's details into the shared range details view.
    connect(d->flameGraphView, &Timeline::FlameGraphWidget::detailsChanged, d->timelineDetails,
            [this](const QString &title, const QList<QPair<QString, QString>> &content) {
        d->timelineDetails->setData(d->flameGraphView, title, content);
    });
    connect(d->flameGraphView, &Timeline::FlameGraphWidget::detailsCleared, d->timelineDetails,
            [this] { d->timelineDetails->clear(d->flameGraphView); });

    const auto addContextMenu = [this](QWidget *view, const QList<QAction *> &extra) {
        QMenu *menu = new QMenu(view);
        for (QAction *action : std::as_const(d->loadSaveActions))
            menu->addAction(action);
        for (QAction *action : extra) {
            if (action) // Null in the standalone viewer, which has no tool.
                menu->addAction(action);
        }
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, menu, [menu, view](const QPoint &pos) {
            menu->exec(view->mapToGlobal(pos));
        });
        return menu;
    };

    QMenu *traceMenu = addContextMenu(d->traceView, {d->limitToRange, d->showFullRange});
    connect(traceMenu->addAction(Tr::tr("Reset Zoom")), &QAction::triggered, this, [this] {
        d->zoomControl.setRange(d->zoomControl.traceStart(), d->zoomControl.traceEnd());
    });

    QMenu *statisticsMenu = addContextMenu(d->statisticsView, {d->limitToRange, d->showFullRange});
    connect(statisticsMenu->addAction(Tr::tr("Copy Table")), &QAction::triggered,
            d->statisticsView, &PerfProfilerStatisticsView::copyFocusedTableToClipboard);
    QAction *copyRow = statisticsMenu->addAction(Tr::tr("Copy Row"));
    connect(copyRow, &QAction::triggered,
            d->statisticsView, &PerfProfilerStatisticsView::copyFocusedSelectionToClipboard);
    connect(statisticsMenu, &QMenu::aboutToShow, this, [this, copyRow] {
        copyRow->setEnabled(d->statisticsView->focusedTableHasValidSelection());
    });

    QMenu *flameGraphMenu = addContextMenu(d->flameGraphView, {d->limitToRange, d->showFullRange});
    flameGraphMenu->addAction(d->flameGraphView->resetAction());

    return {d->traceView, d->flameGraphView, d->statisticsView};
}

QList<QWidget *> PerfProfilerTraceBackend::toolBarWidgets()
{
    return {&d->aggregateButton, &d->recordedLabel, &d->delayLabel, &d->tracePointsButton};
}

void PerfProfilerTraceBackend::cancelBackgroundWork()
{
    traceManager()->cancelBackgroundWork();
}

void PerfProfilerTraceBackend::load(const FilePath &path)
{
    emit busyChanged(true);
#ifndef QTPROFILER_WASM
    // A trace names the paths it recorded; the finder is what maps them onto
    // this machine, as the run and perf-data paths already do.
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    populateFileFinder(project, project ? project->activeKit() : nullptr);
#endif
    d->traceManager.loadFromTraceFile(path);
}

void PerfProfilerTraceBackend::loadPerfData(const FilePath &path, const FilePath &executableDir,
                                            ProjectExplorer::Kit *kit)
{
    emit busyChanged(true);
#ifndef QTPROFILER_WASM
    d->fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
    d->fileFinder.setSysroot(ProjectExplorer::SysRootKitAspect::sysRoot(kit));
    d->fileFinder.setProjectFiles(sourceFiles());
#endif
    d->traceManager.loadFromPerfData(path, executableDir.toUrlishString(), kit);
}

Result<> PerfProfilerTraceBackend::save(const FilePath &path)
{
    emit busyChanged(true);
    setToolActionsEnabled(false);
    d->traceManager.saveToTraceFile(path);
    return ResultOk;
}

// Registered as the trace manager's own clear callback, so it resets what is
// shown and nothing else: clearing the manager from here is what called it.
void PerfProfilerTraceBackend::clearUi()
{
    if (d->traceView)
        d->traceView->clear();
    updateTime(0ns, 0ns);
}

void PerfProfilerTraceBackend::clear()
{
    d->traceManager.clearAll(); // Resets the views through clearUi().
    d->traceManager.setAggregateAddresses(d->aggregateButton.isChecked());
    d->zoomControl.clear();
}

bool PerfProfilerTraceBackend::isEmpty() const
{
    return d->traceManager.isEmpty();
}

bool PerfProfilerTraceBackend::isReaderRunning() const
{
    return d->readerRunning;
}

void PerfProfilerTraceBackend::restrictToSelectedRange()
{
    d->traceManager.restrictByFilter(
        d->traceManager.rangeAndThreadFilter(d->zoomControl.selectionStart(),
                                             d->zoomControl.selectionEnd()));
}

void PerfProfilerTraceBackend::showFullRange()
{
    d->traceManager.restrictByFilter(d->traceManager.rangeAndThreadFilter(-1ns, -1ns));
}

milliseconds PerfProfilerTraceBackend::traceDuration() const
{
    return duration_cast<milliseconds>(d->traceManager.traceDuration());
}

PerfProfilerTraceManager *PerfProfilerTraceBackend::traceManager() const
{
    return &d->traceManager;
}

PerfTimelineModelManager *PerfProfilerTraceBackend::modelManager() const
{
    return &d->modelManager;
}


void PerfProfilerTraceBackend::setupToolBar()
{
    StyleHelper::setPanelWidget(&d->aggregateButton);
    StyleHelper::setPanelWidget(&d->tracePointsButton);
    StyleHelper::setPanelWidget(&d->recordedLabel);
    StyleHelper::setPanelWidget(&d->delayLabel);

    d->aggregateButton.setIcon(Icons::EXPAND_ALL_TOOLBAR.icon());
    d->aggregateButton.setCheckable(true);
    connect(&d->aggregateButton, &QAbstractButton::toggled,
            this, &PerfProfilerTraceBackend::setAggregated);
    setAggregated(false);

    d->recordedLabel.setIndent(StyleHelper::SpacingTokens::PaddingHL);
    d->delayLabel.setIndent(StyleHelper::SpacingTokens::PaddingHL);

    // Context-menu entries owned by the tool. Fetched here rather than injected
    // by the tool afterwards: views() builds its menus from them, and an editor
    // opened through the file-open path never hears from the tool at all. The
    // standalone viewer has no tool, and no menus to fill either.
#ifndef QTPROFILER_WASM
    if (PerfProfilerTool *tool = PerfProfilerTool::instance()) {
        d->loadSaveActions = tool->traceMenuActions();
        d->limitToRange = tool->limitToRangeAction();
        d->showFullRange = tool->showFullRangeAction();
    }
#endif
}

void PerfProfilerTraceBackend::setAggregated(bool aggregated)
{
    d->aggregateButton.setToolTip(aggregated ? Tr::tr("Show addresses of symbols.")
                                             : Tr::tr("Aggregate addresses of symbols."));
    d->aggregateButton.setChecked(aggregated);
    emit aggregatedChanged(aggregated);
}

void PerfProfilerTraceBackend::setToolActionsEnabled(bool on)
{
    d->aggregateButton.setEnabled(on);
    if (d->traceView)
        d->traceView->setEnabled(on);
    if (d->statisticsView)
        d->statisticsView->setEnabled(on);
    if (d->flameGraphView)
        d->flameGraphView->setEnabled(on);
}

void PerfProfilerTraceBackend::initialize()
{
    d->readerRunning = true;
    clearUi();
    setToolActionsEnabled(false);
    emit busyChanged(true);
}

void PerfProfilerTraceBackend::finalize()
{
    d->readerRunning = false;
    const nanoseconds startTime = d->traceManager.traceStart();
    const nanoseconds endTime = d->traceManager.traceEnd();
    QTC_ASSERT(endTime >= startTime, return);
    d->zoomControl.setTrace(startTime, endTime);
    d->zoomControl.setRange(startTime, startTime + (endTime - startTime) / 10);
    updateTime(d->zoomControl.traceDuration(), -1ns);
    setToolActionsEnabled(true);
    emit busyChanged(false);
    emit traceChanged();
}

void PerfProfilerTraceBackend::updateTime(nanoseconds duration, nanoseconds delay)
{
    const auto wholeSeconds = [](nanoseconds time) { return duration_cast<seconds>(time).count(); };
    const auto tenths = [](nanoseconds time) {
        return qAbs(duration_cast<milliseconds>(time).count() / 100) % 10;
    };

    if (duration > 0ns) {
        d->recordedLabel.setText(
            Tr::tr("Recorded: %1.%2s").arg(wholeSeconds(duration)).arg(tenths(duration)));
    } else if (duration == 0ns) {
        d->recordedLabel.clear();
    }

    if (delay > 0ns) {
        d->delayLabel.setText(
            Tr::tr("Processing delay: %1.%2s").arg(wholeSeconds(delay)).arg(tenths(delay)));
    } else if (delay == 0ns) {
        d->delayLabel.clear();
    }
}

void PerfProfilerTraceBackend::populateFileFinder(const ProjectExplorer::Project *project,
                                                  const ProjectExplorer::Kit *kit)
{
#ifdef QTPROFILER_WASM
    Q_UNUSED(project)
    Q_UNUSED(kit)
#else
    d->fileFinder.setProjectFiles(sourceFiles(project));
    if (project)
        d->fileFinder.setProjectDirectory(project->projectDirectory());
    if (kit) {
        d->fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
        d->fileFinder.setSysroot(ProjectExplorer::SysRootKitAspect::sysRoot(kit));
    }
#endif
}

void PerfProfilerTraceBackend::prepareRun(const ProjectExplorer::Project *project,
                                          const ProjectExplorer::Kit *kit)
{
    populateFileFinder(project, kit);
}

} // namespace Profiler::Internal
