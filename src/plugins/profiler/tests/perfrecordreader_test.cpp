// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfrecordreader_test.h"

#include <profiler/perfrecordreader.h>
#include <profiler/perfregisters.h>
#include <profiler/sampletrace.h>

#include <utils/result.h>

#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QtEndian>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#ifdef WITH_LIBDW
#include <profiler/perfdwarfunwinder.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

namespace {

// perf.data record types this reader understands (include/uapi/linux/perf_event.h);
// mirrors the private enum in perfrecordreader.cpp, since these are kernel ABI
// constants, not something the reader exposes.
constexpr quint32 RecordMmap2 = 10;
constexpr quint32 RecordLost = 2;
constexpr quint32 RecordThrottle = 5;
constexpr quint32 RecordLostSamples = 13;
constexpr quint32 RecordComm = 3;
constexpr quint32 RecordFork = 7;
constexpr quint32 RecordSample = 9;
constexpr quint32 RecordHeaderAttr = 64;
constexpr quint32 RecordHeaderTracingData = 66;
constexpr quint32 RecordFinishedRound = 68;

constexpr quint64 SampleIp = 1ull << 0;
constexpr quint64 SampleTid = 1ull << 1;
constexpr quint64 SampleTime = 1ull << 2;
constexpr quint64 SampleCallchain = 1ull << 5;
constexpr quint64 SampleBranchStack = 1ull << 11;
constexpr quint64 SampleRegsUser = 1ull << 12;
constexpr quint64 SampleStackUser = 1ull << 13;

constexpr quint64 PerfContextHv = quint64(-32);
constexpr quint64 PerfContextKernel = quint64(-128);
constexpr quint64 PerfContextUser = quint64(-512);

void appendU16(QByteArray &out, quint16 v)
{
    v = qToLittleEndian(v);
    out.append(reinterpret_cast<const char *>(&v), 2);
}
void appendU32(QByteArray &out, quint32 v)
{
    v = qToLittleEndian(v);
    out.append(reinterpret_cast<const char *>(&v), 4);
}
void appendU64(QByteArray &out, quint64 v)
{
    v = qToLittleEndian(v);
    out.append(reinterpret_cast<const char *>(&v), 8);
}
void appendCString(QByteArray &out, const QString &s)
{
    out.append(s.toUtf8());
    out.append('\0');
}

// Wraps `payload` in a perf_event_header (type + misc + size), matching the
// framing PerfRecordReader::read() splits the stream on.
QByteArray wrapRecord(quint32 type, const QByteArray &payload)
{
    QByteArray record;
    appendU32(record, type);
    appendU16(record, 0); // misc
    appendU16(record, quint16(8 + payload.size()));
    record.append(payload);
    return record;
}

// One PERF_RECORD_HEADER_ATTR: a 40-byte perf_event_attr (only the fields this
// reader looks at; see handleAttr()) followed by the pipe-mode ids[] trailer.
QByteArray buildAttrRecord(quint64 sampleType, const QList<quint64> &ids = {0})
{
    QByteArray payload;
    appendU32(payload, 0);  // perf_event_attr::type
    appendU32(payload, 40); // perf_event_attr::size (== this payload's fixed part)
    appendU64(payload, 0);  // config
    appendU64(payload, 0);  // sample_period_or_freq
    appendU64(payload, sampleType);
    appendU64(payload, 0); // read_format
    for (quint64 id : ids)
        appendU64(payload, id);
    return wrapRecord(RecordHeaderAttr, payload);
}

// Like buildAttrRecord(), but long enough (88 bytes, padded with zeroed
// fields this reader never looks at) to carry perf_event_attr's
// sample_regs_user at its real offset (80) -- see handleAttr(). Needed for
// any dwarf-mode ("--call-graph dwarf") sample: without it, handleSample()
// would not know how many PERF_SAMPLE_REGS_USER register values to expect.
// `branchSampleType` goes to perf_event_attr's branch_sample_type (offset 72).
QByteArray buildAttrRecordWithRegsMask(quint64 sampleType, quint64 sampleRegsUserMask,
                                       const QList<quint64> &ids = {0},
                                       quint64 branchSampleType = 0)
{
    QByteArray payload;
    appendU32(payload, 0);  // type
    appendU32(payload, 88); // size: covers offset 80 (sample_regs_user)
    appendU64(payload, 0);  // config
    appendU64(payload, 0);  // sample_period_or_freq
    appendU64(payload, sampleType);
    appendU64(payload, 0); // read_format
    // bitfields, wakeup_events/watermark, bp_type, bp_addr/config1,
    // bp_len/config2: offsets 40..71, unused here.
    payload.append(QByteArray(32, '\0'));
    appendU64(payload, branchSampleType);   // offset 72: branch_sample_type
    appendU64(payload, sampleRegsUserMask); // offset 80: sample_regs_user
    for (quint64 id : ids)
        appendU64(payload, id);
    return wrapRecord(RecordHeaderAttr, payload);
}

QByteArray buildMmap2Record(quint32 pid, quint64 addr, quint64 len, quint64 pgoff,
                            const QString &path)
{
    QByteArray payload;
    appendU32(payload, pid);
    appendU32(payload, pid); // tid
    appendU64(payload, addr);
    appendU64(payload, len);
    appendU64(payload, pgoff);
    appendU32(payload, 0); // maj
    appendU32(payload, 0); // min
    appendU64(payload, 0); // ino
    appendU64(payload, 0); // ino_generation
    appendU32(payload, 0); // prot
    appendU32(payload, 0); // flags
    appendCString(payload, path);
    return wrapRecord(RecordMmap2, payload);
}

QByteArray buildCommRecord(quint32 pid, quint32 tid, const QString &name)
{
    QByteArray payload;
    appendU32(payload, pid);
    appendU32(payload, tid);
    appendCString(payload, name);
    return wrapRecord(RecordComm, payload);
}

// For sample_type == SampleIp|SampleTid|SampleTime (no callchain): a single
// leaf-PC sample.
QByteArray buildFlatSampleRecord(quint64 ip, quint32 pid, quint32 tid, quint64 timeNs)
{
    QByteArray payload;
    appendU64(payload, ip);
    appendU32(payload, pid);
    appendU32(payload, tid);
    appendU64(payload, timeNs);
    return wrapRecord(RecordSample, payload);
}

// For sample_type == SampleIp|SampleTid|SampleTime|SampleCallchain: `callchain`
// is innermost-first, exactly as the kernel emits it (context markers included).
QByteArray buildCallchainSampleRecord(quint32 pid, quint32 tid, quint64 timeNs,
                                      const QList<quint64> &callchain)
{
    QByteArray payload;
    appendU64(payload, 0); // ip: present because SampleIp is set, unused since
                           // SampleCallchain takes precedence (see handleSample()).
    appendU32(payload, pid);
    appendU32(payload, tid);
    appendU64(payload, timeNs);
    appendU64(payload, quint64(callchain.size()));
    for (quint64 addr : callchain)
        appendU64(payload, addr);
    return wrapRecord(RecordSample, payload);
}

// For sample_type == SampleIp|SampleTid|SampleTime|SampleBranchStack:
// `branchFroms` is the branch stack's "from" (call site) addresses, ordered
// innermost (most recent branch) first, exactly as the kernel emits them.
// "to" and "flags" are filled with zero -- handleSample() doesn't use either.
// A non-empty `callchain` needs SampleCallchain as well, and `hwIdx` an attr
// whose branch_sample_type has PERF_SAMPLE_BRANCH_HW_INDEX.
QByteArray buildLbrSampleRecord(quint64 ip, quint32 pid, quint32 tid, quint64 timeNs,
                                const QList<quint64> &branchFroms,
                                const QList<quint64> &callchain = {},
                                std::optional<quint64> hwIdx = {})
{
    QByteArray payload;
    appendU64(payload, ip);
    appendU32(payload, pid);
    appendU32(payload, tid);
    appendU64(payload, timeNs);
    if (!callchain.isEmpty()) {
        appendU64(payload, quint64(callchain.size()));
        for (quint64 addr : callchain)
            appendU64(payload, addr);
    }
    appendU64(payload, quint64(branchFroms.size()));
    if (hwIdx)
        appendU64(payload, *hwIdx);
    for (quint64 from : branchFroms) {
        appendU64(payload, from);
        appendU64(payload, 0); // to
        appendU64(payload, 0); // flags
    }
    return wrapRecord(RecordSample, payload);
}

// For sample_type == SampleIp|SampleTid|SampleTime|SampleRegsUser|SampleStackUser
// ("--call-graph dwarf"): `regValues` are the register values, one per set bit
// of the attr's own sample_regs_user mask in ascending bit order (see
// buildAttrRecordWithRegsMask()); empty means "abi == 0", i.e. no usable user
// registers at all for this sample (see handleSample()). `stackBytes` is the
// captured chunk of the user stack, with dyn_size always reported equal to
// its full size (nothing "cut off" -- handleSample() only treats dyn_size as
// a size to trust, not as a claim about where it came from).
QByteArray buildDwarfSampleRecord(quint64 ip, quint32 pid, quint32 tid, quint64 timeNs,
                                  const QList<quint64> &regValues, const QByteArray &stackBytes)
{
    QByteArray payload;
    appendU64(payload, ip);
    appendU32(payload, pid);
    appendU32(payload, tid);
    appendU64(payload, timeNs);
    appendU64(payload, regValues.isEmpty() ? 0 : 1); // abi: 0 == NONE, 1 == PERF_SAMPLE_REGS_ABI_64
    for (quint64 v : regValues)
        appendU64(payload, v);
    appendU64(payload, quint64(stackBytes.size())); // size (requested == captured, here)
    payload.append(stackBytes);
    if (!stackBytes.isEmpty())
        appendU64(payload, quint64(stackBytes.size())); // dyn_size
    return wrapRecord(RecordSample, payload);
}

// A dwarf-mode sample the way "perf record --call-graph dwarf" really writes
// it: also carrying a callchain, which holds the kernel frames only -- perf
// sets exclude_callchain_user, leaving the user stack to the unwinder.
QByteArray buildDwarfSampleRecordWithCallchain(quint64 ip, quint32 pid, quint64 timeNs,
                                               const QList<quint64> &callchain,
                                               const QList<quint64> &regValues,
                                               const QByteArray &stackBytes)
{
    QByteArray payload;
    appendU64(payload, ip);
    appendU32(payload, pid);
    appendU32(payload, pid);
    appendU64(payload, timeNs);
    appendU64(payload, quint64(callchain.size()));
    for (quint64 addr : callchain)
        appendU64(payload, addr);
    appendU64(payload, 1); // abi: PERF_SAMPLE_REGS_ABI_64
    for (quint64 v : regValues)
        appendU64(payload, v);
    appendU64(payload, quint64(stackBytes.size()));
    payload.append(stackBytes);
    appendU64(payload, quint64(stackBytes.size())); // dyn_size
    return wrapRecord(RecordSample, payload);
}

QByteArray buildFinishedRoundRecord()
{
    return wrapRecord(RecordFinishedRound, {});
}

// Pipe-mode framing: "PERFILE2" + a u64 header size, then the record stream.
// pipeHeaderSize is fixed at 16 (the two fields just written) -- real
// "perf record -o -" streams carry no extra pipe header data either.
QByteArray buildPipeStream(const QList<QByteArray> &records)
{
    QByteArray stream;
    stream.append("PERFILE2", 8);
    appendU64(stream, 16);
    for (const QByteArray &record : records)
        stream.append(record);
    return stream;
}

Result<SampleTraceData> decode(const QByteArray &stream, const std::function<void(int)> &progress,
                               int *receivedSamples = nullptr)
{
    PerfByteQueue queue;
    queue.push(stream);
    queue.close();
    PerfRecordReader reader;
    Result<SampleTraceData> result = reader.read(queue, progress);
    if (receivedSamples)
        *receivedSamples = reader.receivedSamples();
    return result;
}

Result<SampleTraceData> decode(const QByteArray &stream, int *receivedSamples = nullptr)
{
    return decode(stream, {}, receivedSamples);
}

// A minimal, hand-built ELF64 object exposing a real ".symtab"/".strtab" with
// two mangled C++ function symbols, so PerfRecordReader's ELF-based
// symbolication (and demangling) can be exercised without a real compiled
// binary. Layout: [0,64) ELF header, [64,320) 4 section headers (NULL,
// .shstrtab, .symtab, .strtab), then each section's raw data back to back.
// See loadElfFunctionSymbols() in perfrecordreader.cpp and ElfReader::readIt()
// in utils/elfreader.cpp for what this must satisfy.
// `textAddr`/`textOffset`: where its .text is linked, and where in the file
// it is. The same for a GNU-ld PIE; a non-PIE executable, or lld output, has
// them apart. foo and bar are at the start of .text and 0x100 into it, and
// foo is `fooSize` bytes long.
QByteArray buildMinimalElfWithSymbols(quint64 textAddr = 0x1000, quint64 textOffset = 0x1000,
                                      quint64 fooSize = 0x20)
{
    QByteArray shstrtab;
    shstrtab.append('\0');
    shstrtab.append(".shstrtab");
    shstrtab.append('\0');
    shstrtab.append(".symtab");
    shstrtab.append('\0');
    shstrtab.append(".strtab");
    shstrtab.append('\0');
    shstrtab.append(".text");
    shstrtab.append('\0');
    // Name offsets into shstrtab, computed from the layout just built above.
    constexpr quint32 shstrtabNameOff = 1;
    constexpr quint32 symtabNameOff = 11;
    constexpr quint32 strtabNameOff = 19;
    constexpr quint32 textNameOff = 27;

    QByteArray strtab;
    strtab.append('\0'); // reserved STN_UNDEF entry
    constexpr quint32 fooNameOff = 1;
    strtab.append("_Z3fooi"); // foo(int)
    strtab.append('\0');
    const quint32 barNameOff = quint32(strtab.size());
    strtab.append("_Z3bari"); // bar(int)
    strtab.append('\0');

    const quint64 fooAddr = textAddr;
    const quint64 barAddr = textAddr + 0x100;
    constexpr quint64 barSize = 0x20;
    constexpr quint8 sttFuncGlobal = 0x12; // ST_INFO(STB_GLOBAL, STT_FUNC)

    QByteArray symtab(24, '\0'); // entry 0: reserved, all-zero, filtered by value==0
    const auto appendSym = [&](quint32 nameOff, quint8 info, quint64 value, quint64 size) {
        appendU32(symtab, nameOff);
        symtab.append(char(info));
        symtab.append(char(0)); // other
        appendU16(symtab, 0);   // shndx
        appendU64(symtab, value);
        appendU64(symtab, size);
    };
    appendSym(fooNameOff, sttFuncGlobal, fooAddr, fooSize);
    appendSym(barNameOff, sttFuncGlobal, barAddr, barSize);

    const quint64 shstrtabOff = 64 + 5 * 64;
    const quint64 symtabOff = shstrtabOff + quint64(shstrtab.size());
    const quint64 strtabOff = symtabOff + quint64(symtab.size());

    QByteArray elf(int(strtabOff + strtab.size()), '\0');
    const auto put = [&](qsizetype offset, const void *data, qsizetype size) {
        std::memcpy(elf.data() + offset, data, size);
    };
    const auto putU16 = [&](qsizetype offset, quint16 v) {
        v = qToLittleEndian(v);
        put(offset, &v, 2);
    };
    const auto putU32 = [&](qsizetype offset, quint32 v) {
        v = qToLittleEndian(v);
        put(offset, &v, 4);
    };
    const auto putU64 = [&](qsizetype offset, quint64 v) {
        v = qToLittleEndian(v);
        put(offset, &v, 8);
    };

    elf[0] = char(0x7f);
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 2; // ELFCLASS64
    elf[5] = 1; // ELFDATA2LSB
    elf[6] = 1; // EV_CURRENT

    putU16(16, 2);            // e_type = ET_EXEC
    putU16(18, 62);           // e_machine = EM_X86_64
    putU32(20, 1);            // e_version
    putU64(24, 0);            // e_entry
    putU64(32, 0);            // e_phoff
    putU64(40, 64);           // e_shoff
    putU32(48, 0);            // e_flags
    putU16(52, 64);           // e_ehsize
    putU16(54, 56);           // e_phentsize
    putU16(56, 0);            // e_phnum
    putU16(58, 64);           // e_shentsize
    putU16(60, 5);            // e_shnum
    putU16(62, 1);            // e_shstrndx: section 1 is .shstrtab

    const auto putSectionHeader = [&](int index, quint32 nameOff, quint32 type, quint64 offset,
                                      quint64 size, quint64 flags = 0, quint64 addr = 0) {
        const qsizetype base = 64 + index * 64;
        putU32(base + 0, nameOff);
        putU32(base + 4, type);
        putU64(base + 8, flags);
        putU64(base + 16, addr);
        putU64(base + 24, offset);
        putU64(base + 32, size);
    };
    putSectionHeader(0, 0, 0 /* SHT_NULL */, 0, 0);
    putSectionHeader(1, shstrtabNameOff, 3 /* SHT_STRTAB */, shstrtabOff, quint64(shstrtab.size()));
    putSectionHeader(2, symtabNameOff, 2 /* SHT_SYMTAB */, symtabOff, quint64(symtab.size()));
    putSectionHeader(3, strtabNameOff, 3 /* SHT_STRTAB */, strtabOff, quint64(strtab.size()));
    // Only its header: nothing reads the code itself.
    putSectionHeader(4, textNameOff, 1 /* SHT_PROGBITS */, textOffset, 0x200,
                     0x6 /* SHF_ALLOC | SHF_EXECINSTR */, textAddr);

    put(qsizetype(shstrtabOff), shstrtab.constData(), shstrtab.size());
    put(qsizetype(symtabOff), symtab.constData(), symtab.size());
    put(qsizetype(strtabOff), strtab.constData(), strtab.size());

    return elf;
}

// A stripped ELF64 object: no ".symtab", only a ".note.gnu.build-id" naming
// `buildId`, which makes its debug file something to fetch from debuginfod.
QByteArray buildStrippedElfWithBuildId(const QByteArray &buildId)
{
    QByteArray shstrtab;
    shstrtab.append('\0');
    shstrtab.append(".shstrtab");
    shstrtab.append('\0');
    shstrtab.append(".note.gnu.build-id");
    shstrtab.append('\0');
    constexpr quint32 shstrtabNameOff = 1;
    constexpr quint32 noteNameOff = 11;

    QByteArray note;
    appendU32(note, 4);                      // namesz
    appendU32(note, quint32(buildId.size())); // descsz
    appendU32(note, 3);                      // NT_GNU_BUILD_ID
    note.append("GNU", 4);
    note.append(buildId);

    const quint64 shstrtabOff = 64 + 3 * 64;
    const quint64 noteOff = shstrtabOff + quint64(shstrtab.size());

    QByteArray elf;
    elf.append("\x7f" "ELF", 4);
    elf.append(char(2)); // ELFCLASS64
    elf.append(char(1)); // ELFDATA2LSB
    elf.append(char(1)); // EV_CURRENT
    elf.append(QByteArray(9, '\0'));
    appendU16(elf, 3);  // e_type = ET_DYN
    appendU16(elf, 62); // e_machine = EM_X86_64
    appendU32(elf, 1);  // e_version
    appendU64(elf, 0);  // e_entry
    appendU64(elf, 0);  // e_phoff
    appendU64(elf, 64); // e_shoff
    appendU32(elf, 0);  // e_flags
    appendU16(elf, 64); // e_ehsize
    appendU16(elf, 56); // e_phentsize
    appendU16(elf, 0);  // e_phnum
    appendU16(elf, 64); // e_shentsize
    appendU16(elf, 3);  // e_shnum
    appendU16(elf, 1);  // e_shstrndx: section 1 is .shstrtab

    const auto appendSectionHeader = [&elf](quint32 nameOff, quint32 type, quint64 offset,
                                            quint64 size) {
        appendU32(elf, nameOff);
        appendU32(elf, type);
        appendU64(elf, 0); // flags
        appendU64(elf, 0); // addr
        appendU64(elf, offset);
        appendU64(elf, size);
        appendU32(elf, 0); // link
        appendU32(elf, 0); // info
        appendU64(elf, 0); // addralign
        appendU64(elf, 0); // entsize
    };
    appendSectionHeader(0, 0 /* SHT_NULL */, 0, 0);
    appendSectionHeader(shstrtabNameOff, 3 /* SHT_STRTAB */, shstrtabOff,
                        quint64(shstrtab.size()));
    appendSectionHeader(noteNameOff, 7 /* SHT_NOTE */, noteOff, quint64(note.size()));
    elf.append(shstrtab);
    elf.append(note);
    return elf;
}

#if defined(WITH_LIBDW) && defined(Q_PROCESSOR_X86_64)
// Real DWARF CFI (.eh_frame) data cannot be convincingly hand-built the way
// buildMinimalElfWithSymbols()'s plain .symtab can -- so testUnwindsRealDwarfCallChain()
// below instead captures a *real* register+stack snapshot of this very test
// binary, mid-recursion through these three functions, and feeds it to
// PerfDwarfUnwinder pointed at this binary's own shared object (found via
// dladdr()). volatile/noinline throughout so the compiler can't fold the
// recursion away or reorder the register capture.
volatile int g_dwarfTestSink = 0;

Q_NEVER_INLINE void dwarfTestLeaf(UnwindInput *out)
{
    quint64 regs[24] = {};
    // Captures the x86-64 GPRs in perf's own PERF_REG_X86_* order (see
    // perfdwarfunwinder.cpp's dwarfRegisterFor()) -- simpler and more direct
    // than translating glibc's own, differently-ordered ucontext_t layout.
    asm volatile("movq %%rax, %0\n\t"
                 "movq %%rbx, %1\n\t"
                 "movq %%rcx, %2\n\t"
                 "movq %%rdx, %3\n\t"
                 "movq %%rsi, %4\n\t"
                 "movq %%rdi, %5\n\t"
                 "movq %%rbp, %6\n\t"
                 "movq %%rsp, %7\n\t"
                 "movq %%r8,  %8\n\t"
                 "movq %%r9,  %9\n\t"
                 "movq %%r10, %10\n\t"
                 "movq %%r11, %11\n\t"
                 "movq %%r12, %12\n\t"
                 "movq %%r13, %13\n\t"
                 "movq %%r14, %14\n\t"
                 "movq %%r15, %15\n\t"
                 : "=m"(regs[0]), "=m"(regs[1]), "=m"(regs[2]), "=m"(regs[3]), "=m"(regs[4]),
                   "=m"(regs[5]), "=m"(regs[6]), "=m"(regs[7]), "=m"(regs[16]), "=m"(regs[17]),
                   "=m"(regs[18]), "=m"(regs[19]), "=m"(regs[20]), "=m"(regs[21]), "=m"(regs[22]),
                   "=m"(regs[23])
                 :
                 : "memory");
    // A label's address (a GNU/Clang extension, "labels as values") is a real
    // program counter value inside *this* function, unlike
    // __builtin_return_address(0) (which would give a PC in the caller) --
    // exactly matching what the captured regs[] above are the register state
    // *for*.
    void *pc = &&hereLabel;
hereLabel:
    regs[8] = quint64(reinterpret_cast<quintptr>(pc)); // PERF_REG_X86_IP
    out->regs = QList<quint64>(regs, regs + 24);
    out->stackStartAddr = regs[7]; // PERF_REG_X86_SP
    // A generous window above the current SP: return addresses this unwinder
    // needs live at increasing addresses from here (the stack grows down),
    // well within a default thread's multi-MB stack at only a few frames deep.
    constexpr qsizetype captureSize = 16384;
    out->stackBytes = QByteArray(reinterpret_cast<const char *>(quintptr(regs[7])), captureSize);
    g_dwarfTestSink = 1; // keeps the capture above from being optimized away
}

Q_NEVER_INLINE int dwarfTestMiddle(int depth, UnwindInput *out)
{
    if (depth <= 0) {
        dwarfTestLeaf(out);
        return g_dwarfTestSink;
    }
    // The addition after the recursive call blocks tail-call optimization,
    // which would otherwise collapse this into a loop with no new stack
    // frame per level -- defeating the whole point of this test.
    return dwarfTestMiddle(depth - 1, out) + g_dwarfTestSink;
}

// qsort() (from libc, never inlinable, its comparator called back through a
// real function pointer) puts a genuine, separate-DSO stack frame between
// dwarfTestOuter and the rest of the chain -- unlike libProfiler.so itself
// (built with -g, so it carries embedded DWARF debug info), a system libc is
// always stripped, forcing dwfl_frame_pc()'s internal dwfl_module_getdwarf()
// call to actually go looking for *separate* debug info via the
// Dwfl_Callbacks::find_debuginfo callback. See noDebuginfo() in
// perfdwarfunwinder.cpp for why that used to crash (a null function pointer
// call) whenever a real recording's unwind stepped into exactly this kind of
// module -- this indirection exists purely so this test exercises the same
// path, rather than only ever unwinding through debug-info-rich frames the
// way the rest of this recursion does.
UnwindInput *g_dwarfQsortCaptureTarget = nullptr;

int dwarfTestQsortCompare(const void *, const void *)
{
    dwarfTestMiddle(2, g_dwarfQsortCaptureTarget);
    return 0;
}

Q_NEVER_INLINE int dwarfTestOuter(UnwindInput *out)
{
    g_dwarfQsortCaptureTarget = out;
    int items[2] = {1, 2};
    qsort(items, 2, sizeof(int), dwarfTestQsortCompare);
    g_dwarfQsortCaptureTarget = nullptr;
    return g_dwarfTestSink;
}
#endif // WITH_LIBDW && Q_PROCESSOR_X86_64

} // namespace

class PerfRecordReaderTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSymbolicatesAndDemanglesUserFrames();
    void testSymbolicatesNonPieExecutable();
    void testBuildsCallChainFromBranchStack();
    void testSkipsBranchStackHwIndex();
    void testPrefersBranchStackOverUserCallchain();
    void testSymbolicatesForkedChild();
    void testLooksUpCallersAtTheCall();
    void testRejectsOverflowingCallchainCount();
    void testClampsUserStackToRequestedSize();
    void testSkipsTracingData();
    void testStopsSymbolizingWhenCanceled();
    void testDropsHypervisorFramesAndFlagsKernelFrames();
    void testOrdersAndClampsAcrossRounds();
    void testFiltersPausedSamples()
    {
        PerfByteQueue queue;
        queue.push(buildPipeStream({
            buildAttrRecord(SampleIp | SampleTid | SampleTime),
            buildFlatSampleRecord(0, 7, 7, 1'000),
            buildFlatSampleRecord(0, 7, 7, 2'000),
            buildFlatSampleRecord(0, 7, 7, 3'000),
            buildFlatSampleRecord(0, 7, 7, 4'000),
        }));
        queue.close();
        PerfRecordReader reader;
        reader.setSampleFilter([](qint64 timestampNs) {
            return timestampNs == 2'000 || timestampNs == 4'000;
        });
        const Result<SampleTraceData> result = reader.read(queue);
        QVERIFY_RESULT(result);
        QCOMPARE(reader.receivedSamples(), 2);
        QCOMPARE(reader.firstTimestampNs(), 2'000);
        QCOMPARE(result->samples.size(), 2);
        QCOMPARE(result->samples.at(0).tsUs, 0);
        QCOMPARE(result->samples.at(1).tsUs, 2);
    }
    void testQueueReportsDrainingAndDropsAfterClose();
    void testRejectsUnsupportedSampleType();
    void testReportsNoSamplesCaptured();
    void testReportsEmptyStreamAsEmptyRecording();
    void testCountsSamplesWithoutCallStack();
    void testReportsProgressDuringSymbolication();
    void testRecordsLostAndThrottledSamples();
    void testParsesDwarfModeRegistersAndStack();
    void testRejectsTruncatedDwarfModeStack();
    void testKeepsDwarfSampleWithoutUserCallchain();
#ifdef WITH_LIBDW
    void testSkipsModulesThatAreNoFiles();
#endif
#if defined(WITH_LIBDW) && defined(Q_PROCESSOR_X86_64)
    void testUnwindsRealDwarfCallChain();
    void testUnwindsDwarfSampleThroughReader();
#endif
};

// A non-PIE executable is linked where it runs: its .text at 0x401000, say,
// while it sits at 0x1000 in the file, and its mapping says pgoff 0x1000. The
// symbols are at their link address, which is neither what the mapping gives
// as a file offset nor where the file sits in memory -- lld and mold output
// has the same gap between file offset and link address.
void PerfRecordReaderTest::testSymbolicatesNonPieExecutable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("app");
    const QByteArray elfContents = buildMinimalElfWithSymbols(0x401000, 0x1000);
    QFile elfFile(elfPath);
    QVERIFY(elfFile.open(QIODevice::WriteOnly));
    QCOMPARE(elfFile.write(elfContents), qint64(elfContents.size()));
    elfFile.close();

    constexpr quint32 pid = 4321;
    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain),
        buildMmap2Record(pid, 0x401000, 0x1000, 0x1000, elfPath),
        buildCallchainSampleRecord(pid, pid, 1'000'000, {0x401004, 0x401108}),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
    const QList<int> &frames = result->samples.first().frames;
    QCOMPARE(frames.size(), 2);
    const SampleTraceData::Label &root = result->labels.at(frames.at(0));
    const SampleTraceData::Label &leaf = result->labels.at(frames.at(1));
    QCOMPARE(root.name, u"bar(int)"_s);
    QCOMPARE(root.offset, quint64(7)); // the call before the return address
    QCOMPARE(leaf.name, u"foo(int)"_s);
    QCOMPARE(leaf.offset, quint64(4));
}

// A two-frame frame-pointer callchain into a stripped-down, hand-built ELF
// (see buildMinimalElfWithSymbols()) must come out root-first, resolved by
// name+offset against that ELF's .symtab, and demangled -- the three things
// labelIdFor()/demangleName() are responsible for.
void PerfRecordReaderTest::testSymbolicatesAndDemanglesUserFrames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libtarget.so");
    const QByteArray elfContents = buildMinimalElfWithSymbols();
    QFile elfFile(elfPath);
    QVERIFY(elfFile.open(QIODevice::WriteOnly));
    QCOMPARE(elfFile.write(elfContents), qint64(elfContents.size()));
    elfFile.close();

    constexpr quint32 pid = 4321;
    constexpr quint64 mapAddr = 0x400000;
    // foo() is at file offset 0x1000, bar() at 0x1100 (see
    // buildMinimalElfWithSymbols()); with pgoff 0, mmap addr + file offset
    // gives the absolute address a sample would report for each.
    constexpr quint64 fooIp = mapAddr + 0x1000;
    constexpr quint64 barIp = mapAddr + 0x1100;

    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain),
        buildMmap2Record(pid, mapAddr, 0x2000, 0, elfPath),
        buildCommRecord(pid, pid, u"Worker"_s),
        // Innermost-first: foo() called by bar(), returning 8 bytes into it.
        buildCallchainSampleRecord(pid, pid, 1'000'000, {fooIp, barIp + 8}),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    const SampleTraceData &data = result.value();

    QCOMPARE(data.threadNames.value(pid), u"Worker"_s);
    QCOMPARE(data.samples.size(), 1);
    const SampleTraceData::ThreadSample &sample = data.samples.first();
    QCOMPARE(sample.tid, quint64(pid));
    QVERIFY(sample.running);
    QCOMPARE(sample.tsUs, quint64(0)); // first (and only) sample defines the baseline

    // Root-first: bar (the caller) before foo (the callee).
    QCOMPARE(sample.frames.size(), 2);
    const SampleTraceData::Label &root = data.labels.at(sample.frames.at(0));
    const SampleTraceData::Label &leaf = data.labels.at(sample.frames.at(1));
    QCOMPARE(root.name, u"bar(int)"_s);
    QCOMPARE(root.offset, quint64(7)); // the call before the return address
    QCOMPARE(root.module, u"libtarget.so"_s);
    QCOMPARE(leaf.name, u"foo(int)"_s);
    QCOMPARE(leaf.offset, quint64(0));
}

// The same two-frame scenario as testSymbolicatesAndDemanglesUserFrames(),
// but built from a branch-stack ("--call-graph lbr") sample instead of a
// callchain one: the leaf IP plus one branch-stack entry whose "from" is
// bar()'s address must produce the identical root-first [bar, foo] chain,
// confirming handleSample() treats branch-stack "from" addresses as just
// more (already-resolved) callchain entries -- no unwinding involved.
void PerfRecordReaderTest::testBuildsCallChainFromBranchStack()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libtarget.so");
    const QByteArray elfContents = buildMinimalElfWithSymbols();
    QFile elfFile(elfPath);
    QVERIFY(elfFile.open(QIODevice::WriteOnly));
    QCOMPARE(elfFile.write(elfContents), qint64(elfContents.size()));
    elfFile.close();

    constexpr quint32 pid = 4321;
    constexpr quint64 mapAddr = 0x400000;
    constexpr quint64 fooIp = mapAddr + 0x1000;
    constexpr quint64 barIp = mapAddr + 0x1100;

    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleBranchStack),
        buildMmap2Record(pid, mapAddr, 0x2000, 0, elfPath),
        buildCommRecord(pid, pid, u"Worker"_s),
        // Leaf is the sample's own IP (foo); the one branch-stack entry's
        // "from" (bar) is the caller, exactly like the callchain test.
        buildLbrSampleRecord(fooIp, pid, pid, 1'000'000, {barIp}),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    const SampleTraceData &data = result.value();

    QCOMPARE(data.samples.size(), 1);
    const SampleTraceData::ThreadSample &sample = data.samples.first();
    QCOMPARE(sample.frames.size(), 2);
    const SampleTraceData::Label &root = data.labels.at(sample.frames.at(0));
    const SampleTraceData::Label &leaf = data.labels.at(sample.frames.at(1));
    QCOMPARE(root.name, u"bar(int)"_s);
    QCOMPARE(leaf.name, u"foo(int)"_s);
}

// A callchain mixing hypervisor, kernel and user context markers: the HV
// frame must be dropped outright, the kernel frame resolved against the
// (real, whatever this machine has) kallsyms table or fall back to a raw
// address, and an unmapped user address must fall back the same way -- see
// handleSample()'s context-marker handling and kernelLabelIdFor()/
// labelIdFor()'s "no match" paths.
void PerfRecordReaderTest::testDropsHypervisorFramesAndFlagsKernelFrames()
{
    constexpr quint32 pid = 55;
    constexpr quint64 unmappedUserIp = 0x7f0000001234ull; // no mmap covers this
    constexpr quint64 fakeKernelIp = 0xffffffffa0001234ull;

    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain),
        buildCallchainSampleRecord(pid, pid, 1'000'000, {
            PerfContextHv, 0xdeadbeef,      // dropped: no HV symbol table exists
            PerfContextKernel, fakeKernelIp, // innermost real frame
            PerfContextUser, unmappedUserIp, // caller
        }),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    const SampleTraceData &data = result.value();

    QCOMPARE(data.samples.size(), 1);
    const SampleTraceData::ThreadSample &sample = data.samples.first();
    // The HV frame contributes nothing: only kernel + user frames remain.
    QCOMPARE(sample.frames.size(), 2);

    const SampleTraceData::Label &root = data.labels.at(sample.frames.at(0)); // user
    const SampleTraceData::Label &leaf = data.labels.at(sample.frames.at(1)); // kernel
    QVERIFY(root.module.isEmpty());   // unmapped: no module to attribute it to
    QCOMPARE(leaf.module, u"[kernel]"_s);
}

// perf's pipe-mode stream isn't globally time-sorted across CPUs: samples
// within one PERF_RECORD_FINISHED_ROUND batch can arrive out of order, and a
// batch's first sample can even carry an earlier timestamp than the previous
// batch's last one (clock skew between CPUs). flushPendingRound() is
// responsible for sorting each batch and clamping the result to stay
// monotonic; this pins both behaviors down.
void PerfRecordReaderTest::testOrdersAndClampsAcrossRounds()
{
    constexpr quint32 pid = 7;
    // A throwaway warm-up sample fixes the baseline (tsUs 0) at a known point,
    // so every later timestamp in this test maps to a small, easy-to-check
    // offset instead of an arbitrary one.
    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime),
        buildFlatSampleRecord(0, pid, pid, 0),
        buildFinishedRoundRecord(),
        // One round, three samples arriving out of timestamp order.
        buildFlatSampleRecord(0, pid, pid, 9'000),
        buildFlatSampleRecord(0, pid, pid, 3'000),
        buildFlatSampleRecord(0, pid, pid, 6'000),
        buildFinishedRoundRecord(),
        // Next round's sample is earlier than the previous round's: read late
        // from another CPU's buffer, which is why a round is only complete up
        // to where the one before reached. It still sorts in.
        buildFlatSampleRecord(0, pid, pid, 1'000),
        buildFinishedRoundRecord(),
        // Earlier than what is written already -- must be clamped up, not let
        // the timeline go backwards.
        buildFlatSampleRecord(0, pid, pid, 2'000),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    const SampleTraceData &data = result.value();

    QList<quint64> timestamps;
    for (const SampleTraceData::ThreadSample &sample : data.samples)
        timestamps.append(sample.tsUs);
    QCOMPARE(timestamps, QList<quint64>({0, 1, 3, 6, 9, 9}));
}

// Whoever holds back data while the queue is full hears once it has room
// again; what arrives after the reading side is done goes nowhere.
void PerfRecordReaderTest::testQueueReportsDrainingAndDropsAfterClose()
{
    PerfByteQueue queue;
    int drained = 0;
    queue.setDrainedCallback(10, [&drained] { ++drained; });
    queue.push(QByteArray(6, 'a'));
    queue.push(QByteArray(6, 'b'));
    QCOMPARE(queue.size(), 12);
    QCOMPARE(queue.pop(), QByteArray(6, 'a'));
    QCOMPARE(drained, 1);
    QCOMPARE(queue.pop(), QByteArray(6, 'b'));
    QCOMPARE(drained, 1);

    queue.close();
    queue.push(QByteArray(6, 'c'));
    QCOMPARE(queue.size(), 0);
    QVERIFY(queue.atEnd());
}

// An unsupported sample_type bit (e.g. PERF_SAMPLE_WEIGHT, used for memory
// profiling events this reader has no use for) must be rejected outright at
// the attribute record, before any sample is misparsed against a layout this
// reader doesn't understand.
void PerfRecordReaderTest::testRejectsUnsupportedSampleType()
{
    constexpr quint64 sampleWeight = 1ull << 14;
    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | sampleWeight),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY(!result.has_value());
}

// A stream with a valid attribute but no PERF_RECORD_SAMPLE at all (e.g. the
// target exited before perf ever wrote one) is an empty recording, not a
// broken stream: PerfSampler explains it from what it knows about "perf
// record", which is more than the reader does.
void PerfRecordReaderTest::testReportsNoSamplesCaptured()
{
    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime),
    });

    int receivedSamples = -1;
    const Result<SampleTraceData> result = decode(stream, &receivedSamples);
    QVERIFY_RESULT(result);
    QVERIFY(result->samples.isEmpty());
    QCOMPARE(receivedSamples, 0);
}

// "perf record" that could not open its events (e.g. under a
// perf_event_paranoid that denies it) writes no stream at all. That has to
// reach PerfSampler as the same empty recording, so it is explained by why
// perf failed rather than by a parse error about a missing header.
void PerfRecordReaderTest::testReportsEmptyStreamAsEmptyRecording()
{
    int receivedSamples = -1;
    const Result<SampleTraceData> result = decode({}, &receivedSamples);
    QVERIFY_RESULT(result);
    QVERIFY(result->samples.isEmpty());
    QCOMPARE(receivedSamples, 0);
}

// A sample whose callchain holds nothing this reader keeps (here: only a
// hypervisor frame) is dropped, but still counted, so that PerfSampler can
// tell "perf sampled nothing" from "perf sampled, but no stack survived".
void PerfRecordReaderTest::testCountsSamplesWithoutCallStack()
{
    constexpr quint32 pid = 42;
    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain),
        buildCallchainSampleRecord(pid, pid, 1000, {PerfContextHv, 0xdeadbeef}),
        buildCallchainSampleRecord(pid, pid, 2000, {PerfContextHv, 0xdeadbeef}),
    });

    int receivedSamples = -1;
    const Result<SampleTraceData> result = decode(stream, &receivedSamples);
    QVERIFY_RESULT(result);
    QVERIFY(result->samples.isEmpty());
    QCOMPARE(receivedSamples, 2);
}

// Post-recording symbolication (see PerfRecordDecoder::symbolize()) used to
// report nothing at all, so a UI polling it (see PerfSampler) would show 0%
// for however long that took -- potentially the bulk of the wall-clock time
// for a large trace. resolveSamples() reports every 1024 samples (see its
// own comment); enough samples here to cross that at least twice.
void PerfRecordReaderTest::testReportsProgressDuringSymbolication()
{
    constexpr quint32 pid = 99;
    QList<QByteArray> records = {buildAttrRecord(SampleIp | SampleTid | SampleTime)};
    for (int i = 0; i < 3000; ++i)
        records.append(buildFlatSampleRecord(0, pid, pid, quint64(i) * 1000));
    records.append(buildFinishedRoundRecord());
    const QByteArray stream = buildPipeStream(records);

    QList<int> progressValues;
    const Result<SampleTraceData> result = decode(stream, [&progressValues](int percent) {
        progressValues.append(percent);
    });
    QVERIFY_RESULT(result);

    QVERIFY(!progressValues.isEmpty());
    QCOMPARE(progressValues.last(), 100);
    for (qsizetype i = 1; i < progressValues.size(); ++i)
        QVERIFY(progressValues.at(i) >= progressValues.at(i - 1));
}

// PERF_SAMPLE_REGS_USER/PERF_SAMPLE_STACK_USER ("--call-graph dwarf") must be
// consumed without upsetting the rest of the stream -- this only pins down
// the wire-format parsing itself (the register count comes from
// sample_regs_user's popcount, not a count on the wire; dyn_size, not the
// requested size, is what actually gets kept). Actually exercising
// PerfDwarfUnwinder needs a real ELF with real CFI data, which no hand-built
// byte stream can convincingly provide -- see testUnwindsRealDwarfCallChain()
// below. With no mmap covering this sample's address, resolution falls back
// to the plain (unresolved) leaf PC, same as any other address perf reports
// with no matching module -- proving the new fields don't break the existing
// fallback path either.
void PerfRecordReaderTest::testParsesDwarfModeRegistersAndStack()
{
    constexpr quint32 pid = 321;
    constexpr quint64 ip = 0x555555550000ull;
    // Two registers requested: bit 7 (PERF_REG_X86_SP) and bit 8 (PERF_REG_X86_IP).
    constexpr quint64 regsMask = (1ull << 7) | (1ull << 8);
    const QByteArray stack(256, '\x42');

    const QByteArray stream = buildPipeStream({
        buildAttrRecordWithRegsMask(SampleIp | SampleTid | SampleTime | SampleRegsUser
                                        | SampleStackUser,
                                    regsMask),
        buildDwarfSampleRecord(ip, pid, pid, 1'000'000, {0x7fffffffe000ull, ip}, stack),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    const SampleTraceData &data = result.value();

    QCOMPARE(data.samples.size(), 1);
    const SampleTraceData::ThreadSample &sample = data.samples.first();
    QCOMPARE(sample.frames.size(), 1);
    const SampleTraceData::Label &label = data.labels.at(sample.frames.at(0));
    QCOMPARE(label.offset, ip);
}

// A PERF_SAMPLE_STACK_USER whose declared `size` claims more bytes than the
// record actually has left must be rejected as truncated, not read out of
// bounds.
void PerfRecordReaderTest::testRejectsTruncatedDwarfModeStack()
{
    constexpr quint32 pid = 321;
    constexpr quint64 ip = 0x555555550000ull;
    constexpr quint64 regsMask = 1ull << 8; // PERF_REG_X86_IP only

    QByteArray payload;
    appendU64(payload, ip);
    appendU32(payload, pid);
    appendU32(payload, pid);
    appendU64(payload, 1'000'000);
    appendU64(payload, 1);        // abi
    appendU64(payload, ip);       // the one requested register
    appendU64(payload, 0x10000);  // claims a 64KiB stack capture...
    payload.append(QByteArray(16, '\0')); // ...but the record has nowhere near that much left

    const QByteArray stream = buildPipeStream({
        buildAttrRecordWithRegsMask(SampleIp | SampleTid | SampleTime | SampleRegsUser
                                        | SampleStackUser,
                                    regsMask),
        wrapRecord(RecordSample, payload),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY(!result.has_value());
}

#ifdef WITH_LIBDW
// A process can map what is no regular file, such as a GPU render node.
// Opening one, or reading it, can block for good; a FIFO without a writer
// stands in for one here.
void PerfRecordReaderTest::testSkipsModulesThatAreNoFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QByteArray fifoPath = dir.filePath("fifo").toLocal8Bit();
    QCOMPARE(::mkfifo(fifoPath.constData(), 0600), 0);

    std::unique_ptr<QThread> unwinding(QThread::create([&fifoPath] {
        PerfDwarfUnwinder unwinder({{QString::fromLocal8Bit(fifoPath), 0}});
    }));
    const auto join = qScopeGuard([&] {
        // Releases an open() waiting for a writer, should the FIFO not be
        // skipped.
        if (!unwinding->isFinished()) {
            const int fd = ::open(fifoPath.constData(), O_WRONLY | O_NONBLOCK);
            if (fd >= 0)
                ::close(fd);
        }
        unwinding->wait();
    });
    unwinding->start();
    QTRY_VERIFY_WITH_TIMEOUT(unwinding->isFinished(), 10000);
}
#endif // WITH_LIBDW

#if defined(WITH_LIBDW) && defined(Q_PROCESSOR_X86_64)
// The one test in this file that exercises PerfDwarfUnwinder against *real*
// DWARF CFI data rather than hand-built bytes (see the dwarfTest* helpers
// above): captures a genuine register+stack snapshot several real recursive
// calls deep into this very test binary -- routed through a real libc
// qsort() frame partway up, so the unwind genuinely steps from a
// debug-info-rich module (this plugin's own, built with -g) into a stripped
// one with none (libc) -- then checks the unwind recovers more than just the
// leaf PC, and includes a frame inside libc itself: proving libdw's CFI
// engine actually walked real frames across a real module boundary, not
// merely that it didn't crash on the easy (single-module) case.
void PerfRecordReaderTest::testUnwindsRealDwarfCallChain()
{
    UnwindInput input;
    dwarfTestOuter(&input);
    QVERIFY(input.isValid());
    const quint64 capturedIp = input.regs.at(8); // PERF_REG_X86_IP; see dwarfTestLeaf()

    Dl_info info = {};
    QVERIFY(dladdr(reinterpret_cast<void *>(&dwarfTestLeaf), &info) != 0);
    QVERIFY(info.dli_fname != nullptr);
    const QString modulePath = QString::fromLocal8Bit(info.dli_fname);
    const quint64 bias = quint64(reinterpret_cast<quintptr>(info.dli_fbase));

    Dl_info libcInfo = {};
    QVERIFY(dladdr(reinterpret_cast<void *>(&qsort), &libcInfo) != 0);
    QVERIFY(libcInfo.dli_fname != nullptr);
    const QString libcPath = QString::fromLocal8Bit(libcInfo.dli_fname);
    const quint64 libcBias = quint64(reinterpret_cast<quintptr>(libcInfo.dli_fbase));

    PerfDwarfUnwinder unwinder({{modulePath, bias}, {libcPath, libcBias}});
    QVERIFY(unwinder.isValid());

    const QList<quint64> pcs = unwinder.unwind(input); // innermost-first
    // Real recursion three levels deep plus the leaf and outer frames: a
    // correct CFI-based unwind recovers considerably more than just the one
    // leaf PC a broken (or absent) unwinder would be stuck with.
    QVERIFY2(pcs.size() >= 5, qPrintable(u"only recovered %1 frame(s)"_s.arg(pcs.size())));
    QCOMPARE(pcs.first(), capturedIp); // frame 0 is exactly the captured PC
    // The leaf-through-outer frames (dwarfTestLeaf, the dwarfTestMiddle
    // levels, dwarfTestOuter) are small functions defined right next to each
    // other in this same source file -- a correct unwind keeps them within a
    // small address span of one another, without relying on symbol names
    // being exported (this test binary need not keep any particular
    // visibility for them). Deeper frames beyond that (whatever in this
    // plugin's own Qt metacall/test-dispatch machinery called
    // dwarfTestOuter) are perfectly legitimate too, just not constrained
    // here -- there is no reason for them to stay nearby in the binary.
    const qsizetype closeFrames = qMin<qsizetype>(5, pcs.size());
    quint64 minPc = pcs.first(), maxPc = pcs.first();
    for (qsizetype i = 1; i < closeFrames; ++i) {
        minPc = qMin(minPc, pcs.at(i));
        maxPc = qMax(maxPc, pcs.at(i));
    }
    QVERIFY2(maxPc - minPc < 0x4000,
             qPrintable(u"pc span 0x%1 among the first %2 frames looks implausible for small "
                        "local functions"_s.arg(maxPc - minPc, 0, 16).arg(closeFrames)));
    // The whole point of routing this recursion through qsort(): a frame
    // genuinely inside libc -- a stripped module with no embedded debug
    // info, forcing dwfl_frame_pc()'s internal dwfl_module_getdwarf() call to
    // actually invoke Dwfl_Callbacks::find_debuginfo. Getting here at all
    // (without the whole test process crashing first) already exercises that
    // path; this additionally confirms the unwind kept going correctly
    // afterwards, rather than e.g. silently truncating at the module
    // boundary.
    const bool steppedThroughLibc = std::any_of(pcs.cbegin(), pcs.cend(), [libcBias](quint64 pc) {
        return pc >= libcBias && pc - libcBias < 0x2000000; // generous bound for libc's own size
    });
    QVERIFY2(steppedThroughLibc,
             "expected at least one unwound frame inside libc (via qsort()) -- see noDebuginfo() "
             "in perfdwarfunwinder.cpp for why this exact module-boundary case used to crash");
}

// One mapping of this very process, as /proc/self/maps lists it.
struct SelfMapping
{
    quint64 addr = 0;
    quint64 len = 0;
    quint64 pgoff = 0;
    QString path;
};

static QList<SelfMapping> selfMappingsOf(const QStringList &paths)
{
    QList<SelfMapping> mappings;
    QFile maps("/proc/self/maps");
    if (!maps.open(QIODevice::ReadOnly | QIODevice::Text))
        return mappings;
    // "start-end perms offset dev inode path"
    for (const QByteArray &line : maps.readAll().split('\n')) {
        const QList<QByteArray> fields = line.simplified().split(' ');
        if (fields.size() < 6)
            continue;
        const QString path = QString::fromLocal8Bit(fields.at(5));
        if (!paths.contains(path))
            continue;
        const QList<QByteArray> range = fields.at(0).split('-');
        SelfMapping mapping;
        mapping.addr = range.at(0).toULongLong(nullptr, 16);
        mapping.len = range.at(1).toULongLong(nullptr, 16) - mapping.addr;
        mapping.pgoff = fields.at(2).toULongLong(nullptr, 16);
        mapping.path = path;
        mappings.append(mapping);
    }
    return mappings;
}

// A "--call-graph dwarf" recording of this process, as "perf record" would
// write it: this test binary and libc mapped where they really are, then one
// sample carrying the register+stack snapshot dwarfTestOuter() captures.
// The kernel reports only the leaf PC in dwarf mode, so a sample
// that comes back with more frames than that was unwound by the reader.
static QByteArray buildDwarfRecording(quint32 pid)
{
    UnwindInput input;
    dwarfTestOuter(&input);
    if (!input.isValid())
        return {};

    Dl_info info = {};
    Dl_info libcInfo = {};
    if (!dladdr(reinterpret_cast<void *>(&dwarfTestLeaf), &info)
        || !dladdr(reinterpret_cast<void *>(&qsort), &libcInfo)) {
        return {};
    }
    const QList<SelfMapping> mappings = selfMappingsOf(
        {QFileInfo(QString::fromLocal8Bit(info.dli_fname)).canonicalFilePath(),
         QFileInfo(QString::fromLocal8Bit(libcInfo.dli_fname)).canonicalFilePath()});
    if (mappings.isEmpty())
        return {};

    constexpr quint64 allRegs = (1ull << 24) - 1; // PERF_REG_X86_AX..R15
    QList<QByteArray> records{buildAttrRecordWithRegsMask(
        SampleIp | SampleTid | SampleTime | SampleRegsUser | SampleStackUser, allRegs)};
    for (const SelfMapping &mapping : mappings) {
        records.append(
            buildMmap2Record(pid, mapping.addr, mapping.len, mapping.pgoff, mapping.path));
    }
    records.append(buildDwarfSampleRecord(input.regs.at(8), pid, pid, 1000, input.regs,
                                          input.stackBytes));
    records.append(buildFinishedRoundRecord());
    return buildPipeStream(records);
}

// The whole dwarf path of the reader -- regs and stack parsed from the
// sample, modules built from the recording's own mmaps, then unwound --
// rather than PerfDwarfUnwinder on its own.
void PerfRecordReaderTest::testUnwindsDwarfSampleThroughReader()
{
    const QByteArray stream = buildDwarfRecording(4242);
    QVERIFY(!stream.isEmpty());

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
    const qsizetype frames = result->samples.first().frames.size();
    QVERIFY2(frames >= 5, qPrintable(u"only %1 frame(s)"_s.arg(frames)));
}
#endif // WITH_LIBDW && Q_PROCESSOR_X86_64

// A dwarf-mode sample's callchain holds no user frames (see
// buildDwarfSampleRecordWithCallchain()). The sample must not be dropped as
// stackless for that: its user leaf comes from the captured registers, outside
// the kernel frames of a sample taken in a system call. Here nothing can be
// unwound -- no module is mapped -- so that leaf is all there is of the user
// stack.
void PerfRecordReaderTest::testKeepsDwarfSampleWithoutUserCallchain()
{
    constexpr quint32 pid = 321;
    constexpr quint64 userIp = 0x555555550000ull;
    constexpr quint64 kernelIp = 0xffffffff81000000ull;
    // The registers are numbered as perf does for this machine, which the
    // reader takes the recording to be from.
    const PerfRegisterLayout layout = perfRegisterLayout(hostPerfArchitecture());
    if (layout.ip < 0)
        QSKIP("perf has no user registers for this architecture.");
    const quint64 regsMask = (1ull << layout.sp) | (1ull << layout.ip); // sp sorts first

    const QByteArray stream = buildPipeStream({
        buildAttrRecordWithRegsMask(SampleIp | SampleTid | SampleTime | SampleCallchain
                                        | SampleRegsUser | SampleStackUser,
                                    regsMask),
        buildDwarfSampleRecordWithCallchain(kernelIp, pid, 1000, {PerfContextKernel, kernelIp},
                                            {0x7fffffffe000ull, userIp}, QByteArray(256, '\0')),
        buildDwarfSampleRecordWithCallchain(userIp, pid, 2000, {}, {0x7fffffffe000ull, userIp},
                                            QByteArray(256, '\0')),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    const SampleTraceData &data = result.value();
    QCOMPARE(data.samples.size(), 2);

    const SampleTraceData::ThreadSample &inKernel = data.samples.at(0); // root-first
    QCOMPARE(inKernel.frames.size(), 2);
    QCOMPARE(data.labels.at(inKernel.frames.at(0)).offset, userIp);

    const SampleTraceData::ThreadSample &inUser = data.samples.at(1);
    QCOMPARE(inUser.frames.size(), 1);
    QCOMPARE(data.labels.at(inUser.frames.at(0)).offset, userIp);
}

// perf reports a ring buffer that overflowed (PERF_RECORD_LOST) where it
// notices, which has no time of its own and so sits at the newest sample
// before it, and the kernel lowering the sampling rate (PERF_RECORD_THROTTLE)
// at the time it carries. At the end it writes each event's total of lost
// samples (PERF_RECORD_LOST_SAMPLES), which includes those reported one by
// one: as seen from a real "perf record", which wrote LOST records adding up
// to 7482 and a LOST_SAMPLES record of 7482. Only what the total adds beyond
// them is a further loss, at the end.
void PerfRecordReaderTest::testRecordsLostAndThrottledSamples()
{
    constexpr quint32 pid = 99;
    const auto lostRecord = [](quint64 count) {
        QByteArray payload;
        appendU64(payload, 0); // id
        appendU64(payload, count);
        return wrapRecord(RecordLost, payload);
    };
    const auto lostSamplesRecord = [](quint64 count) {
        QByteArray payload;
        appendU64(payload, count);
        return wrapRecord(RecordLostSamples, payload);
    };
    QByteArray throttle;
    appendU64(throttle, 1'000'000 + 7'000); // time, 7 us into the recording
    appendU64(throttle, 0); // id
    appendU64(throttle, 0); // stream_id

    const auto decodeWithTotal = [&](quint64 total) {
        return decode(buildPipeStream({
            buildAttrRecord(SampleIp | SampleTid | SampleTime),
            buildFlatSampleRecord(0x1000, pid, pid, 1'000'000),
            buildFlatSampleRecord(0x1000, pid, pid, 1'000'000 + 3'000),
            lostRecord(5),
            lostRecord(2),
            wrapRecord(RecordThrottle, throttle),
            buildFlatSampleRecord(0x1000, pid, pid, 1'000'000 + 9'000),
            lostRecord(4),
            buildFinishedRoundRecord(),
            lostSamplesRecord(total),
        }));
    };

    const Result<SampleTraceData> covered = decodeWithTotal(11);
    QVERIFY_RESULT(covered);
    const QList<SampleTraceData::LostSamples> located{{3, 7}, {9, 4}};
    QCOMPARE(covered->lostSamples, located);
    QCOMPARE(covered->throttledTsUs, QList<quint64>{7});

    const Result<SampleTraceData> beyond = decodeWithTotal(14);
    QVERIFY_RESULT(beyond);
    const QList<SampleTraceData::LostSamples> withRest{{3, 7}, {9, 7}};
    QCOMPARE(beyond->lostSamples, withRest);
}

// Writes `contents` to `path`.
static bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

// "perf record --call-graph lbr" asks for PERF_SAMPLE_BRANCH_HW_INDEX, which
// puts a hw_idx between the branch stack's nr and its entries.
void PerfRecordReaderTest::testSkipsBranchStackHwIndex()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libtarget.so");
    QVERIFY(writeFile(elfPath, buildMinimalElfWithSymbols()));

    constexpr quint32 pid = 4321;
    constexpr quint64 mapAddr = 0x400000;
    constexpr quint64 fooIp = mapAddr + 0x1000;
    constexpr quint64 barIp = mapAddr + 0x1100;
    constexpr quint64 branchHwIndex = 1ull << 17;

    const QByteArray stream = buildPipeStream({
        buildAttrRecordWithRegsMask(SampleIp | SampleTid | SampleTime | SampleBranchStack, 0, {0},
                                    branchHwIndex),
        buildMmap2Record(pid, mapAddr, 0x2000, 0, elfPath),
        buildLbrSampleRecord(fooIp, pid, pid, 1'000'000, {barIp + 4}, {}, quint64(-1)),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
    const QList<int> &frames = result->samples.first().frames;
    QCOMPARE(frames.size(), 2);
    QCOMPARE(result->labels.at(frames.at(0)).name, u"bar(int)"_s);
    QCOMPARE(result->labels.at(frames.at(0)).offset, quint64(4));
    QCOMPARE(result->labels.at(frames.at(1)).name, u"foo(int)"_s);
}

// A child forked without exec() runs in its parent's mappings, which the
// kernel does not report again for it.
void PerfRecordReaderTest::testSymbolicatesForkedChild()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libtarget.so");
    QVERIFY(writeFile(elfPath, buildMinimalElfWithSymbols()));

    constexpr quint32 parent = 4321;
    constexpr quint32 child = 4322;
    constexpr quint64 mapAddr = 0x400000;
    QByteArray fork;
    appendU32(fork, child);
    appendU32(fork, parent);
    appendU32(fork, child);  // tid
    appendU32(fork, parent); // ptid
    appendU64(fork, 500'000);

    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime),
        buildMmap2Record(parent, mapAddr, 0x2000, 0, elfPath),
        wrapRecord(RecordFork, fork),
        buildFlatSampleRecord(mapAddr + 0x1004, child, child, 1'000'000),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
    const QList<int> &frames = result->samples.first().frames;
    QCOMPARE(frames.size(), 1);
    QCOMPARE(result->labels.at(frames.at(0)).name, u"foo(int)"_s);
}

// In lbr mode the kernel's callchain still has a user part, walked by frame
// pointer through code that has none. Only the sampled address is taken from
// it; the branch stack has the callers.
void PerfRecordReaderTest::testPrefersBranchStackOverUserCallchain()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libtarget.so");
    QVERIFY(writeFile(elfPath, buildMinimalElfWithSymbols()));

    constexpr quint32 pid = 4321;
    constexpr quint64 mapAddr = 0x400000;
    constexpr quint64 fooIp = mapAddr + 0x1000;
    constexpr quint64 barIp = mapAddr + 0x1100;

    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain | SampleBranchStack),
        buildMmap2Record(pid, mapAddr, 0x2000, 0, elfPath),
        buildLbrSampleRecord(fooIp, pid, pid, 1'000'000, {barIp + 4},
                             {PerfContextUser, fooIp, 0x1234, 0x5678}),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
    const QList<int> &frames = result->samples.first().frames;
    QCOMPARE(frames.size(), 2);
    QCOMPARE(result->labels.at(frames.at(0)).name, u"bar(int)"_s);
    QCOMPARE(result->labels.at(frames.at(1)).name, u"foo(int)"_s);
}

// A caller's address in a callchain is where the call returns to. After a
// call that ends a function, that is the start of the next one.
void PerfRecordReaderTest::testLooksUpCallersAtTheCall()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libtarget.so");
    QVERIFY(writeFile(elfPath, buildMinimalElfWithSymbols(0x1000, 0x1000, 0x100)));

    constexpr quint32 pid = 4321;
    constexpr quint64 mapAddr = 0x400000;
    constexpr quint64 barIp = mapAddr + 0x1100; // right after the end of foo()

    const QByteArray stream = buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain),
        buildMmap2Record(pid, mapAddr, 0x2000, 0, elfPath),
        // Sampled in bar(), called by the last instruction of foo().
        buildCallchainSampleRecord(pid, pid, 1'000'000, {PerfContextUser, barIp + 4, barIp}),
        buildFinishedRoundRecord(),
    });

    const Result<SampleTraceData> result = decode(stream);
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
    const QList<int> &frames = result->samples.first().frames;
    QCOMPARE(frames.size(), 2);
    const SampleTraceData::Label &root = result->labels.at(frames.at(0));
    const SampleTraceData::Label &leaf = result->labels.at(frames.at(1));
    QCOMPARE(root.name, u"foo(int)"_s);
    QCOMPARE(root.offset, quint64(0xff));
    QCOMPARE(leaf.name, u"bar(int)"_s);
    QCOMPARE(leaf.offset, quint64(4));
}

// A count whose byte size does not fit into 64 bits must not pass the
// bounds check by wrapping around.
void PerfRecordReaderTest::testRejectsOverflowingCallchainCount()
{
    QByteArray payload;
    appendU64(payload, 0);         // ip
    appendU32(payload, 1);         // pid
    appendU32(payload, 1);         // tid
    appendU64(payload, 1'000'000); // time
    appendU64(payload, (1ull << 61) + 1); // nr: times 8 wraps around to 8
    appendU64(payload, 0x1000);

    const Result<SampleTraceData> result = decode(buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime | SampleCallchain),
        wrapRecord(RecordSample, payload),
    }));
    QVERIFY(!result);
}

// dyn_size is what the kernel could copy of the requested size; a larger one
// is corrupt and must not make the reader copy past the captured stack.
void PerfRecordReaderTest::testClampsUserStackToRequestedSize()
{
    constexpr quint32 pid = 321;
    constexpr quint64 ip = 0x555555550000ull;

    QByteArray payload;
    appendU64(payload, ip);
    appendU32(payload, pid);
    appendU32(payload, pid);
    appendU64(payload, 1'000'000);
    appendU64(payload, 1);  // abi
    appendU64(payload, ip); // the one requested register, PERF_REG_X86_IP
    appendU64(payload, 16); // size
    payload.append(QByteArray(16, '\x42'));
    appendU64(payload, 0x7fffffff); // dyn_size

    const Result<SampleTraceData> result = decode(buildPipeStream({
        buildAttrRecordWithRegsMask(SampleIp | SampleTid | SampleTime | SampleRegsUser
                                        | SampleStackUser,
                                    1ull << 8),
        wrapRecord(RecordSample, payload),
        buildFinishedRoundRecord(),
    }));
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
}

// With a tracepoint among the events, "perf record" writes the tracing data
// right after its PERF_RECORD_HEADER_TRACING_DATA, outside of the record.
void PerfRecordReaderTest::testSkipsTracingData()
{
    QByteArray tracingData("\x17\x08\x44tracing0.6", 14);
    tracingData.append(QByteArray(32 - tracingData.size(), '\x5a'));
    QByteArray header;
    appendU32(header, quint32(tracingData.size()));
    appendU32(header, 0); // pad

    const Result<SampleTraceData> result = decode(buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime),
        wrapRecord(RecordHeaderTracingData, header) + tracingData,
        buildFlatSampleRecord(0x1000, 7, 7, 1'000'000),
        buildFinishedRoundRecord(),
    }));
    QVERIFY_RESULT(result);
    QCOMPARE(result->samples.size(), 1);
}

// Debug information fetched from a server that does not answer is waited for
// up to DEBUGINFOD_TIMEOUT. A recording that is canceled meanwhile stops
// waiting.
void PerfRecordReaderTest::testStopsSymbolizingWhenCanceled()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString elfPath = dir.filePath("libstripped.so");
    QVERIFY(writeFile(elfPath, buildStrippedElfWithBuildId(QByteArray(20, '\x5c'))));

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    const QList<std::pair<const char *, QByteArray>> environment{
        {"DEBUGINFOD_URLS", "http://127.0.0.1:" + QByteArray::number(server.serverPort())},
        {"DEBUGINFOD_CACHE_PATH", dir.filePath("cache").toLocal8Bit()},
        {"DEBUGINFOD_TIMEOUT", "600"},
    };
    QList<std::optional<QByteArray>> saved;
    for (const auto &[name, value] : environment) {
        saved.append(qEnvironmentVariableIsSet(name) ? std::optional(qgetenv(name))
                                                     : std::nullopt);
        qputenv(name, value);
    }
    const auto restoreEnvironment = qScopeGuard([&] {
        for (qsizetype i = 0; i < environment.size(); ++i) {
            if (saved.at(i))
                qputenv(environment.at(i).first, *saved.at(i));
            else
                qunsetenv(environment.at(i).first);
        }
    });

    constexpr quint32 pid = 4321;
    constexpr quint64 mapAddr = 0x400000;
    PerfByteQueue queue;
    queue.push(buildPipeStream({
        buildAttrRecord(SampleIp | SampleTid | SampleTime),
        buildMmap2Record(pid, mapAddr, 0x2000, 0, elfPath),
        buildFlatSampleRecord(mapAddr + 0x10, pid, pid, 1'000'000),
        buildFinishedRoundRecord(),
    }));
    queue.close();

    std::atomic_bool canceled = false;
    std::optional<Result<SampleTraceData>> result;
    std::unique_ptr<QThread> reading(QThread::create([&] {
        PerfRecordReader reader;
        reader.setDownloadDebugInfo(true);
        reader.setCancelCheck([&canceled] { return canceled.load(); });
        result = reader.read(queue);
    }));
    const auto join = qScopeGuard([&] {
        // Fails the fetch, and its retry, should canceling not end it.
        canceled = true;
        QList<QTcpSocket *> connections;
        while (QTcpSocket *connection = server.nextPendingConnection())
            connections.append(connection);
        server.close();
        for (QTcpSocket *connection : std::as_const(connections))
            connection->abort();
        reading->wait();
    });
    reading->start();

    // The fetch has started.
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 30000);
    canceled = true;
    QTRY_VERIFY_WITH_TIMEOUT(reading->isFinished(), 30000);
    QVERIFY(result);
    QVERIFY(!*result);
}

QObject *createPerfRecordReaderTest()
{
    return new PerfRecordReaderTest;
}

} // namespace Profiler::Internal

#include "perfrecordreader_test.moc"
