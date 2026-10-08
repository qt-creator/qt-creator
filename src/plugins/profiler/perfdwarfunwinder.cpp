// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdwarfunwinder.h"

#include <utils/filepath.h>

#include <QtEndian>

#include <dwarf.h>
#include <elfutils/version.h>
#include <libdw.h>
#include <libdwfl.h>
#include <libelf.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <utility>

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

    // Where libdw starts, by DWARF register numbers: at the sampled PC, or
    // at a return address where it starts again below a frame it had no CFI
    // for (see stepByFrameRecords()).
    Dwarf_Word startRegs[32] = {};
    Dwarf_Addr startPc = 0;
    bool startsAtReturnAddress = false;
    bool atStart = false; // the next frame is the one libdw starts at

    // The frame libdw stopped at for having no CFI, and whether it is the
    // sampled one.
    bool stoppedWithoutCfi = false;
    Dwarf_Word stopRegs[32] = {};
    bool stoppedAtLeaf = false;

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

    // dwfl_thread_state_registers() sets one *contiguous* DWARF-numbered
    // block at a time; each architecture's general-purpose registers are
    // contiguous from 0, covering every perf register mapped, so one call
    // suffices.
    if (!dwfl_thread_state_registers(thread, 0, dwarfRegisterCount(input->arch),
                                     unwinder->startRegs)) {
        return false;
    }

    // The PC is not part of that block everywhere (x86-64's RIP is not);
    // libdw wants it set separately (see libdwfl.h's
    // dwfl_thread_state_register_pc doc).
    dwfl_thread_state_register_pc(thread, unwinder->startPc);
    return true;
}

bool readStackWord(const UnwindInput &input, quint64 addr, quint64 *result)
{
    if (addr < input.stackStartAddr)
        return false;
    const quint64 offset = addr - input.stackStartAddr;
    const int wordSize = perfRegisterLayout(input.arch).wordSize;
    if (offset + wordSize > quint64(input.stackBytes.size()))
        return false; // outside the captured window -- stop unwinding here
    const auto *word = reinterpret_cast<const uchar *>(input.stackBytes.constData() + offset);
    // libdw strips a pointer authentication signature only where the CFI says
    // a return address was signed, not where it falls back to the frame
    // pointer chain, so it is stripped from everything it reads instead.
    *result = withoutPointerAuthentication(input.arch,
                                           wordSize == 4 ? qFromLittleEndian<quint32>(word)
                                                         : qFromLittleEndian<quint64>(word),
                                           input.useHostPointerAuthentication);
    return true;
}

bool memoryRead(Dwfl *, Dwarf_Addr addr, Dwarf_Word *result, void *arg)
{
    auto *unwinder = static_cast<PerfDwarfUnwinderPrivate *>(arg);
    quint64 value = 0;
    if (!unwinder->current || !readStackWord(*unwinder->current, addr, &value))
        return false;
    *result = value;
    return true;
}

// What the CFI says about the frame at `pc`, to be freed with std::free(),
// or null where libdw has no CFI for it.
Dwarf_Frame *cfiFrameAt(Dwfl *dwfl, Dwarf_Addr pc)
{
    Dwfl_Module *module = dwfl_addrmodule(dwfl, pc);
    if (!module)
        return nullptr;
    for (const auto cfiOf : {dwfl_module_eh_cfi, dwfl_module_dwarf_cfi}) {
        Dwarf_Addr bias = 0;
        Dwarf_CFI *cfi = cfiOf(module, &bias);
        Dwarf_Frame *frame = nullptr;
        if (cfi && dwarf_cfi_addrframe(cfi, pc - bias, &frame) == 0)
            return frame;
    }
    return nullptr;
}

// Whether libdw has CFI for `pc`, which is in the call for a return address.
bool hasCfi(Dwfl *dwfl, Dwarf_Addr pc)
{
    Dwarf_Frame *frame = cfiFrameAt(dwfl, pc);
    std::free(frame);
    return frame != nullptr;
}

constexpr int maxFrames = 256;
constexpr int aarch64Fp = 29;
constexpr int aarch64Lr = 30;
constexpr int aarch64Sp = 31;

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
    // Where libdw starts again, it takes the return address for the PC of
    // an activation.
    const bool isStart = std::exchange(unwinder->atStart, false);
    if (isStart && unwinder->startsAtReturnAddress)
        isActivation = false;
    if (!isActivation)
        --pc;
    unwinder->pcs.append(quint64(pc));
    // A generous but finite cap: a correct unwind terminates on its own (no
    // more CFI, or an unwindable root like _start); this only guards
    // against turning an actual bug into an unbounded loop.
    if (unwinder->pcs.size() >= maxFrames)
        return DWARF_CB_ABORT;

    // Without CFI, libdw steps out of an aarch64 frame by taking the PC from
    // the link register and popping the frame record at the frame pointer.
    // That is one record too many for a frame that has not called anything,
    // and one too few for one that has, so the next frame with CFI starts
    // from the wrong stack and everything beyond it is garbage. Such frames
    // -- in the vdso, JIT-compiled code, hand-written assembly -- are stepped
    // out of by frame records instead (see stepByFrameRecords()).
    if (unwinder->current->arch != PerfArchitecture::Aarch64
        || hasCfi(dwfl_thread_dwfl(dwfl_frame_thread(state)), pc)) {
        return DWARF_CB_OK;
    }
    if (isStart) {
        std::copy(std::begin(unwinder->startRegs), std::end(unwinder->startRegs),
                  std::begin(unwinder->stopRegs));
    } else {
#if _ELFUTILS_PREREQ(0, 188)
        for (unsigned regno = 0; regno < std::size(unwinder->stopRegs); ++regno) {
            if (dwfl_frame_reg(state, regno, &unwinder->stopRegs[regno]) != 0)
                unwinder->stopRegs[regno] = 0;
        }
#else
        return DWARF_CB_OK; // no access to the frame's registers
#endif
    }
    unwinder->stoppedAtLeaf = isStart && !unwinder->startsAtReturnAddress;
    unwinder->stoppedWithoutCfi = true;
    return DWARF_CB_ABORT;
}

// The stack pointer of the caller that `pc` returns to and whose frame
// pointer is `fp`, by its CFI: its frame pointer points at its own frame
// record, where the CFI says it saved its caller's frame pointer from the
// CFA, and the CFA is its stack pointer plus what the CFI says. `sp`, the
// lowest it can be, where the CFI does not say, as where the CFA is taken
// from the frame pointer and the stack pointer does not matter.
quint64 callerStackPointer(Dwfl *dwfl, quint64 pc, quint64 fp, quint64 sp)
{
    Dwarf_Frame *frame = cfiFrameAt(dwfl, pc - 1);
    if (!frame)
        return sp;
    Dwarf_Op *cfaOps = nullptr;
    size_t cfaOpCount = 0;
    Dwarf_Op fpOpsMem[3];
    Dwarf_Op *fpOps = nullptr;
    size_t fpOpCount = 0;
    quint64 callerSp = sp;
    // libdw's "register plus offset" CFA, and "saved at CFA plus offset".
    if (dwarf_frame_cfa(frame, &cfaOps, &cfaOpCount) == 0 && cfaOpCount == 1
        && cfaOps[0].atom == DW_OP_bregx && cfaOps[0].number == aarch64Sp
        && dwarf_frame_register(frame, aarch64Fp, fpOpsMem, &fpOps, &fpOpCount) == 0
        && fpOpCount == 2 && fpOps[0].atom == DW_OP_call_frame_cfa
        && fpOps[1].atom == DW_OP_plus_uconst) {
        // The offsets are signed, kept in unsigned words.
        const quint64 cfa = fp - fpOps[1].number;
        const quint64 fromCfi = cfa - cfaOps[0].number2;
        // A caller's stack pointer is below its frame record.
        if (fromCfi >= sp && fromCfi <= fp)
            callerSp = fromCfi;
    }
    std::free(frame);
    return callerSp;
}

// Steps out of the frame libdw stopped at for having no CFI, and out of
// every further one without, by the frame records an aarch64 frame pointer
// chains: the caller's frame pointer, then the return address. Every frame
// that has called something has its record at its frame pointer. A sampled
// frame need not have a record of its own: then the return address is still
// in the link register. Sets up where libdw starts again, at the first caller
// it has CFI for. Its stack pointer is right above the last record only where
// that frame pushed its record first, as Clang and JIT-compiled code do, not
// where it saved registers above it, as GCC does, so it is taken from the
// caller's CFI.
//
// Once a sampled frame has called something, the link register points back
// into it, not to its caller. Without CFI that cannot be told from a frame
// without a record of its own, unless the link register is the return address
// the record holds, or points into a caller that has CFI, which the sampled
// frame does not.
bool stepByFrameRecords(PerfDwarfUnwinderPrivate *d)
{
    const UnwindInput &input = *d->current;
    Dwarf_Word fp = d->stopRegs[aarch64Fp];
    Dwarf_Word sp = d->stopRegs[aarch64Sp];
    quint64 pc = 0;
    const auto popRecord = [&input, &fp, &sp, &pc] {
        quint64 callerFp = 0;
        if (!readStackWord(input, fp + 8, &pc) || !readStackWord(input, fp, &callerFp))
            return false;
        // The stack grows down, so a caller's record is above its callee's.
        if (callerFp != 0 && callerFp <= fp)
            return false;
        sp = fp + 16;
        fp = callerFp;
        return true;
    };

    if (d->stoppedAtLeaf) {
        pc = d->stopRegs[aarch64Lr];
        quint64 recordedLr = 0;
        const bool lrIsRecorded = readStackWord(input, fp + 8, &recordedLr) && recordedLr == pc;
        const bool lrIsCaller = !lrIsRecorded && pc != 0 && hasCfi(d->dwfl, pc - 1);
        if (!lrIsCaller && !popRecord())
            return false;
    } else if (!popRecord()) {
        return false;
    }
    while (pc != 0 && !hasCfi(d->dwfl, pc - 1)) {
        d->pcs.append(pc - 1);
        if (d->pcs.size() >= maxFrames || fp == 0 || !popRecord())
            return false;
    }
    if (pc == 0)
        return false;

    std::copy(std::begin(d->stopRegs), std::end(d->stopRegs), std::begin(d->startRegs));
    d->startRegs[aarch64Fp] = fp;
    d->startRegs[aarch64Sp] = callerStackPointer(d->dwfl, pc, fp, sp);
    d->startRegs[aarch64Lr] = 0;
    d->startPc = pc;
    d->startsAtReturnAddress = true;
    return true;
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

bool PerfDwarfUnwinder::isReturnAddressInRegister(quint64 pc) const
{
    if (!d->dwfl)
        return false;
    Dwarf_Frame *frame = cfiFrameAt(d->dwfl, pc);
    if (!frame)
        return false;
    Dwarf_Addr start = 0;
    Dwarf_Addr end = 0;
    bool isSignalFrame = false;
    const int returnAddressRegister = dwarf_frame_info(frame, &start, &end, &isSignalFrame);
    // No operations and no array for them is libdw's "same value": the
    // register still holds what the call put there.
    Dwarf_Op opsMem[3];
    Dwarf_Op *ops = opsMem;
    size_t opCount = 0;
    const bool inRegister = returnAddressRegister >= 0
                            && dwarf_frame_register(frame, returnAddressRegister, opsMem, &ops,
                                                    &opCount) == 0
                            && opCount == 0 && ops == nullptr;
    std::free(frame);
    return inRegister;
}

QList<quint64> PerfDwarfUnwinder::unwind(const UnwindInput &input)
{
    if (!d->dwfl || !input.isValid())
        return {};
    const PerfRegisterLayout layout = perfRegisterLayout(input.arch);
    const int count = dwarfRegisterCount(input.arch);
    if (layout.ip < 0 || input.regs.size() <= layout.ip || count == 0)
        return {};
    std::fill(std::begin(d->startRegs), std::end(d->startRegs), 0);
    for (int perfReg = 0; perfReg < input.regs.size(); ++perfReg) {
        const int dwarfReg = dwarfRegisterFor(input.arch, perfReg);
        if (dwarfReg >= 0 && dwarfReg < count)
            d->startRegs[dwarfReg] = withoutPointerAuthentication(input.arch,
                                                                   input.regs.at(perfReg),
                                                                   input.useHostPointerAuthentication);
    }
    d->startPc = input.regs.at(layout.ip);
    d->startsAtReturnAddress = false;

    d->current = &input;
    d->pcs.clear();
    do {
        d->atStart = true;
        d->stoppedWithoutCfi = false;
        dwfl_getthread_frames(d->dwfl, SyntheticTid, frameCallback, d.get());
    } while (d->stoppedWithoutCfi && stepByFrameRecords(d.get()));
    d->current = nullptr;
    return d->pcs;
}

} // namespace Profiler::Internal
