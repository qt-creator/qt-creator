// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfregisters.h"
#include "profiler_global.h"

#include <utils/filepath.h>

#include <QList>
#include <QSet>
#include <QString>

#include <functional>
#include <memory>
#include <optional>

namespace Profiler::Internal {

class PerfSymbolizerPrivate;

// What a recording names the kernel's vdso mapping.
const char vdsoName[] = "[vdso]";

// An address in a mapped module: the module, and where in it, as a link-time
// address -- the one its symbols, line rows and CFI are at.
struct ModuleAddress
{
    QString modulePath;   // where the module is on this machine
    quint64 linkAddress = 0;
    QString recordedPath; // where it was on the machine recorded; not part of the key

    friend bool operator==(const ModuleAddress &a, const ModuleAddress &b)
    {
        return a.modulePath == b.modulePath && a.linkAddress == b.linkAddress;
    }
    friend size_t qHash(const ModuleAddress &a, size_t seed = 0)
    {
        return qHashMulti(seed, a.modulePath, a.linkAddress);
    }
};

// A function inlined at an address: which, and where it was inlined -- the
// call site, in the function it is part of.
struct InlinedFunction
{
    QString function;
    QString callFile;
    int callLine = 0;
};

// What an address in a recording is.
struct ResolvedAddress
{
    QString function;          // demangled; empty where no symbol covers it
    quint64 functionOffset = 0; // of the address into the function; else as linkAddress
    quint64 functionSize = 0;  // 0 where the symbol table does not say
    quint64 linkAddress = 0;   // within the module; the address itself for the kernel
    QString file;              // source, from .debug_line; empty where unknown
    int line = 0;
    // The functions inlined at the address, innermost first, from
    // .debug_info; none without libdw. `file`/`line` are in the innermost.
    QList<InlinedFunction> inlined;
};

// Tells what the addresses of a perf recording are: the modules they are
// mapped from, their symbols -- demangled, from .symtab or .dynsym, or from a
// separate debug file in the shared debuginfod cache -- and their source
// lines, from .debug_line. Kernel addresses resolve against /proc/kallsyms.
// Unwinds dwarf-mode samples, too, where the build has libdw.
//
// Not thread-safe; used by one decoder on one thread.
class PROFILER_EXPORT PerfSymbolizer
{
public:
    PerfSymbolizer();
    ~PerfSymbolizer();

    PerfSymbolizer(const PerfSymbolizer &) = delete;
    PerfSymbolizer &operator=(const PerfSymbolizer &) = delete;

    // Whether debug files missing locally may be fetched from the debuginfod
    // servers in DEBUGINFOD_URLS (see fetchDebugFiles()). Off by default.
    void setDownloadDebugInfo(bool download);

    // Called while debug files are fetched, with the share done (0..100) and
    // the servers asked, and once with -1 and no servers when that is over.
    void setDebugInfoDownloadHandler(
        const std::function<void(int percent, const QString &urls)> &handler);

    // Where the binaries of a recording from another machine are to be
    // found on this one: under `sysroot`, the target's root, by their path
    // there, or by their name in one of `searchPaths` or below. A binary
    // found by name has to have the build id the recording gives it, where
    // it gives one.
    void setBinaryLocations(const Utils::FilePath &sysroot, const Utils::FilePaths &searchPaths);

    // Whether kernel addresses resolve against this machine's /proc/kallsyms,
    // which is only right for a recording taken here. On by default.
    void setUseKallsyms(bool use);

    // The build id of the binary at `path`, as the recording has it.
    void addBuildId(const QString &path, const QByteArray &buildId);

    // [addr, addr + len) of `pid`'s address space maps `path` from file
    // offset `pgoff`. A later mapping over an earlier one replaces it.
    void addMapping(quint32 pid, quint64 addr, quint64 len, quint64 pgoff, const QString &path,
                    const QByteArray &buildId = {}, bool executable = true);

    // `childPid` was forked from `parentPid`, whose mappings it inherits:
    // the kernel reports none of them for the child.
    void addFork(quint32 parentPid, quint32 childPid);

    // The module `addr` in `pid` is mapped from, and where in it; nullopt
    // where no mapping covers it. Cheap: a key to memoize resolve() by.
    std::optional<ModuleAddress> moduleAddress(quint32 pid, quint64 addr);

    // Whether `addr` in `pid` is in an executable mapping; false where no
    // mapping covers it.
    bool isExecutable(quint32 pid, quint64 addr) const;

    ResolvedAddress resolve(const ModuleAddress &address);
    ResolvedAddress resolveKernel(quint64 addr);

    // What the JIT symbol map of `pid` -- /tmp/perf-<pid>.map, which a JIT
    // such as QML's writes for perf -- says `addr` is; unresolved where it has
    // nothing, or does not exist. Only for a recording taken here, which has
    // the map where the process left it (see setUseJitMaps()).
    ResolvedAddress resolveJit(quint32 pid, quint64 addr);
    void setUseJitMaps(bool use);

    // Fetches the debug files the stripped ones of `modulePaths` lack, from
    // the debuginfod servers, in parallel, into the shared cache, so that
    // resolve() finds them. Blocks; `progress` is called with 0..100. Returns
    // whether there was anything to fetch. Must come before the first
    // resolve() of those modules. Fetching stops early once `isCanceled`
    // returns true.
    bool fetchDebugFiles(const QSet<QString> &modulePaths,
                         const std::function<void(int)> &progress = {},
                         const std::function<bool()> &isCanceled = {});

    // The user stack a "--call-graph dwarf" sample captured, unwound:
    // addresses innermost first, or none where that fails or the build has no
    // libdw. `regs` are indexed by perf's register numbers for `arch`, and
    // `stack` starts at the stack pointer among them.
    // Whether this build can unwind() at all, which takes libdw.
    static bool canUnwind();
    QList<quint64> unwind(quint32 pid, PerfArchitecture arch, const QList<quint64> &regs,
                          const QByteArray &stack);
    // Whether unwinding takes from this machine's CPU how return addresses
    // are signed, which is only right for a recording taken here. On by
    // default.
    void setUseHostPointerAuthentication(bool use);

    // Whether, at `pc` in `pid`, the CFI says the return address is still in
    // the link register. False where that cannot be told, as without libdw.
    bool isReturnAddressInRegister(quint32 pid, quint64 pc);

private:
    std::unique_ptr<PerfSymbolizerPrivate> d;
};

} // namespace Profiler::Internal
