// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdwarfunwinder.h"

#include <utils/filepath.h>

#include <QtEndian>

#include <libdw.h>
#include <libdwfl.h>
#include <libelf.h>

#include <fcntl.h>
#include <unistd.h>

namespace Profiler::Internal {

struct PerfDwarfUnwinderPrivate
{
    Dwfl *dwfl = nullptr;
    // What the process state's architecture backend is opened from, for as
    // long as it lives; see the constructor.
    int archFd = -1;
    Elf *archElf = nullptr;
    const UnwindInput *current = nullptr; // the sample currently being unwound
    QList<quint64> pcs; // accumulated by frameCallback(), innermost-first

    ~PerfDwarfUnwinderPrivate()
    {
        if (dwfl)
            dwfl_end(dwfl);
        if (archElf)
            elf_end(archElf);
        if (archFd >= 0)
            ::close(archFd);
    }
};

namespace {

// perf's own x86-64 general-purpose register indices (the stable,
// kernel-defined PERF_REG_X86_* enum, arch/x86/include/uapi/asm/perf_regs.h
// -- hardcoded rather than depending on that header being present at build
// time, matching how perfrecordreader.cpp already hardcodes other perf ABI
// constants directly).
enum : int {
    PerfRegX86Ax = 0,
    PerfRegX86Bx = 1,
    PerfRegX86Cx = 2,
    PerfRegX86Dx = 3,
    PerfRegX86Si = 4,
    PerfRegX86Di = 5,
    PerfRegX86Bp = 6,
    PerfRegX86Sp = 7,
    PerfRegX86Ip = 8,
    PerfRegX86R8 = 16,
    PerfRegX86R9 = 17,
    PerfRegX86R10 = 18,
    PerfRegX86R11 = 19,
    PerfRegX86R12 = 20,
    PerfRegX86R13 = 21,
    PerfRegX86R14 = 22,
    PerfRegX86R15 = 23,
};

// Maps a perf x86-64 GPR index to its DWARF register number (System V AMD64
// ABI's "DWARF Register Number Mapping" table -- a stable, widely-relied-on
// standard, not something specific to this reader). Returns -1 for a perf
// register CFI programs never actually reference (segment selectors,
// rflags): those are simply not passed to libdw at all.
int x86_64DwarfRegisterFor(int perfReg)
{
    switch (perfReg) {
    case PerfRegX86Ax: return 0;
    case PerfRegX86Dx: return 1;
    case PerfRegX86Cx: return 2;
    case PerfRegX86Bx: return 3;
    case PerfRegX86Si: return 4;
    case PerfRegX86Di: return 5;
    case PerfRegX86Bp: return 6;
    case PerfRegX86Sp: return 7;
    case PerfRegX86R8: return 8;
    case PerfRegX86R9: return 9;
    case PerfRegX86R10: return 10;
    case PerfRegX86R11: return 11;
    case PerfRegX86R12: return 12;
    case PerfRegX86R13: return 13;
    case PerfRegX86R14: return 14;
    case PerfRegX86R15: return 15;
    default: return -1;
    }
}

// i386's DWARF numbering: eax, ecx, edx, ebx, esp, ebp, esi, edi (the System V
// i386 psABI); perf's: AX, BX, CX, DX, SI, DI, BP, SP, IP.
int x86DwarfRegisterFor(int perfReg)
{
    switch (perfReg) {
    case PerfRegX86Ax: return 0;
    case PerfRegX86Cx: return 1;
    case PerfRegX86Dx: return 2;
    case PerfRegX86Bx: return 3;
    case PerfRegX86Sp: return 4;
    case PerfRegX86Bp: return 5;
    case PerfRegX86Si: return 6;
    case PerfRegX86Di: return 7;
    default: return -1;
    }
}

// The DWARF register a perf register of `arch` is; the PC is set apart.
int dwarfRegisterFor(PerfArchitecture arch, int perfReg)
{
    switch (arch) {
    case PerfArchitecture::X86_64:
        return x86_64DwarfRegisterFor(perfReg);
    case PerfArchitecture::X86:
        return x86DwarfRegisterFor(perfReg);
    case PerfArchitecture::Aarch64:
        return perfReg <= 31 ? perfReg : -1; // x0..x30, sp: numbered alike
    case PerfArchitecture::Arm:
        return perfReg <= 15 ? perfReg : -1; // r0..r15: numbered alike
    case PerfArchitecture::Unknown:
        break;
    }
    return -1;
}

// How many DWARF registers, from 0, the ones mapped above make up.
int dwarfRegisterCount(PerfArchitecture arch)
{
    switch (arch) {
    case PerfArchitecture::X86_64: return 16;
    case PerfArchitecture::X86: return 8;
    case PerfArchitecture::Aarch64: return 32;
    case PerfArchitecture::Arm: return 16;
    case PerfArchitecture::Unknown: break;
    }
    return 0;
}

// Exactly one synthetic "thread" per PerfDwarfUnwinder::unwind() call --
// there is no real process or thread here, just a captured register+stack
// snapshot -- identified by this arbitrary, fixed id.
constexpr pid_t SyntheticTid = 1;

pid_t nextThread(Dwfl *, void *dwflArg, void **threadArgp)
{
    if (*threadArgp)
        return 0; // already handed out the one synthetic thread
    *threadArgp = dwflArg;
    return SyntheticTid;
}

bool getThread(Dwfl *, pid_t tid, void *dwflArg, void **threadArgp)
{
    if (tid != SyntheticTid)
        return false;
    *threadArgp = dwflArg;
    return true;
}

bool setInitialRegisters(Dwfl_Thread *thread, void *arg)
{
    auto *unwinder = static_cast<PerfDwarfUnwinderPrivate *>(arg);
    const UnwindInput *input = unwinder->current;
    if (!input)
        return false;
    const PerfRegisterLayout layout = perfRegisterLayout(input->arch);
    if (layout.ip < 0 || input->regs.size() <= layout.ip)
        return false;

    // dwfl_thread_state_registers() sets one *contiguous* DWARF-numbered
    // block at a time; each architecture's general-purpose registers are
    // contiguous from 0, covering every perf register mapped, so one call
    // suffices.
    const int count = dwarfRegisterCount(input->arch);
    Dwarf_Word dwarfRegs[32] = {};
    bool haveAny = false;
    for (int perfReg = 0; perfReg < input->regs.size(); ++perfReg) {
        const int dwarfReg = dwarfRegisterFor(input->arch, perfReg);
        if (dwarfReg < 0 || dwarfReg >= count)
            continue;
        dwarfRegs[dwarfReg] = input->regs.at(perfReg);
        haveAny = true;
    }
    if (!haveAny || !dwfl_thread_state_registers(thread, 0, count, dwarfRegs))
        return false;

    // The PC is not part of that block everywhere (x86-64's RIP is not);
    // libdw wants it set separately (see libdwfl.h's
    // dwfl_thread_state_register_pc doc).
    dwfl_thread_state_register_pc(thread, input->regs.at(layout.ip));
    return true;
}

bool memoryRead(Dwfl *, Dwarf_Addr addr, Dwarf_Word *result, void *arg)
{
    auto *unwinder = static_cast<PerfDwarfUnwinderPrivate *>(arg);
    const UnwindInput *input = unwinder->current;
    if (!input || addr < input->stackStartAddr)
        return false;
    const quint64 offset = quint64(addr) - input->stackStartAddr;
    const int wordSize = perfRegisterLayout(input->arch).wordSize;
    if (offset + wordSize > quint64(input->stackBytes.size()))
        return false; // outside the captured window -- stop unwinding here
    const auto *word = reinterpret_cast<const uchar *>(input->stackBytes.constData() + offset);
    *result = wordSize == 4 ? qFromLittleEndian<quint32>(word) : qFromLittleEndian<quint64>(word);
    return true;
}

int frameCallback(Dwfl_Frame *state, void *arg)
{
    auto *unwinder = static_cast<PerfDwarfUnwinderPrivate *>(arg);
    Dwarf_Addr pc = 0;
    bool isActivation = false;
    if (!dwfl_frame_pc(state, &pc, &isActivation))
        return DWARF_CB_ABORT;
    // A return address of 0 is libdw's own end-of-chain marker -- the
    // conventional value a CFI program (or the outermost real frame, e.g.
    // below _start/__libc_start_main) uses for "the return address register
    // is undefined here", i.e. there is no caller left to step into. Must be
    // checked before the `--pc` adjustment below: decrementing a zero PC
    // would otherwise wrap around to an enormous, very much *not* end-of-chain
    // looking address (0xfff...fff) and get appended as if it were real.
    if (pc == 0)
        return DWARF_CB_ABORT;
    // A non-activation frame's PC is a return address (the instruction
    // *after* the call); stepping back into the call instruction itself
    // keeps symbol lookup from landing on an unrelated function that
    // happens to start right after a tail call. See dwfl_frame_pc()'s doc.
    if (!isActivation)
        --pc;
    unwinder->pcs.append(quint64(pc));
    // A generous but finite cap: a correct unwind terminates on its own (no
    // more CFI, or an unwindable root like _start); this only guards
    // against turning an actual bug into an unbounded loop.
    return unwinder->pcs.size() < 256 ? DWARF_CB_OK : DWARF_CB_ABORT;
}

// dwfl_frame_pc() -- called from frameCallback() above -- internally calls
// dwfl_module_getdwarf() on whichever module covers the frame's PC, and for
// any module lacking *embedded* debug info (i.e. essentially every stripped
// system library: libc, ld.so, the various Qt shared libraries, etc. --
// this reader's own Debug-built plugin binaries are the exception, not the
// rule) that unconditionally calls the Dwfl_Callbacks::find_debuginfo
// callback with no null check. Leaving it null -- on the assumption it would
// go as unused as find_elf, since dwfl_report_elf() below reports every
// module directly and needs neither -- was wrong: it crashes (PC == 0,
// SIGSEGV, fault address 0x0 -- a call through a null function pointer) the
// first time a real recording steps into any such module, discovered
// against a real target after this reader itself, and every unit test
// fixture, happened to only ever unwind through symbols with embedded debug
// info. This callback exists solely to be non-null: it always reports "no
// separate debug info available" without searching the filesystem or
// network at all -- this unwinder only ever needs CFI (already present in
// whatever the module's own ELF has), never full DWARF debug info; symbols
// and source lines are resolved entirely independently elsewhere in this
// reader (see ModuleSymbols/DwarfLineTable in perfrecordreader.cpp).
int noDebuginfo(Dwfl_Module *, void **, const char *, Dwarf_Addr, const char *, const char *,
                GElf_Word, char **)
{
    return -1;
}

constexpr Dwfl_Thread_Callbacks threadCallbacks = {
    nextThread,
    getThread,
    memoryRead,
    setInitialRegisters,
    nullptr, // detach
    nullptr, // thread_detach
};

} // namespace

PerfDwarfUnwinder::PerfDwarfUnwinder(const QList<UnwindModule> &modules)
    : d(std::make_unique<PerfDwarfUnwinderPrivate>())
{
    // find_elf is genuinely unused: dwfl_report_elf() below reports each
    // module directly by file path, bypassing on-demand lookup entirely (per
    // its own doc: "the find_elf callback will not be used for this
    // module"). find_debuginfo is NOT unused -- see noDebuginfo()'s own
    // comment for why it must not be left null. section_address must be
    // dwfl_offline_section_address for this kind of (non-live) module
    // reporting, per libdwfl.h's own doc on that function.
    static const Dwfl_Callbacks dwflCallbacks = {
        nullptr,
        noDebuginfo,
        dwfl_offline_section_address,
        nullptr,
    };
    d->dwfl = dwfl_begin(&dwflCallbacks);
    if (!d->dwfl)
        return;

    setModules(modules);

    // Without an ELF of its own, dwfl_attach_state() borrows the architecture
    // backend of the first module, which a later setModules() that does not
    // report that module again frees. So the backend is opened from a file
    // of the first module this unwinder keeps open itself.
    for (const UnwindModule &module : modules) {
        if (!Utils::FilePath::fromString(module.path).isFile())
            continue;
        const int fd = ::open(module.path.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            continue;
        Elf *elf = elf_begin(fd, ELF_C_READ_MMAP, nullptr);
        if (elf && elf_kind(elf) == ELF_K_ELF) {
            d->archFd = fd;
            d->archElf = elf;
            break;
        }
        if (elf)
            elf_end(elf);
        ::close(fd);
    }
    if (!d->archElf
        || !dwfl_attach_state(d->dwfl, d->archElf, SyntheticTid, &threadCallbacks, d.get())) {
        dwfl_end(d->dwfl);
        d->dwfl = nullptr;
    }
}

PerfDwarfUnwinder::~PerfDwarfUnwinder() = default;

void PerfDwarfUnwinder::setModules(const QList<UnwindModule> &modules)
{
    if (!d->dwfl)
        return;
    // A report pass replaces the module list, keeping -- CFI already parsed
    // and all -- each module reported again as it was: so modules are known
    // the moment they are mapped, and the list is rebuilt, not grown.
    dwfl_report_begin(d->dwfl);
    for (const UnwindModule &module : modules) {
        // libdw opens what it is given, and a device such as a GPU render
        // node, or a FIFO, can block that, or the read after it, for good.
        if (!Utils::FilePath::fromString(module.path).isFile())
            continue;
        const QByteArray path = module.path.toLocal8Bit();
        // A module libdw can't open/parse just means frames in it won't
        // unwind past it -- not fatal for the whole unwinder, so the
        // return value (the new Dwfl_Module*, or null on failure) is not
        // checked here.
        dwfl_report_elf(d->dwfl, path.constData(), path.constData(), -1, module.bias, false);
    }
    dwfl_report_end(d->dwfl, nullptr, nullptr);
}

bool PerfDwarfUnwinder::isValid() const
{
    return d->dwfl != nullptr;
}

QList<quint64> PerfDwarfUnwinder::unwind(const UnwindInput &input)
{
    if (!d->dwfl || !input.isValid())
        return {};
    d->current = &input;
    d->pcs.clear();
    dwfl_getthread_frames(d->dwfl, SyntheticTid, frameCallback, d.get());
    d->current = nullptr;
    return d->pcs;
}

} // namespace Profiler::Internal
