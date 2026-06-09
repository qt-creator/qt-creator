// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizertool.h"

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

#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <commontraceformat/binary/fieldvalue.h>
#include <commontraceformat/stream/datastreamreader.h>
#include <commontraceformat/stream/tracedirectory.h>
#include <commontraceformat/stream/tracereader.h>

#include <utils/async.h>
#include <utils/shutdownguard.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QtCore/qendian.h>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <fstream>
#include <set>
#include <string_view>
#include <unordered_map>

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

    QToolButton *const m_restrictToThreadsButton;
    QMenu *const m_restrictToThreadsMenu;
};

CtfVisualizerTool::CtfVisualizerTool()
    : m_restrictToThreadsButton(new QToolButton)
    , m_restrictToThreadsMenu(new QMenu(m_restrictToThreadsButton))
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

    m_restrictToThreadsButton->setIcon(Icons::FILTER.icon());
    m_restrictToThreadsButton->setToolTip(Tr::tr("Restrict to Threads"));
    m_restrictToThreadsButton->setPopupMode(QToolButton::InstantPopup);
    m_restrictToThreadsButton->setProperty(StyleHelper::C_NO_ARROW, true);
    m_restrictToThreadsButton->setMenu(m_restrictToThreadsMenu);
    connect(m_restrictToThreadsMenu, &QMenu::triggered,
            this, &CtfVisualizerTool::toggleThreadRestriction);

    m_perspective.addToolBarWidget(m_restrictToThreadsButton);
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
    if (m_taskTreeRunner.isRunning() || fileName.isEmpty())
        return;

    const auto onSetup = [this, fileName](Async<json> &async) {
        m_traceManager.clearAll();
        async.setConcurrentCallData(load, fileName);
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

static std::optional<std::string> fieldValueToString(const CommonTraceFormat::FieldValue &fv)
{
    return std::visit(
        [](const auto &v) -> std::optional<std::string> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, quint64>)
                return std::to_string(v);
            if constexpr (std::is_same_v<T, qint64>)
                return std::to_string(v);
            if constexpr (std::is_same_v<T, double>)
                return std::to_string(v);
            if constexpr (std::is_same_v<T, bool>)
                return v ? std::string("true") : std::string("false");
            if constexpr (std::is_same_v<T, QString>)
                return v.toStdString();
            return std::nullopt;
        },
        fv);
}

static json fieldValueToJson(const CommonTraceFormat::FieldValue &fv);

static json structureValueToJson(const CommonTraceFormat::StructureValue &sv)
{
    json obj = json::object();
    for (const QString &key : sv.order)
        obj[key.toStdString()] = fieldValueToJson(sv.members.value(key));
    return obj;
}

static json fieldValueToJson(const CommonTraceFormat::FieldValue &fv)
{
    return std::visit(
        [](const auto &v) -> json {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>)
                return nullptr;
            if constexpr (std::is_same_v<T, bool>)
                return v;
            if constexpr (std::is_same_v<T, quint64>)
                return v;
            if constexpr (std::is_same_v<T, qint64>)
                return v;
            if constexpr (std::is_same_v<T, double>)
                return v;
            if constexpr (std::is_same_v<T, QString>)
                return v.toStdString();
            if constexpr (std::is_same_v<T, QByteArray>)
                return v.toHex().toStdString();
            if constexpr (std::is_same_v<T, std::shared_ptr<CommonTraceFormat::StructureValue>>)
                return structureValueToJson(*v);
            if constexpr (std::is_same_v<T, std::shared_ptr<CommonTraceFormat::ArrayValue>>) {
                json arr = json::array();
                for (const auto &elem : v->elements)
                    arr.push_back(fieldValueToJson(elem));
                return arr;
            }
            if constexpr (std::is_same_v<T, std::shared_ptr<CommonTraceFormat::VariantValue>>)
                return v->value ? fieldValueToJson(*v->value) : json(nullptr);
            return nullptr;
        },
        fv);
}

static void loadCtf2Data(QPromise<json> &promise, const QString &dirPath)
{
    using namespace Qt::StringLiterals;
    using namespace Profiler::Constants;
    using namespace CommonTraceFormat;

    // TraceDirectory handles metadata discovery (domain/per-PID/session-rotation
    // subdirectories), rotated-tracefile concatenation, and per-packet data
    // stream class selection.
    auto tdResult = TraceDirectory::open(dirPath);
    if (!tdResult) {
        promise.future().cancel();
        return;
    }
    const TraceDirectory &traceDir = *tdResult;

    // Collect all events first so we can sort by timestamp before emitting.
    // CtfTraceManager sets the global time offset from the first event it receives,
    // so events must arrive in chronological order to avoid a negative trace range
    // when multiple streams cover different (possibly overlapping) time windows.
    std::vector<json> events;

    auto parseDur = [](const std::string &s) -> std::optional<double> {
        char *end = nullptr;
        const double val = std::strtod(s.c_str(), &end);
        if (end == s.c_str() || *end != '\0')
            return std::nullopt;
        return val;
    };

    // Kernel CTF traces (e.g. LTTng) carry no per-event pid/tid context: every
    // event would otherwise share a single per-CPU (stream) lane. Reconstruct
    // the thread running on each CPU from `sched_switch` and recover human
    // names from `sched_switch` (prev_comm/next_comm) and the LTTng statedump
    // (lttng_statedump_process_state: tid/pid/name). These maps are filled while
    // streaming and applied in a post-pass so ordering does not matter.
    std::unordered_map<std::string, std::string> pidByTid; // thread -> owning process
    std::unordered_map<std::string, std::string> nameByTid; // thread -> command name
    std::unordered_map<std::string, std::string> nameByPid; // process -> command name

    quint64 fallbackStreamId = 0;
    for (const TraceDirectory::Trace &trace : traceDir.traces()) {
        const Schema &schema = *trace.schema;
        for (const TraceDirectory::Stream &sf : trace.streams) {
            const DataStreamClass *dsc = sf.dsc;
            if (!dsc)
                continue;

            // Resolve clock frequency and offset-from-origin for µs timestamp
            // conversion (spec 5.7).
            quint64 frequency = 1000000000ULL;
            qint64 offsetSeconds = 0;
            quint64 offsetCycles = 0;
            if (!dsc->defaultClockClassName.isEmpty()) {
                for (const ClockClass &cc : schema.clockClasses) {
                    // CTF2 links by clock class id; legacy/TSDL schemas link by name.
                    const bool matches = cc.id == dsc->defaultClockClassName
                                         || cc.name == dsc->defaultClockClassName;
                    if (matches && cc.frequency > 0) {
                        frequency = cc.frequency;
                        offsetSeconds = cc.offsetSeconds;
                        offsetCycles = cc.offsetCycles;
                        break;
                    }
                }
            }

            // Stream instance id, used as a pid/tid lane fallback.
            const quint64 streamId = sf.streamId ? *sf.streamId : fallbackStreamId++;
            DataStreamReader *stream = sf.reader;

            // Thread currently scheduled on this CPU (stream), updated by
            // sched_switch. Empty until the first switch is seen.
            std::string currentTid;

            while (!stream->atEnd()) {
                if (promise.isCanceled())
                    return;

                auto eventResult = stream->nextEvent();
                if (!eventResult)
                    break;

                const EventRecord &rec = *eventResult;
                // Cycles → seconds from the clock origin, then µs (spec 5.7):
                // offsetSeconds + (cycles + offsetCycles) / frequency.
                const double ts = (double(offsetSeconds)
                                   + (double(rec.timestamp) + double(offsetCycles))
                                         / double(frequency))
                                  * 1.0e6;

                json event;
                std::string evName = rec.eventClass ? rec.eventClass->name.toStdString()
                                                    : std::to_string(rec.eventClassId);
                event[CtfTracingClockTimestampKey] = ts;

                auto findStrField = [&](const char *key) -> std::optional<std::string> {
                    for (const StructureValue *sv :
                         {&rec.commonContext, &rec.header, &rec.payload, &rec.specificContext}) {
                        if (const FieldValue *fv = sv->get(QString::fromUtf8(key)))
                            if (auto s = fieldValueToString(*fv))
                                return s;
                    }
                    return std::nullopt;
                };
                // Lane identity comes from event *context* only. A payload "pid"/
                // "tid" (e.g. the subject of an LTTng statedump or the prev/next
                // thread of a sched_switch) describes another entity, not the
                // thread that produced the event.
                auto findCtxField = [&](const char *key) -> std::optional<std::string> {
                    for (const StructureValue *sv :
                         {&rec.commonContext, &rec.specificContext, &rec.header}) {
                        if (const FieldValue *fv = sv->get(QString::fromUtf8(key)))
                            if (auto s = fieldValueToString(*fv))
                                return s;
                    }
                    return std::nullopt;
                };

                auto ctxPid = findCtxField("pid");
                auto ctxTid = findCtxField("tid");
                if (ctxTid || ctxPid) {
                    // Traces with per-event context (e.g. Qt tracegen): trust it.
                    const std::string lanePid = ctxPid ? *ctxPid : std::to_string(streamId);
                    event[CtfProcessIdKey] = lanePid;
                    event[CtfThreadIdKey] = ctxTid ? *ctxTid : lanePid;
                } else {
                    // Kernel-style trace: harvest names and reconstruct the
                    // running thread per CPU. The owning process id is resolved
                    // in the post-pass once the statedump has been fully read.
                    if (evName == "sched_switch") {
                        auto pt = findStrField("prev_tid");
                        auto nt = findStrField("next_tid");
                        if (auto pc = findStrField("prev_comm"); pt && pc)
                            nameByTid[*pt] = *pc;
                        if (auto nc = findStrField("next_comm"); nt && nc)
                            nameByTid[*nt] = *nc;
                        // The event marks the hand-over: attribute it to the
                        // outgoing thread, then run the incoming one.
                        if (pt)
                            currentTid = *pt;
                        event[CtfThreadIdKey] = currentTid.empty() ? std::to_string(streamId)
                                                                   : currentTid;
                        if (nt)
                            currentTid = *nt;
                    } else {
                        if (evName == "lttng_statedump_process_state") {
                            auto st = findStrField("tid");
                            auto sp = findStrField("pid");
                            auto sn = findStrField("name");
                            if (st && sp)
                                pidByTid[*st] = *sp;
                            if (st && sn)
                                nameByTid[*st] = *sn;
                            if (sp && sn)
                                nameByPid[*sp] = *sn;
                        }
                        event[CtfThreadIdKey] = currentTid.empty() ? std::to_string(streamId)
                                                                   : currentTid;
                    }
                    // CtfProcessIdKey filled in the post-pass.
                }

                auto ph = findStrField("ph");
                auto dur = findStrField("dur");

                // CTF events are instantaneous; the Chrome-format timeline needs a
                // span. Derive Begin/End phases from the common entry/exit
                // tracepoint naming conventions (Qt tracegen "<name>_entry"/"_exit",
                // LTTng "syscall_entry_*"/"syscall_exit_*") so paired events render
                // with a duration. The base name (suffix/prefix stripped) is used so
                // an entry and its exit share a display name and selection id.
                auto stripSuffix = [](std::string &n, std::string_view suf) {
                    if (n.size() > suf.size()
                        && n.compare(n.size() - suf.size(), suf.size(), suf) == 0) {
                        n.resize(n.size() - suf.size());
                        return true;
                    }
                    return false;
                };
                auto stripPrefix = [](std::string &n, std::string_view pre) {
                    if (n.size() > pre.size() && n.compare(0, pre.size(), pre) == 0) {
                        n.erase(0, pre.size());
                        return true;
                    }
                    return false;
                };

                if (ph) {
                    event[CtfEventPhaseKey] = *ph;
                    if (*ph == CtfEventTypeComplete && dur)
                        if (auto d = parseDur(*dur))
                            event[CtfDurationKey] = *d;
                } else if (dur) {
                    if (auto d = parseDur(*dur)) {
                        event[CtfEventPhaseKey] = std::string(CtfEventTypeComplete);
                        event[CtfDurationKey] = *d;
                    } else {
                        event[CtfEventPhaseKey] = std::string(CtfEventTypeInstant);
                    }
                } else if (stripSuffix(evName, "_entry") || stripPrefix(evName, "syscall_entry_")) {
                    event[CtfEventPhaseKey] = std::string(CtfEventTypeBegin);
                } else if (stripSuffix(evName, "_exit") || stripPrefix(evName, "syscall_exit_")) {
                    event[CtfEventPhaseKey] = std::string(CtfEventTypeEnd);
                } else {
                    event[CtfEventPhaseKey] = std::string(CtfEventTypeInstant);
                }

                event[CtfEventNameKey] = evName;

                if (!rec.payload.order.isEmpty())
                    event["args"] = structureValueToJson(rec.payload);

                events.push_back(std::move(event));
            }
        }
    }

    std::sort(events.begin(), events.end(), [&](const json &a, const json &b) {
        return a.value(CtfTracingClockTimestampKey, 0.0)
               < b.value(CtfTracingClockTimestampKey, 0.0);
    });

    // Pair entry/exit events into Complete (duration) events here rather than
    // leaning on the timeline model's blind LIFO matching. Kernel syscalls do
    // not nest within a lane (a thread can block in one syscall while another
    // runs), and traces without per-event thread context lump every event on a
    // CPU into one lane -- so the model would close the wrong begin (e.g. an
    // exit_ppoll ending a stale entry_futex left open after a thread migrated to
    // another CPU), producing absurd multi-hour spans. Match by (lane, base
    // name) so an end only closes a begin of the same tracepoint on the same
    // lane; unmatched begins/ends degrade to instant events.
    {
        std::unordered_map<std::string, std::vector<size_t>> openByKey;
        std::vector<bool> drop(events.size(), false);
        auto laneKey = [](const json &e) {
            std::string k = e.contains(CtfThreadIdKey) ? e[CtfThreadIdKey].dump() : std::string();
            k += '\0';
            k += e.value(CtfEventNameKey, std::string());
            return k;
        };
        for (size_t i = 0; i < events.size(); ++i) {
            const std::string ph = events[i].value(CtfEventPhaseKey, std::string());
            if (ph == CtfEventTypeBegin) {
                openByKey[laneKey(events[i])].push_back(i);
            } else if (ph == CtfEventTypeEnd) {
                auto it = openByKey.find(laneKey(events[i]));
                if (it != openByKey.end() && !it->second.empty()) {
                    const size_t b = it->second.back();
                    it->second.pop_back();
                    const double begin = events[b].value(CtfTracingClockTimestampKey, 0.0);
                    const double end = events[i].value(CtfTracingClockTimestampKey, 0.0);
                    events[b][CtfEventPhaseKey] = std::string(CtfEventTypeComplete);
                    events[b][CtfDurationKey] = end - begin;
                    drop[i] = true; // folded into the begin's duration
                } else {
                    events[i][CtfEventPhaseKey] = std::string(CtfEventTypeInstant);
                }
            }
        }
        for (auto &[key, stack] : openByKey)
            for (size_t b : stack)
                events[b][CtfEventPhaseKey] = std::string(CtfEventTypeInstant);

        std::vector<json> kept;
        kept.reserve(events.size());
        for (size_t i = 0; i < events.size(); ++i) {
            if (!drop[i])
                kept.push_back(std::move(events[i]));
        }
        events.swap(kept);
    }

    // Resolve the owning process for every reconstructed (context-less) lane and
    // synthesize Chrome-format process_name/thread_name metadata so the timeline
    // models pick up real names. A thread whose process is unknown is treated as
    // its own process (pid == tid).
    std::vector<json> metadata;
    {
        std::set<std::string> laneTids;
        for (json &ev : events) {
            if (ev.contains(CtfProcessIdKey))
                continue; // explicit-context lane, already complete
            const std::string tid = ev.value(CtfThreadIdKey, std::string());
            auto it = pidByTid.find(tid);
            ev[CtfProcessIdKey] = (it != pidByTid.end()) ? it->second : tid;
            laneTids.insert(tid);
        }

        const double t0 = events.empty()
                              ? 0.0
                              : events.front().value(CtfTracingClockTimestampKey, 0.0);
        const auto metadataEvent = [&](const char *name, const std::string &pid,
                                       const std::string &tid, const std::string &value) {
            json ev;
            ev[CtfTracingClockTimestampKey] = t0;
            ev[CtfEventPhaseKey] = std::string(CtfEventTypeMetadata);
            ev[CtfEventNameKey] = std::string(name);
            ev[CtfProcessIdKey] = pid;
            ev[CtfThreadIdKey] = tid;
            ev["args"]["name"] = value;
            metadata.push_back(std::move(ev));
        };
        for (const std::string &tid : laneTids) {
            auto pit = pidByTid.find(tid);
            const std::string pid = (pit != pidByTid.end()) ? pit->second : tid;
            // Keep metadata on the existing lane tid so no phantom lanes appear.
            if (auto nit = nameByTid.find(tid); nit != nameByTid.end())
                metadataEvent("thread_name", pid, tid, nit->second);
            if (auto pnit = nameByPid.find(pid); pnit != nameByPid.end())
                metadataEvent("process_name", pid, tid, pnit->second);
        }
    }

    // Metadata first: these are non-visible and share the timestamp of the first
    // real event. CtfTraceManager resets its global time offset when a
    // non-visible event matches the current offset, so emitting them ahead of the
    // visible events lets the first real event establish the offset cleanly
    // (emitting them afterwards would reset the offset and blow up the range).
    for (json &ev : metadata) {
        if (promise.isCanceled())
            return;
        promise.addResult(std::move(ev));
    }
    for (json &ev : events) {
        if (promise.isCanceled())
            return;
        promise.addResult(std::move(ev));
    }
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
