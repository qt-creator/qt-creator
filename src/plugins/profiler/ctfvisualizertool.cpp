// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizertool.h"

#include "ctfloader.h"
#include "ctfstatisticsmodel.h"
#include "ctfstatisticsview.h"
#include "ctftimelinemodel.h"
#include "ctftracemanager.h"
#include "ctfvisualizerconstants.h"
#include "profilertr.h"
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/perspective.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/async.h>
#include <utils/shutdownguard.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QtTaskTree/QSingleTaskTreeRunner>

using namespace Core;
using namespace Profiler::Constants;
using namespace QtTaskTree;
using namespace Utils;

namespace Profiler::Internal {

using json = nlohmann::json;

class CtfVisualizerTool : public QObject
{
public:
    CtfVisualizerTool();
    ~CtfVisualizerTool() { delete m_traceView; delete m_statisticsView; }

    Timeline::TimelineModelAggregator *modelAggregator();
    Timeline::TimelineZoomControl *zoomControl();

    void loadJson(const QString &fileName);
    void loadCtf2(const QString &dirPath);

private:
    void createViews();

    void initialize();
    void finalize();

    void setAvailableThreads(const QList<CtfTimelineModel *> &threads);
    void toggleThreadRestriction(QAction *action);

    Core::Perspective m_perspective{Constants::CtfVisualizerPerspectiveId,
                                     QCoreApplication::translate("QtC::CtfVisualizer",
                                                                 "Chrome Trace Format Visualizer")};

    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    QAction m_loadJson;
    QAction m_loadCtf2;

    Timeline::TimelineModelAggregator m_modelAggregator;
    Timeline::TimelineZoomControl m_zoomControl;
    CtfStatisticsModel m_statisticsModel{nullptr};
    CtfTraceManager m_traceManager{nullptr, &m_modelAggregator, &m_statisticsModel};

    Timeline::TimelineWidget *m_traceView = nullptr;
    CtfStatisticsView *m_statisticsView = nullptr;

    QToolButton m_restrictToThreadsButton;
    QMenu *m_restrictToThreadsMenu = new QMenu(&m_restrictToThreadsButton);
};

CtfVisualizerTool::CtfVisualizerTool()
{
    ActionContainer *menu = ActionManager::actionContainer(Core::Constants::M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(Constants::CtfVisualizerMenuId);
    options->menu()->setTitle(Tr::tr("Chrome Trace Format Viewer"));
    menu->addMenu(options, Core::Constants::G_ANALYZER_REMOTE_TOOLS);
    options->menu()->setEnabled(true);

    const Core::Context globalContext(Core::Constants::C_GLOBAL);

    m_loadJson.setText(Tr::tr("Load JSON File"));
    Core::Command *command = Core::ActionManager::registerAction(&m_loadJson,
                                                                 Constants::CtfVisualizerTaskLoadJson,
                                                                 globalContext);
    connect(&m_loadJson, &QAction::triggered, this, [this] {
        QString filename = m_loadJson.data().toString();
        if (filename.isEmpty())
            filename = QFileDialog::getOpenFileName(ICore::dialogParent(),
                                                    Tr::tr("Load Chrome Trace Format File"),
                                                    "",
                                                    Tr::tr("JSON File (*.json)"));
        loadJson(filename);
    });
    options->addAction(command);

    m_loadCtf2.setText(Tr::tr("Load CTF2 Trace"));
    Core::Command *ctf2Command = Core::ActionManager::registerAction(
        &m_loadCtf2, Constants::CtfVisualizerTaskLoadCtf2, globalContext);
    connect(&m_loadCtf2, &QAction::triggered, this, [this] {
        QString dirPath = m_loadCtf2.data().toString();
        if (dirPath.isEmpty())
            dirPath = QFileDialog::getExistingDirectory(
                ICore::dialogParent(), Tr::tr("Load CTF2 Trace Directory"));
        loadCtf2(dirPath);
    });
    options->addAction(ctf2Command);

    m_perspective.setAboutToActivateCallback([this] { createViews(); });

    m_restrictToThreadsButton.setIcon(Icons::FILTER.icon());
    m_restrictToThreadsButton.setToolTip(Tr::tr("Restrict to Threads"));
    m_restrictToThreadsButton.setPopupMode(QToolButton::InstantPopup);
    m_restrictToThreadsButton.setProperty(StyleHelper::C_NO_ARROW, true);
    m_restrictToThreadsButton.setMenu(m_restrictToThreadsMenu);
    connect(m_restrictToThreadsMenu, &QMenu::triggered,
            this, &CtfVisualizerTool::toggleThreadRestriction);

    m_perspective.addToolBarWidget(&m_restrictToThreadsButton);
}

void CtfVisualizerTool::createViews()
{
    m_traceView = new Timeline::TimelineWidget(&m_modelAggregator, &m_zoomControl, nullptr);
    m_traceView->setObjectName(QLatin1String("CtfVisualizerTraceView"));
    m_traceView->setWindowTitle(Tr::tr("Timeline"));

    QMenu *contextMenu = new QMenu(m_traceView);
    contextMenu->addAction(&m_loadJson);
    contextMenu->addAction(&m_loadCtf2);
    connect(contextMenu->addAction(Tr::tr("Reset Zoom")), &QAction::triggered, this, [this] {
        m_zoomControl.setRange(m_zoomControl.traceStart(), m_zoomControl.traceEnd());
    });

    m_traceView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_traceView, &QWidget::customContextMenuRequested,
            contextMenu, [contextMenu, this](const QPoint &pos) {
        contextMenu->exec(m_traceView->mapToGlobal(pos));
    });

    m_perspective.addWindow(m_traceView, Perspective::OperationType::SplitVertical, nullptr);
    // Split the details off before tabbing other views onto the trace view; otherwise
    // QMainWindow::splitDockWidget() would just add it as a tab (the anchor is tabbed).
    m_perspective.addWindow(m_traceView->rangeDetailsWidget(), Perspective::SplitHorizontal,
                            m_traceView);

    m_statisticsView = new CtfStatisticsView(&m_statisticsModel);
    m_statisticsView->setWindowTitle(Tr::tr("Statistics"));
    connect(m_statisticsView, &CtfStatisticsView::eventTypeSelected, this, [this](QString title) {
        int typeId = m_traceManager.getSelectionId(title.toStdString());
        m_traceView->selectByTypeId(typeId);
    });
    connect(&m_traceManager, &CtfTraceManager::detailsRequested, m_statisticsView,
            &CtfStatisticsView::selectByTitle);

    m_perspective.addWindow(m_statisticsView, Perspective::AddToTab, m_traceView);

    m_perspective.setAboutToActivateCallback(Perspective::Callback());
}

void CtfVisualizerTool::setAvailableThreads(const QList<CtfTimelineModel *> &threads)
{
    m_restrictToThreadsMenu->clear();

    for (auto timelineModel : threads) {
        QAction *action = m_restrictToThreadsMenu->addAction(timelineModel->displayName());
        action->setCheckable(true);
        action->setData(timelineModel->tid());
        action->setChecked(m_traceManager.isRestrictedTo(timelineModel->tid()));
        action->setEnabled(timelineModel->count());
    }
}

void CtfVisualizerTool::toggleThreadRestriction(QAction *action)
{
    const QString tid = action->data().toString();

    // deselect possibly current event
    // (avoids crashes as next / previous would act afterwards on different or even nullptr models)
    m_traceView->selectByIndices(-1, -1);

    m_traceManager.setThreadRestriction(tid, action->isChecked());
}

Timeline::TimelineModelAggregator *CtfVisualizerTool::modelAggregator()
{
    return &m_modelAggregator;
}

Timeline::TimelineZoomControl *CtfVisualizerTool::zoomControl()
{
    return &m_zoomControl;
}

void CtfVisualizerTool::loadJson(const QString &fileName)
{
    if (m_taskTreeRunner.isRunning() || fileName.isEmpty())
        return;

    const auto onSetup = [this, fileName](Async<json> &async) {
        m_traceManager.clearAll();
        async.setConcurrentCallData(loadChromeJson, fileName);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            m_traceManager.addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onTaskTreeSetup = [](QTaskTree &taskTree) {
        auto progress = new TaskProgress(&taskTree);
        progress->setDisplayName(Tr::tr("Loading CTF File"));
    };
    const auto onTaskTreeDone = [this](DoneWith result) {
        if (result == DoneWith::Success) {
            m_traceManager.updateStatistics();
            if (m_traceManager.isEmpty()) {
                QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("CTF Visualizer"),
                                     Tr::tr("The file does not contain any trace data."));
            } else if (!m_traceManager.errorString().isEmpty()) {
                QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("CTF Visualizer"),
                                     m_traceManager.errorString());
            } else {
                m_traceManager.finalize();
                m_perspective.select();
                const auto end = m_traceManager.traceEnd() + m_traceManager.traceDuration() / 20;
                m_zoomControl.setTrace(m_traceManager.traceBegin(), end);
                m_zoomControl.setRange(m_traceManager.traceBegin(), end);
            }
            setAvailableThreads(m_traceManager.getSortedThreads());
        } else {
            QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("CTF Visualizer"),
                                 Tr::tr("Cannot read the CTF file."));
        }
    };
    m_taskTreeRunner.start({AsyncTask<json>(onSetup)}, onTaskTreeSetup, onTaskTreeDone);
}

void CtfVisualizerTool::loadCtf2(const QString &dirPath)
{
    if (m_taskTreeRunner.isRunning() || dirPath.isEmpty())
        return;

    const auto onSetup = [this, dirPath](Async<json> &async) {
        m_traceManager.clearAll();
        async.setConcurrentCallData(loadCtf2Data, dirPath);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            m_traceManager.addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onTaskTreeSetup = [](QTaskTree &taskTree) {
        auto progress = new TaskProgress(&taskTree);
        progress->setDisplayName(Tr::tr("Loading CTF2 Trace"));
    };
    const auto onTaskTreeDone = [this](DoneWith result) {
        if (result == DoneWith::Success) {
            m_traceManager.updateStatistics();
            if (m_traceManager.isEmpty()) {
                QMessageBox::warning(
                    Core::ICore::dialogParent(),
                    Tr::tr("CTF Visualizer"),
                    Tr::tr("The trace does not contain any trace data."));
            } else if (!m_traceManager.errorString().isEmpty()) {
                QMessageBox::warning(
                    Core::ICore::dialogParent(),
                    Tr::tr("CTF Visualizer"),
                    m_traceManager.errorString());
            } else {
                m_traceManager.finalize();
                m_perspective.select();
                const auto end = m_traceManager.traceEnd() + m_traceManager.traceDuration() / 20;
                zoomControl()->setTrace(m_traceManager.traceBegin(), end);
                zoomControl()->setRange(m_traceManager.traceBegin(), end);
            }
            setAvailableThreads(m_traceManager.getSortedThreads());
        } else {
            QMessageBox::warning(
                Core::ICore::dialogParent(),
                Tr::tr("CTF Visualizer"),
                Tr::tr("Cannot read the CTF2 trace."));
        }
    };
    m_taskTreeRunner.start({AsyncTask<json>(onSetup)}, onTaskTreeSetup, onTaskTreeDone);
}

void setupCtfVisualizerTool()
{
    static GuardedObject<CtfVisualizerTool> theTool;
}

} // namespace Profiler::Internal
