// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "ctfvisualizertool.h"

#include "ctftracemanager.h"

#include "ctfstatisticsmodel.h"
#include "ctfstatisticsview.h"
#include "ctftimelinemodel.h"
#include "ctfvisualizertr.h"
#include "ctfvisualizertraceview.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <debugger/analyzer/analyzerconstants.h>

#include <utils/async.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

#include <fstream>

using namespace Core;
using namespace CtfVisualizer::Constants;
using namespace Utils;

namespace CtfVisualizer::Internal {

using json = nlohmann::json;

CtfVisualizerTool::CtfVisualizerTool()
    : m_modelAggregator(new Timeline::TimelineModelAggregator(this))
    , m_zoomControl(new Timeline::TimelineZoomControl(this))
    , m_statisticsModel(new CtfStatisticsModel(this))
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
    connect(m_loadJson.get(), &QAction::triggered, this, [this] {
        QString filename = m_loadJson->data().toString();
        if (filename.isEmpty())
            filename = QFileDialog::getOpenFileName(ICore::dialogParent(),
                                                    Tr::tr("Load Chrome Trace Format File"),
                                                    "",
                                                    Tr::tr("JSON File (*.json)"));
        loadJson(filename);
    });
    options->addAction(command);

    m_perspective.setAboutToActivateCallback([this] { createViews(); });

    m_restrictToThreadsButton->setIcon(Icons::FILTER.icon());
    m_restrictToThreadsButton->setToolTip(Tr::tr("Restrict to Threads"));
    m_restrictToThreadsButton->setPopupMode(QToolButton::InstantPopup);
    m_restrictToThreadsButton->setProperty(StyleHelper::C_NO_ARROW, true);
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
    connect(contextMenu->addAction(Tr::tr("Reset Zoom")), &QAction::triggered, this, [this] {
        m_zoomControl->setRange(m_zoomControl->traceStart(), m_zoomControl->traceEnd());
    });

    m_traceView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_traceView, &QWidget::customContextMenuRequested,
            contextMenu, [contextMenu, this](const QPoint &pos) {
        contextMenu->exec(m_traceView->mapToGlobal(pos));
    });

    m_perspective.addWindow(m_traceView, Perspective::OperationType::SplitVertical, nullptr);

    m_statisticsView = new CtfStatisticsView(m_statisticsModel.get());
    m_statisticsView->setWindowTitle(Tr::tr("Statistics"));
    connect(m_statisticsView, &CtfStatisticsView::eventTypeSelected, this, [this](QString title) {
        int typeId = m_traceManager->getSelectionId(title.toStdString());
        m_traceView->selectByTypeId(typeId);
    });
    connect(m_traceManager.get(), &CtfTraceManager::detailsRequested, m_statisticsView,
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
        action->setChecked(m_traceManager->isRestrictedTo(timelineModel->tid()));
    }
}

void CtfVisualizerTool::toggleThreadRestriction(QAction *action)
{
    const QString tid = action->data().toString();
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

class CtfJsonParserFunctor
{
public:
    CtfJsonParserFunctor(QPromise<json> &promise)
        : m_promise(promise) {}

    bool operator()(int depth, json::parse_event_t event, json &parsed)
    {
        if ((event == json::parse_event_t::array_start && depth == 0)
            || (event == json::parse_event_t::key && depth == 1 && parsed == json(CtfTraceEventsKey))) {
            m_isInTraceArray = true;
            m_traceArrayDepth = depth;
            return true;
        }
        if (m_isInTraceArray && event == json::parse_event_t::array_end && depth == m_traceArrayDepth) {
            m_isInTraceArray = false;
            return false;
        }
        if (m_isInTraceArray && event == json::parse_event_t::object_end && depth == m_traceArrayDepth + 1) {
            m_promise.addResult(parsed);
            return false;
        }
        if (m_isInTraceArray || (event == json::parse_event_t::object_start && depth == 0)) {
            // keep outer object and values in trace objects:
            return true;
        }
        // discard any objects outside of trace array:
        // TODO: parse other data, e.g. stack frames
        return false;
    }

protected:
    QPromise<json> &m_promise;
    bool m_isInTraceArray = false;
    int m_traceArrayDepth = 0;
};

static void load(QPromise<json> &promise, const QString &fileName)
{
    std::ifstream file(fileName.toStdString());
    if (!file.is_open()) {
        promise.future().cancel();
        return;
    }

    CtfJsonParserFunctor functor(promise);
    json::parser_callback_t callback = [&functor](int depth, json::parse_event_t event, json &parsed) {
        return functor(depth, event, parsed);
    };

    try {
        json unusedValues = json::parse(file, callback, /*allow_exceptions*/ false);
    } catch (...) {
        // nlohmann::json can throw exceptions when requesting type that is wrong
    }

    file.close();
}

void CtfVisualizerTool::loadJson(const QString &fileName)
{
    using namespace Tasking;

    if (m_loader || fileName.isEmpty())
        return;

    const auto onSetup = [this, fileName](Async<json> &async) {
        m_traceManager->clearAll();
        async.setConcurrentCallData(load, fileName);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            m_traceManager->addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onDone = [this] {
        m_traceManager->updateStatistics();
        if (m_traceManager->isEmpty()) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("CTF Visualizer"),
                                 Tr::tr("The file does not contain any trace data."));
        } else if (!m_traceManager->errorString().isEmpty()) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("CTF Visualizer"),
                                 m_traceManager->errorString());
        } else {
            m_traceManager->finalize();
            m_perspective.select();
            zoomControl()->setTrace(m_traceManager->traceBegin(), m_traceManager->traceEnd() + m_traceManager->traceDuration() / 20);
            zoomControl()->setRange(m_traceManager->traceBegin(), m_traceManager->traceEnd() + m_traceManager->traceDuration() / 20);
        }
        setAvailableThreads(m_traceManager->getSortedThreads());
        m_loader.release()->deleteLater();
    };
    const auto onError = [this] {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("CTF Visualizer"),
                             Tr::tr("Cannot read the CTF file."));
        m_loader.release()->deleteLater();
    };

    const Group recipe { AsyncTask<json>(onSetup) };
    m_loader.reset(new TaskTree(recipe));
    connect(m_loader.get(), &TaskTree::done, this, onDone);
    connect(m_loader.get(), &TaskTree::errorOccurred, this, onError);
    auto progress = new TaskProgress(m_loader.get());
    progress->setDisplayName(Tr::tr("Loading CTF File"));
    m_loader->start();
}

}  // namespace CtfVisualizer::Internal
