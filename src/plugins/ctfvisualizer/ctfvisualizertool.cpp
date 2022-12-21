// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "ctfvisualizertool.h"

#include "ctftracemanager.h"

#include "ctfstatisticsmodel.h"
#include "ctfstatisticsview.h"
#include "ctftimelinemodel.h"
#include "ctfvisualizertraceview.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <debugger/analyzer/analyzerconstants.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFutureInterface>
#include <QMenu>
#include <QMessageBox>
#include <QThread>

using namespace Core;
using namespace CtfVisualizer::Constants;


namespace CtfVisualizer {
namespace Internal {

CtfVisualizerTool::CtfVisualizerTool()
    : QObject (nullptr)
    , m_isLoading(false)
    , m_loadJson(nullptr)
    , m_traceView(nullptr)
    , m_modelAggregator(new Timeline::TimelineModelAggregator(this))
    , m_zoomControl(new Timeline::TimelineZoomControl(this))
    , m_statisticsModel(new CtfStatisticsModel(this))
    , m_statisticsView(nullptr)
    , m_traceManager(new CtfTraceManager(this, m_modelAggregator.get(), m_statisticsModel.get()))
    , m_restrictToThreadsButton(new QToolButton)
    , m_restrictToThreadsMenu(new QMenu(m_restrictToThreadsButton))
{
    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(Constants::CtfVisualizerMenuId);
    options->menu()->setTitle(Tr::tr("Chrome Trace Format Viewer"));
    menu->addMenu(options, Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    options->menu()->setEnabled(true);

    const Core::Context globalContext(Core::Constants::C_GLOBAL);

    m_loadJson.reset(new QAction(Tr::tr("Load JSON File"), options));
    Core::Command *command = Core::ActionManager::registerAction(m_loadJson.get(), Constants::CtfVisualizerTaskLoadJson,
                                                  globalContext);
    connect(m_loadJson.get(), &QAction::triggered, this, &CtfVisualizerTool::loadJson);
    options->addAction(command);

    m_perspective.setAboutToActivateCallback([this]() { createViews(); });

    m_restrictToThreadsButton->setIcon(Utils::Icons::FILTER.icon());
    m_restrictToThreadsButton->setToolTip(Tr::tr("Restrict to Threads"));
    m_restrictToThreadsButton->setPopupMode(QToolButton::InstantPopup);
    m_restrictToThreadsButton->setProperty("noArrow", true);
    m_restrictToThreadsButton->setMenu(m_restrictToThreadsMenu);
    connect(m_restrictToThreadsMenu, &QMenu::triggered,
            this, &CtfVisualizerTool::toggleThreadRestriction);

    m_perspective.addToolBarWidget(m_restrictToThreadsButton);
}

CtfVisualizerTool::~CtfVisualizerTool() = default;

void CtfVisualizerTool::createViews()
{
    m_traceView = new CtfVisualizerTraceView(nullptr, this);
    m_traceView->setWindowTitle(Tr::tr("Timeline"));

    QMenu *contextMenu = new QMenu(m_traceView);
    contextMenu->addAction(m_loadJson.get());
    connect(contextMenu->addAction(Tr::tr("Reset Zoom")), &QAction::triggered, this, [this](){
        m_zoomControl->setRange(m_zoomControl->traceStart(), m_zoomControl->traceEnd());
    });

    m_traceView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_traceView, &QWidget::customContextMenuRequested,
            contextMenu, [contextMenu, this](const QPoint &pos) {
        contextMenu->exec(m_traceView->mapToGlobal(pos));
    });

    m_perspective.addWindow(m_traceView, Utils::Perspective::OperationType::SplitVertical, nullptr);

    m_statisticsView = new CtfStatisticsView(m_statisticsModel.get());
    m_statisticsView->setWindowTitle(Tr::tr("Statistics"));
    connect(m_statisticsView, &CtfStatisticsView::eventTypeSelected, [this] (QString title)
    {
        int typeId = m_traceManager->getSelectionId(title.toStdString());
        m_traceView->selectByTypeId(typeId);
    });
    connect(m_traceManager.get(), &CtfTraceManager::detailsRequested, m_statisticsView,
            &CtfStatisticsView::selectByTitle);

    m_perspective.addWindow(m_statisticsView, Utils::Perspective::AddToTab, m_traceView);

    m_perspective.setAboutToActivateCallback(Utils::Perspective::Callback());
}

void CtfVisualizerTool::setAvailableThreads(const QList<CtfTimelineModel *> &threads)
{
    m_restrictToThreadsMenu->clear();

    for (auto timelineModel : threads) {
        QAction *action = m_restrictToThreadsMenu->addAction(timelineModel->displayName());
        action->setCheckable(true);
        action->setData(timelineModel->tid());
        action->setChecked(m_traceManager->isRestrictedTo(timelineModel->tid()));
    }
}

void CtfVisualizerTool::toggleThreadRestriction(QAction *action)
{
    const int tid = action->data().toInt();
    m_traceManager->setThreadRestriction(tid, action->isChecked());
}

Timeline::TimelineModelAggregator *CtfVisualizerTool::modelAggregator() const
{
    return m_modelAggregator.get();
}

CtfTraceManager *CtfVisualizerTool::traceManager() const
{
    return m_traceManager.get();
}

Timeline::TimelineZoomControl *CtfVisualizerTool::zoomControl() const
{
    return m_zoomControl.get();
}

void CtfVisualizerTool::loadJson()
{
    if (m_isLoading)
        return;

    m_isLoading = true;

    QString filename = QFileDialog::getOpenFileName(
                ICore::dialogParent(), Tr::tr("Load Chrome Trace Format File"),
                "", Tr::tr("JSON File (*.json)"));
    if (filename.isEmpty()) {
        m_isLoading = false;
        return;
    }

    auto *futureInterface = new QFutureInterface<void>();
    auto *task = new QFuture<void>(futureInterface);

    QThread *thread = QThread::create([this, filename, futureInterface]() {
        m_traceManager->load(filename);

        m_modelAggregator->moveToThread(QApplication::instance()->thread());
        m_modelAggregator->setParent(this);
        futureInterface->reportFinished();
    });

    connect(thread, &QThread::finished, this, [this, thread, task, futureInterface]() {
        // in main thread:
        if (m_traceManager->isEmpty()) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("CTF Visualizer"),
                                 Tr::tr("The file does not contain any trace data."));
        } else {
            m_traceManager->finalize();
            m_perspective.select();
            zoomControl()->setTrace(m_traceManager->traceBegin(), m_traceManager->traceEnd() + m_traceManager->traceDuration() / 20);
            zoomControl()->setRange(m_traceManager->traceBegin(), m_traceManager->traceEnd() + m_traceManager->traceDuration() / 20);
        }
        setAvailableThreads(m_traceManager->getSortedThreads());
        thread->deleteLater();
        delete task;
        delete futureInterface;
        m_isLoading = false;
    }, Qt::QueuedConnection);

    m_modelAggregator->setParent(nullptr);
    m_modelAggregator->moveToThread(thread);

    thread->start();
    Core::ProgressManager::addTask(*task, Tr::tr("Loading CTF File"), CtfVisualizerTaskLoadJson);
}

}  // namespace Internal
}  // namespace CtfVisualizer
