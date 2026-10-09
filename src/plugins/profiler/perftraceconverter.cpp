// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perftraceconverter.h"

#include "perfeventtype.h"
#include "perfprofilerconstants.h"
#include "profilertr.h"

#include <utils/filepath.h>

#include <QtEndian>

#include <algorithm>
#include <limits>

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

namespace {

constexpr QDataStream::Version StreamVersion = QDataStream::Qt_DefaultCompiledVersion;

// Callchain sentinel values (include/uapi/linux/perf_event.h): not real
// addresses, they mark which context the *following* entries belong to.
constexpr quint64 PerfContextHv = quint64(-32);
constexpr quint64 PerfContextKernel = quint64(-128);
constexpr quint64 PerfContextUser = quint64(-512);
constexpr quint64 PerfContextMax = quint64(-4095);

constexpr quint32 PerfTypeHardware = 0;
constexpr quint32 PerfTypeSoftware = 1;
constexpr quint32 PerfTypeTracepoint = 2;

// The names perf gives the generic events, for a recording that does not
// name its events itself.
QByteArray genericEventName(quint32 type, quint64 config)
{
    static const char *const hardware[] = {"cpu-cycles", "instructions", "cache-references",
                                           "cache-misses", "branch-instructions", "branch-misses",
                                           "bus-cycles", "stalled-cycles-frontend",
                                           "stalled-cycles-backend", "ref-cycles"};
    static const char *const software[] = {"cpu-clock", "task-clock", "page-faults",
                                           "context-switches", "cpu-migrations", "minor-faults",
                                           "major-faults", "alignment-faults", "emulation-faults",
                                           "dummy", "bpf-output", "cgroup-switches"};
    if (type == PerfTypeHardware && config < std::size(hardware))
        return hardware[config];
    if (type == PerfTypeSoftware && config < std::size(software))
        return software[config];
    return "type" + QByteArray::number(type) + ":" + QByteArray::number(config);
}

} // namespace

PerfTraceConverter::PerfTraceConverter(const PerfData::PerfDataParser &parser,
                                       const std::function<void(const QByteArray &)> &write)
    : m_parser(parser)
    , m_write(write)
{
    // Magic, NUL included, and the stream version, as PerfProfilerTraceFile
    // expects them ahead of the messages.
    m_output.append(Constants::PerfStreamMagic, sizeof(Constants::PerfStreamMagic));
    const qint32 version = qToLittleEndian(qint32(StreamVersion));
    m_output.append(reinterpret_cast<const char *>(&version), sizeof(version));
}

void PerfTraceConverter::writeMessage(const std::function<void(QDataStream &)> &fill)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(StreamVersion);
    fill(stream);
    const quint32 size = qToLittleEndian(quint32(payload.size()));
    m_output.append(reinterpret_cast<const char *>(&size), sizeof(size));
    m_output.append(payload);
    if (m_output.size() > (1 << 20))
        flush();
}

void PerfTraceConverter::writeEvent(quint8 feature, quint32 pid, quint32 tid, quint64 time,
                                    quint32 cpu, const std::function<void(QDataStream &)> &fill)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(StreamVersion);
    stream << feature << pid << tid << time << cpu;
    if (fill)
        fill(stream);
    QByteArray message;
    const quint32 size = qToLittleEndian(quint32(payload.size()));
    message.append(reinterpret_cast<const char *>(&size), sizeof(size));
    message.append(payload);
    m_round.append({time, message});
    m_roundEnd = std::max(m_roundEnd, time);
}

void PerfTraceConverter::flush()
{
    if (m_output.isEmpty())
        return;
    m_write(m_output);
    m_output.clear();
}

void PerfTraceConverter::finishedRound()
{
    // A round's events come from several per-CPU buffers, so they are only in
    // time order once sorted. And only those up to where the round before
    // reached are complete: a buffer read late in this round may still hold
    // earlier ones, which the next round brings.
    flushEvents(m_previousRoundEnd);
    m_previousRoundEnd = std::max(m_previousRoundEnd, m_roundEnd);
    m_roundEnd = 0;
}

void PerfTraceConverter::finish()
{
    flushEvents(std::numeric_limits<quint64>::max());
}

void PerfTraceConverter::flushEvents(quint64 until)
{
    // The definitions the events use are out already.
    std::stable_sort(m_round.begin(), m_round.end(),
                     [](const Event &a, const Event &b) { return a.time < b.time; });
    const auto end = std::upper_bound(m_round.cbegin(), m_round.cend(), until,
                                      [](quint64 time, const Event &e) { return time < e.time; });
    for (auto it = m_round.cbegin(); it != end; ++it)
        m_output.append(it->message);
    m_round.erase(m_round.cbegin(), end);
    flush();
}

qint32 PerfTraceConverter::stringId(const QByteArray &string)
{
    if (const auto it = m_stringIds.constFind(string); it != m_stringIds.constEnd())
        return it.value();
    const qint32 id = qint32(m_stringIds.size());
    m_stringIds.insert(string, id);
    writeMessage([&](QDataStream &s) {
        s << quint8(PerfEventType::StringDefinition) << id << string;
    });
    return id;
}

qint32 PerfTraceConverter::attributeId(int attrIndex)
{
    if (const auto it = m_attributeIds.constFind(attrIndex); it != m_attributeIds.constEnd())
        return it.value();

    const PerfData::EventAttr &attr = m_parser.attrs().at(attrIndex);
    const TracepointFormat *tracepoint = attr.type == PerfTypeTracepoint
                                             ? m_tracepointFormats.format(attr.config)
                                             : nullptr;
    QByteArray name = attr.name;
    if (name.isEmpty()) {
        name = tracepoint ? tracepoint->system + ':' + tracepoint->name
                          : genericEventName(attr.type, attr.config);
    }
    if (tracepoint) {
        const qint32 system = stringId(tracepoint->system);
        const qint32 tracepointName = stringId(tracepoint->name);
        writeMessage([&](QDataStream &s) {
            // flags: none that the analyzer knows of.
            s << quint8(PerfEventType::TracePointFormat) << qint32(attr.config) << system
              << tracepointName << quint32(0);
        });
    }

    const qint32 id = qint32(m_attributeIds.size());
    m_attributeIds.insert(attrIndex, id);
    const qint32 nameId = stringId(name);
    writeMessage([&](QDataStream &s) {
        s << quint8(PerfEventType::AttributesDefinition) << id << attr.type << attr.config
          << nameId << attr.freq << attr.samplePeriodOrFreq;
    });
    return id;
}

qint32 PerfTraceConverter::defineLocation(quint64 address, quint32 pid, const QString &file,
                                          int line, qint32 parentLocationId, quint64 relAddr)
{
    const qint32 id = m_nextLocationId++;
    const qint32 fileId = file.isEmpty() ? -1 : stringId(file.toUtf8());
    writeMessage([&](QDataStream &s) {
        s << quint8(PerfEventType::LocationDefinition) << id << address << fileId << pid
          << qint32(line > 0 ? line : -1) << qint32(-1) << parentLocationId << relAddr;
    });
    return id;
}

void PerfTraceConverter::defineSymbol(qint32 locationId, const QString &name,
                                      const QString &binary, const QString &path,
                                      const QString &actualPath, bool isKernel, quint64 relAddr,
                                      quint64 size)
{
    const qint32 nameId = stringId(name.toUtf8());
    const qint32 binaryId = binary.isEmpty() ? -1 : stringId(binary.toUtf8());
    const qint32 pathId = path.isEmpty() ? -1 : stringId(path.toUtf8());
    const qint32 actualPathId = actualPath.isEmpty() ? -1 : stringId(actualPath.toUtf8());
    writeMessage([&](QDataStream &s) {
        s << quint8(PerfEventType::SymbolDefinition) << locationId << nameId << binaryId << pathId
          << isKernel << relAddr << size << actualPathId;
    });
}

PerfArchitecture PerfTraceConverter::architecture()
{
    if (m_architecture == PerfArchitecture::Unknown) {
        const QByteArray recorded = m_parser.architecture();
        m_architecture = recorded.isEmpty()
                             ? hostPerfArchitecture()
                             : perfArchitectureFromName(QString::fromLatin1(recorded));
    }
    return m_architecture;
}

qint32 PerfTraceConverter::locationId(quint32 pid, quint64 addr, bool isKernel)
{
    const LocationKey key{pid, addr, isKernel};
    if (const auto it = m_locationIds.constFind(key); it != m_locationIds.constEnd())
        return it.value();

    QString binary;
    QString path;
    QString actualPath;
    ResolvedAddress resolved;
    std::optional<ModuleAddress> at;
    if (isKernel) {
        // This machine's kernel symbols are only the recording's if it was
        // taken here, or at least on the same architecture.
        if (!m_kernelSymbolsDecided) {
            m_kernelSymbolsDecided = true;
            m_symbolizer.setUseKallsyms(!m_foreignRecording
                                        && architecture() == hostPerfArchitecture());
        }
        binary = u"[kernel]"_s;
        path = u"[kernel.kallsyms]"_s;
        resolved = m_symbolizer.resolveKernel(addr);
    } else if ((at = m_symbolizer.moduleAddress(pid, addr))) {
        path = at->recordedPath;
        actualPath = at->modulePath;
        binary = FilePath::fromString(path).fileName();
        resolved = m_symbolizer.resolve(*at);
    } else {
        resolved.linkAddress = addr;
    }
    // Code a JIT generated -- QML's, say -- is in no module's symbols, but
    // may be in the map the JIT keeps for perf.
    if (!isKernel && resolved.function.isEmpty()) {
        const ResolvedAddress jit = m_symbolizer.resolveJit(pid, addr);
        if (!jit.function.isEmpty()) {
            resolved = jit;
            at.reset(); // no module to look the function's start up in
        }
    }

    qint32 id = -1;
    if (resolved.function.isEmpty()) {
        // No function to aggregate by: the address is its own symbol.
        id = defineLocation(addr, pid, resolved.file, resolved.line, -1, resolved.linkAddress);
        defineSymbol(id, u"0x%1"_s.arg(resolved.linkAddress, 0, 16), binary, path, actualPath,
                     isKernel, resolved.linkAddress, 0);
    } else {
        const quint64 start = resolved.linkAddress - resolved.functionOffset;
        const FunctionKey functionKey{pid, actualPath.isEmpty() ? path : actualPath, start};
        qint32 functionId = m_functionLocationIds.value(functionKey, -1);
        if (functionId < 0) {
            // Where the function starts, and its own source line there.
            ResolvedAddress atStart;
            if (at)
                atStart = m_symbolizer.resolve({at->modulePath, start});
            functionId = defineLocation(addr - resolved.functionOffset, pid, atStart.file,
                                        atStart.line, -1, start);
            defineSymbol(functionId, resolved.function, binary, path, actualPath, isKernel, start,
                         resolved.functionSize);
            m_functionLocationIds.insert(functionKey, functionId);
        }
        // Functions inlined at the address, outermost first: each carries its
        // own symbol, and its parent is where it was called, in the function
        // it was inlined into -- which is how the analyzer expands them.
        qint32 parentId = functionId;
        for (auto it = resolved.inlined.crbegin(); it != resolved.inlined.crend(); ++it) {
            const InlineKey inlineKey{parentId, it->function, it->callFile, it->callLine};
            qint32 inlinedId = m_inlineLocationIds.value(inlineKey, -1);
            if (inlinedId < 0) {
                const qint32 callSite = defineLocation(addr, pid, it->callFile, it->callLine,
                                                       parentId, resolved.linkAddress);
                inlinedId = defineLocation(addr, pid, {}, 0, callSite, resolved.linkAddress);
                defineSymbol(inlinedId, it->function, binary, path, actualPath, isKernel,
                             resolved.linkAddress, 0);
                m_inlineLocationIds.insert(inlineKey, inlinedId);
            }
            parentId = inlinedId;
        }
        // A sample at the function's very first byte is the function itself.
        id = resolved.functionOffset == 0 && resolved.inlined.isEmpty()
                 ? functionId
                 : defineLocation(addr, pid, resolved.file, resolved.line, parentId,
                                  resolved.linkAddress);
    }
    m_locationIds.insert(key, id);
    return id;
}

Result<> PerfTraceConverter::attrAdded(int index)
{
    Q_UNUSED(index)
    // Defined at first use: a stream may only name its events later.
    return ResultOk;
}

void PerfTraceConverter::tracingData(const QByteArray &data)
{
    m_tracepointFormats.parse(data);
}

void PerfTraceConverter::mmap(const PerfData::Mmap &mmap)
{
    if (mmap.path.isEmpty() || mmap.path.startsWith('['))
        return; // anonymous/special mapping (e.g. "[heap]", "[stack]"): nothing to symbolize
    m_symbolizer.addMapping(mmap.pid, mmap.addr, mmap.len, mmap.pgoff, mmap.path, mmap.buildId);
}

void PerfTraceConverter::buildId(const PerfData::BuildId &buildId)
{
    m_symbolizer.addBuildId(buildId.path, buildId.buildId);
}

void PerfTraceConverter::comm(const PerfData::Comm &comm)
{
    const qint32 name = stringId(comm.name.toUtf8());
    const qint64 start = comm.sampleId.hasTime ? qint64(comm.sampleId.time) : -1;
    writeMessage([&](QDataStream &s) {
        s << quint8(PerfEventType::Command) << comm.pid << comm.tid << start
          << comm.sampleId.cpu << name;
    });
}

void PerfTraceConverter::task(const PerfData::Task &task)
{
    m_latestTime = qMax(m_latestTime, task.time);
    if (task.exit) {
        writeEvent(PerfEventType::ThreadEnd, task.pid, task.tid, task.time, 0);
    } else {
        m_symbolizer.addFork(task.ppid, task.pid);
        writeEvent(PerfEventType::ThreadStart, task.pid, task.tid, task.time, 0,
                   [&task](QDataStream &s) { s << qint32(task.ppid); });
    }
}

void PerfTraceConverter::contextSwitch(const PerfData::ContextSwitch &contextSwitch)
{
    const PerfData::SampleId &id = contextSwitch.sampleId;
    if (!id.hasTime)
        return; // nowhere to put it
    m_latestTime = qMax(m_latestTime, id.time);
    writeEvent(PerfEventType::ContextSwitchDefinition, id.pid, id.tid, id.time, id.cpu,
               [&contextSwitch](QDataStream &s) { s << contextSwitch.out; });
}

void PerfTraceConverter::lost(const PerfData::Lost &lost)
{
    const PerfData::SampleId &id = lost.sampleId;
    // Without a time of its own, a loss is placed where perf noticed it: at
    // the newest event.
    const quint64 time = id.hasTime ? id.time : m_latestTime;
    writeEvent(PerfEventType::LostDefinition, id.pid, id.tid, time, id.cpu,
               [&lost](QDataStream &s) { s << lost.count; });
}

Result<> PerfTraceConverter::sample(const PerfData::Sample &sample)
{
    if (m_canceled && *m_canceled)
        return ResultError(Tr::tr("Canceled."));
    m_latestTime = qMax(m_latestTime, sample.time);

    // The call chain, innermost first, with PERF_CONTEXT_* markers saying
    // which part is the kernel's. Hypervisor frames have nothing to resolve
    // against. For lbr the branch stack's call sites are the user callers:
    // the user part of the call chain is whatever a frame pointer walk found,
    // in code that typically has none, so only its first address is kept.
    struct Frame
    {
        quint64 addr;
        bool isKernel;
    };
    QList<Frame> frames;
    frames.reserve(sample.callchain.size() + sample.branchFroms.size());
    quint64 context = PerfContextUser;
    bool isFirstOfContext = true;
    for (quint64 addr : sample.callchain) {
        if (addr >= PerfContextMax) {
            context = addr;
            isFirstOfContext = true;
            continue;
        }
        const bool isCaller = !isFirstOfContext;
        isFirstOfContext = false;
        if (context == PerfContextHv)
            continue;
        if (context == PerfContextUser && isCaller && !sample.branchFroms.isEmpty())
            continue;
        frames.append({addr, context == PerfContextKernel});
    }
    for (quint64 addr : sample.branchFroms)
        frames.append({addr, false});

    // "--call-graph dwarf": perf leaves the user part out of the callchain,
    // for the user stack to be unwound here, from the captured registers.
    const PerfRegisterLayout layout = perfRegisterLayout(architecture());
    if (!sample.userRegs.isEmpty() && layout.ip >= 0) {
        QList<quint64> regs = sample.userRegs;
        regs.resize(layout.count, 0);
        const bool hasUserFrame = std::any_of(frames.cbegin(), frames.cend(),
                                              [](const Frame &f) { return !f.isKernel; });
        if (!hasUserFrame && !m_canUnwind && !m_warnedCannotUnwind) {
            m_warnedCannotUnwind = true;
            if (m_onWarning) {
                m_onWarning(Tr::tr("This build cannot unwind \"dwarf\" call graphs: the samples "
                                   "show where they were taken, but not how the program got "
                                   "there. Record with the \"frame pointer\" or \"last branch "
                                   "record\" call graph mode instead."));
            }
        }
        if (!hasUserFrame) {
            QList<quint64> pcs = m_canUnwind ? m_symbolizer.unwind(sample.pid, architecture(),
                                                                   regs, sample.userStack)
                                             : QList<quint64>();
            if (pcs.isEmpty() && regs.at(layout.ip) != 0)
                pcs.append(regs.at(layout.ip));
            for (quint64 pc : std::as_const(pcs))
                frames.append({pc, false});
        }
    }

    QList<qint32> locations;
    locations.reserve(frames.size());
    for (const Frame &frame : std::as_const(frames))
        locations.append(locationId(sample.pid, frame.addr, frame.isKernel));

    // The sample's own event, and with PERF_SAMPLE_READ those of its group.
    QList<QPair<qint32, quint64>> values;
    values.append({attributeId(sample.attrIndex), sample.period ? sample.period : 1});
    for (const PerfData::Sample::ReadValue &read : sample.readValues) {
        const int index = m_parser.attrIndexForId(read.id);
        if (index >= 0 && index != sample.attrIndex)
            values.append({attributeId(index), read.value});
    }

    const PerfData::EventAttr &attr = m_parser.attrs().at(sample.attrIndex);
    const TracepointFormat *tracepoint = attr.type == PerfTypeTracepoint
                                             ? m_tracepointFormats.format(attr.config)
                                             : nullptr;
    QHash<qint32, QVariant> traceData;
    if (tracepoint) {
        for (const auto &[name, value] : tracepoint->decode(sample.raw))
            traceData.insert(stringId(name), value);
    }

    const quint8 feature = attr.type == PerfTypeTracepoint ? PerfEventType::TracePointSample
                                                           : PerfEventType::Sample;
    writeEvent(feature, sample.pid, sample.tid, sample.time, sample.cpu, [&](QDataStream &s) {
        s << locations << quint8(0) << values; // no frames guessed
        if (feature == PerfEventType::TracePointSample)
            s << traceData;
    });
    return ResultOk;
}

} // namespace Profiler::Internal
