// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfloader.h"

#include "ctfvisualizerconstants.h"

#include <commontraceformat/binary/fieldvalue.h>
#include <commontraceformat/stream/datastreamreader.h>
#include <commontraceformat/stream/tracedirectory.h>
#include <commontraceformat/stream/tracereader.h>

#include <utils/filepath.h>

#include <QByteArrayView>
#include <QFuture>
#include <QHash>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <string_view>
#include <tuple>
#include <unordered_map>

using namespace Profiler::Constants;

namespace Profiler::Internal {

using json = nlohmann::json;

namespace {

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

std::optional<std::string> fieldValueToString(const CommonTraceFormat::FieldValue &fv)
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

json fieldValueToJson(const CommonTraceFormat::FieldValue &fv);

json structureValueToJson(const CommonTraceFormat::StructureValue &sv)
{
    json obj = json::object();
    for (const QString &key : sv.order)
        obj[key.toStdString()] = fieldValueToJson(sv.members.value(key));
    return obj;
}

json fieldValueToJson(const CommonTraceFormat::FieldValue &fv)
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

// The thread that wrote a packet, as Qt's CTF backend identifies it: that
// backend writes one stream per thread and puts the thread's id and name in
// the packet context, where no event field carries them.
struct PacketIdentity
{
    std::optional<std::string> threadId;
    std::optional<std::string> threadName;
};

PacketIdentity packetIdentity(const CommonTraceFormat::DataStreamReader &stream)
{
    static const QString threadIdKey = QStringLiteral("thread_id");
    static const QString threadNameKey = QStringLiteral("thread_name");

    PacketIdentity identity;
    const CommonTraceFormat::StructureValue &context = stream.packetContext();
    if (const CommonTraceFormat::FieldValue *fv = context.get(threadIdKey))
        identity.threadId = fieldValueToString(*fv);
    if (const CommonTraceFormat::FieldValue *fv = context.get(threadNameKey))
        identity.threadName = fieldValueToString(*fv);
    return identity;
}

// What to call the trace stored in `directory` when its metadata does not name
// the session. An LTTng recording keeps the streams of a domain in a
// subdirectory of that domain's name, and Qt's CTF backend writes into a "ust"
// of its own, so the name of such a directory says nothing about the recording
// while the one above it does.
QString traceDirectoryName(const QString &directory)
{
    const Utils::FilePath dir = Utils::FilePath::fromString(directory);
    const QString name = dir.fileName();
    if (name != QLatin1StringView("ust") && name != QLatin1StringView("kernel"))
        return name;
    const QString parent = dir.parentDir().fileName();
    return parent.isEmpty() ? name : parent;
}

// Beyond this many seconds a timestamp says nothing a timeline holding
// microseconds in a double could tell apart, while the integer arithmetic that
// rebases one would overflow. A trace states its clock's origin and its events'
// cycle counts itself, and is not trusted to keep either below it.
constexpr qint64 maxClockSeconds = Q_INT64_C(1) << 40;

// Where a tracepoint came from, which for Qt is the module that declares it:
// its CTF2 namespace, or the "<provider>:" its name carries in a TSDL 1.8 trace
// -- Qt's CTF backend writes "qtcore:QObject_..." and the like. An event class
// with neither belongs to no provider, as a kernel trace's do not.
QString eventProvider(const CommonTraceFormat::EventRecordClass &eventClass)
{
    if (!eventClass.namespaceName.isEmpty())
        return eventClass.namespaceName;
    const qsizetype colon = eventClass.name.indexOf(u':');
    return colon > 0 ? eventClass.name.left(colon) : QString();
}

} // namespace

QStringList ctfTraceProviders(const QString &dirPath)
{
    using namespace CommonTraceFormat;

    const Utils::Result<TraceDirectory> traceDir = TraceDirectory::open(dirPath);
    if (!traceDir)
        return {};

    QStringList providers;
    for (const TraceDirectory::Trace &trace : traceDir->traces()) {
        if (!trace.schema)
            continue;
        for (const DataStreamClass &dsc : trace.schema->dataStreamClasses) {
            for (const EventRecordClass &erc : dsc.eventRecordClasses) {
                const QString provider = eventProvider(erc);
                if (!provider.isEmpty() && !providers.contains(provider))
                    providers.append(provider);
            }
        }
    }
    providers.sort();
    return providers;
}

void loadChromeJson(QPromise<json> &promise, const QString &fileName)
{
    std::ifstream file(fileName.toStdString());
    if (!file.is_open()) {
        promise.future().cancel();
        return;
    }

    try {
        // The result is discarded by the callback; we only want the side effects
        // (events pushed into the promise).
        std::ignore = json::parse(file, CtfJsonParserFunctor(promise), /*allow_exceptions*/ false);
    } catch (...) {
        // nlohmann::json can throw exceptions when requesting type that is wrong
        promise.future().cancel();
    }

    file.close();
}

void loadCtf2Data(QPromise<json> &promise, const QString &dirPath, const QStringList &providers)
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

    // The providers the load keeps. Nothing states an event's provider per
    // event: it is a property of its class, so the decision is made once per
    // declared class below and looked up per event.
    std::set<std::string> keptProviders;
    for (const QString &provider : providers)
        keptProviders.insert(provider.toStdString());

    // Collect all events first so we can sort by timestamp before emitting.
    // CtfTraceManager sets the global time offset from the first event it receives,
    // so events must arrive in chronological order to avoid a negative trace range
    // when multiple streams cover different (possibly overlapping) time windows.
    std::vector<json> events;

    auto parseDur = [](const std::string &s) -> std::optional<double> {
        bool ok = false;
        const double val = QByteArrayView(s).toDouble(&ok);
        if (!ok)
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

    // Qt's CTF backend writes one stream per thread and puts that thread's
    // identity in the *packet* context (thread_id/thread_name), where neither of
    // the two schemes above finds it. Such a lane knows its process from the
    // start; only its name has to be emitted by the post-pass.
    //
    // What the trace called the thread and the process goes with it: the ids a
    // lane is filed under are qualified to stay unique across the load (see
    // below), which is an internal key and no name for either.
    struct PacketLane
    {
        std::string processId;   // Owning process, as the lane's events carry it.
        std::string processName; // What to call that process.
        std::string threadName;
        std::string threadId; // Thread id as the trace stated it.
    };
    std::unordered_map<std::string, PacketLane> packetLanes; // lane -> identity

    // Thread ids are unique within a trace, not across traces, so a lane built
    // from them needs the trace to stay distinct when several were loaded.
    const bool manyTraces = traceDir.traces().size() > 1;

    // What a trace is called: the session it was recorded under, which CTF
    // states in its environment (spec 5.6), or the directory it was found in
    // when the metadata names none. Two recordings of one application name the
    // same session, so a name that several traces of the load share is numbered
    // -- the reader has nothing else to tell them apart by.
    QStringList traceNames;
    for (const TraceDirectory::Trace &trace : traceDir.traces()) {
        const QString directoryName = traceDirectoryName(trace.directory);
        traceNames.append(trace.schema && trace.schema->traceClass
                              ? trace.schema->traceClass->environment.value(u"trace_name"_s,
                                                                            directoryName)
                              : directoryName);
    }
    QHash<QString, int> traceNameCount;
    for (const QString &name : traceNames)
        ++traceNameCount[name];
    QHash<QString, int> traceNameOrdinal;
    QStringList traceDisplayNames;
    for (const QString &name : traceNames) {
        traceDisplayNames.append(traceNameCount.value(name) > 1
                                     ? u"%1 #%2"_s.arg(name).arg(++traceNameOrdinal[name])
                                     : name);
    }

    // Clock parameters of one stream: an event's cycle count is
    // offsetSeconds + (cycles + offsetCycles) / frequency seconds from the
    // clock's origin (spec 5.7).
    struct StreamClock
    {
        quint64 frequency = 1000000000ULL;
        qint64 offsetSeconds = 0; // Within +-maxClockSeconds.
        quint64 offsetCycles = 0;
        // Whether the stream's class names a clock the metadata declares.
        // Without one every event of the stream carries the timestamp 0
        // (spec 4.2.2), so the stream has no origin -- not an origin of zero.
        bool hasClock = false;

        // Cycles from the clock's origin of an event timestamped `timestamp`,
        // which counts from the clock's offset. The trace states both halves of
        // that sum, so it saturates rather than wraps: a wrapped sum is small,
        // and neither the clamp below nor anything after it can tell it from a
        // timestamp the trace really holds -- the event would land at a
        // plausible-looking wrong point in time.
        quint64 cyclesFromOrigin(quint64 timestamp) const
        {
            constexpr quint64 maxCycles = std::numeric_limits<quint64>::max();
            return timestamp > maxCycles - offsetCycles ? maxCycles : timestamp + offsetCycles;
        }

        // Whole seconds of `cycles`, which count from the clock's origin and
        // include offsetCycles, as wall-clock time.
        qint64 seconds(quint64 cycles) const
        {
            const quint64 whole = cycles / frequency;
            if (whole > quint64(maxClockSeconds))
                return maxClockSeconds;
            return std::min(offsetSeconds + qint64(whole), maxClockSeconds);
        }

        // Whole seconds at or below every timestamp of the stream, which is the
        // offset alone: an event's own cycle count is counted from the origin,
        // so it cannot be negative.
        qint64 originSeconds() const { return seconds(cyclesFromOrigin(0)); }
    };

    const auto streamClock = [](const Schema &schema, const DataStreamClass *dsc) {
        StreamClock clock;
        if (dsc->defaultClockClassName.isEmpty())
            return clock;
        for (const ClockClass &cc : schema.clockClasses) {
            // CTF2 links by clock class id; legacy/TSDL schemas link by name.
            const bool matches = cc.id == dsc->defaultClockClassName
                                 || cc.name == dsc->defaultClockClassName;
            if (matches && cc.frequency > 0) {
                clock.frequency = cc.frequency;
                clock.offsetSeconds = std::clamp(cc.offsetSeconds, -maxClockSeconds,
                                                 maxClockSeconds);
                clock.offsetCycles = cc.offsetCycles;
                // Told by the clock that was found, not by the name that was
                // looked up: a stream may name a clock the metadata never
                // declares, and then nothing at all is known about its origin.
                clock.hasClock = true;
                break;
            }
        }
        return clock;
    };

    // Timestamps are emitted relative to the earliest clock origin of the whole
    // load rather than to the origin itself. A tracer states that origin as a
    // wall-clock offset -- Qt's is the epoch, in nanoseconds -- and a double
    // holding that many microseconds resolves to no better than a quarter of a
    // microsecond, which is coarser than the tracepoints being timed. Subtracting
    // whole seconds in integers first keeps the double down to the length of the
    // trace, where it resolves far below a nanosecond. Nothing shows an absolute
    // time (the timeline is relative to its first event, see CtfTraceManager),
    // and every clocked stream subtracts the same base, so traces recorded at
    // different times still line up against each other.
    //
    // A stream with no clock is left out: its events are all timestamped 0, so
    // counting it in would pull the base down to 0 and leave every clocked
    // stream of the load with the precision loss above. Its own events are
    // emitted against a base of 0, which puts them where they were before --
    // at the very start of the timeline -- rather than a wall-clock era before
    // it.
    qint64 baseSeconds = 0;
    bool haveBase = false;
    for (const TraceDirectory::Trace &trace : traceDir.traces()) {
        for (const TraceDirectory::Stream &sf : trace.streams) {
            if (!sf.dsc)
                continue;
            const StreamClock clock = streamClock(*trace.schema, sf.dsc);
            if (!clock.hasClock)
                continue;
            const qint64 origin = clock.originSeconds();
            if (!haveBase || origin < baseSeconds) {
                baseSeconds = origin;
                haveBase = true;
            }
        }
    }

    quint64 fallbackStreamId = 0;
    int traceIndex = -1;
    for (const TraceDirectory::Trace &trace : traceDir.traces()) {
        const Schema &schema = *trace.schema;
        ++traceIndex;

        // Thread ids are unique within a trace, not across traces, so a lane
        // built from them is qualified by the trace it came from. That has to be
        // the position in the load rather than any name: sibling traces of one
        // recording session can share their directory name, and two recordings
        // of one application share their session name. A lane shared by two
        // traces would interleave two threads' events -- and pair one thread's
        // entry with the other's exit.
        const std::string laneQualifier = manyTraces ? std::to_string(traceIndex) + '/'
                                                     : std::string();

        // Every stream of one trace comes from one traced process. CTF has no
        // field for its pid, so the trace's name stands in for it. It is
        // qualified like the lanes, and for the same reason: two recordings of
        // one application name the same session, and would otherwise be shown
        // as one process -- in one colour.
        const std::string tracePid = laneQualifier + traceNames.at(traceIndex).toStdString();
        const std::string traceDisplayName = traceDisplayNames.at(traceIndex).toStdString();

        // The provider of every event class the trace declares, which an event
        // is shown and filtered by. The classes live in the schema the reader
        // holds open for the whole load, which is what an event record points
        // at, so they can be keyed by their address.
        std::unordered_map<const EventRecordClass *, std::string> providerOf;
        for (const DataStreamClass &streamClass : schema.dataStreamClasses) {
            for (const EventRecordClass &eventClass : streamClass.eventRecordClasses)
                providerOf[&eventClass] = eventProvider(eventClass).toStdString();
        }

        for (const TraceDirectory::Stream &sf : trace.streams) {
            const DataStreamClass *dsc = sf.dsc;
            if (!dsc)
                continue;

            const StreamClock clock = streamClock(schema, dsc);
            // See the base computation above: a stream left out of it counts
            // from its own origin instead.
            const qint64 streamBase = clock.hasClock ? baseSeconds : 0;

            // Stream instance id, used as a pid/tid lane fallback.
            const quint64 streamId = sf.streamId ? *sf.streamId : fallbackStreamId++;
            DataStreamReader *stream = sf.reader;

            // Thread currently scheduled on this CPU (stream), updated by
            // sched_switch. Empty until the first switch is seen.
            std::string currentTid;

            // What the packet context of the packet being read says about the
            // thread that wrote it, and whether its lane has been named. A
            // packet holds many events and the reader keeps it open until the
            // event after its last, so this is refreshed per packet rather than
            // read per event.
            PacketIdentity packet;
            quint64 packetsRead = 0;
            bool laneNamed = false;

            while (!stream->atEnd()) {
                if (promise.isCanceled())
                    return;

                auto eventResult = stream->nextEvent();
                if (!eventResult)
                    break;

                if (const quint64 packets = stream->packetCount(); packets != packetsRead) {
                    packetsRead = packets;
                    packet = packetIdentity(*stream);
                    laneNamed = false;
                }

                const EventRecord &rec = *eventResult;

                // A restricted load leaves out the events of every provider it
                // was not asked for. An event whose class names no provider at
                // all belongs to none of them -- a kernel recording's
                // "sched_switch", read beside a Qt one -- so no provider being
                // cleared takes it away: it has no entry of its own to be
                // brought back by, and clearing one Qt module would be the end
                // of the kernel side of such a trace.
                const auto provider = providerOf.find(rec.eventClass);
                const std::string eventProviderName = provider != providerOf.end() ? provider->second
                                                                                   : std::string();
                if (!keptProviders.empty() && !eventProviderName.empty()
                    && !keptProviders.count(eventProviderName)) {
                    continue;
                }

                // Cycles to microseconds from `streamBase`: the whole seconds
                // are counted in integers, only the remainder is divided.
                const quint64 cycles = clock.cyclesFromOrigin(rec.timestamp);
                const double ts = double(clock.seconds(cycles) - streamBase) * 1.0e6
                                  + double(cycles % clock.frequency) * 1.0e6
                                        / double(clock.frequency);

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
                    // Traces with per-event context: trust it.
                    const std::string lanePid = ctxPid ? *ctxPid : std::to_string(streamId);
                    event[CtfProcessIdKey] = lanePid;
                    event[CtfThreadIdKey] = ctxTid ? *ctxTid : lanePid;
                } else if (packet.threadId) {
                    // Qt's CTF backend: one stream per thread, named in the
                    // packet context.
                    const std::string lane = laneQualifier + *packet.threadId;
                    event[CtfProcessIdKey] = tracePid;
                    event[CtfThreadIdKey] = lane;
                    // Named once per packet, and only for a lane an event of it
                    // actually lands on: a name for a lane that stays empty
                    // would show up as a thread of its own.
                    if (!laneNamed && packet.threadName && !packet.threadName->empty()) {
                        packetLanes[lane] = {tracePid, traceDisplayName, *packet.threadName,
                                             *packet.threadId};
                        laneNamed = true;
                    }
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
                // Where the tracepoint came from, in the place the Chrome format
                // keeps it, so that the details of an event name its provider.
                if (!eventProviderName.empty())
                    event[CtfEventCategoryKey] = eventProviderName;

                if (!rec.payload.order.isEmpty())
                    event["args"] = structureValueToJson(rec.payload);

                events.push_back(std::move(event));
            }
        }
    }

    // Stable: events of one stream were read in the order the tracer wrote them,
    // and at nanosecond resolution two of them can share a timestamp. Reordering
    // those would let an exit precede its own entry, which the pairing below
    // then cannot match.
    std::stable_sort(events.begin(), events.end(), [&](const json &a, const json &b) {
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
                                       const std::string &tid, const std::string &value,
                                       const std::string &displayId = {}) {
            json ev;
            ev[CtfTracingClockTimestampKey] = t0;
            ev[CtfEventPhaseKey] = std::string(CtfEventTypeMetadata);
            ev[CtfEventNameKey] = std::string(name);
            ev[CtfProcessIdKey] = pid;
            ev[CtfThreadIdKey] = tid;
            ev["args"]["name"] = value;
            // The id the trace itself stated, where the one the lane is filed
            // under is not it -- a thread id qualified by its trace, a process
            // id that is a session name.
            if (!displayId.empty())
                ev["args"][CtfMetadataDisplayIdKey] = displayId;
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
        for (const auto &[tid, lane] : packetLanes) {
            metadataEvent("thread_name", lane.processId, tid, lane.threadName, lane.threadId);
            // The process is named too, although a Qt trace has no pid to name
            // it by: unnamed, the lane falls back to the id it is filed under
            // -- the session name, qualified by the trace it came from.
            metadataEvent("process_name", lane.processId, tid, lane.processName, lane.processName);
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

} // namespace Profiler::Internal
