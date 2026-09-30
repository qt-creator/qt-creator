// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfrecordreader.h"

#include "dwarflinetable.h"
#include "profilertr.h"

#ifdef WITH_LIBDW
#include "perfdwarfunwinder.h"
#endif

#include <utils/elfreader.h>
#include <utils/networkaccessmanager.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QTaskTree>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QtEndian>

#include <algorithm>
#include <memory>
#include <optional>
#include <unordered_map>

#ifdef Q_CC_GNU
#  include <cstdlib>
#  include <cxxabi.h>
#endif

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

// PerfByteQueue

void PerfByteQueue::push(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    std::lock_guard lock(m_mutex);
    m_chunks.push_back(data);
    m_cv.notify_one();
}

void PerfByteQueue::close()
{
    std::lock_guard lock(m_mutex);
    m_closed = true;
    m_cv.notify_one();
}

QByteArray PerfByteQueue::pop()
{
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [this] { return !m_chunks.empty() || m_closed; });
    if (m_chunks.empty())
        return {};
    QByteArray chunk = std::move(m_chunks.front());
    m_chunks.pop_front();
    return chunk;
}

bool PerfByteQueue::atEnd() const
{
    std::lock_guard lock(m_mutex);
    return m_chunks.empty() && m_closed;
}

namespace {

// perf.data record types (include/uapi/linux/perf_event.h). Only the ones
// this reader actually interprets are named; everything else is skipped by
// its own declared size.
enum PerfRecordType : quint32 {
    PerfRecordMmap = 1,
    PerfRecordLost = 2,
    PerfRecordComm = 3,
    PerfRecordThrottle = 5,
    PerfRecordSample = 9,
    PerfRecordMmap2 = 10,
    PerfRecordLostSamples = 13,
    PerfRecordHeaderAttr = 64,
    // { u32 size; u32 pad; }: in pipe mode the `size` bytes of tracing data
    // follow the record rather than being part of it.
    PerfRecordHeaderTracingData = 66,
    // Userspace-level marker "perf record" itself emits (not a kernel ABI
    // record) once it has drained every per-CPU ring buffer for this round;
    // see PerfRecordDecoder::flushPendingRound() for what that buys us.
    PerfRecordFinishedRound = 68,
};

// perf_event_attr::sample_type bits actually understood below, in the exact
// order the kernel places the corresponding fields in a PERF_RECORD_SAMPLE
// (include/uapi/linux/perf_event.h, the big comment on "PERF_RECORD_SAMPLE").
// Bits outside this mask are rejected outright rather than mis-parsed.
constexpr quint64 SampleIdentifier = 1ull << 16;
constexpr quint64 SampleIp = 1ull << 0;
constexpr quint64 SampleTid = 1ull << 1;
constexpr quint64 SampleTime = 1ull << 2;
constexpr quint64 SampleAddr = 1ull << 3;
constexpr quint64 SampleRead = 1ull << 4;
constexpr quint64 SampleCallchain = 1ull << 5;
constexpr quint64 SampleId = 1ull << 6;
constexpr quint64 SampleCpu = 1ull << 7;
constexpr quint64 SamplePeriod = 1ull << 8;
constexpr quint64 SampleStreamId = 1ull << 9;
constexpr quint64 SampleRaw = 1ull << 10;
constexpr quint64 SampleBranchStack = 1ull << 11;
constexpr quint64 SampleRegsUser = 1ull << 12;
constexpr quint64 SampleStackUser = 1ull << 13;

constexpr quint64 SupportedSampleBits = SampleIdentifier | SampleIp | SampleTid | SampleTime
                                        | SampleAddr | SampleRead | SampleCallchain | SampleId
                                        | SampleCpu | SamplePeriod | SampleStreamId | SampleRaw
                                        | SampleBranchStack | SampleRegsUser | SampleStackUser;

// Perf's x86-64 general-purpose register count (PERF_REG_X86_64_MAX in
// arch/x86/include/uapi/asm/perf_regs.h); bit i in perf_event_attr's own
// sample_regs_user mask requests PERF_REG_X86_* register i. See
// perfdwarfunwinder.h's UnwindInput for what consumes this.
constexpr int MaxDwarfRegs = 24;
// PERF_REG_X86_SP: PERF_SAMPLE_STACK_USER's captured bytes always start at
// the sampled stack pointer (see the kernel's perf_output_sample_ustack()).
constexpr int PerfRegX86Sp = 7;
constexpr int PerfRegX86Ip = 8;

// perf_event_attr::read_format bits, needed only to correctly size (and thus
// skip past) a PERF_SAMPLE_READ field -- call-stack sampling has no use for
// the counter-read value itself.
constexpr quint64 FormatTotalTimeEnabled = 1ull << 0;
constexpr quint64 FormatTotalTimeRunning = 1ull << 1;
constexpr quint64 FormatId = 1ull << 2;
constexpr quint64 FormatGroup = 1ull << 3;

// perf_event_attr::branch_sample_type: with it, a PERF_SAMPLE_BRANCH_STACK
// carries a u64 hw_idx between its nr and its entries. "perf record
// --call-graph lbr" asks for it wherever the kernel supports it.
constexpr quint64 BranchHwIndex = 1ull << 17;

// Callchain sentinel values (include/uapi/linux/perf_event.h): not real
// addresses, they mark which context the *following* entries belong to.
constexpr quint64 PerfContextHv = quint64(-32);
constexpr quint64 PerfContextKernel = quint64(-128);
constexpr quint64 PerfContextUser = quint64(-512);
constexpr quint64 PerfContextMax = quint64(-4095); // anything >= this is a context marker

bool isContextMarker(quint64 v) { return v >= PerfContextMax; }

// The subset of one perf_event_attr's fields needed to decode samples that
// reference it (see PerfRecordDecoder::handleAttr()'s multi-event id lookup).
struct AttrInfo
{
    quint64 sampleType = 0;
    quint64 readFormat = 0;
    // perf_event_attr::sample_regs_user (offset 80): which PERF_REG_X86_*
    // registers PERF_SAMPLE_REGS_USER carries, and in which order -- see
    // handleSample()'s dwarf-mode register parsing. 0 (the default) when the
    // attr record is too short to have this field at all (an older kernel
    // ABI, or simply a non-dwarf-mode recording), which correctly implies
    // "no registers requested" either way.
    quint64 sampleRegsUser = 0;
    quint64 branchSampleType = 0;
};

// One resolved ELF function symbol, sorted by address for binary search.
struct ElfSymbol
{
    quint64 address = 0;
    quint64 size = 0;
    QString name;
};

// Per-binary symbol table (lazily loaded, cached for the recording session)
// plus a memo of label ids already produced for specific file offsets in it.
struct ModuleSymbols
{
    QList<ElfSymbol> symbols; // sorted by address
    QHash<quint64, int> labelIdByOffset;

    // The file that actually supplied `symbols` -- the mmap'd binary itself
    // when it wasn't stripped, or the separately-fetched debug file
    // otherwise (see symbolsFor()). ".debug_line" is read from here, lazily,
    // by lineTableFor(); std::nullopt until that first access, distinct from
    // an empty-but-parsed table (a module with no usable line info at all).
    FilePath debugInfoPath;
    std::optional<DwarfLineTable> lineTable;
};

// One "perf record" mmap: [addr, addr+len) in the target's address space maps
// to `path` starting at file offset `pgoff`.
// The link-time address of `fileOffset` within the part [fileBegin, fileEnd)
// of a file one mapping covers, by the file's loaded `sections`: by the
// section holding it, or else by any section in that part, all of one segment
// sharing its offset-to-address delta. Symbols, line rows and CFI are all at
// link-time addresses, which a file offset -- what a mapping gives -- only is
// where a segment's p_vaddr is its p_offset: not for a non-PIE executable, nor
// for lld or mold output.
quint64 linkAddress(const QList<ElfSectionHeader> &sections, quint64 fileOffset,
                    quint64 fileBegin, quint64 fileEnd)
{
    const ElfSectionHeader *inRange = nullptr;
    for (const ElfSectionHeader &section : sections) {
        if (fileOffset >= section.offset && fileOffset < section.offset + section.size)
            return section.addr + (fileOffset - section.offset);
        if (!inRange && section.offset >= fileBegin && section.offset < fileEnd)
            inRange = &section;
    }
    if (inRange)
        return fileOffset - inRange->offset + inRange->addr;
    return fileOffset;
}

struct MappedRegion
{
    quint64 addr = 0;
    quint64 len = 0;
    quint64 pgoff = 0;
    QString path;
};

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
    // perfdwarfunwinder.h). Sized MaxDwarfRegs and indexed by PERF_REG_X86_*
    // number when present; empty when this sample carried no usable user
    // registers at all (PERF_SAMPLE_REGS_USER wasn't requested, or its abi
    // field came back 0 -- see handleSample()).
    QList<quint64> dwarfRegs;
    quint64 dwarfStackAddr = 0; // virtual address dwarfStack[0] corresponds to
    QByteArray dwarfStack;
};

quint64 readU64(const char *p) { return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(p)); }
quint32 readU32(const char *p) { return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(p)); }

// NUL-terminated string starting at `p`, never reading past `end`.
QString readCString(const char *p, const char *end)
{
    const char *nul = p;
    while (nul < end && *nul != '\0')
        ++nul;
    return QString::fromUtf8(p, int(nul - p));
}

// Demangles an Itanium C++ ABI mangled `name` (the convention every
// compiler targeted by "perf record" on Linux uses), or returns it
// unchanged if it isn't one. Symbol tables carry mangled names as-is; this
// is the only place that needs to know that, since it runs once per label
// (see labelIdFor()/kernelLabelIdFor(), both memoized).
QString demangleName(const QString &name)
{
#ifdef Q_CC_GNU
    if (name.startsWith(u"_Z"_s)) {
        const QByteArray mangled = name.toUtf8();
        int status = 0;
        if (char *demangled = abi::__cxa_demangle(mangled.constData(), nullptr, nullptr, &status)) {
            const QString result = QString::fromUtf8(demangled);
            std::free(demangled);
            if (status == 0)
                return result;
        }
    }
#endif
    return name;
}

// Extracts STT_FUNC symbols from `path`'s ELF symbol table into `out`.
// Returns true if the full ".symtab" was used, false if it fell back to the
// stripped-binary ".dynsym" subset (or found nothing) -- the caller uses that
// to decide whether asking debuginfod for a richer debug file is worthwhile.
// `buildId`, if given, is filled with the module's hex build-id for that same
// debuginfod lookup.
bool loadElfFunctionSymbols(const FilePath &path, QList<ElfSymbol> &out, QByteArray *buildId = nullptr)
{
    ElfReader reader(path);
    const ElfData elfData = reader.readHeaders();
    if (buildId)
        *buildId = elfData.buildId;
    const bool is64Bit = elfData.elfclass == Elf_ELFCLASS64;
    const qsizetype symEntrySize = is64Bit ? 24 : 16;

    // .symtab/.strtab first (full static symbols); .dynsym/.dynstr as a
    // fallback for stripped binaries, which only keep the dynamic subset.
    static const QByteArray symtabNames[][2] = {{".symtab", ".strtab"}, {".dynsym", ".dynstr"}};
    for (int table = 0; table < 2; ++table) {
        std::unique_ptr<ElfMapper> symtab = reader.readSection(symtabNames[table][0]);
        std::unique_ptr<ElfMapper> strtab = reader.readSection(symtabNames[table][1]);
        if (!symtab || !strtab || symtab->fdlen == 0)
            continue;
        const qsizetype count = qsizetype(symtab->fdlen) / symEntrySize;
        out.reserve(out.size() + count);
        for (qsizetype i = 0; i < count; ++i) {
            const char *sym = symtab->start + i * symEntrySize;
            quint32 nameIndex;
            quint64 value, size;
            quint8 info;
            if (is64Bit) {
                nameIndex = readU32(sym);
                info = quint8(sym[4]);
                value = readU64(sym + 8);
                size = readU64(sym + 16);
            } else {
                nameIndex = readU32(sym);
                value = readU32(sym + 4);
                size = readU32(sym + 8);
                info = quint8(sym[12]);
            }
            constexpr quint8 SttFunc = 2;
            if ((info & 0xf) != SttFunc || value == 0)
                continue;
            if (nameIndex >= strtab->fdlen)
                continue;
            const QString name = readCString(strtab->start + nameIndex, strtab->start + strtab->fdlen);
            if (name.isEmpty())
                continue;
            out.append({value, size, name});
        }
        return table == 0; // table 0 == ".symtab", the full (non-stripped) table
    }
    return false;
}

} // namespace

// Owns the actual parsing state; PerfRecordReader::read() just drives it.
class PerfRecordDecoder
{
public:
    PerfRecordDecoder(bool downloadDebugInfo,
                      const std::function<void(int, const QString &)> &onDebugInfoDownload,
                      const std::function<bool(qint64)> &sampleFilter,
                      const std::function<bool()> &isCanceled)
        : m_downloadDebugInfo(downloadDebugInfo)
        , m_onDebugInfoDownload(onDebugInfoDownload)
        , m_sampleFilter(sampleFilter)
        , m_isCanceled(isCanceled)
    {}

    Result<> handleRecord(quint32 type, const QByteArray &record);
    // Flushes whatever the last, incomplete round left pending -- call once
    // after the stream ends (a clean recording's last round has no trailing
    // FINISHED_ROUND, since nothing follows it to prompt "perf record" to
    // emit one), and adds the lost samples only a final total reported.
    void finish();

    // Fetches any missing debug info and turns the decoded raw samples into
    // labelled ones, both as tasks in a single QtTaskTree recipe: the
    // per-module debug-file fetches (from a debuginfod server into the shared
    // cache) run in parallel, then a QSyncTask resolves every raw frame
    // against the now-complete symbol tables. The fetch stage is a no-op when
    // downloading is disabled or no debuginfod server is configured.
    // `progress`, if set, is called with 0..100 across both stages (see
    // PerfRecordReader::read()).
    void symbolize(const std::function<void(int)> &progress);

    SampleTraceData &data() { return m_data; }
    int receivedSamples() const { return m_receivedSamples; }
    qint64 firstTimestampNs() const { return m_firstTimestampNs; }

private:
    Result<> handleAttr(const QByteArray &record);
    void handleMmap(const QByteArray &record, bool isMmap2);
    void handleComm(const QByteArray &record);
    void handleLost(const QByteArray &record);
    void handleLostSamples(const QByteArray &record);
    void addLostSamples(quint64 count);
    void handleThrottle(const QByteArray &record);
    // Microseconds since the first sample, as a sample's own tsUs.
    quint64 relativeUs(quint64 timeNs) const;
    Result<> handleSample(const QByteArray &record);
    void flushPendingRound();

    // The mmap'd region covering `addr` in `pid`'s address space, or nullptr.
    // Searches most-recently-added first so a later mmap that reused an address
    // range wins over a stale earlier one.
    const MappedRegion *findRegion(quint32 pid, quint64 addr) const;

    // Resolves one absolute address in `pid`'s address space to a label id,
    // memoized. Always returns a valid label (falls back to a raw hex name),
    // mirroring the convention macsampler.cpp's LiveLabeler established.
    int labelIdFor(quint32 pid, quint64 addr);
    ModuleSymbols &symbolsFor(const QString &path);
    // The loaded sections of the binary at `path` (see linkAddress()),
    // memoized apart from symbolsFor(), which must not run before debug files
    // are fetched.
    const QList<ElfSectionHeader> &loadSectionsFor(const QString &path);
#ifdef WITH_LIBDW
    // The pid's unwinder, with every module the pid has mapped so far.
    PerfDwarfUnwinder *dwarfUnwinderFor(quint32 pid);
    // Replaces a dwarf-mode sample's frames with its unwound stack, and drops
    // the register and stack copy it took for that.
    void unwindDwarfSample(RawSample &sample);
#endif

    // `module`'s parsed ".debug_line" (from module.debugInfoPath, the same
    // file its symbols came from), lazily parsed and cached on first call.
    // Empty (never null) when that file has no ".debug_line" at all.
    const DwarfLineTable &lineTableFor(ModuleSymbols &module);

    // Resolves every decoded raw sample into a labelled ThreadSample -- the
    // body of symbolize()'s QSyncTask. Assumes any fetchable debug files are
    // already in the cache. `progress` and `progressBase`: see symbolize().
    void resolveSamples(const std::function<void(int)> &progress, int progressBase);

    // Same idea as labelIdFor(), but against the one global kernel symbol
    // table instead of a per-module ELF symtab.
    int kernelLabelIdFor(quint64 addr);
    void ensureKallsymsLoaded();

    // The shared-cache path (existing or not) of the debug file for `buildId`.
    FilePath debuginfodCachePath(const QByteArray &buildId) const;
    // The cached debug file for `buildId`, if one is present in the shared
    // cache (populated locally, by resolveDebugInfo(), or by perf/gdb). Never
    // touches the network -- fetching is resolveDebugInfo()'s job.
    std::optional<FilePath> debuginfodDebugFile(const QByteArray &buildId);
    void ensureDebuginfodConfig();

    SampleTraceData m_data;
    QHash<quint32, QList<MappedRegion>> m_regionsByPid; // most-recently-added last
    QHash<QString, ModuleSymbols> m_moduleCache;
    QHash<quint64, int> m_unknownLabelIds; // addr -> label id, when no region covers it

    // debuginfod configuration, read once from the environment (see
    // ensureDebuginfodConfig()). Empty m_debuginfodUrls means network fetching
    // is disabled (the local cache is still consulted). The default timeout
    // is deliberately much shorter than elfutils/gdb/perf's own (30s): those
    // are thorough-analysis tools where waiting is expected, but this is a
    // quick sample-and-view tool where debug info is a nice-to-have
    // enrichment, not worth a long silent stall against a slow or
    // unreachable server (a real, commonly-preconfigured case: Ubuntu sets
    // DEBUGINFOD_URLS system-wide by default). An explicit DEBUGINFOD_TIMEOUT
    // still overrides this, same as it does for those other tools.
    const bool m_downloadDebugInfo;
    const std::function<void(int, const QString &)> m_onDebugInfoDownload;
    const std::function<bool(qint64)> m_sampleFilter;
    const std::function<bool()> m_isCanceled;
    bool m_debuginfodChecked = false;
    QStringList m_debuginfodUrls;
    FilePath m_debuginfodCacheDir;
    int m_debuginfodTimeoutMs = 5000;

    // /proc/kallsyms, read once per session (see ensureKallsymsLoaded()),
    // sorted by address for the same binary-search pattern as ELF symbols.
    bool m_kallsymsLoaded = false;
    QList<ElfSymbol> m_kallsyms;
    QHash<quint64, int> m_kernelLabelIds;

    // Populated from each PERF_RECORD_HEADER_ATTR's trailing ids[] array (see
    // handleAttr()); m_globalAttr is the first attr seen, used as a fallback
    // when a sample has no id field to demux by, and as the sole source when
    // there is exactly one active attr (the common, single-event case).
    QHash<quint64, AttrInfo> m_attrsById;
    std::optional<AttrInfo> m_globalAttr;

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

#ifdef WITH_LIBDW
    // Built as a pid's dwarf-mode samples are flushed (see
    // unwindDwarfSample()) -- only for a pid that has any -- from that pid's
    // own mmap'd modules alone, and brought up to date as it maps more. Keyed by pid rather than shared across the whole recording:
    // "perf record" inherits child processes by default (no --no-inherit in
    // perfRecordArguments()), so a multi-process recording can easily contain
    // several *different* address spaces; two unrelated processes mapping
    // different files at the same address (common for non-PIE binaries, or
    // just ASLR coincidence) would otherwise get silently conflated into one
    // Dwfl, corrupting -- or, as observed against a real recording, crashing
    // -- libdw's own module/CFI lookup. Left empty for an fp/lbr recording,
    // which needs no unwinding at all. std::unordered_map, not QHash: a
    // move-only value (PerfDwarfUnwinder is neither copyable nor movable,
    // owning a raw Dwfl handle) doesn't fit QHash's implicitly-shared,
    // copy-on-write design.
    struct DwarfUnwinderEntry
    {
        std::unique_ptr<PerfDwarfUnwinder> unwinder;
        qsizetype regionCount = -1; // how many of the pid's mappings it knows
    };
    std::unordered_map<quint32, DwarfUnwinderEntry> m_dwarfUnwinders;
#endif
    QHash<QString, QList<ElfSectionHeader>> m_loadSections;
};

Result<> PerfRecordDecoder::handleAttr(const QByteArray &record)
{
    // perf_event_attr, fields we need (include/uapi/linux/perf_event.h):
    // u32 type (0); u32 size (4); u64 config (8); u64 sample_period_or_freq
    // (16); u64 sample_type (24); u64 read_format (32). `size` is the attr's
    // own reported length (varies by kernel version/feature set) -- used
    // below to find where the attr struct ends and its trailing ids[] (if
    // any) begins, rather than assuming a fixed size.
    const char *attr = record.constData() + 8;
    if (record.size() < 8 + 40)
        return ResultError(Tr::tr("Truncated perf attribute record."));
    const quint32 attrSize = readU32(attr + 4);
    if (attrSize < 40 || 8 + qsizetype(attrSize) > record.size())
        return ResultError(Tr::tr("Truncated perf attribute record."));

    const quint64 sampleType = readU64(attr + 24);
    const quint64 readFormat = readU64(attr + 32);
    if (sampleType & ~SupportedSampleBits) {
        return ResultError(
            Tr::tr("This backend cannot decode this perf recording: it uses a sample "
                   "format this simplified reader does not support (sample_type=0x%1). "
                   "Only frame-pointer call graphs with standard fields are supported.")
                .arg(QString::number(sampleType, 16)));
    }

    // sample_regs_user sits at offset 80 (see perf_event_attr in
    // include/uapi/linux/perf_event.h); only read it when this attr record is
    // actually long enough to carry it (an older kernel ABI may report a
    // shorter attrSize, in which case the field simply does not exist and 0 --
    // "no registers requested" -- is the correct value anyway).
    const quint64 sampleRegsUser = attrSize >= 88 ? readU64(attr + 80) : 0;
    const quint64 branchSampleType = attrSize >= 80 ? readU64(attr + 72) : 0;
    const AttrInfo info{sampleType, readFormat, sampleRegsUser, branchSampleType};
    if (!m_globalAttr)
        m_globalAttr = info;

    // Pipe-mode framing (distinct from the file-mode PerfFileSection-indirected
    // id table): the record body is the attr struct (attrSize bytes) followed
    // directly by a trailing u64 ids[] array filling out the rest of the
    // record. Each id names one opened event using this attr (e.g. one per
    // CPU, or one per multiplexed "-e" event) -- see handleSample()'s lookup.
    const qsizetype idsBytes = record.size() - 8 - attrSize;
    const qsizetype nIds = idsBytes / 8;
    const char *idsStart = attr + attrSize;
    for (qsizetype i = 0; i < nIds; ++i)
        m_attrsById.insert(readU64(idsStart + i * 8), info);
    return ResultOk;
}

void PerfRecordDecoder::handleMmap(const QByteArray &record, bool isMmap2)
{
    const char *p = record.constData() + 8;
    const char *end = record.constData() + record.size();
    if (p + 8 > end)
        return;
    const quint32 pid = readU32(p);
    p += 8; // pid, tid
    if (p + 24 > end)
        return;
    MappedRegion region;
    region.addr = readU64(p);
    p += 8;
    region.len = readU64(p);
    p += 8;
    region.pgoff = readU64(p);
    p += 8;
    if (isMmap2) {
        // maj, min (4+4), ino (8), ino_generation (8), prot, flags (4+4)
        p += 4 + 4 + 8 + 8 + 4 + 4;
        if (p > end)
            return;
    }
    region.path = readCString(p, end);
    if (region.path.isEmpty() || region.path.startsWith('['))
        return; // anonymous/special mapping (e.g. "[heap]", "[stack]"): nothing to symbolize
    m_regionsByPid[pid].append(region);
}

quint64 PerfRecordDecoder::relativeUs(quint64 timeNs) const
{
    if (m_firstTimestampNs < 0)
        return 0;
    return quint64(qMax<qint64>(0, qint64(timeNs) - m_firstTimestampNs) / 1000);
}

// PERF_RECORD_LOST: { u64 id; u64 lost; } -- a ring buffer that overflowed
// before "perf record" drained it.
void PerfRecordDecoder::handleLost(const QByteArray &record)
{
    if (record.size() < 8 + 16)
        return;
    const quint64 count = readU64(record.constData() + 8 + 8);
    if (count == 0)
        return;
    m_lostSamplesLocated += count;
    addLostSamples(count);
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

// PERF_RECORD_LOST_SAMPLES: { u64 lost; } -- the kernel's count of an event's
// lost samples, which "perf record" writes as it finishes. That count covers
// the losses PERF_RECORD_LOST already reported one by one, so only what it
// adds beyond them is new.
void PerfRecordDecoder::handleLostSamples(const QByteArray &record)
{
    if (record.size() < 8 + 8)
        return;
    m_lostSamplesTotal += readU64(record.constData() + 8);
}

void PerfRecordDecoder::finish()
{
    flushPendingRound();
    if (m_lostSamplesTotal > m_lostSamplesLocated) {
        addLostSamples(m_lostSamplesTotal - m_lostSamplesLocated);
        m_lostSamplesLocated = m_lostSamplesTotal;
    }
}

// PERF_RECORD_THROTTLE: { u64 time; u64 id; u64 stream_id; } -- the kernel
// lowered the sampling rate, because more samples were due than it takes.
void PerfRecordDecoder::handleThrottle(const QByteArray &record)
{
    if (record.size() < 8 + 8)
        return;
    const quint64 tsUs = relativeUs(readU64(record.constData() + 8));
    m_data.throttledTsUs.append(
        m_data.throttledTsUs.isEmpty() ? tsUs : qMax(tsUs, m_data.throttledTsUs.last()));
}

void PerfRecordDecoder::handleComm(const QByteArray &record)
{
    const char *p = record.constData() + 8;
    const char *end = record.constData() + record.size();
    if (p + 8 > end)
        return;
    readU32(p); // pid, unused: threadNames is keyed by tid alone (see SampleTraceData)
    const quint32 tid = readU32(p + 4);
    p += 8;
    const QString name = readCString(p, end);
    if (!name.isEmpty())
        m_data.threadNames.insert(tid, name);
}

Result<> PerfRecordDecoder::handleSample(const QByteArray &record)
{
    if (!m_globalAttr)
        return ResultError(Tr::tr("Got a sample before any attribute record."));

    const char *base = record.constData() + 8;
    const char *end = record.constData() + record.size();

    // Multi-event ("-e a,b,...") recordings interleave samples belonging to
    // different attrs in one stream; each such sample carries an id field
    // naming which attr it came from (see handleAttr()). Peek it -- without
    // consuming any bytes yet -- using the first attr's own field layout: the
    // kernel always adds PERF_SAMPLE_IDENTIFIER (a fixed id at offset 0)
    // whenever distinct sample_types are multiplexed together, precisely so
    // this demux never has a layout chicken-and-egg problem. A bare
    // PERF_SAMPLE_ID (whose offset depends on which of IP/TID/TIME/ADDR
    // precede it) only stays unambiguous for one uniform sample_type, which
    // is exactly the single-attr case below that needs no lookup at all.
    AttrInfo attrInfo = *m_globalAttr;
    if (m_attrsById.size() > 1) {
        const quint64 refType = m_globalAttr->sampleType;
        std::optional<qsizetype> idOffset;
        if (refType & SampleIdentifier) {
            idOffset = 0;
        } else if (refType & SampleId) {
            qsizetype offset = 0;
            if (refType & SampleIp)
                offset += 8;
            if (refType & SampleTid)
                offset += 8;
            if (refType & SampleTime)
                offset += 8;
            if (refType & SampleAddr)
                offset += 8;
            idOffset = offset;
        }
        if (idOffset && base + *idOffset + 8 <= end) {
            const quint64 id = readU64(base + *idOffset);
            if (auto it = m_attrsById.constFind(id); it != m_attrsById.constEnd())
                attrInfo = it.value();
        }
    }
    const quint64 sampleType = attrInfo.sampleType;
    const quint64 readFormat = attrInfo.readFormat;

    const char *p = base;
    bool truncated = false;
    // Consumes and returns the next 8 bytes if `bit` is set in sample_type;
    // returns 0 (and flags `truncated`) if the field is declared present but
    // the record has no room left for it -- callers must check `truncated`
    // once after the whole fixed-field block, not per field, since a field
    // bit being set is a hard, self-consistent guarantee from the attr record.
    const auto takeIfSet = [&](quint64 bit) -> quint64 {
        if (!(sampleType & bit))
            return 0;
        if (p + 8 > end) {
            truncated = true;
            return 0;
        }
        const quint64 v = readU64(p);
        p += 8;
        return v;
    };

    takeIfSet(SampleIdentifier);
    const quint64 ip = takeIfSet(SampleIp);
    quint32 pid = 0, tid = 0;
    if (sampleType & SampleTid) {
        if (p + 8 > end) {
            truncated = true;
        } else {
            pid = readU32(p);
            tid = readU32(p + 4);
            p += 8;
        }
    }
    const quint64 timeNs = takeIfSet(SampleTime);
    takeIfSet(SampleAddr);
    takeIfSet(SampleId);
    takeIfSet(SampleStreamId);
    takeIfSet(SampleCpu);
    takeIfSet(SamplePeriod);

    // PERF_SAMPLE_READ: a counter-read value we have no use for when merely
    // sampling call stacks -- skip it, don't reject it. Its own size depends
    // on read_format, an independent bitmask on the owning attr (confirmed
    // wire shape: non-group "{u64 value; [time_enabled;] [time_running;] u64
    // id;}", group "{u64 nr; [time_enabled;] [time_running;] {u64 value; u64
    // id;}[nr]}", with the trailing id present only when FORMAT_ID is set).
    if (!truncated && (sampleType & SampleRead)) {
        if (readFormat & FormatGroup) {
            if (p + 8 > end) {
                truncated = true;
            } else {
                const quint64 nr = readU64(p);
                p += 8;
                qsizetype groupHeader = 0;
                if (readFormat & FormatTotalTimeEnabled)
                    groupHeader += 8;
                if (readFormat & FormatTotalTimeRunning)
                    groupHeader += 8;
                if (p + groupHeader > end) {
                    truncated = true;
                } else {
                    p += groupHeader;
                    const qsizetype perEntry = 8 + ((readFormat & FormatId) ? 8 : 0);
                    if (nr > quint64(end - p) / perEntry)
                        truncated = true;
                    else
                        p += qsizetype(nr) * perEntry;
                }
            }
        } else {
            qsizetype fixed = 8;
            if (readFormat & FormatTotalTimeEnabled)
                fixed += 8;
            if (readFormat & FormatTotalTimeRunning)
                fixed += 8;
            if (readFormat & FormatId)
                fixed += 8;
            if (p + fixed > end)
                truncated = true;
            else
                p += fixed;
        }
    }
    if (truncated)
        return ResultError(Tr::tr("Truncated perf sample record."));

    QList<quint64> ips;
    if (sampleType & SampleCallchain) {
        if (p + 8 > end)
            return ResultError(Tr::tr("Truncated perf sample record."));
        const quint64 nr = readU64(p);
        p += 8;
        if (nr > quint64(end - p) / 8)
            return ResultError(Tr::tr("Truncated perf sample callchain."));
        ips.reserve(qsizetype(nr));
        for (quint64 i = 0; i < nr; ++i) {
            ips.append(readU64(p));
            p += 8;
        }
    } else if (sampleType & SampleIp) {
        ips.append(ip); // no callchain requested: at least keep the leaf PC
    }

    // PERF_SAMPLE_RAW: a tracepoint's own event-specific fields (e.g.
    // sched_switch's prev_pid/next_pid). Decoding them needs the trace event
    // format blob and a schema extension this reader does not have; skip the
    // payload so a tracepoint-triggered sample's *call stack* still decodes.
    if (sampleType & SampleRaw) {
        if (p + 4 > end)
            return ResultError(Tr::tr("Truncated perf sample record."));
        const quint32 rawSize = readU32(p);
        p += 4;
        if (rawSize > quint64(end - p))
            return ResultError(Tr::tr("Truncated perf sample record."));
        p += rawSize;
    }

    // PERF_SAMPLE_BRANCH_STACK ("--call-graph lbr"): a hardware branch-history
    // buffer (Intel, some AMD CPUs) of the last few taken branches, each a
    // {from,to,flags} triple. Needs no unwinding at all -- unlike dwarf mode,
    // see PERF_SAMPLE_REGS_USER below -- the "from" addresses are the user
    // callers, already resolved; see the loop below that turns ips[] into
    // frames. "to" and the misprediction/cycle-count/type "flags" word aren't
    // needed for a plain call chain. A `hw_idx` sits between `nr` and the
    // entries when the attr's branch_sample_type has
    // PERF_SAMPLE_BRANCH_HW_INDEX.
    QList<quint64> branchFroms;
    if (sampleType & SampleBranchStack) {
        if (p + 8 > end)
            return ResultError(Tr::tr("Truncated perf sample record."));
        const quint64 bnr = readU64(p);
        p += 8;
        if (attrInfo.branchSampleType & BranchHwIndex) {
            if (p + 8 > end)
                return ResultError(Tr::tr("Truncated perf branch stack."));
            p += 8;
        }
        constexpr qsizetype BranchEntrySize = 24; // {u64 from; u64 to; u64 flags;}
        if (bnr > quint64(end - p) / BranchEntrySize)
            return ResultError(Tr::tr("Truncated perf branch stack."));
        branchFroms.reserve(qsizetype(bnr));
        for (quint64 i = 0; i < bnr; ++i) {
            branchFroms.append(readU64(p));
            p += BranchEntrySize;
        }
    }

    // PERF_SAMPLE_REGS_USER ("--call-graph dwarf"): the x86-64 GPR values the
    // kernel captured, feeding PerfDwarfUnwinder's DWARF CFI unwind later (see
    // resolveSamples()) -- the kernel's own reported chain above is normally
    // just the leaf PC in this mode, since frame-pointer chasing (what
    // PERF_SAMPLE_CALLCHAIN uses) is exactly what dwarf mode exists to work
    // around. Wire shape: a u64 `abi` (enum perf_sample_regs_abi) always
    // comes first; only when it is non-zero do `popcount(sample_regs_user)`
    // register values follow, one per set mask bit, in ascending bit order --
    // abi == 0 means the kernel had no usable user registers for this sample
    // (e.g. it fired while the thread was in the kernel with no user
    // context), and no register values are emitted at all in that case.
    QList<quint64> dwarfRegs;
    if (sampleType & SampleRegsUser) {
        if (p + 8 > end)
            return ResultError(Tr::tr("Truncated perf sample record."));
        const quint64 abi = readU64(p);
        p += 8;
        if (abi != 0) {
            const int regCount = qPopulationCount(attrInfo.sampleRegsUser);
            if (p + qsizetype(regCount) * 8 > end)
                return ResultError(Tr::tr("Truncated perf sample record."));
            dwarfRegs.resize(MaxDwarfRegs, 0);
            int slot = 0;
            for (int bit = 0; bit < 64; ++bit) {
                if (!(attrInfo.sampleRegsUser & (1ull << bit)))
                    continue;
                const quint64 value = readU64(p + slot * 8);
                if (bit < MaxDwarfRegs)
                    dwarfRegs[bit] = value;
                ++slot;
            }
            p += qsizetype(regCount) * 8;
        }
    }

    // PERF_SAMPLE_STACK_USER ("--call-graph dwarf"): a captured chunk of the
    // userspace stack the CFI unwinder walks, always starting at the sampled
    // stack pointer. `size` is what was *requested* ("--call-graph
    // dwarf,<size>"'s own argument, see PerfSettings::perfRecordArguments());
    // the trailing `dyn_size` -- present only when `size` != 0 -- is how much
    // the kernel could actually copy (less near the top of the stack, or 0
    // for a sample with no valid user stack at all).
    quint64 dwarfStackAddr = 0;
    QByteArray dwarfStack;
    if (sampleType & SampleStackUser) {
        if (p + 8 > end)
            return ResultError(Tr::tr("Truncated perf sample record."));
        const quint64 size = readU64(p);
        p += 8;
        if (size > quint64(end - p))
            return ResultError(Tr::tr("Truncated perf sample stack."));
        const char *stackStart = p;
        p += qsizetype(size);
        quint64 dynSize = size;
        if (size != 0) {
            if (p + 8 > end)
                return ResultError(Tr::tr("Truncated perf sample record."));
            dynSize = qMin(readU64(p), size);
            p += 8;
        }
        if (dynSize > 0) {
            dwarfStack = QByteArray(stackStart, qsizetype(dynSize));
            if (dwarfRegs.size() > PerfRegX86Sp)
                dwarfStackAddr = dwarfRegs.at(PerfRegX86Sp);
        }
    }

    if (m_sampleFilter && !m_sampleFilter(qint64(timeNs)))
        return ResultOk;
    ++m_receivedSamples;

    // ips[] is innermost-first, with PERF_CONTEXT_* sentinels marking the
    // context of what follows. Kernel frames are tagged so symbolize() can
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
    // In lbr mode the user part of the callchain is whatever a frame pointer
    // walk found, in code that typically has none; only its first address is
    // kept, and the branch stack's call sites are the callers.
    quint64 context = PerfContextUser;
    bool isFirstOfContext = true;
    QList<RawFrame> innermostFirst;
    innermostFirst.reserve(ips.size() + branchFroms.size());
    for (quint64 addr : std::as_const(ips)) {
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
    for (quint64 addr : std::as_const(branchFroms))
        innermostFirst.append({addr, false});
    // In dwarf mode "perf record" leaves the user part out of the kernel's
    // callchain (exclude_callchain_user): the user stack is unwound here
    // instead, from the leaf the captured registers hold. Kept as the sample's
    // own user frame, so it survives an unwind that recovers nothing more.
    if (dwarfRegs.size() > PerfRegX86Ip && dwarfRegs.at(PerfRegX86Ip) != 0
        && std::none_of(innermostFirst.cbegin(), innermostFirst.cend(),
                        [](const RawFrame &frame) { return !frame.isKernel; })) {
        innermostFirst.append({dwarfRegs.at(PerfRegX86Ip), false}); // outside any kernel frames
    }
    if (innermostFirst.isEmpty())
        return ResultOk; // matches macsampler.cpp: samples with no resolved stack are dropped

    m_data.pid = pid;
    RawSample sample;
    sample.tid = tid;
    sample.pid = pid;
    sample.running = true; // a perf sample always fires while its thread is on-CPU
    if (m_firstTimestampNs < 0)
        m_firstTimestampNs = qint64(timeNs);
    m_latestSampleNs = qMax(m_latestSampleNs, timeNs);
    sample.tsUs = relativeUs(timeNs);

    sample.frames.reserve(innermostFirst.size());
    for (auto it = innermostFirst.crbegin(); it != innermostFirst.crend(); ++it) // root-first
        sample.frames.append(*it);
    sample.dwarfRegs = std::move(dwarfRegs);
    sample.dwarfStackAddr = dwarfStackAddr;
    sample.dwarfStack = std::move(dwarfStack);

    // Held back, not appended directly: perf's pipe-mode stream isn't globally
    // time-sorted (samples from different per-CPU ring buffers can interleave
    // slightly out of order); see flushPendingRound() for how this gets sorted
    // out before landing in m_rawSamples.
    m_pendingSamples.append(std::move(sample));
    if (m_pendingSamples.size() >= MaxPendingSamples)
        flushPendingRound();
    return ResultOk;
}

void PerfRecordDecoder::flushPendingRound()
{
    if (m_pendingSamples.isEmpty())
        return;
    // "perf record" periodically emits PERF_RECORD_FINISHED_ROUND once it has
    // drained every per-CPU ring buffer for this round; that bounds how far
    // samples from different CPUs can be interleaved out of order to "within
    // one round" (the same guarantee perf's own ordered-events flushing
    // relies on), so sorting just this round's worth of samples by timestamp
    // fixes the vast majority of the reordering, verified against real
    // multi-CPU captures. A tiny residual overlap can still straddle a round
    // boundary; the clamp below is a cheap backstop for that rare remainder,
    // not the primary ordering mechanism.
    std::stable_sort(m_pendingSamples.begin(), m_pendingSamples.end(),
                     [](const RawSample &a, const RawSample &b) { return a.tsUs < b.tsUs; });
    m_rawSamples.reserve(m_rawSamples.size() + m_pendingSamples.size());
    for (RawSample &sample : m_pendingSamples) {
#ifdef WITH_LIBDW
        // Now, rather than once the recording is over: a dwarf-mode sample
        // carries a copy of the stack, kilobytes of it, which is only needed
        // until it is unwound.
        unwindDwarfSample(sample);
#endif
        sample.tsUs = qMax(sample.tsUs, m_lastTsUs);
        m_lastTsUs = sample.tsUs;
        m_rawSamples.append(std::move(sample));
    }
    m_pendingSamples.clear();
}

ModuleSymbols &PerfRecordDecoder::symbolsFor(const QString &path)
{
    auto it = m_moduleCache.find(path);
    if (it != m_moduleCache.end())
        return it.value();

    ModuleSymbols module;
    QByteArray buildId;
    module.debugInfoPath = FilePath::fromString(path);
    const bool full = loadElfFunctionSymbols(module.debugInfoPath, module.symbols, &buildId);

    // A stripped binary yields only the ".dynsym" subset (or nothing); its
    // separate, unstripped debug file -- if a debuginfod server or the local
    // cache has it for this build-id -- carries the full ".symtab" (and,
    // unlike the stripped binary, an actual ".debug_line"; see
    // lineTableFor()). Prefer that when available; otherwise keep the
    // stripped-binary symbols.
    if (!full) {
        if (const std::optional<FilePath> debug = debuginfodDebugFile(buildId)) {
            QList<ElfSymbol> better;
            if (loadElfFunctionSymbols(*debug, better) && !better.isEmpty()) {
                module.symbols = std::move(better);
                module.debugInfoPath = *debug;
            }
        }
    }

    std::sort(module.symbols.begin(), module.symbols.end(),
             [](const ElfSymbol &a, const ElfSymbol &b) { return a.address < b.address; });

    return m_moduleCache.insert(path, std::move(module)).value();
}

const DwarfLineTable &PerfRecordDecoder::lineTableFor(ModuleSymbols &module)
{
    if (module.lineTable)
        return *module.lineTable;

    ElfReader reader(module.debugInfoPath);
    reader.readHeaders();
    // Matches loadElfFunctionSymbols()'s own guard: readSection() returns a
    // non-null but unmapped (fdlen == 0, `start` unset) ElfMapper when the
    // section doesn't exist or couldn't be mapped, never null for "missing".
    const auto sectionBytes = [&](const QByteArray &name) -> QByteArray {
        const std::unique_ptr<ElfMapper> mapper = reader.readSection(name);
        if (!mapper || mapper->fdlen == 0)
            return {};
        return QByteArray(mapper->start, int(mapper->fdlen));
    };
    module.lineTable = DwarfLineTable::parse(sectionBytes(".debug_line"),
                                             sectionBytes(".debug_line_str"),
                                             sectionBytes(".debug_str"));
    return *module.lineTable;
}

void PerfRecordDecoder::ensureDebuginfodConfig()
{
    if (m_debuginfodChecked)
        return;
    m_debuginfodChecked = true;

    // Standard debuginfod knobs, shared with elfutils/gdb/perf. Reusing them
    // (rather than a bespoke setting) means a server the user already
    // configured for those tools just works, and leaving DEBUGINFOD_URLS
    // unset disables network fetching exactly as it does there. Downloading
    // at all is the user's call, though (see setDownloadDebugInfo()).
    if (m_downloadDebugInfo) {
        const QString urls = qEnvironmentVariable("DEBUGINFOD_URLS");
        m_debuginfodUrls = urls.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts);
    }

    QString cache = qEnvironmentVariable("DEBUGINFOD_CACHE_PATH");
    if (cache.isEmpty()) {
        const QString xdg = qEnvironmentVariable("XDG_CACHE_HOME");
        cache = (xdg.isEmpty() ? QDir::homePath() + u"/.cache"_s : xdg) + u"/debuginfod_client"_s;
    }
    m_debuginfodCacheDir = FilePath::fromString(cache);

    bool ok = false;
    const int secs = qEnvironmentVariable("DEBUGINFOD_TIMEOUT").toInt(&ok);
    if (ok && secs > 0)
        m_debuginfodTimeoutMs = secs * 1000;
}

FilePath PerfRecordDecoder::debuginfodCachePath(const QByteArray &buildId) const
{
    // The shared cache layout every debuginfod client uses: a debug file that
    // perf/gdb (or resolveDebugInfo()) already fetched is found here for free,
    // and anything we write lands where those tools will find it too.
    return m_debuginfodCacheDir.pathAppended(QString::fromLatin1(buildId)).pathAppended(u"debuginfo"_s);
}

std::optional<FilePath> PerfRecordDecoder::debuginfodDebugFile(const QByteArray &buildId)
{
    if (buildId.isEmpty())
        return std::nullopt;
    ensureDebuginfodConfig();
    const FilePath cached = debuginfodCachePath(buildId);
    if (cached.exists() && cached.fileSize() > 0)
        return cached;
    return std::nullopt;
}

const MappedRegion *PerfRecordDecoder::findRegion(quint32 pid, quint64 addr) const
{
    // Search most-recently-added regions first: if a later mmap reused an
    // address range, prefer it over a stale earlier one.
    auto it = m_regionsByPid.constFind(pid);
    if (it == m_regionsByPid.constEnd())
        return nullptr;
    const QList<MappedRegion> &regions = it.value();
    for (auto r = regions.crbegin(); r != regions.crend(); ++r) {
        if (addr >= r->addr && addr < r->addr + r->len)
            return &*r;
    }
    return nullptr;
}

int PerfRecordDecoder::labelIdFor(quint32 pid, quint64 addr)
{
    if (const MappedRegion *region = findRegion(pid, addr)) {
        const quint64 fileOffset = (addr - region->addr) + region->pgoff;
        ModuleSymbols &module = symbolsFor(region->path);
        if (int id = module.labelIdByOffset.value(fileOffset, -1); id >= 0)
            return id;
        const quint64 linkAddr = linkAddress(loadSectionsFor(region->path), fileOffset,
                                             region->pgoff, region->pgoff + region->len);

        SampleTraceData::Label label;
        label.module = FilePath::fromString(region->path).fileName();
        // Best-effort; most modules have no ".debug_line" at all (stripped
        // release binaries with no debuginfod match), which just leaves
        // label.file/line empty, same as before this lookup existed.
        if (const std::optional<SourceLocation> loc = lineTableFor(module).lookup(linkAddr)) {
            label.file = loc->file;
            label.line = loc->line;
        }
        const auto sym = std::upper_bound(module.symbols.cbegin(), module.symbols.cend(), linkAddr,
                                          [](quint64 a, const ElfSymbol &s) { return a < s.address; });
        if (sym != module.symbols.cbegin()) {
            const ElfSymbol &found = *(sym - 1);
            if (linkAddr >= found.address && (found.size == 0 || linkAddr < found.address + found.size)) {
                label.name = demangleName(found.name);
                label.offset = linkAddr - found.address;
                const int id = int(m_data.labels.size());
                m_data.labels.append(label);
                module.labelIdByOffset.insert(fileOffset, id);
                return id;
            }
        }
        label.name = u"0x%1"_s.arg(linkAddr, 0, 16);
        label.offset = linkAddr;
        const int id = int(m_data.labels.size());
        m_data.labels.append(label);
        module.labelIdByOffset.insert(fileOffset, id);
        return id;
    }

    if (int id = m_unknownLabelIds.value(addr, -1); id >= 0)
        return id;
    SampleTraceData::Label label(u"0x%1"_s.arg(addr, 0, 16));
    label.offset = addr;
    const int id = int(m_data.labels.size());
    m_data.labels.append(label);
    m_unknownLabelIds.insert(addr, id);
    return id;
}

void PerfRecordDecoder::ensureKallsymsLoaded()
{
    if (m_kallsymsLoaded)
        return;
    m_kallsymsLoaded = true;

    QFile file(u"/proc/kallsyms"_s);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return; // e.g. no permission at all: kernelLabelIdFor() falls back to raw addresses

    // "<hex address> <type-char> <name> [module]" per line; the trailing
    // module field (for symbols belonging to a loaded kernel module) is not
    // needed here, only the address and name.
    while (!file.atEnd()) {
        const QList<QByteArray> parts = file.readLine().simplified().split(' ');
        if (parts.size() < 3)
            continue;
        bool ok = false;
        const quint64 addr = parts[0].toULongLong(&ok, 16);
        // kptr_restrict (on by default on most distros) zeroes every address
        // in this file for non-root readers; such entries are useless.
        if (!ok || addr == 0)
            continue;
        const QString name = QString::fromUtf8(parts[2]);
        if (name.isEmpty())
            continue;
        m_kallsyms.append({addr, 0, name});
    }
    std::sort(m_kallsyms.begin(), m_kallsyms.end(),
             [](const ElfSymbol &a, const ElfSymbol &b) { return a.address < b.address; });
}

int PerfRecordDecoder::kernelLabelIdFor(quint64 addr)
{
    ensureKallsymsLoaded();

    if (int id = m_kernelLabelIds.value(addr, -1); id >= 0)
        return id;

    SampleTraceData::Label label;
    label.module = u"[kernel]"_s;
    // Kernel symbols have no reported size in kallsyms, unlike ELF .symtab
    // entries; assume the nearest preceding one covers addr (the same
    // "nearest, not bounded" fallback labelIdFor() itself uses for ELF
    // symbols with size == 0).
    const auto sym = std::upper_bound(m_kallsyms.cbegin(), m_kallsyms.cend(), addr,
                                      [](quint64 a, const ElfSymbol &s) { return a < s.address; });
    if (sym != m_kallsyms.cbegin()) {
        const ElfSymbol &found = *(sym - 1);
        label.name = demangleName(found.name);
        label.offset = addr - found.address;
    } else {
        // No kallsyms at all (unreadable, or every address restricted to 0):
        // fall back to a raw address, same convention as an unresolved
        // userspace frame.
        label.name = u"0x%1"_s.arg(addr, 0, 16);
        label.offset = addr;
    }
    const int id = int(m_data.labels.size());
    m_data.labels.append(label);
    m_kernelLabelIds.insert(addr, id);
    return id;
}

void PerfRecordDecoder::symbolize(const std::function<void(int)> &progress)
{
    using namespace QtTaskTree;

    ensureDebuginfodConfig();

    // Collect the distinct sampled modules that are stripped, carry a build-id,
    // and aren't already cached -- one debug file to fetch each. Only worth
    // doing when a server is configured; otherwise symbolication still runs and
    // consults whatever is already in the local cache.
    struct Fetch { QString url; FilePath cachePath; };
    QList<Fetch> fetches;
    if (!m_debuginfodUrls.isEmpty()) {
        QSet<QString> examinedPaths;
        QSet<QByteArray> plannedBuildIds;
        for (const RawSample &sample : std::as_const(m_rawSamples)) {
            for (const RawFrame &frame : sample.frames) {
                if (frame.isKernel)
                    continue;
                const MappedRegion *region = findRegion(sample.pid, frame.addr);
                if (!region || examinedPaths.contains(region->path))
                    continue;
                examinedPaths.insert(region->path);

                ElfReader reader(FilePath::fromString(region->path));
                const ElfData elf = reader.readHeaders();
                if (elf.buildId.isEmpty() || elf.indexOf(".symtab") >= 0)
                    continue; // no build-id, or not stripped (already has full symbols)
                if (plannedBuildIds.contains(elf.buildId))
                    continue;
                plannedBuildIds.insert(elf.buildId);

                const FilePath cachePath = debuginfodCachePath(elf.buildId);
                if (cachePath.exists() && cachePath.fileSize() > 0)
                    continue; // already in the shared cache

                // One fetch task per (build-id, server): they run in parallel
                // and whichever server has the file writes it; a 404 or an
                // unreachable server just fails its task, leaving the module
                // its ".dynsym".
                for (const QString &base : std::as_const(m_debuginfodUrls)) {
                    QString server = base;
                    while (server.endsWith('/'))
                        server.chop(1);
                    fetches.append({server + u"/buildid/"_s + QString::fromLatin1(elf.buildId)
                                        + u"/debuginfo"_s,
                                    cachePath});
                }
            }
        }
    }

    // One recipe expresses the whole fetch-then-symbolicate dependency: the
    // per-module debug-file fetches run in parallel, and once they finish a
    // QSyncTask resolves every raw frame against the now-complete symbol
    // tables. The fetches are explicit parallel children (one task per fetch)
    // rather than a For-loop, because a loop's iterations run sequentially even
    // inside a parallel group whereas sibling children genuinely run at once.
    // runBlocking drives the whole thing on this (worker) thread's own event
    // loop -- the documented way to run a task tree off the main thread; the
    // NetworkAccessManager is likewise created and used only on this thread.
    NetworkAccessManager nam;
    const int timeoutMs = m_debuginfodTimeoutMs;

    // Progress budget across both stages: fetching (network-bound, one
    // reply per planned fetch) gets a share only when there is anything to
    // fetch; resolving every sample against the now-complete symbol tables
    // gets the rest (all of it, when there is nothing to fetch).
    const int fetchWeight = fetches.isEmpty() ? 0 : 40;
    std::atomic<int> fetchesDone{0};
    const int totalFetches = int(fetches.size());

    // Both the elapsed-time ticker below and completion-driven reporting
    // react to independent events (a timer tick; a reply finishing) that can
    // land in either order -- e.g. the ticker's time-based estimate can be
    // temporarily ahead of "1 of 2 fetches done". Routing every fetch-stage
    // update through this shared high-water mark keeps what's actually
    // displayed non-decreasing regardless of which one fires "first" for a
    // given instant, instead of visibly jumping backward.
    int maxFetchProgressReported = 0;
    const auto reportProgress = [&progress, &maxFetchProgressReported](int percent) {
        if (progress && percent > maxFetchProgressReported) {
            maxFetchProgressReported = percent;
            progress(percent);
        }
    };
    const auto reportFetchProgress = [&reportProgress, &fetchesDone, totalFetches, fetchWeight] {
        if (totalFetches > 0)
            reportProgress(fetchesDone.load() * fetchWeight / totalFetches);
    };

    // A slow or unreachable debuginfod server (a real, common case: Ubuntu
    // sets DEBUGINFOD_URLS system-wide by default) can leave every fetch in
    // flight for up to the full timeout, and reportFetchProgress() above only
    // ever fires on a fetch *completing* -- so without this, the whole stage
    // stays silent for that entire stretch. Nudges progress up as a fraction
    // of the timeout elapsed instead; stops itself once every fetch is done.
    QTimer fetchProgressTicker;
    QElapsedTimer fetchElapsed;
    if (totalFetches > 0 && progress) {
        fetchElapsed.start();
        fetchProgressTicker.setInterval(200);
        QObject::connect(&fetchProgressTicker, &QTimer::timeout, [&] {
            if (fetchesDone.load() >= totalFetches) {
                fetchProgressTicker.stop();
                return;
            }
            const qint64 elapsedFraction = qMin<qint64>(90, fetchElapsed.elapsed() * 100
                                                                 / qMax(1, timeoutMs));
            reportProgress(int(elapsedFraction) * fetchWeight / 100);
        });
        fetchProgressTicker.start();
    }

    // Reported as the share of fetches done: they run in parallel, so no single
    // download's progress stands for the whole wait.
    const QString servers = m_debuginfodUrls.join(u' ');
    const auto reportDownload = [this, &servers, &fetchesDone, totalFetches] {
        if (m_onDebugInfoDownload)
            m_onDebugInfoDownload(fetchesDone.load() * 100 / totalFetches, servers);
    };
    const auto countFetchDone = [&fetchesDone, reportFetchProgress, reportDownload] {
        fetchesDone.fetch_add(1);
        reportFetchProgress();
        reportDownload();
    };

    GroupItems recipe;
    recipe.append(continueOnError); // a failed fetch must not skip symbolication
    if (!fetches.isEmpty()) {
        GroupItems fetchGroup;
        fetchGroup.reserve(fetches.size() + 2);
        fetchGroup.append(parallel);
        fetchGroup.append(continueOnError);
        for (const Fetch &fetch : std::as_const(fetches)) {
            const QString url = fetch.url;
            const FilePath cachePath = fetch.cachePath;
            const auto onQuerySetup = [url, timeoutMs, &nam](QNetworkReplyWrapper &query) {
                QNetworkRequest request{QUrl(url)};
                request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                     QNetworkRequest::NoLessSafeRedirectPolicy);
                request.setTransferTimeout(timeoutMs);
                query.setRequest(request);
                query.setNetworkAccessManager(&nam);
            };
            const auto onQueryDone = [cachePath, countFetchDone](const QNetworkReplyWrapper &query,
                                                                 DoneWith result) {
                if (result != DoneWith::Success) {
                    countFetchDone();
                    return;
                }
                const QByteArray body = query.reply()->readAll();
                if (!body.isEmpty()) {
                    // Commit atomically (QSaveFile = temp + rename) so no
                    // reader -- ours below, or another debuginfod client --
                    // sees a partial file.
                    QDir().mkpath(cachePath.parentDir().toFSPathString());
                    QSaveFile out(cachePath.toFSPathString());
                    if (out.open(QIODevice::WriteOnly)) {
                        out.write(body);
                        out.commit();
                    }
                }
                countFetchDone();
            };
            fetchGroup.append(QNetworkReplyWrapperTask{onQuerySetup, onQueryDone} || successItem);
        }
        recipe.append(Group{
            onGroupSetup(reportDownload),
            Group{fetchGroup},
            onGroupDone([this] {
                if (m_onDebugInfoDownload)
                    m_onDebugInfoDownload(-1, {});
            }),
        });
    }
    recipe.append(QSyncTask([this, &progress, fetchWeight] { resolveSamples(progress, fetchWeight); }));

    QTaskTree tree(Group{recipe});
    QTimer cancelPoll;
    if (m_isCanceled) {
        cancelPoll.setInterval(100);
        QObject::connect(&cancelPoll, &QTimer::timeout, &tree, [this, &tree] {
            if (m_isCanceled())
                tree.cancel();
        });
        cancelPoll.start();
    }
    tree.runBlocking();
    if (progress)
        progress(100);
}

const QList<ElfSectionHeader> &PerfRecordDecoder::loadSectionsFor(const QString &path)
{
    auto it = m_loadSections.find(path);
    if (it != m_loadSections.end())
        return it.value();
    // From the mapped binary itself: in a separate debug file, the sections
    // that hold code are NOBITS.
    constexpr quint32 ShfAlloc = 2;
    constexpr quint32 ShtNobits = 8;
    QList<ElfSectionHeader> sections;
    for (const ElfSectionHeader &section : ElfReader(FilePath::fromString(path)).readHeaders()
                                               .sectionHeaders) {
        if ((section.flags & ShfAlloc) && section.type != ShtNobits && section.size > 0)
            sections.append(section);
    }
    return m_loadSections.insert(path, sections).value();
}

#ifdef WITH_LIBDW
PerfDwarfUnwinder *PerfRecordDecoder::dwarfUnwinderFor(quint32 pid)
{
    const QList<MappedRegion> regions = m_regionsByPid.value(pid);
    DwarfUnwinderEntry &entry = m_dwarfUnwinders[pid];
    if (entry.unwinder && entry.regionCount == regions.size())
        return entry.unwinder.get();

    // m_regionsByPid[pid] is every mmap this pid made so far, in
    // chronological order -- for a long-running or JIT-heavy target that can
    // be hundreds of entries, many of them stale: this reader gets no explicit
    // "munmap" record, so an address range getting reused later (a dlopen'd
    // plugin unloaded and a different one loaded at the same address, a JIT
    // compiler's code buffers being freed and reallocated, etc.) shows up only
    // as a later mmap silently overlapping an earlier one. findRegion()
    // already handles this for symbol lookup by searching most-recent-first;
    // libdw has no "prefer the newer one" logic when modules are reported, so
    // the same "later wins" rule is applied here, by dropping any earlier
    // region a later one's range overlaps.
    QList<MappedRegion> activeRegions;
    for (const MappedRegion &region : regions) {
        activeRegions.removeIf([&region](const MappedRegion &old) {
            return region.addr < old.addr + old.len && old.addr < region.addr + region.len;
        });
        activeRegions.append(region);
    }

    QList<UnwindModule> modules;
    QSet<QString> seen;
    for (const MappedRegion &region : std::as_const(activeRegions)) {
        // What libdw adds to a link-time address to get the runtime one.
        const quint64 bias = region.addr
                             - linkAddress(loadSectionsFor(region.path), region.pgoff,
                                           region.pgoff, region.pgoff + region.len);
        const QString key = region.path + u"@"_s + QString::number(bias, 16);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        modules.append({region.path, bias});
    }
    // One that could not attach -- no module known yet to tell the
    // architecture by -- gets another go, now that there are more.
    if (entry.unwinder && entry.unwinder->isValid())
        entry.unwinder->setModules(modules);
    else
        entry.unwinder = std::make_unique<PerfDwarfUnwinder>(modules);
    entry.regionCount = regions.size();
    return entry.unwinder.get();
}

void PerfRecordDecoder::unwindDwarfSample(RawSample &sample)
{
    if (sample.dwarfRegs.isEmpty())
        return;
    PerfDwarfUnwinder *unwinder = dwarfUnwinderFor(sample.pid);
    if (unwinder->isValid()) {
        UnwindInput input;
        input.regs = sample.dwarfRegs;
        input.stackStartAddr = sample.dwarfStackAddr;
        input.stackBytes = sample.dwarfStack;
        // Falls back to the kernel-reported chain (just the leaf PC in dwarf
        // mode -- see handleSample()) when the unwind comes back empty.
        const QList<quint64> pcs = unwinder->unwind(input); // innermost-first
        if (!pcs.isEmpty()) {
            QList<RawFrame> frames;
            frames.reserve(pcs.size() + sample.frames.size());
            for (auto it = pcs.crbegin(); it != pcs.crend(); ++it) // root-first
                frames.append({*it, false});
            // The kernel frames a sample taken in a system call carries sit
            // on top of the user stack; only the user part is unwound.
            for (const RawFrame &frame : std::as_const(sample.frames)) {
                if (frame.isKernel)
                    frames.append(frame);
            }
            sample.frames = std::move(frames);
        }
    }
    sample.dwarfRegs = {};
    sample.dwarfStack = {};
}
#endif

void PerfRecordDecoder::resolveSamples(const std::function<void(int)> &progress, int progressBase)
{
    m_data.samples.reserve(m_rawSamples.size());
    const qsizetype total = m_rawSamples.size();
    qsizetype done = 0;
    for (const RawSample &raw : std::as_const(m_rawSamples)) {
        if ((done & 0x3ff) == 0 && m_isCanceled && m_isCanceled())
            break;
        SampleTraceData::ThreadSample sample;
        sample.tsUs = raw.tsUs;
        sample.tid = raw.tid;
        sample.running = raw.running;

        sample.frames.reserve(raw.frames.size());
        for (const RawFrame &frame : raw.frames) {
            sample.frames.append(frame.isKernel ? kernelLabelIdFor(frame.addr)
                                                 : labelIdFor(raw.pid, frame.addr));
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

Result<> PerfRecordDecoder::handleRecord(quint32 type, const QByteArray &record)
{
    switch (type) {
    case PerfRecordHeaderAttr:
        return handleAttr(record);
    case PerfRecordMmap:
        handleMmap(record, false);
        return ResultOk;
    case PerfRecordMmap2:
        handleMmap(record, true);
        return ResultOk;
    case PerfRecordComm:
        handleComm(record);
        return ResultOk;
    case PerfRecordLost:
        handleLost(record);
        return ResultOk;
    case PerfRecordLostSamples:
        handleLostSamples(record);
        return ResultOk;
    case PerfRecordThrottle:
        handleThrottle(record);
        return ResultOk;
    case PerfRecordSample:
        return handleSample(record);
    case PerfRecordFinishedRound:
        flushPendingRound();
        return ResultOk;
    default:
        return ResultOk; // not needed for symbolication (feature/meta records, etc.)
    }
}

Result<SampleTraceData> PerfRecordReader::read(PerfByteQueue &queue,
                                               const std::function<void(int)> &progress)
{
    QByteArray buffer;
    const auto fill = [&](qsizetype need) {
        while (buffer.size() < need) {
            if (queue.atEnd())
                return false;
            buffer += queue.pop();
        }
        return true;
    };

    m_receivedSamples = 0;
    m_firstTimestampNs = -1;
    if (!fill(16)) {
        // Not a single byte means "perf record" never got to write a stream,
        // e.g. because perf_event_paranoid refused it: an empty recording.
        if (buffer.isEmpty())
            return SampleTraceData();
        return ResultError(Tr::tr("Truncated perf.data stream header."));
    }
    if (memcmp(buffer.constData(), "PERFILE2", 8) != 0)
        return ResultError(Tr::tr("Unrecognized perf.data stream header."));
    const quint64 pipeHeaderSize = readU64(buffer.constData() + 8);
    if (pipeHeaderSize < 16 || !fill(qsizetype(pipeHeaderSize)))
        return ResultError(Tr::tr("Truncated perf.data stream header."));
    buffer.remove(0, int(pipeHeaderSize));

    const auto canceled = [this] { return m_isCanceled && m_isCanceled(); };
    const auto canceledError = [] { return ResultError(Tr::tr("The recording was canceled.")); };
    PerfRecordDecoder decoder(m_downloadDebugInfo, m_debugInfoDownloadHandler, m_sampleFilter,
                              m_isCanceled);
    for (;;) {
        if (canceled())
            return canceledError();
        if (!fill(8))
            break; // clean (or target-died) end of stream
        const quint32 type = readU32(buffer.constData());
        const quint16 size = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar *>(buffer.constData()) + 6);
        if (size < 8)
            return ResultError(Tr::tr("Corrupt perf.data record."));
        if (!fill(size))
            break; // stopped mid-record: keep whatever was already decoded
        if (type == PerfRecordHeaderTracingData) {
            if (size < 16)
                return ResultError(Tr::tr("Corrupt perf.data record."));
            const quint32 dataSize = readU32(buffer.constData() + 8);
            buffer.remove(0, size);
            if (!fill(dataSize))
                break;
            buffer.remove(0, dataSize);
            continue;
        }
        if (Result<> r = decoder.handleRecord(type, buffer.left(size)); !r)
            return ResultError(r.error());
        buffer.remove(0, size);
    }
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
