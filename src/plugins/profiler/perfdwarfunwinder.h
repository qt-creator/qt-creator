// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <memory>

// Opaque libdw handle; only pointers to it appear in this header, so callers
// of PerfDwarfUnwinder don't need <libdwfl.h> themselves.
typedef struct Dwfl Dwfl;

namespace Profiler::Internal {

// Defined in perfdwarfunwinder.cpp. A free (non-nested) type, not a private
// class member: the libdw callback functions there are free functions, not
// members or friends of PerfDwarfUnwinder, and need to reference it too.
struct PerfDwarfUnwinderPrivate;

// One ELF module known to the unwinder: the file to read ".eh_frame"/CFI
// from, and the load bias -- the value that, added to a file offset (the
// same "file offset" convention perfrecordreader.cpp's own ELF symbol
// lookup already uses), gives the runtime virtual address. Only covers
// userspace modules: dwarf-mode unwinding only ever has user registers/stack
// to work with (see UnwindInput), so kernel frames never arise here.
struct UnwindModule
{
    QString path;
    quint64 bias = 0;
};

// One sample's raw input: the x86-64 general-purpose register values the
// kernel captured (PERF_SAMPLE_REGS_USER), indexed by the Linux perf
// register-number scheme (arch/x86/include/uapi/asm/perf_regs.h's
// PERF_REG_X86_* enum -- AX=0, BX=1, ..., R15=23; unused/unrequested
// entries are 0), and a captured chunk of the userspace stack
// (PERF_SAMPLE_STACK_USER) starting at whatever the captured SP was.
struct UnwindInput
{
    QList<quint64> regs; // sized PERF_REG_X86_64_MAX (24); see perf_regs.h
    quint64 stackStartAddr = 0; // virtual address stackBytes[0] corresponds to
    QByteArray stackBytes;

    bool isValid() const { return !regs.isEmpty(); }
};

// Unwinds "--call-graph dwarf" call chains using libdw's DWARF CFI
// (Call Frame Information) engine, driven entirely from a captured
// register+stack snapshot -- never ptrace, never live process access. This
// is what makes it usable here: a whole recording is decoded well after the
// fact, quite possibly on a different machine than the one that ran
// "perf record".
//
// libdw (part of elfutils) is dual-licensed LGPL-3.0-or-later or
// GPL-2.0-or-later (see /usr/share/doc/libdw-dev/copyright on a Debian-like
// system, or elfutils' own COPYING/COPYING-LGPLV3 files); linking it under
// the LGPL election is what makes this possible in a plugin that is itself
// dual LicenseRef-Qt-Commercial/GPL-3.0-with-Qt-exception. Only linked
// dynamically -- see CMakeLists.txt/profiler.qbs -- which keeps LGPL
// compliance simple (no relinkable-object-code obligations). Not available
// on machines without elfutils' development headers at build time; see
// PerfSamplerSettings::createSession(), which is where the resulting
// absence is surfaced to the user.
//
// x86-64 only for now (the register mapping and PERF_SAMPLE_REGS_USER
// layout are architecture-specific); modules are reported to libdw once and
// reused across every unwind() call, since parsing a module's CFI data is
// the expensive part and a whole recording's module set is already fully
// known by the time unwinding runs (see PerfRecordDecoder::symbolize()).
class PROFILER_EXPORT PerfDwarfUnwinder
{
public:
    explicit PerfDwarfUnwinder(const QList<UnwindModule> &modules);
    ~PerfDwarfUnwinder();

    PerfDwarfUnwinder(const PerfDwarfUnwinder &) = delete;
    PerfDwarfUnwinder &operator=(const PerfDwarfUnwinder &) = delete;

    // True if libdw accepted at least the setup call; false means every
    // unwind() call will just return an empty list (e.g. no modules could
    // be reported at all). Individual frames still fail independently and
    // gracefully within a successful setup -- this only reports whether
    // unwinding could be attempted at all.
    bool isValid() const;

    // Replaces the modules the unwinder knows, e.g. once more were mapped.
    void setModules(const QList<UnwindModule> &modules);

    // Unwinds one sample's call chain. Returns addresses innermost (leaf)
    // first, matching the same convention PerfRecordDecoder::handleSample()
    // already uses for callchain/branch-stack addresses before reversing
    // them to root-first. Returns an empty list -- not an error -- for
    // anything that stops the unwind early (no CFI for an address, the
    // captured stack running out, and so on): this is a best-effort
    // enrichment on top of already-working symbol resolution, same
    // philosophy as DwarfLineTable.
    QList<quint64> unwind(const UnwindInput &input);

private:
    std::unique_ptr<PerfDwarfUnwinderPrivate> d;
};

} // namespace Profiler::Internal
