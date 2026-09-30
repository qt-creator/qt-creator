// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfsymbolizer.h"

#include "dwarflinetable.h"

#ifdef WITH_LIBDW
#include "perfdwarfunwinder.h"

#include <dwarf.h>
#include <fcntl.h>
#include <libdw.h>
#include <unistd.h>
#endif

#include <utils/elfreader.h>
#include <utils/networkaccessmanager.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QTaskTree>

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#include <QtEndian>

#include <algorithm>
#include <atomic>
#include <unordered_map>

#ifdef Q_CC_GNU
#  include <cstdlib>
#  include <cxxabi.h>
#endif

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

namespace {

// One resolved ELF function symbol, sorted by address for binary search.
struct ElfSymbol
{
    quint64 address = 0;
    quint64 size = 0;
    QString name;
};

// Per-binary symbol table (lazily loaded, cached for the recording session).
struct ModuleSymbols
{
    QList<ElfSymbol> symbols; // sorted by address

    // The file that actually supplied `symbols` -- the mmap'd binary itself
    // when it wasn't stripped, or the separately-fetched debug file
    // otherwise (see symbolsFor()). ".debug_line" is read from here, lazily,
    // by lineTableFor(); std::nullopt until that first access, distinct from
    // an empty-but-parsed table (a module with no usable line info at all).
    FilePath debugInfoPath;
    std::optional<DwarfLineTable> lineTable;
};

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

// One "perf record" mmap: [addr, addr+len) in the target's address space maps
// to `path` starting at file offset `pgoff`.
struct MappedRegion
{
    quint64 addr = 0;
    quint64 len = 0;
    quint64 pgoff = 0;
    QString path;         // on this machine
    QString recordedPath; // as the recording has it
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
// is the only place that needs to know that.
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

class PerfSymbolizerPrivate
{
public:
    ModuleSymbols &symbolsFor(const QString &path);
    // `module`'s parsed ".debug_line" (from module.debugInfoPath, the same
    // file its symbols came from), lazily parsed and cached on first call.
    // Empty (never null) when that file has no ".debug_line" at all.
    const DwarfLineTable &lineTableFor(ModuleSymbols &module);
    // The loaded sections of the binary at `path` (see linkAddress()),
    // memoized apart from symbolsFor(), which must not run before debug files
    // are fetched.
    const QList<ElfSectionHeader> &loadSectionsFor(const QString &path);

    // The mmap'd region covering `addr` in `pid`'s address space, or nullptr.
    // Searches most-recently-added first so a later mmap that reused an address
    // range wins over a stale earlier one.
    const MappedRegion *findRegion(quint32 pid, quint64 addr) const;

    void ensureKallsymsLoaded();

    // Where the binary the recording has at `recordedPath` is on this machine
    // (see PerfSymbolizer::setBinaryLocations()), memoized.
    QString hostPathFor(const QString &recordedPath);
    // The files named `name` in the search paths or below.
    QStringList filesNamed(const QString &name);

    // The shared-cache path (existing or not) of the debug file for `buildId`.
    FilePath debuginfodCachePath(const QByteArray &buildId) const;
    // The cached debug file for `buildId`, if one is present in the shared
    // cache (populated locally, by fetchDebugFiles(), or by perf/gdb). Never
    // touches the network -- fetching is fetchDebugFiles()'s job.
    std::optional<FilePath> debuginfodDebugFile(const QByteArray &buildId);
    void ensureDebuginfodConfig();
    bool fetchDebugFiles(const QSet<QString> &modulePaths,
                         const std::function<void(int)> &progress,
                         const std::function<bool()> &isCanceled);

#ifdef WITH_LIBDW
    // The pid's unwinder, with every module the pid has mapped so far.
    PerfDwarfUnwinder *dwarfUnwinderFor(quint32 pid);

    // The functions inlined at `linkAddress` in the file at `debugInfoPath`.
    QList<InlinedFunction> inlinedAt(const FilePath &debugInfoPath, quint64 linkAddress);

    // The .debug_info of a module, opened once.
    struct DebugInfo
    {
        int fd = -1;
        Dwarf *dwarf = nullptr;
    };
    std::unordered_map<QString, DebugInfo> m_debugInfos;

    ~PerfSymbolizerPrivate()
    {
        for (const auto &[path, info] : m_debugInfos) {
            if (info.dwarf)
                dwarf_end(info.dwarf);
            if (info.fd >= 0)
                close(info.fd);
        }
    }
#endif

    QHash<quint32, QList<MappedRegion>> m_regionsByPid; // most-recently-added last

    FilePath m_sysroot;
    FilePaths m_searchPaths;
    QHash<QString, QByteArray> m_buildIds; // recorded path -> hex build id
    QHash<QString, QString> m_hostPaths;   // recorded path -> path here
    std::optional<QHash<QString, QStringList>> m_filesByName; // in m_searchPaths
    bool m_useKallsyms = true;

    // The JIT symbol maps, by pid, and how large the file was when read: a
    // live process keeps adding to its map, so a miss rereads a grown one.
    struct JitMap
    {
        QList<ElfSymbol> symbols; // sorted by address
        qint64 size = -1;
    };
    QHash<quint32, JitMap> m_jitMaps;
    bool m_useJitMaps = true;
    QHash<QString, ModuleSymbols> m_moduleCache;
    QHash<QString, QList<ElfSectionHeader>> m_loadSections;

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
    bool m_downloadDebugInfo = false;
    std::function<void(int, const QString &)> m_onDebugInfoDownload;
    bool m_debuginfodChecked = false;
    QStringList m_debuginfodUrls;
    FilePath m_debuginfodCacheDir;
    int m_debuginfodTimeoutMs = 5000;

    // /proc/kallsyms, read once per session (see ensureKallsymsLoaded()),
    // sorted by address for the same binary-search pattern as ELF symbols.
    bool m_kallsymsLoaded = false;
    QList<ElfSymbol> m_kallsyms;

#ifdef WITH_LIBDW
    // Built for a pid on its first dwarf-mode sample, from that pid's own
    // mmap'd modules alone, and brought up to date as it maps more. Keyed by
    // pid rather than shared across the whole recording: "perf record"
    // inherits child processes by default (no --no-inherit in
    // perfRecordArguments()), so a multi-process recording can easily contain
    // several *different* address spaces; two unrelated processes mapping
    // different files at the same address (common for non-PIE binaries, or
    // just ASLR coincidence) would otherwise get conflated into one Dwfl.
    // std::unordered_map, not QHash: a move-only value (PerfDwarfUnwinder is
    // neither copyable nor movable, owning a raw Dwfl handle) doesn't fit
    // QHash's implicitly-shared, copy-on-write design.
    struct DwarfUnwinderEntry
    {
        std::unique_ptr<PerfDwarfUnwinder> unwinder;
        qsizetype regionCount = -1; // how many of the pid's mappings it knows
    };
    std::unordered_map<quint32, DwarfUnwinderEntry> m_dwarfUnwinders;
#endif
};

ModuleSymbols &PerfSymbolizerPrivate::symbolsFor(const QString &path)
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
const DwarfLineTable &PerfSymbolizerPrivate::lineTableFor(ModuleSymbols &module)
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
void PerfSymbolizerPrivate::ensureDebuginfodConfig()
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
FilePath PerfSymbolizerPrivate::debuginfodCachePath(const QByteArray &buildId) const
{
    // The shared cache layout every debuginfod client uses: a debug file that
    // perf/gdb (or resolveDebugInfo()) already fetched is found here for free,
    // and anything we write lands where those tools will find it too.
    return m_debuginfodCacheDir.pathAppended(QString::fromLatin1(buildId)).pathAppended(u"debuginfo"_s);
}
std::optional<FilePath> PerfSymbolizerPrivate::debuginfodDebugFile(const QByteArray &buildId)
{
    if (buildId.isEmpty())
        return std::nullopt;
    ensureDebuginfodConfig();
    const FilePath cached = debuginfodCachePath(buildId);
    if (cached.exists() && cached.fileSize() > 0)
        return cached;
    return std::nullopt;
}
const MappedRegion *PerfSymbolizerPrivate::findRegion(quint32 pid, quint64 addr) const
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
void PerfSymbolizerPrivate::ensureKallsymsLoaded()
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
const QList<ElfSectionHeader> &PerfSymbolizerPrivate::loadSectionsFor(const QString &path)
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
PerfDwarfUnwinder *PerfSymbolizerPrivate::dwarfUnwinderFor(quint32 pid)
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
#endif

bool PerfSymbolizerPrivate::fetchDebugFiles(const QSet<QString> &modulePaths,
                                            const std::function<void(int)> &progress,
                                            const std::function<bool()> &isCanceled)
{
    using namespace QtTaskTree;

    ensureDebuginfodConfig();

    // Collect the modules that are stripped, carry a build-id, and aren't
    // already cached -- one debug file to fetch each. Only worth doing when a
    // server is configured; otherwise resolve() consults whatever is already
    // in the local cache.
    struct Fetch { QString url; FilePath cachePath; };
    QList<Fetch> fetches;
    if (!m_debuginfodUrls.isEmpty()) {
        QSet<QByteArray> plannedBuildIds;
        for (const QString &path : modulePaths) {
            ElfReader reader(FilePath::fromString(path));
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

    if (fetches.isEmpty())
        return false;

    // The per-module debug-file fetches run in parallel, as explicit parallel
    // children (one task per fetch) rather than a For-loop, because a loop's
    // iterations run sequentially even inside a parallel group whereas sibling
    // children genuinely run at once.
    // runBlocking drives the whole thing on this (worker) thread's own event
    // loop -- the documented way to run a task tree off the main thread; the
    // NetworkAccessManager is likewise created and used only on this thread.
    NetworkAccessManager nam;
    const int timeoutMs = m_debuginfodTimeoutMs;

    // All of `progress` is the fetches': they are one stage of the caller's.
    const int fetchWeight = 100;
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
    recipe.append(continueOnError); // a failed fetch must not fail the others
    {
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
                    // reader -- resolve(), or another debuginfod client --
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
    QTaskTree tree(Group{recipe});
    QTimer cancelPoll;
    if (isCanceled) {
        cancelPoll.setInterval(100);
        QObject::connect(&cancelPoll, &QTimer::timeout, &tree, [&isCanceled, &tree] {
            if (isCanceled())
                tree.cancel();
        });
        cancelPoll.start();
    }
    tree.runBlocking();
    if (progress)
        progress(100);
    return true;
}

QStringList PerfSymbolizerPrivate::filesNamed(const QString &name)
{
    if (!m_filesByName) {
        m_filesByName.emplace();
        for (const FilePath &root : std::as_const(m_searchPaths)) {
            QDirIterator it(root.toFSPathString(), QDir::Files | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QFileInfo info = it.nextFileInfo();
                (*m_filesByName)[info.fileName()].append(info.filePath());
            }
        }
    }
    return m_filesByName->value(name);
}

QString PerfSymbolizerPrivate::hostPathFor(const QString &recordedPath)
{
    if (const auto it = m_hostPaths.constFind(recordedPath); it != m_hostPaths.constEnd())
        return it.value();

    // A recording taken here names its binaries where they are.
    if (m_sysroot.isEmpty() && m_searchPaths.isEmpty())
        return m_hostPaths.insert(recordedPath, recordedPath).value();

    const QByteArray buildId = m_buildIds.value(recordedPath);
    const auto matches = [&buildId](const FilePath &candidate) {
        if (!candidate.isFile())
            return false;
        return buildId.isEmpty() || ElfReader(candidate).readHeaders().buildId == buildId;
    };
    QString found;
    // The sysroot is the target's own filesystem, so the path within it is
    // the one to trust first.
    if (!m_sysroot.isEmpty()) {
        const FilePath inSysroot = m_sysroot.pathAppended(recordedPath);
        if (matches(inSysroot))
            found = inSysroot.path();
    }
    if (found.isEmpty()) {
        for (const QString &candidate : filesNamed(FilePath::fromString(recordedPath).fileName())) {
            if (matches(FilePath::fromString(candidate))) {
                found = candidate;
                break;
            }
        }
    }
    // This machine's file of that path, only where its build id proves it the
    // one recorded: a library of the same path here is another build.
    if (found.isEmpty() && !buildId.isEmpty() && matches(FilePath::fromString(recordedPath)))
        found = recordedPath;
    if (found.isEmpty()) {
        found = m_sysroot.isEmpty() ? recordedPath
                                    : m_sysroot.pathAppended(recordedPath).path();
    }
    return m_hostPaths.insert(recordedPath, found).value();
}

#ifdef WITH_LIBDW
// The name of `die`, following an inlined instance to its abstract origin:
// the linkage name, demangled, where there is one, as the symbol tables name
// functions; else the plain one.
static QString functionName(Dwarf_Die *die)
{
    Dwarf_Attribute attribute;
    for (int name : {DW_AT_linkage_name, DW_AT_MIPS_linkage_name}) {
        if (const char *linkage = dwarf_formstring(dwarf_attr_integrate(die, name, &attribute)))
            return demangleName(QString::fromUtf8(linkage));
    }
    if (const char *plain = dwarf_formstring(dwarf_attr_integrate(die, DW_AT_name, &attribute)))
        return QString::fromUtf8(plain);
    return {};
}

QList<InlinedFunction> PerfSymbolizerPrivate::inlinedAt(const FilePath &debugInfoPath,
                                                        quint64 linkAddress)
{
    const QString key = debugInfoPath.path();
    auto it = m_debugInfos.find(key);
    if (it == m_debugInfos.end()) {
        DebugInfo info;
        info.fd = open(debugInfoPath.toFSPathString().toLocal8Bit().constData(), O_RDONLY);
        if (info.fd >= 0)
            info.dwarf = dwarf_begin(info.fd, DWARF_C_READ);
        it = m_debugInfos.emplace(key, info).first;
    }
    Dwarf *dwarf = it->second.dwarf;
    if (!dwarf)
        return {};

    Dwarf_Die cu;
    if (!dwarf_addrdie(dwarf, linkAddress, &cu))
        return {};
    Dwarf_Die *scopes = nullptr;
    const int count = dwarf_getscopes(&cu, linkAddress, &scopes);
    if (count <= 0)
        return {};

    Dwarf_Files *files = nullptr;
    size_t fileCount = 0;
    const bool haveFiles = dwarf_getsrcfiles(&cu, &files, &fileCount) == 0;

    // The scopes of the address run from the innermost only up to the first
    // inlined instance: past that, they are its abstract definition's. The
    // concrete chain the instance is part of -- the instances it was inlined
    // into, up to the subprogram -- is the instance's own scopes.
    int firstInlined = -1;
    for (int i = 0; i < count && firstInlined < 0; ++i) {
        if (dwarf_tag(&scopes[i]) == DW_TAG_inlined_subroutine)
            firstInlined = i;
    }
    Dwarf_Die *chain = nullptr;
    const int chainCount = firstInlined < 0 ? 0 : dwarf_getscopes_die(&scopes[firstInlined], &chain);
    free(scopes);

    // Innermost first; the scopes also hold lexical blocks, and end at the
    // subprogram the rest were inlined into.
    QList<InlinedFunction> inlined;
    for (int i = 0; i < chainCount; ++i) {
        Dwarf_Die *scope = &chain[i];
        const int tag = dwarf_tag(scope);
        if (tag == DW_TAG_subprogram)
            break;
        if (tag != DW_TAG_inlined_subroutine)
            continue;
        InlinedFunction function;
        function.function = functionName(scope);
        Dwarf_Attribute attribute;
        Dwarf_Word value = 0;
        if (haveFiles && dwarf_formudata(dwarf_attr(scope, DW_AT_call_file, &attribute), &value) == 0) {
            if (const char *file = dwarf_filesrc(files, value, nullptr, nullptr))
                function.callFile = QString::fromUtf8(file);
        }
        if (dwarf_formudata(dwarf_attr(scope, DW_AT_call_line, &attribute), &value) == 0)
            function.callLine = int(value);
        inlined.append(function);
    }
    free(chain);
    return inlined;
}
#endif

PerfSymbolizer::PerfSymbolizer()
    : d(std::make_unique<PerfSymbolizerPrivate>())
{}

PerfSymbolizer::~PerfSymbolizer() = default;

void PerfSymbolizer::setDownloadDebugInfo(bool download)
{
    d->m_downloadDebugInfo = download;
}

void PerfSymbolizer::setDebugInfoDownloadHandler(
    const std::function<void(int percent, const QString &urls)> &handler)
{
    d->m_onDebugInfoDownload = handler;
}

void PerfSymbolizer::setBinaryLocations(const FilePath &sysroot, const FilePaths &searchPaths)
{
    d->m_sysroot = sysroot;
    d->m_searchPaths = searchPaths;
    d->m_filesByName.reset();
    d->m_hostPaths.clear();
}

void PerfSymbolizer::setUseKallsyms(bool use)
{
    d->m_useKallsyms = use;
}

void PerfSymbolizer::addBuildId(const QString &path, const QByteArray &buildId)
{
    if (!buildId.isEmpty())
        d->m_buildIds.insert(path, buildId.toHex());
}

void PerfSymbolizer::addMapping(quint32 pid, quint64 addr, quint64 len, quint64 pgoff,
                                const QString &path, const QByteArray &buildId)
{
    addBuildId(path, buildId);
    d->m_regionsByPid[pid].append({addr, len, pgoff, d->hostPathFor(path), path});
}

void PerfSymbolizer::addFork(quint32 parentPid, quint32 childPid)
{
    if (parentPid == childPid)
        return;
    d->m_regionsByPid.insert(childPid, d->m_regionsByPid.value(parentPid));
}

std::optional<ModuleAddress> PerfSymbolizer::moduleAddress(quint32 pid, quint64 addr)
{
    const MappedRegion *region = d->findRegion(pid, addr);
    if (!region)
        return std::nullopt;
    const quint64 fileOffset = (addr - region->addr) + region->pgoff;
    return ModuleAddress{region->path,
                         linkAddress(d->loadSectionsFor(region->path), fileOffset, region->pgoff,
                                     region->pgoff + region->len),
                         region->recordedPath};
}

ResolvedAddress PerfSymbolizer::resolve(const ModuleAddress &address)
{
    ModuleSymbols &module = d->symbolsFor(address.modulePath);
    const quint64 linkAddr = address.linkAddress;

    ResolvedAddress resolved;
    resolved.linkAddress = linkAddr;
    resolved.functionOffset = linkAddr;
    // Best-effort; most modules have no ".debug_line" at all (stripped
    // release binaries with no debuginfod match), which just leaves
    // file/line empty.
    if (const std::optional<SourceLocation> loc = d->lineTableFor(module).lookup(linkAddr)) {
        resolved.file = loc->file;
        resolved.line = loc->line;
    }
    const auto sym = std::upper_bound(module.symbols.cbegin(), module.symbols.cend(), linkAddr,
                                      [](quint64 a, const ElfSymbol &s) { return a < s.address; });
    if (sym != module.symbols.cbegin()) {
        const ElfSymbol &found = *(sym - 1);
        if (linkAddr >= found.address && (found.size == 0 || linkAddr < found.address + found.size)) {
            resolved.function = demangleName(found.name);
            resolved.functionOffset = linkAddr - found.address;
            resolved.functionSize = found.size;
        }
    }
#ifdef WITH_LIBDW
    if (!resolved.function.isEmpty())
        resolved.inlined = d->inlinedAt(module.debugInfoPath, linkAddr);
#endif
    return resolved;
}

ResolvedAddress PerfSymbolizer::resolveJit(quint32 pid, quint64 addr)
{
    ResolvedAddress resolved;
    resolved.linkAddress = addr;
    resolved.functionOffset = addr;
    if (!d->m_useJitMaps)
        return resolved;

    // "<start> <size> <name>" per line, both numbers in hex.
    const QString path = u"/tmp/perf-%1.map"_s.arg(pid);
    PerfSymbolizerPrivate::JitMap &map = d->m_jitMaps[pid];
    const auto find = [&map, addr]() -> const ElfSymbol * {
        const auto sym = std::upper_bound(map.symbols.cbegin(), map.symbols.cend(), addr,
                                          [](quint64 a, const ElfSymbol &s) { return a < s.address; });
        if (sym == map.symbols.cbegin())
            return nullptr;
        const ElfSymbol &found = *(sym - 1);
        return addr < found.address + found.size ? &found : nullptr;
    };
    const ElfSymbol *found = find();
    if (!found) {
        QFile file(path);
        if (file.size() != map.size && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            map.size = file.size();
            map.symbols.clear();
            while (!file.atEnd()) {
                const QByteArray line = file.readLine().trimmed();
                const qsizetype first = line.indexOf(' ');
                const qsizetype second = first < 0 ? -1 : line.indexOf(' ', first + 1);
                if (second < 0)
                    continue;
                bool okStart = false;
                bool okSize = false;
                const quint64 start = line.left(first).toULongLong(&okStart, 16);
                const quint64 size = line.mid(first + 1, second - first - 1).toULongLong(&okSize, 16);
                if (okStart && okSize && size > 0)
                    map.symbols.append({start, size, QString::fromUtf8(line.mid(second + 1))});
            }
            std::stable_sort(map.symbols.begin(), map.symbols.end(),
                             [](const ElfSymbol &a, const ElfSymbol &b) {
                                 return a.address < b.address;
                             });
            found = find();
        }
    }
    if (found) {
        resolved.function = demangleName(found->name);
        resolved.functionOffset = addr - found->address;
        resolved.functionSize = found->size;
    }
    return resolved;
}

void PerfSymbolizer::setUseJitMaps(bool use)
{
    d->m_useJitMaps = use;
}

ResolvedAddress PerfSymbolizer::resolveKernel(quint64 addr)
{
    if (d->m_useKallsyms)
        d->ensureKallsymsLoaded();

    ResolvedAddress resolved;
    resolved.linkAddress = addr;
    resolved.functionOffset = addr;
    // Kernel symbols have no reported size in kallsyms, unlike ELF .symtab
    // entries; assume the nearest preceding one covers addr (the same
    // "nearest, not bounded" fallback resolve() uses for ELF symbols with
    // size == 0). Without kallsyms at all -- unreadable, or every address
    // restricted to 0 -- the address stays unresolved.
    const auto sym = std::upper_bound(d->m_kallsyms.cbegin(), d->m_kallsyms.cend(), addr,
                                      [](quint64 a, const ElfSymbol &s) { return a < s.address; });
    if (sym != d->m_kallsyms.cbegin()) {
        const ElfSymbol &found = *(sym - 1);
        resolved.function = demangleName(found.name);
        resolved.functionOffset = addr - found.address;
    }
    return resolved;
}

bool PerfSymbolizer::fetchDebugFiles(const QSet<QString> &modulePaths,
                                     const std::function<void(int)> &progress,
                                     const std::function<bool()> &isCanceled)
{
    return d->fetchDebugFiles(modulePaths, progress, isCanceled);
}

bool PerfSymbolizer::canUnwind()
{
#ifdef WITH_LIBDW
    return true;
#else
    return false;
#endif
}

QList<quint64> PerfSymbolizer::unwind(quint32 pid, PerfArchitecture arch,
                                      const QList<quint64> &regs, const QByteArray &stack)
{
#ifdef WITH_LIBDW
    const PerfRegisterLayout layout = perfRegisterLayout(arch);
    if (layout.sp < 0 || regs.size() <= qMax(layout.sp, layout.ip))
        return {};
    PerfDwarfUnwinder *unwinder = d->dwarfUnwinderFor(pid);
    if (!unwinder->isValid())
        return {};
    UnwindInput input;
    input.arch = arch;
    input.regs = regs;
    input.stackStartAddr = regs.at(layout.sp);
    input.stackBytes = stack;
    return unwinder->unwind(input);
#else
    Q_UNUSED(pid)
    Q_UNUSED(arch)
    Q_UNUSED(regs)
    Q_UNUSED(stack)
    return {};
#endif
}

} // namespace Profiler::Internal
