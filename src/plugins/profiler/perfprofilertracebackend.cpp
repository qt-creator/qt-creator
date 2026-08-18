// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertracebackend.h"

#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilerstatisticsview.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"
#include "perfprofilertool.h"
#include "perfprofilertracemanager.h"
#include "perfsettings.h"
#include "perftimelinemodelmanager.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/target.h>

#ifndef __EMSCRIPTEN__
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#endif

#include <tracing/rangedetailswidget.h>
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

    QPointer<Timeline::RangeDetailsWidget> rangeDetails; // Not owned; shared with
                                                         // the document's other views.
    QPointer<Timeline::TimelineWidget> traceView;
    QPointer<PerfProfilerStatisticsView> statisticsView;
    QPointer<PerfProfilerFlameGraphView> flameGraphView;

    QToolButton recordButton;
    QToolButton clearButton;
    QToolButton filterButton;
    QMenu filterMenu;
    QToolButton aggregateButton;
    QToolButton tracePointsButton;
    QLabel recordedLabel;
    QLabel delayLabel;
    QAction stopAction;
    QToolButton stopButton;

    QList<QAction *> loadSaveActions; // Owned by the tool; shown in context menus.
    QAction *limitToRange = nullptr;
    QAction *showFullRange = nullptr;

    bool readerRunning = false;
};

static FilePaths collectQtIncludePaths(const ProjectExplorer::Kit *kit)
{
#ifdef __EMSCRIPTEN__
    // QtSupport is not part of the WebAssembly build.
    Q_UNUSED(kit)
    return {};
#else
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
#endif
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

PerfProfilerTraceBackend::PerfProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                                   QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new PerfProfilerTraceBackendPrivate)
{
    d->rangeDetails = details;
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
        Core::MessageManager::writeDisrupting(
            Tr::tr("The profiler did not produce any samples. "
                   "Make sure that you are running a recent Linux kernel and that "
                   "the \"perf\" utility is available and generates useful call "
                   "graphs.\nYou might find further explanations in the "
                   "Application Output view."));
        clear();
    });
    connect(this, &PerfProfilerTraceBackend::aggregatedChanged,
            &d->traceManager, &PerfProfilerTraceManager::setAggregateAddresses);

    setupToolBar();
}

PerfProfilerTraceBackend::~PerfProfilerTraceBackend()
{
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
                                                d->rangeDetails, parent);
    d->traceView->setObjectName("PerfProfilerTraceView");
    d->traceView->setWindowTitle(Tr::tr("Timeline"));

    d->flameGraphView = new PerfProfilerFlameGraphView(&d->flameGraphModel);
    d->flameGraphView->setParent(parent);
    d->flameGraphView->setWindowTitle(Tr::tr("Flame Graph"));

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
    connect(d->flameGraphView, &PerfProfilerFlameGraphView::gotoSourceLocation, this, goTo);

    connect(d->statisticsView, &PerfProfilerStatisticsView::typeSelected,
            d->traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(d->flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            d->traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(d->traceView, &Timeline::TimelineWidget::typeSelected,
            d->statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(d->flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            d->statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(d->traceView, &Timeline::TimelineWidget::typeSelected,
            d->flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);
    connect(d->statisticsView, &PerfProfilerStatisticsView::typeSelected,
            d->flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);

    // Route the flame graph's details into the shared range details view.
    connect(d->flameGraphView, &Timeline::FlameGraphWidget::detailsChanged, d->rangeDetails,
            [this](const QString &title, const QList<QPair<QString, QString>> &content) {
        d->rangeDetails->setData(d->flameGraphView, title, content);
    });
    connect(d->flameGraphView, &Timeline::FlameGraphWidget::detailsCleared, d->rangeDetails,
            [this] { d->rangeDetails->clear(d->flameGraphView); });

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
    return {&d->recordButton, &d->stopButton, &d->clearButton, &d->filterButton,
            &d->aggregateButton, &d->recordedLabel, &d->delayLabel, &d->tracePointsButton};
}

void PerfProfilerTraceBackend::load(const FilePath &path)
{
    emit busyChanged(true);
    // A trace names the paths it recorded; the finder is what maps them onto
    // this machine, as the run and perf-data paths already do.
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    populateFileFinder(project, project ? project->activeKit() : nullptr);
    d->traceManager.loadFromTraceFile(path);
}

void PerfProfilerTraceBackend::loadPerfData(const FilePath &path, const FilePath &executableDir,
                                            ProjectExplorer::Kit *kit)
{
    emit busyChanged(true);
    d->fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
    d->fileFinder.setSysroot(ProjectExplorer::SysRootKitAspect::sysRoot(kit));
    d->fileFinder.setProjectFiles(sourceFiles());
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
    updateTime(0, 0);
    updateFilterMenu();
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

bool PerfProfilerTraceBackend::isRecording() const
{
    return d->recordButton.isChecked();
}

bool PerfProfilerTraceBackend::isReaderRunning() const
{
    return d->readerRunning;
}

QAction *PerfProfilerTraceBackend::stopAction() const
{
    return &d->stopAction;
}

void PerfProfilerTraceBackend::restrictToSelectedRange()
{
    d->traceManager.restrictByFilter(
        d->traceManager.rangeAndThreadFilter(d->zoomControl.selectionStart(),
                                             d->zoomControl.selectionEnd()));
}

void PerfProfilerTraceBackend::showFullRange()
{
    d->traceManager.restrictByFilter(d->traceManager.rangeAndThreadFilter(-1, -1));
}

milliseconds PerfProfilerTraceBackend::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->traceManager.traceDuration()});
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
    StyleHelper::setPanelWidget(&d->recordButton);
    StyleHelper::setPanelWidget(&d->clearButton);
    StyleHelper::setPanelWidget(&d->filterButton);
    StyleHelper::setPanelWidget(&d->aggregateButton);
    StyleHelper::setPanelWidget(&d->tracePointsButton);
    StyleHelper::setPanelWidget(&d->recordedLabel);
    StyleHelper::setPanelWidget(&d->delayLabel);

    d->recordButton.setCheckable(true);
    QMenu *recordMenu = new QMenu(&d->recordButton);
    connect(recordMenu, &QMenu::aboutToShow, recordMenu, [recordMenu] {
        recordMenu->hide();
        PerfSettings *settings = nullptr;
        ProjectExplorer::Target *target = ProjectExplorer::ProjectManager::startupTarget();
        if (target) {
            if (auto runConfig = ProjectExplorer::activeRunConfigForActiveProject())
                settings = runConfig->currentSettings<PerfSettings>(Constants::PerfSettingsId);
        }
        QWidget *widget = settings ? settings->createPerfConfigWidget(target)
                                   : globalSettings().createPerfConfigWidget(target);
        widget->setWindowFlags(Qt::Dialog);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        widget->show();
    }, Qt::QueuedConnection);
    d->recordButton.setPopupMode(QToolButton::MenuButtonPopup);
    d->recordButton.setMenu(recordMenu);
    connect(&d->recordButton, &QAbstractButton::clicked,
            this, &PerfProfilerTraceBackend::setRecording);
    setRecording(true);

    d->clearButton.setIcon(Icons::CLEAN_TOOLBAR.icon());
    d->clearButton.setToolTip(Tr::tr("Discard data."));
    connect(&d->clearButton, &QAbstractButton::clicked, this, [this] { clear(); });

    d->filterButton.setIcon(Icons::FILTER.icon());
    d->filterButton.setPopupMode(QToolButton::InstantPopup);
    d->filterButton.setProperty(StyleHelper::C_NO_ARROW, true);
    d->filterButton.setMenu(&d->filterMenu);

    d->aggregateButton.setIcon(Icons::EXPAND_ALL_TOOLBAR.icon());
    d->aggregateButton.setCheckable(true);
    connect(&d->aggregateButton, &QAbstractButton::toggled,
            this, &PerfProfilerTraceBackend::setAggregated);
    setAggregated(false);

    d->recordedLabel.setIndent(StyleHelper::SpacingTokens::PaddingHL);
    d->delayLabel.setIndent(StyleHelper::SpacingTokens::PaddingHL);

    d->stopAction.setText(Tr::tr("Stop"));
    d->stopAction.setIcon(Icons::STOP_SMALL_TOOLBAR.icon());
    d->stopAction.setEnabled(false);
    d->stopButton.setDefaultAction(&d->stopAction);

    // Context-menu entries owned by the tool. Fetched here rather than injected
    // by the tool afterwards: views() builds its menus from them, and an editor
    // opened through the file-open path never hears from the tool at all. The
    // standalone viewer has no tool, and no menus to fill either.
    if (PerfProfilerTool *tool = PerfProfilerTool::instance()) {
        d->loadSaveActions = tool->traceMenuActions();
        d->limitToRange = tool->limitToRangeAction();
        d->showFullRange = tool->showFullRangeAction();
    }
}

void PerfProfilerTraceBackend::setRecording(bool recording)
{
    const static QIcon recordOn = ProjectExplorer::Icons::RECORD_ON.icon();
    const static QIcon recordOff = ProjectExplorer::Icons::RECORD_OFF.icon();
    d->recordButton.setToolTip(recording ? Tr::tr("Stop collecting profile data.")
                                         : Tr::tr("Collect profile data."));
    d->recordButton.setIcon(recording ? recordOn : recordOff);
    d->recordButton.setChecked(recording);
    emit recordingChanged(recording);
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
    d->clearButton.setEnabled(on);
    d->filterButton.setEnabled(on);
    d->aggregateButton.setEnabled(on);
    d->filterMenu.setEnabled(on);
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
    const qint64 startTime = d->traceManager.traceStart();
    const qint64 endTime = d->traceManager.traceEnd();
    QTC_ASSERT(endTime >= startTime, return);
    d->zoomControl.setTrace(startTime, endTime);
    d->zoomControl.setRange(startTime, startTime + (endTime - startTime) / 10);
    updateTime(d->zoomControl.traceDuration(), -1);
    updateFilterMenu();
    setToolActionsEnabled(true);
    d->stopAction.setEnabled(false); // The run's data flow has ended.
    emit busyChanged(false);
    emit traceChanged();
}

void PerfProfilerTraceBackend::updateTime(qint64 duration, qint64 delay)
{
    const qint64 e9 = 1e9, e8 = 1e8, ten = 10; // The compiler would cast to double.
    if (duration > 0) {
        d->recordedLabel.setText(
            Tr::tr("Recorded: %1.%2s").arg(duration / e9).arg(qAbs(duration / e8) % ten));
    } else if (duration == 0) {
        d->recordedLabel.clear();
    }

    if (delay > 0) {
        d->delayLabel.setText(
            Tr::tr("Processing delay: %1.%2s").arg(delay / e9).arg(qAbs(delay / e8) % ten));
    } else if (delay == 0) {
        d->delayLabel.clear();
    }
}

static bool operator<(const PerfProfilerTraceManager::Thread &a,
                      const PerfProfilerTraceManager::Thread &b)
{
    return a.tid < b.tid;
}

void PerfProfilerTraceBackend::updateFilterMenu()
{
    d->filterMenu.clear();

    QAction *enableAll = d->filterMenu.addAction(Tr::tr("Enable All"));
    QAction *disableAll = d->filterMenu.addAction(Tr::tr("Disable All"));
    d->filterMenu.addSeparator();

    QList<PerfProfilerTraceManager::Thread> threads = d->traceManager.threads().values();
    std::sort(threads.begin(), threads.end());

    for (const PerfProfilerTraceManager::Thread &thread : std::as_const(threads)) {
        QAction *action = d->filterMenu.addAction(
            QString::fromLatin1("%1 (%2)")
                .arg(QString::fromUtf8(d->traceManager.string(thread.name)))
                .arg(thread.tid));
        action->setCheckable(true);
        action->setData(thread.tid);
        action->setChecked(thread.enabled);
        if (thread.tid == 0) {
            action->setEnabled(false);
        } else {
            connect(action, &QAction::toggled, this, [this, action](bool checked) {
                d->traceManager.setThreadEnabled(action->data().toUInt(), checked);
            });
            connect(enableAll, &QAction::triggered,
                    action, [action] { action->setChecked(true); });
            connect(disableAll, &QAction::triggered,
                    action, [action] { action->setChecked(false); });
        }
    }
}

void PerfProfilerTraceBackend::populateFileFinder(const ProjectExplorer::Project *project,
                                                  const ProjectExplorer::Kit *kit)
{
    d->fileFinder.setProjectFiles(sourceFiles(project));
    if (project)
        d->fileFinder.setProjectDirectory(project->projectDirectory());
    if (kit) {
        d->fileFinder.setAdditionalSearchDirectories(collectQtIncludePaths(kit));
        d->fileFinder.setSysroot(ProjectExplorer::SysRootKitAspect::sysRoot(kit));
    }
}

void PerfProfilerTraceBackend::prepareRun(const ProjectExplorer::Project *project,
                                          const ProjectExplorer::Kit *kit)
{
    populateFileFinder(project, kit);
    d->stopAction.setEnabled(true);
}

} // namespace Profiler::Internal
