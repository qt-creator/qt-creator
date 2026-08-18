// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertool.h"

#include "perfloaddialog.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"
#include "perfprofilertracebackend.h"
#include "perfprofilertracemanager.h"
#include "perftracepointdialog.h"
#include "profilermode.h"
#include "profilertracedocument.h"
#include "profilertraceeditor.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/action.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>

#include <QAction>
#include <QMenu>
#include <QPointer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Profiler::Constants;
using namespace Utils;

namespace Profiler::Internal {

class PerfProfilerToolPrivate
{
public:
    // The document a run records into. Each run gets its own.
    QPointer<ProfilerTraceDocument> liveDocument;
    int runCount = 0;

    QAction m_startAction;
    Utils::Action m_loadPerfData;
    Utils::Action m_loadTrace;
    Utils::Action m_saveTrace;
    Utils::Action m_limitToRange;
    Utils::Action m_showFullRange;
    QAction *m_tracePointsAction = nullptr;

    bool m_processRunning = false;
};

static PerfProfilerTool *s_instance;

PerfProfilerTool::PerfProfilerTool()
    : d(new PerfProfilerToolPrivate)
{
    s_instance = this;

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
            if (PerfProfilerTraceBackend *backend = currentBackend())
                backend->restrictToSelectedRange();
        });

    ActionBuilder(options, Constants::PerfProfilerTaskFullRange)
        .adopt(&d->m_showFullRange)
        .setText(Tr::tr("Show Full Range"))
        .addToContainer(subMenu)
        .addOnTriggered(this, [this] {
            if (PerfProfilerTraceBackend *backend = currentBackend())
                backend->showFullRange();
        });

    ActionBuilder(options, Constants::PerfProfilerTaskTracePoints)
        .setText(Tr::tr("Create Memory Trace Points"))
        .bindContextAction(&d->m_tracePointsAction)
        .setIcon(ProjectExplorer::Icons::TRACEPOINT_TOOLBAR.icon())
        .setIconVisibleInMenu(false)
        .setToolTip(Tr::tr("Create trace points for memory profiling on the target device."))
        .addToContainer(subMenu)
        .addOnTriggered(this, &PerfProfilerTool::createTracePoints);

    QAction *action = nullptr;
    ActionBuilder(this, Constants::PerfProfilerLocalActionId)
        .setText(Tr::tr("Performance Analyzer"))
        .bindContextAction(&action)
        .setToolTip(Tr::tr("Finds performance bottlenecks."))
        .addToContainer(Core::Constants::M_DEBUG_ANALYZER, Core::Constants::G_ANALYZER_TOOLS)
        .addOnTriggered(this, &PerfProfilerTool::profileStartupProject);

    d->m_startAction.setText(Tr::tr("Start"));
    d->m_startAction.setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR.icon());
    QObject::connect(&d->m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(&d->m_startAction, &QAction::changed, action, [this, action] {
        action->setEnabled(d->m_startAction.isEnabled());
        d->m_tracePointsAction->setEnabled(d->m_startAction.isEnabled());
    });

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &PerfProfilerTool::updateRunActions);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &PerfProfilerTool::updateRunActions);
    updateRunActions();
}

PerfProfilerTool::~PerfProfilerTool()
{
    delete d;
    s_instance = nullptr;
}

PerfProfilerTool *PerfProfilerTool::instance()
{
    return s_instance;
}

PerfProfilerTraceBackend *PerfProfilerTool::liveBackend() const
{
    if (!d->liveDocument || d->liveDocument->backends().isEmpty())
        return nullptr;
    return qobject_cast<PerfProfilerTraceBackend *>(d->liveDocument->backends().first());
}

void PerfProfilerTool::profileStartupProject()
{
    activateProfilerMode();
    ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
}

PerfProfilerTraceBackend *PerfProfilerTool::currentBackend() const
{
    if (auto trace = qobject_cast<ProfilerTraceDocument *>(EditorManager::currentDocument())) {
        if (!trace->backends().isEmpty()) {
            if (auto backend = qobject_cast<PerfProfilerTraceBackend *>(trace->backends().first()))
                return backend;
        }
    }
    return liveBackend();
}

bool PerfProfilerTool::isRecording() const
{
    PerfProfilerTraceBackend *backend = liveBackend();
    return backend && backend->isRecording();
}

void PerfProfilerTool::onWorkerCreation(RunControl *runControl)
{
    d->liveDocument = openLiveTrace(TraceFormat::Perf,
                                    Tr::tr("Performance Analysis %1").arg(++d->runCount),
                                    QString("PerfProfiler.Run.%1").arg(d->runCount));
    PerfProfilerTraceBackend *backend = liveBackend();
    QTC_ASSERT(backend, return);

    connect(backend->stopAction(), &QAction::triggered, runControl, &RunControl::initiateStop);
    emit liveBackendChanged(backend);

    backend->prepareRun(runControl->project(), runControl->kit());
    updateRunActions();
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

void PerfProfilerTool::updateTime(qint64 duration, qint64 delay)
{
    if (PerfProfilerTraceBackend *backend = liveBackend())
        backend->updateTime(duration, delay);
}

void PerfProfilerTool::updateRunActions()
{
    PerfProfilerTraceBackend *backend = liveBackend();
    const bool readerRunning = backend && backend->isReaderRunning();
    if (readerRunning || d->m_processRunning) {
        d->m_startAction.setEnabled(false);
        d->m_startAction.setToolTip(Tr::tr("A performance analysis is still in progress."));
        d->m_loadPerfData.setEnabled(false);
        d->m_loadTrace.setEnabled(false);
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(
            ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
        d->m_startAction.setToolTip(canRun ? Tr::tr("Start a performance analysis.")
                                           : canRun.error());
        d->m_startAction.setEnabled(canRun.has_value());
        d->m_loadPerfData.setEnabled(true);
        d->m_loadTrace.setEnabled(true);
    }

    PerfProfilerTraceBackend *current = currentBackend();
    // Saving writes the document being shown (see showSaveTraceDialog), so the
    // liveBackend() fallback in currentBackend() must not enable it: a QML
    // trace would end up in a .ptq.
    const auto shown = qobject_cast<ProfilerTraceDocument *>(EditorManager::currentDocument());
    d->m_saveTrace.setEnabled(shown && shown->format() == TraceFormat::Perf
                              && current && !current->isEmpty());
    d->m_limitToRange.setEnabled(current);
    d->m_showFullRange.setEnabled(current);
}

// Opens an editor for a trace that is loaded rather than recorded.
PerfProfilerTraceBackend *PerfProfilerTool::openLoadedTrace()
{
    d->liveDocument = openLiveTrace(TraceFormat::Perf,
                                    Tr::tr("Performance Analysis %1").arg(++d->runCount),
                                    QString("PerfProfiler.Load.%1").arg(d->runCount));
    return liveBackend();
}

QList<QAction *> PerfProfilerTool::traceMenuActions() const
{
    return {&d->m_loadPerfData, &d->m_loadTrace, &d->m_saveTrace};
}

QAction *PerfProfilerTool::limitToRangeAction() const
{
    return &d->m_limitToRange;
}

QAction *PerfProfilerTool::showFullRangeAction() const
{
    return &d->m_showFullRange;
}

void PerfProfilerTool::showLoadPerfDialog()
{
    PerfLoadDialog dlg(ICore::dialogParent());
    if (dlg.exec() != PerfLoadDialog::Accepted)
        return;

    if (PerfProfilerTraceBackend *backend = openLoadedTrace()) {
        backend->loadPerfData(FilePath::fromUserInput(dlg.traceFilePath()),
                              FilePath::fromUserInput(dlg.executableDirPath()), dlg.kit());
    }
}

void PerfProfilerTool::loadTraceFile(const FilePath &filePath)
{
    openTraceFile(filePath);
}

void PerfProfilerTool::showLoadTraceDialog()
{
    const FilePath filePath = FileUtils::getOpenFilePath(Tr::tr("Load Trace File"), {},
                                                         Tr::tr("Trace File (*.ptq)"));
    if (!filePath.isEmpty())
        loadTraceFile(filePath);
}

void PerfProfilerTool::showSaveTraceDialog()
{
    auto trace = qobject_cast<ProfilerTraceDocument *>(EditorManager::currentDocument());
    if (!trace || trace->format() != TraceFormat::Perf)
        return;

    FilePath filePath = FileUtils::getSaveFilePath(Tr::tr("Save Trace File"), {},
                                                   Tr::tr("Trace File (*.ptq)"));
    if (filePath.isEmpty())
        return;
    if (!filePath.endsWith(".ptq"))
        filePath = filePath.stringAppended(".ptq");
    trace->save(filePath);
}

void PerfProfilerTool::createTracePoints()
{
    PerfTracePointDialog dialog;
    dialog.exec();
}

void setupPerfProfilerTool()
{
    static GuardedObject<PerfProfilerTool> thePerfProfilerTool;
}

void destroyPerfProfilerTool()
{
}

} // namespace Profiler::Internal
