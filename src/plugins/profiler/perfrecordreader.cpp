// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfrecordreader.h"

#include "perfdataparser.h"
#include "perfregisters.h"
#include "perfsymbolizer.h"
#include "profilertr.h"

#include <utils/filepath.h>

#include <QHash>
#include <QList>
#include <QSet>

#include <algorithm>
#include <limits>
#include <optional>

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

// PerfByteQueue

void PerfByteQueue::push(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    std::lock_guard lock(m_mutex);
    if (m_closed)
        return;
    m_chunks.push_back(data);
    m_size += data.size();
    m_cv.notify_one();
}

void PerfByteQueue::close()
{
    std::lock_guard lock(m_mutex);
    m_closed = true;
    m_cv.notify_one();
}

qint64 PerfByteQueue::size() const
{
    std::lock_guard lock(m_mutex);
    return m_size;
}

void PerfByteQueue::setDrainedCallback(qint64 threshold, const std::function<void()> &drained)
{
    std::lock_guard lock(m_mutex);
    m_drainThreshold = threshold;
    m_drained = drained;
}

QByteArray PerfByteQueue::pop()
{
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [this] { return !m_chunks.empty() || m_closed; });
    if (m_chunks.empty())
        return {};
    QByteArray chunk = std::move(m_chunks.front());
    m_chunks.pop_front();
    const bool drained = m_drained && m_size >= m_drainThreshold
                         && m_size - chunk.size() < m_drainThreshold;
    m_size -= chunk.size();
    const std::function<void()> callback = drained ? m_drained : std::function<void()>();
    lock.unlock();
    if (callback)
        callback();
    return chunk;
}

bool PerfByteQueue::atEnd() const
{
    std::lock_guard lock(m_mutex);
    return m_chunks.empty() && m_closed;
}

namespace {

// The sampler records this machine, so its user registers are this
// machine's (see perfregisters.h).
const PerfRegisterLayout &hostRegisterLayout()
{
    static const PerfRegisterLayout layout = perfRegisterLayout(hostPerfArchitecture());
    return layout;
}

// Callchain sentinel values (include/uapi/linux/perf_event.h): not real
// addresses, they mark which context the *following* entries belong to.
constexpr quint64 PerfContextHv = quint64(-32);
constexpr quint64 PerfContextKernel = quint64(-128);
constexpr quint64 PerfContextUser = quint64(-512);
constexpr quint64 PerfContextMax = quint64(-4095); // anything >= this is a context marker

bool isContextMarker(quint64 v) { return v >= PerfContextMax; }

// One decoded call-stack frame, before symbolication: an absolute address plus
// which symbol table (kernel vs the frame's process image) should resolve it.
struct RawFrame
{
    quint64 addr = 0;
    bool isKernel = false;
};

// One decoded sample, before symbolication. Held between the decode pass and
// the symbolication pass so that debug info for every sampled module can be
// fetched (in parallel, see PerfRecordDecoder::resolveDebugInfo()) in between.
struct RawSample
{
    quint64 tsUs = 0;
    quint64 tid = 0;
    quint32 pid = 0;
    bool running = false;
    QList<RawFrame> frames; // root-first; the kernel-reported chain (fp/lbr),
                            // or, in dwarf mode, the unwound one -- see
                            // unwindDwarfSample().

    // "--call-graph dwarf" only, until the sample is unwound: a captured
    // register+stack snapshot to feed PerfDwarfUnwinder::UnwindInput (see
    // perfdwarfunwinder.h), indexed by perf's register numbers when present;
    // empty when this sample carried no usable user registers at all
    // (PERF_SAMPLE_REGS_USER wasn't requested, or its abi field came back 0).
    // The stack starts at the stack pointer among them.
    QList<quint64> dwarfRegs;
    QByteArray dwarfStack;

    // "--call-graph fp" on aarch64 only: the link register, for the caller
    // of the leaf function (see addLeafCaller()).
    quint64 linkRegister = 0;
};

} // namespace

// Owns the actual parsing state; PerfRecordReader::read() just drives it.
class PerfRecordDecoder : public PerfData::PerfDataHandler
{
public:
    PerfRecordDecoder(bool downloadDebugInfo,
                      const std::function<void(int, const QString &)> &onDebugInfoDownload,
                      const std::function<bool(qint64)> &sampleFilter,
                      const std::function<bool()> &isCanceled)
        : m_sampleFilter(sampleFilter)
        , m_isCanceled(isCanceled)
    {
        m_symbolizer.setDownloadDebugInfo(downloadDebugInfo);
        m_symbolizer.setDebugInfoDownloadHandler(onDebugInfoDownload);
    }

    void mmap(const PerfData::Mmap &mmap) override;
    void task(const PerfData::Task &task) override;
    void comm(const PerfData::Comm &comm) override;
    void lost(const PerfData::Lost &lost) override;
    // The total "perf record" writes as it finishes; see finish().
    void lostSamples(quint64 count) override { m_lostSamplesTotal += count; }
    void throttle(const PerfData::Throttle &throttle) override;
    Result<> sample(const PerfData::Sample &sample) override;
    void finishedRound() override;

    // Flushes whatever the last, incomplete round left pending -- call once
    // after the stream ends (a clean recording's last round has no trailing
    // FINISHED_ROUND, since nothing follows it to prompt "perf record" to
    // emit one), and adds the lost samples only a final total reported.
    void finish();

    // Fetches any missing debug info (see PerfSymbolizer::fetchDebugFiles())
    // and then turns the decoded raw samples into labelled ones. `progress`,
    // if set, is called with 0..100 across both stages (see
    // PerfRecordReader::read()).
    void symbolize(const std::function<void(int)> &progress);

    SampleTraceData &data() { return m_data; }
    int receivedSamples() const { return m_receivedSamples; }
    qint64 firstTimestampNs() const { return m_firstTimestampNs; }

private:
    void addLostSamples(quint64 count);
    // Microseconds since the first sample, as a sample's own tsUs.
    quint64 relativeUs(quint64 timeNs) const;
    // Moves the pending samples up to `untilUs` over, all by default.
    void flushPendingRound(quint64 untilUs = std::numeric_limits<quint64>::max());

    // Resolves one absolute address in `pid`'s address space to label ids,
    // memoized: root first, one per function inlined there besides the one it
    // is in. Always returns a valid label (falls back to a raw hex name),
    // mirroring the convention macsampler.cpp's LiveLabeler established.
    QList<int> labelIdsFor(quint32 pid, quint64 addr);
    // Same idea as labelIdsFor(), but against the one global kernel symbol
    // table instead of a per-module ELF symtab.
    int kernelLabelIdFor(quint64 addr);

    // Replaces a dwarf-mode sample's frames with its unwound stack, and drops
    // the register and stack copy it took for that.
    void unwindDwarfSample(RawSample &sample);
    void addLeafCaller(RawSample &sample);

    // Resolves every decoded raw sample into a labelled ThreadSample. Assumes
    // any fetchable debug files are already in the cache. `progress` and
    // `progressBase`: see symbolize().
    void resolveSamples(const std::function<void(int)> &progress, int progressBase);

    bool isCanceled() const { return m_isCanceled && m_isCanceled(); }

    PerfSymbolizer m_symbolizer;
    SampleTraceData m_data;
    QHash<ModuleAddress, QList<int>> m_labelIds;
    QHash<quint64, int> m_unknownLabelIds; // addr -> label id, when no region covers it
    const std::function<bool(qint64)> m_sampleFilter;
    const std::function<bool()> m_isCanceled;
    QHash<quint64, int> m_kernelLabelIds;

    qint64 m_firstTimestampNs = -1;
    // The newest sample time seen yet: a lost-samples record carries no time
    // of its own, and perf writes it as soon as it notices the loss.
    quint64 m_latestSampleNs = 0;
    // What PERF_RECORD_LOST_SAMPLES records say was lost in all, and what the
    // PERF_RECORD_LOST ones found along the way add up to (see finish()).
    quint64 m_lostSamplesTotal = 0;
    quint64 m_lostSamplesLocated = 0;
    quint64 m_lastTsUs = 0;
    int m_receivedSamples = 0;

    // Raw (pre-symbolication) samples in final timestamp order, produced by
    // the decode pass and consumed by symbolize() once debug info is fetched.
    QList<RawSample> m_rawSamples;

    // Raw samples accumulated since the last PERF_RECORD_FINISHED_ROUND, held
    // back so they can be timestamp-sorted before joining m_rawSamples -- see
    // flushPendingRound(). MaxPendingSamples is a safety net, not the normal
    // flush trigger: real "perf record" streams emit a round roughly every
    // page's worth of samples per open event (tens to low hundreds), so this
    // would only be hit if a stream never emits FINISHED_ROUND at all.
    static constexpr qsizetype MaxPendingSamples = 200'000;
    QList<RawSample> m_pendingSamples;
    quint64 m_roundEndUs = 0;         // the latest sample since the last round ended
    quint64 m_previousRoundEndUs = 0; // and before that
};

quint64 PerfRecordDecoder::relativeUs(quint64 timeNs) const
{
    if (m_firstTimestampNs < 0)
        return 0;
    return quint64(qMax<qint64>(0, qint64(timeNs) - m_firstTimestampNs) / 1000);
}

void PerfRecordDecoder::mmap(const PerfData::Mmap &mmap)
{
    // Anonymous and special mappings ("[heap]", "[stack]") have nothing to
    // symbolize, except the vdso, which the symbolizer finds elsewhere.
    if (mmap.path.isEmpty() || (mmap.path.startsWith('[') && mmap.path != QLatin1StringView(vdsoName)))
        return;
    m_symbolizer.addMapping(mmap.pid, mmap.addr, mmap.len, mmap.pgoff, mmap.path, {},
                            mmap.executable);
}

// A child forked without exec() keeps its parent's mappings, which the
// kernel does not report again.
void PerfRecordDecoder::task(const PerfData::Task &task)
{
    if (!task.exit)
        m_symbolizer.addFork(task.ppid, task.pid);
}

void PerfRecordDecoder::comm(const PerfData::Comm &comm)
{
    // Keyed by tid alone, as SampleTraceData::threadNames is.
    if (!comm.name.isEmpty())
        m_data.threadNames.insert(comm.tid, comm.name);
}

// A ring buffer that overflowed before "perf record" drained it. The record
// has no time of its own; perf writes it as soon as it notices, so the loss is
// placed at the newest sample.
void PerfRecordDecoder::lost(const PerfData::Lost &lost)
{
    if (lost.count == 0)
        return;
    m_lostSamplesLocated += lost.count;
    addLostSamples(lost.count);
}

void PerfRecordDecoder::throttle(const PerfData::Throttle &throttle)
{
    if (throttle.unthrottle)
        return;
    const quint64 tsUs = relativeUs(throttle.time);
    m_data.throttledTsUs.append(
        m_data.throttledTsUs.isEmpty() ? tsUs : qMax(tsUs, m_data.throttledTsUs.last()));
}

Result<> PerfRecordDecoder::sample(const PerfData::Sample &sample)
{
    if (isCanceled())
        return ResultError(Tr::tr("The recording was canceled."));
    if (m_sampleFilter && !m_sampleFilter(qint64(sample.time)))
        return ResultOk;
    ++m_receivedSamples;

    // The call chain is innermost-first, with PERF_CONTEXT_* sentinels marking
    // the context of what follows. Kernel frames are tagged so symbolize() can
    // resolve them against /proc/kallsyms (see kernelLabelIdFor()) rather than
    // the process image -- though with kptr_restrict enabled (the default on
    // most distros) they read back as address 0 and stay unresolvable, same as
    // "perf record" itself warns about. Hypervisor frames are dropped outright:
    // there is no symbol table to resolve them against.
    // Past the first address of a context, the sampled one or where a system
    // call returns to, every address is a return address. It is looked up one
    // byte earlier, in the call, as the dwarf unwinder does (see
    // perfdwarfunwinder.cpp): the return address can be the next line, or the
    // start of the next function after a call that does not return.
    // For lbr the branch stack's call sites are the user callers. The user
    // part of the call chain is whatever a frame pointer walk found, in code
    // that typically has none; only its first address is kept.
    const QList<quint64> &branchFroms = sample.branchFroms;
    quint64 context = PerfContextUser;
    bool isFirstOfContext = true;
    QList<RawFrame> innermostFirst;
    innermostFirst.reserve(sample.callchain.size() + branchFroms.size());
    for (quint64 addr : sample.callchain) {
        if (isContextMarker(addr)) {
            context = addr;
            isFirstOfContext = true;
            continue;
        }
        const bool isReturnAddress = !isFirstOfContext;
        isFirstOfContext = false;
        if (context == PerfContextHv)
            continue;
        if (context == PerfContextUser && isReturnAddress && !branchFroms.isEmpty())
            continue;
        innermostFirst.append({isReturnAddress && addr != 0 ? addr - 1 : addr,
                               context == PerfContextKernel});
    }
    for (quint64 addr : branchFroms)
        innermostFirst.append({addr, false});

    // "--call-graph dwarf": the user registers, as PerfDwarfUnwinder takes
    // them, and the user stack, which starts at the sampled stack pointer.
    const PerfRegisterLayout &layout = hostRegisterLayout();
    QList<quint64> dwarfRegs;
    if (!sample.userRegs.isEmpty() && layout.ip >= 0) {
        dwarfRegs = sample.userRegs;
        dwarfRegs.resize(layout.count, 0);
    }

    // In dwarf mode "perf record" leaves the user part out of the kernel's
    // callchain (exclude_callchain_user): the user stack is unwound here
    // instead, from the leaf the captured registers hold. Kept as the sample's
    // own user frame, so it survives an unwind that recovers nothing more.
    if (!dwarfRegs.isEmpty() && dwarfRegs.at(layout.ip) != 0
        && std::none_of(innermostFirst.cbegin(), innermostFirst.cend(),
                        [](const RawFrame &frame) { return !frame.isKernel; })) {
        innermostFirst.append({dwarfRegs.at(layout.ip), false}); // outside any kernel frames
    }
    if (innermostFirst.isEmpty())
        return ResultOk; // matches macsampler.cpp: samples with no resolved stack are dropped

    m_data.pid = sample.pid;
    RawSample raw;
    raw.tid = sample.tid;
    raw.pid = sample.pid;
    raw.running = true; // a perf sample always fires while its thread is on-CPU
    if (m_firstTimestampNs < 0)
        m_firstTimestampNs = qint64(sample.time);
    m_latestSampleNs = qMax(m_latestSampleNs, sample.time);
    raw.tsUs = relativeUs(sample.time);

    raw.frames.reserve(innermostFirst.size());
    for (auto it = innermostFirst.crbegin(); it != innermostFirst.crend(); ++it) // root-first
        raw.frames.append(*it);
    // "--call-graph fp" on aarch64: perf samples the link register as well,
    // and no stack.
    if (sample.userStack.isEmpty() && layout.lr >= 0 && layout.lr < sample.userRegs.size()) {
        raw.linkRegister = withoutPointerAuthentication(hostPerfArchitecture(),
                                                        sample.userRegs.at(layout.lr));
    }
    raw.dwarfRegs = std::move(dwarfRegs);
    if (!raw.dwarfRegs.isEmpty())
        raw.dwarfStack = sample.userStack;

    // Held back, not appended directly: perf's pipe-mode stream isn't globally
    // time-sorted (samples from different per-CPU ring buffers can interleave
    // slightly out of order); see flushPendingRound() for how this gets sorted
    // out before landing in m_rawSamples.
    m_roundEndUs = qMax(m_roundEndUs, raw.tsUs);
    m_pendingSamples.append(std::move(raw));
    if (m_pendingSamples.size() >= MaxPendingSamples)
        flushPendingRound();
    return ResultOk;
}

// At the newest sample, merged with a loss already recorded there.
void PerfRecordDecoder::addLostSamples(quint64 count)
{
    const quint64 tsUs = relativeUs(m_latestSampleNs);
    if (!m_data.lostSamples.isEmpty() && m_data.lostSamples.last().tsUs == tsUs)
        m_data.lostSamples.last().count += count;
    else
        m_data.lostSamples.append({tsUs, count});
}

void PerfRecordDecoder::finish()
{
    flushPendingRound();
    // The total of lost samples "perf record" writes as it finishes includes
    // those it reported one by one along the way, so only what it adds beyond
    // them is a further loss.
    if (m_lostSamplesTotal > m_lostSamplesLocated) {
        addLostSamples(m_lostSamplesTotal - m_lostSamplesLocated);
        m_lostSamplesLocated = m_lostSamplesTotal;
    }
}

void PerfRecordDecoder::finishedRound()
{
    // "perf record" emits PERF_RECORD_FINISHED_ROUND once it has drained every
    // per-CPU ring buffer for a round. The samples up to where the round
    // before reached are then complete; later ones may still be followed by
    // earlier ones from a buffer read late, which the next round brings (the
    // same rule perf's own ordered-events flushing follows).
    flushPendingRound(m_previousRoundEndUs);
    m_previousRoundEndUs = qMax(m_previousRoundEndUs, m_roundEndUs);
    m_roundEndUs = 0;
}

void PerfRecordDecoder::flushPendingRound(quint64 untilUs)
{
    if (m_pendingSamples.isEmpty())
        return;
    // The clamp below is a backstop for a stream without rounds, which is
    // only flushed every MaxPendingSamples.
    std::stable_sort(m_pendingSamples.begin(), m_pendingSamples.end(),
                     [](const RawSample &a, const RawSample &b) { return a.tsUs < b.tsUs; });
    const auto end = std::upper_bound(m_pendingSamples.begin(), m_pendingSamples.end(), untilUs,
                                      [](quint64 us, const RawSample &s) { return us < s.tsUs; });
    m_rawSamples.reserve(m_rawSamples.size() + (end - m_pendingSamples.begin()));
    for (auto it = m_pendingSamples.begin(); it != end; ++it) {
        RawSample &sample = *it;
        // Now, rather than once the recording is over: a dwarf-mode sample
        // carries a copy of the stack, kilobytes of it, which is only needed
        // until it is unwound.
        unwindDwarfSample(sample);
        addLeafCaller(sample);
        sample.tsUs = qMax(sample.tsUs, m_lastTsUs);
        m_lastTsUs = sample.tsUs;
        m_rawSamples.append(std::move(sample));
    }
    m_pendingSamples.erase(m_pendingSamples.begin(), end);
}

QList<int> PerfRecordDecoder::labelIdsFor(quint32 pid, quint64 addr)
{
    if (const std::optional<ModuleAddress> at = m_symbolizer.moduleAddress(pid, addr)) {
        if (const auto it = m_labelIds.constFind(*at); it != m_labelIds.constEnd())
            return it.value();
        const ResolvedAddress resolved = m_symbolizer.resolve(*at);
        const QString module = FilePath::fromString(at->modulePath).fileName();
        const auto addLabel = [this, &module, &resolved](const QString &name, const QString &file,
                                                         int line) {
            SampleTraceData::Label label;
            label.module = module;
            label.file = file;
            label.line = line;
            label.name = name;
            label.offset = resolved.functionOffset;
            m_data.labels.append(label);
            return int(m_data.labels.size() - 1);
        };

        // Root first: the function, at the call site of what was inlined into
        // it, then each inlined function, down to the one the address is in,
        // at the address's own line.
        QList<int> ids;
        if (resolved.function.isEmpty()) {
            ids.append(addLabel(u"0x%1"_s.arg(resolved.linkAddress, 0, 16), resolved.file,
                                resolved.line));
        } else {
            QString name = resolved.function;
            for (auto it = resolved.inlined.crbegin(); it != resolved.inlined.crend(); ++it) {
                ids.append(addLabel(name, it->callFile, it->callLine));
                name = it->function;
            }
            ids.append(addLabel(name, resolved.file, resolved.line));
        }
        m_labelIds.insert(*at, ids);
        return ids;
    }

    if (int id = m_unknownLabelIds.value(addr, -1); id >= 0)
        return {id};
    SampleTraceData::Label label(u"0x%1"_s.arg(addr, 0, 16));
    label.offset = addr;
    const int id = int(m_data.labels.size());
    m_data.labels.append(label);
    m_unknownLabelIds.insert(addr, id);
    return {id};
}

int PerfRecordDecoder::kernelLabelIdFor(quint64 addr)
{
    if (int id = m_kernelLabelIds.value(addr, -1); id >= 0)
        return id;

    const ResolvedAddress resolved = m_symbolizer.resolveKernel(addr);
    SampleTraceData::Label label;
    label.module = u"[kernel]"_s;
    // A raw address where kallsyms has nothing, same convention as an
    // unresolved userspace frame.
    label.name = resolved.function.isEmpty() ? u"0x%1"_s.arg(addr, 0, 16) : resolved.function;
    label.offset = resolved.functionOffset;
    const int id = int(m_data.labels.size());
    m_data.labels.append(label);
    m_kernelLabelIds.insert(addr, id);
    return id;
}

void PerfRecordDecoder::symbolize(const std::function<void(int)> &progress)
{
    // The modules the samples are in, for the debug files they may lack.
    QSet<QString> modulePaths;
    for (const RawSample &sample : std::as_const(m_rawSamples)) {
        for (const RawFrame &frame : sample.frames) {
            if (frame.isKernel)
                continue;
            if (const std::optional<ModuleAddress> at = m_symbolizer.moduleAddress(sample.pid,
                                                                                   frame.addr)) {
                modulePaths.insert(at->modulePath);
            }
        }
    }

    // Progress budget across both stages: fetching (network-bound) gets a
    // share only when there is anything to fetch; resolving every sample
    // against the now-complete symbol tables gets the rest.
    constexpr int fetchWeight = 40;
    const bool fetched = m_symbolizer.fetchDebugFiles(modulePaths, [&progress](int percent) {
        if (progress)
            progress(percent * fetchWeight / 100);
    }, m_isCanceled);
    resolveSamples(progress, fetched ? fetchWeight : 0);
    if (progress)
        progress(100);
}

void PerfRecordDecoder::unwindDwarfSample(RawSample &sample)
{
    if (sample.dwarfRegs.isEmpty())
        return;
    // Falls back to the kernel-reported chain (just the leaf PC in dwarf
    // mode -- see sample()) when the unwind comes back empty.
    QList<quint64> pcs = m_symbolizer.unwind(sample.pid, hostPerfArchitecture(),
                                             sample.dwarfRegs, sample.dwarfStack);
    // Where the captured stack ends, or a frame is described by neither CFI
    // nor a frame record, the unwind can take any word for a return address,
    // which then points at data: the heap, the stack. Return addresses are in
    // code, so the call chain ends before the first one that is not.
    qsizetype inCode = qMin<qsizetype>(1, pcs.size()); // the sampled PC
    while (inCode < pcs.size() && m_symbolizer.isExecutable(sample.pid, pcs.at(inCode)))
        ++inCode;
    pcs.resize(inCode);
    if (!pcs.isEmpty()) {
        QList<RawFrame> frames;
        frames.reserve(pcs.size() + sample.frames.size());
        for (auto it = pcs.crbegin(); it != pcs.crend(); ++it) // root-first
            frames.append({*it, false});
        // The kernel frames a sample taken in a system call carries sit on top
        // of the user stack; only the user part is unwound.
        for (const RawFrame &frame : std::as_const(sample.frames)) {
            if (frame.isKernel)
                frames.append(frame);
        }
        sample.frames = std::move(frames);
    }
    sample.dwarfRegs = {};
    sample.dwarfStack = {};
}

// The kernel follows the frame records that functions save when they call
// another, so a frame pointer chain lacks the caller of a sampled function
// that has not saved its own: a leaf function, or one still in its prologue.
// On aarch64 that caller is still in the link register, which perf samples
// for it, as long as the CFI says the return address has not been saved.
void PerfRecordDecoder::addLeafCaller(RawSample &sample)
{
    if (sample.linkRegister == 0)
        return;
    const auto leaf = std::find_if(sample.frames.crbegin(), sample.frames.crend(),
                                   [](const RawFrame &frame) { return !frame.isKernel; });
    if (leaf == sample.frames.crend())
        return;
    const qsizetype leafIndex = sample.frames.crend() - leaf - 1; // root-first
    const quint64 caller = sample.linkRegister - 1; // in the call, as other callers are
    if (leafIndex > 0 && sample.frames.at(leafIndex - 1).addr == caller)
        return;
    if (!m_symbolizer.isReturnAddressInRegister(sample.pid, leaf->addr))
        return;
    sample.frames.insert(leafIndex, {caller, false});
}

void PerfRecordDecoder::resolveSamples(const std::function<void(int)> &progress, int progressBase)
{
    m_data.samples.reserve(m_rawSamples.size());
    const qsizetype total = m_rawSamples.size();
    qsizetype done = 0;
    for (const RawSample &raw : std::as_const(m_rawSamples)) {
        if ((done & 0x3ff) == 0 && isCanceled())
            break;
        SampleTraceData::ThreadSample sample;
        sample.tsUs = raw.tsUs;
        sample.tid = raw.tid;
        sample.running = raw.running;

        sample.frames.reserve(raw.frames.size());
        for (const RawFrame &frame : raw.frames) {
            if (frame.isKernel)
                sample.frames.append(kernelLabelIdFor(frame.addr));
            else
                sample.frames.append(labelIdsFor(raw.pid, frame.addr));
        }
        m_data.samples.append(std::move(sample));
        // Throttled like writeSampleTrace()'s own progress reporting
        // (sampletrace.cpp): frequent enough to look alive, rare enough
        // not to matter for a trace with millions of samples.
        if ((++done & 0x3ff) == 0 && progress)
            progress(progressBase + int((100 - progressBase) * done / total));
    }
    m_rawSamples.clear();
}

Result<SampleTraceData> PerfRecordReader::read(PerfByteQueue &queue,
                                               const std::function<void(int)> &progress)
{
    m_receivedSamples = 0;
    m_firstTimestampNs = -1;
    const auto canceled = [this] { return m_isCanceled && m_isCanceled(); };
    const auto canceledError = [] { return ResultError(Tr::tr("The recording was canceled.")); };
    PerfRecordDecoder decoder(m_downloadDebugInfo, m_debugInfoDownloadHandler, m_sampleFilter,
                              m_isCanceled);
    // An empty stream is no error but an empty recording: "perf record" never
    // got to write one, e.g. because perf_event_paranoid refused it.
    PerfData::PerfDataParser parser;
    if (Result<> r = parser.parseStream(queue, decoder); !r)
        return ResultError(r.error());
    // The last round has no trailing FINISHED_ROUND (nothing follows it to
    // prompt "perf record" to emit one) -- flush it explicitly now.
    decoder.finish();

    // Now that every sampled module is known, fetch any missing debug info (in
    // parallel) and resolve the raw samples into labelled ones -- one recipe.
    if (canceled())
        return canceledError();
    decoder.symbolize(progress);
    if (canceled())
        return canceledError();

    m_receivedSamples = decoder.receivedSamples();
    m_firstTimestampNs = decoder.firstTimestampNs();
    return decoder.data();
}

} // namespace Profiler::Internal
