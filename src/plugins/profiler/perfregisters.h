// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringView>

namespace Profiler::Internal {

// The architectures whose user registers a "--call-graph dwarf" sample can
// be unwound from (see PerfDwarfUnwinder).
enum class PerfArchitecture { Unknown, X86_64, X86, Aarch64, Arm };

// Where perf keeps an architecture's stack pointer, program counter and link
// register, if it has one, among the user registers it samples, by its own
// register numbers (arch/*/include/uapi/asm/perf_regs.h), and how many of
// those there are.
struct PerfRegisterLayout
{
    int count = 0;
    int sp = -1;
    int ip = -1;
    int wordSize = 8;
    int lr = -1;
};

inline PerfRegisterLayout perfRegisterLayout(PerfArchitecture arch)
{
    switch (arch) {
    case PerfArchitecture::X86_64:
        return {24, 7, 8, 8};  // PERF_REG_X86_SP, _IP; up to PERF_REG_X86_R15
    case PerfArchitecture::X86:
        return {9, 7, 8, 4};   // PERF_REG_X86_SP, _IP; the 32-bit ones end at IP
    case PerfArchitecture::Aarch64:
        return {33, 31, 32, 8, 30}; // PERF_REG_ARM64_SP, _PC, _LR
    case PerfArchitecture::Arm:
        return {16, 13, 15, 4, 14}; // PERF_REG_ARM_SP, _PC, _LR
    case PerfArchitecture::Unknown:
        break;
    }
    return {};
}

// By the names "uname -m", perf's "arch" feature and Qt Creator's ABIs use.
inline PerfArchitecture perfArchitectureFromName(QStringView name)
{
    if (name == u"x86_64" || name == u"amd64")
        return PerfArchitecture::X86_64;
    if (name == u"x86" || name == u"i386" || name == u"i486" || name == u"i586"
        || name == u"i686") {
        return PerfArchitecture::X86;
    }
    if (name == u"aarch64" || name == u"arm64")
        return PerfArchitecture::Aarch64;
    if (name.startsWith(u"arm"))
        return PerfArchitecture::Arm;
    return PerfArchitecture::Unknown;
}

// With pointer authentication, aarch64 return addresses carry a signature in
// the bits above the address, up to bit 54. Where it starts depends on the
// virtual address size the kernel was built for, so for a recording taken
// here it is taken from what the CPU strips as signature off an address with
// all bits up to 54 set. For one taken elsewhere, which this CPU says nothing
// about, and where it cannot be asked or signs nothing, 48-bit virtual
// addresses are assumed. No user space pointer has these bits set.
inline quint64 aarch64UserPacMask(bool recordedHere)
{
    constexpr quint64 assumedMask = 0x007f000000000000ull;
#ifdef Q_PROCESSOR_ARM_64
    static const quint64 hostMask = [] {
        constexpr quint64 allAddressBits = (1ull << 55) - 1;
        quint64 stripped = 0;
        // xpaclri, a NOP without pointer authentication, where nothing is signed.
        asm("mov x30, %1\n\t"
            "hint #7\n\t"
            "mov %0, x30"
            : "=r"(stripped)
            : "r"(allAddressBits)
            : "x30");
        return allAddressBits & ~stripped;
    }();
    if (recordedHere && hostMask != 0)
        return hostMask;
#else
    Q_UNUSED(recordedHere)
#endif
    return assumedMask;
}

inline quint64 withoutPointerAuthentication(PerfArchitecture arch, quint64 value,
                                            bool recordedHere = true)
{
    return arch == PerfArchitecture::Aarch64 ? value & ~aarch64UserPacMask(recordedHere) : value;
}

inline PerfArchitecture hostPerfArchitecture()
{
#if defined(Q_PROCESSOR_X86_64)
    return PerfArchitecture::X86_64;
#elif defined(Q_PROCESSOR_X86_32)
    return PerfArchitecture::X86;
#elif defined(Q_PROCESSOR_ARM_64)
    return PerfArchitecture::Aarch64;
#elif defined(Q_PROCESSOR_ARM_32)
    return PerfArchitecture::Arm;
#else
    return PerfArchitecture::Unknown;
#endif
}

} // namespace Profiler::Internal
