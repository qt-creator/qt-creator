/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "abi.h"

#include <utils/fileutils.h>

#include <QDebug>
#include <QtEndian>
#include <QFile>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QSysInfo>

/*!
    \class ProjectExplorer::Abi

    \brief The Abi class represents the Application Binary Interface (ABI) of
    a target platform.

    \sa ProjectExplorer::ToolChain
*/

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

static quint8 getUint8(const QByteArray &data, int pos)
{
    return static_cast<quint8>(data.at(pos));
}

static quint32 getLEUint32(const QByteArray &ba, int pos)
{
    Q_ASSERT(ba.size() >= pos + 3);
    return (static_cast<quint32>(static_cast<quint8>(ba.at(pos + 3))) << 24)
            + (static_cast<quint32>(static_cast<quint8>(ba.at(pos + 2)) << 16))
            + (static_cast<quint32>(static_cast<quint8>(ba.at(pos + 1))) << 8)
            + static_cast<quint8>(ba.at(pos));
}

static quint32 getBEUint32(const QByteArray &ba, int pos)
{
    Q_ASSERT(ba.size() >= pos + 3);
    return (static_cast<quint32>(static_cast<quint8>(ba.at(pos))) << 24)
            + (static_cast<quint32>(static_cast<quint8>(ba.at(pos + 1))) << 16)
            + (static_cast<quint32>(static_cast<quint8>(ba.at(pos + 2))) << 8)
            + static_cast<quint8>(ba.at(pos + 3));
}

static quint32 getLEUint16(const QByteArray &ba, int pos)
{
    Q_ASSERT(ba.size() >= pos + 1);
    return (static_cast<quint16>(static_cast<quint8>(ba.at(pos + 1))) << 8) + static_cast<quint8>(ba.at(pos));
}

static quint32 getBEUint16(const QByteArray &ba, int pos)
{
    Q_ASSERT(ba.size() >= pos + 1);
    return (static_cast<quint16>(static_cast<quint8>(ba.at(pos))) << 8) + static_cast<quint8>(ba.at(pos + 1));
}

static Abi macAbiForCpu(quint32 type) {
    switch (type) {
    case 7: // CPU_TYPE_X86, CPU_TYPE_I386
        return Abi(Abi::X86Architecture, Abi::DarwinOS, Abi::GenericDarwinFlavor, Abi::MachOFormat, 32);
    case 0x01000000 +  7: // CPU_TYPE_X86_64
        return Abi(Abi::X86Architecture, Abi::DarwinOS, Abi::GenericDarwinFlavor, Abi::MachOFormat, 64);
    case 18: // CPU_TYPE_POWERPC
        return Abi(Abi::PowerPCArchitecture, Abi::DarwinOS, Abi::GenericDarwinFlavor, Abi::MachOFormat, 32);
    case 0x01000000 + 18: // CPU_TYPE_POWERPC64
        return Abi(Abi::PowerPCArchitecture, Abi::DarwinOS, Abi::GenericDarwinFlavor, Abi::MachOFormat, 32);
    case 12: // CPU_TYPE_ARM
        return Abi(Abi::ArmArchitecture, Abi::DarwinOS, Abi::GenericDarwinFlavor, Abi::MachOFormat, 32);
    case 0x01000000 + 12: // CPU_TYPE_ARM64
        return Abi(Abi::ArmArchitecture, Abi::DarwinOS, Abi::GenericDarwinFlavor, Abi::MachOFormat, 64);
    default:
        return Abi();
    }
}

static QList<Abi> parseCoffHeader(const QByteArray &data)
{
    QList<Abi> result;
    if (data.size() < 20)
        return result;

    Abi::Architecture arch = Abi::UnknownArchitecture;
    Abi::OSFlavor flavor = Abi::UnknownFlavor;
    int width = 0;

    // Get machine field from COFF file header
    quint16 machine = getLEUint16(data, 0);
    switch (machine) {
    case 0x01c0: // ARM LE
    case 0x01c2: // ARM or thumb
    case 0x01c4: // ARMv7 thumb
        arch = Abi::ArmArchitecture;
        width = 32;
        break;
    case 0x8664: // x86_64
        arch = Abi::X86Architecture;
        width = 64;
        break;
    case 0x014c: // i386
        arch = Abi::X86Architecture;
        width = 32;
        break;
    case 0x0166: // MIPS, little endian
        arch = Abi::MipsArchitecture;
        width = 32;
        break;
    case 0x0200: // ia64
        arch = Abi::ItaniumArchitecture;
        width = 64;
        break;
    }

    if (data.size() >= 24) {
        // Get Major and Minor Image Version from optional header fields
        quint8 minorLinker = data.at(23);
        switch (data.at(22)) {
        case 2:
        case 3: // not yet reached:-)
            flavor = Abi::WindowsMSysFlavor;
            break;
        case 8:
            flavor = Abi::WindowsMsvc2005Flavor;
            break;
        case 9:
            flavor = Abi::WindowsMsvc2008Flavor;
            break;
        case 10:
            flavor = Abi::WindowsMsvc2010Flavor;
            break;
        case 11:
            flavor = Abi::WindowsMsvc2012Flavor;
            break;
        case 12:
            flavor = Abi::WindowsMsvc2013Flavor;
            break;
        case 14:
            flavor = minorLinker >= quint8(10)
                ? Abi::WindowsMsvc2017Flavor // MSVC2017 RC
                : Abi::WindowsMsvc2015Flavor;
            break;
        case 15:
            flavor = Abi::WindowsMsvc2017Flavor;
            break;
        default: // Keep unknown flavor
            if (minorLinker != 0)
                flavor = Abi::WindowsMSysFlavor; // MSVC seems to avoid using minor numbers
            else
                qWarning("%s: Unknown MSVC flavour encountered.", Q_FUNC_INFO);
            break;
        }
    }

    if (arch != Abi::UnknownArchitecture && width != 0)
        result.append(Abi(arch, Abi::WindowsOS, flavor, Abi::PEFormat, width));

    return result;
}

static QList<Abi> abiOf(const QByteArray &data)
{
    QList<Abi> result;
    if (data.size() <= 8)
        return result;

    if (data.size() >= 20
            && getUint8(data, 0) == 0x7f && getUint8(data, 1) == 'E' && getUint8(data, 2) == 'L'
            && getUint8(data, 3) == 'F') {
        // ELF format:
        bool isLE = (getUint8(data, 5) == 1);
        quint16 machine = isLE ? getLEUint16(data, 18) : getBEUint16(data, 18);
        quint8 osAbi = getUint8(data, 7);

        Abi::OS os = Abi::UnixOS;
        Abi::OSFlavor flavor = Abi::GenericUnixFlavor;
        // http://www.sco.com/developers/gabi/latest/ch4.eheader.html#elfid
        switch (osAbi) {
#if defined(Q_OS_NETBSD)
        case 0: // NetBSD: ELFOSABI_NETBSD  2, however, NetBSD uses 0
            os = Abi::BsdOS;
            flavor = Abi::NetBsdFlavor;
            break;
#elif defined(Q_OS_OPENBSD)
        case 0: // OpenBSD: ELFOSABI_OPENBSD 12, however, OpenBSD uses 0
            os = Abi::BsdOS;
            flavor = Abi::OpenBsdFlavor;
            break;
#else
        case 0: // no extra info available: Default to Linux:
#endif
        case 3: // Linux:
        case 97: // ARM, also linux most of the time.
            os = Abi::LinuxOS;
            flavor = Abi::GenericLinuxFlavor;
            break;
        case 6: // Solaris:
            os = Abi::UnixOS;
            flavor = Abi::SolarisUnixFlavor;
            break;
        case 9: // FreeBSD:
            os = Abi::BsdOS;
            flavor = Abi::FreeBsdFlavor;
            break;
        }

        switch (machine) {
        case 3: // EM_386
            result.append(Abi(Abi::X86Architecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 8: // EM_MIPS
            result.append(Abi(Abi::MipsArchitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 20: // EM_PPC
            result.append(Abi(Abi::PowerPCArchitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 21: // EM_PPC64
            result.append(Abi(Abi::PowerPCArchitecture, os, flavor, Abi::ElfFormat, 64));
            break;
        case 40: // EM_ARM
            result.append(Abi(Abi::ArmArchitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 183: // EM_AARCH64
            result.append(Abi(Abi::ArmArchitecture, os, flavor, Abi::ElfFormat, 64));
            break;
        case 62: // EM_X86_64
            result.append(Abi(Abi::X86Architecture, os, flavor, Abi::ElfFormat, 64));
            break;
        case 42: // EM_SH
            result.append(Abi(Abi::ShArchitecture, os, flavor, Abi::ElfFormat, 32));
            break;
        case 50: // EM_IA_64
            result.append(Abi(Abi::ItaniumArchitecture, os, flavor, Abi::ElfFormat, 64));
            break;
        default:
            ;
        }
    } else if (((getUint8(data, 0) == 0xce || getUint8(data, 0) == 0xcf)
             && getUint8(data, 1) == 0xfa && getUint8(data, 2) == 0xed && getUint8(data, 3) == 0xfe
            )
            ||
            (getUint8(data, 0) == 0xfe && getUint8(data, 1) == 0xed && getUint8(data, 2) == 0xfa
             && (getUint8(data, 3) == 0xce || getUint8(data, 3) == 0xcf)
            )
           ) {
            // Mach-O format (Mac non-fat binary, 32 and 64bit magic):
            quint32 type = (getUint8(data, 1) ==  0xfa) ? getLEUint32(data, 4) : getBEUint32(data, 4);
            result.append(macAbiForCpu(type));
    } else if ((getUint8(data, 0) == 0xbe && getUint8(data, 1) == 0xba
                && getUint8(data, 2) == 0xfe && getUint8(data, 3) == 0xca)
               ||
               (getUint8(data, 0) == 0xca && getUint8(data, 1) == 0xfe
                && getUint8(data, 2) == 0xba && getUint8(data, 3) == 0xbe)
              ) {
        // Mach-0 format Fat binary header:
        bool isLE = (getUint8(data, 0) == 0xbe);
        quint32 count = isLE ? getLEUint32(data, 4) : getBEUint32(data, 4);
        int pos = 8;
        for (quint32 i = 0; i < count; ++i) {
            if (data.size() <= pos + 4)
                break;

            quint32 type = isLE ? getLEUint32(data, pos) : getBEUint32(data, pos);
            result.append(macAbiForCpu(type));
            pos += 20;
        }
    } else if (data.size() >= 64){
        // Windows PE: values are LE (except for a few exceptions which we will not use here).

        // MZ header first (ZM is also allowed, but rarely used)
        const quint8 firstChar = getUint8(data, 0);
        const quint8 secondChar = getUint8(data, 1);
        if ((firstChar != 'M' || secondChar != 'Z') && (firstChar != 'Z' || secondChar != 'M'))
            return result;

        // Get PE/COFF header position from MZ header:
        qint32 pePos = getLEUint32(data, 60);
        if (pePos <= 0 || data.size() < pePos + 4 + 20) // PE magic bytes plus COFF header
            return result;
        if (getUint8(data, pePos) == 'P' && getUint8(data, pePos + 1) == 'E'
            && getUint8(data, pePos + 2) == 0 && getUint8(data, pePos + 3) == 0)
            result = parseCoffHeader(data.mid(pePos + 4));
    }
    return result;
}

// --------------------------------------------------------------------------
// Abi
// --------------------------------------------------------------------------

Abi::Abi(const Architecture &a, const OS &o,
         const OSFlavor &of, const BinaryFormat &f, unsigned char w) :
    m_architecture(a), m_os(o), m_osFlavor(of), m_binaryFormat(f), m_wordWidth(w)
{
    switch (m_os) {
    case UnknownOS:
        m_osFlavor = UnknownFlavor;
        break;
    case LinuxOS:
        if (m_osFlavor < GenericLinuxFlavor || m_osFlavor > AndroidLinuxFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case BsdOS:
        if (m_osFlavor < FreeBsdFlavor || m_osFlavor > OpenBsdFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case DarwinOS:
        if (m_osFlavor < GenericDarwinFlavor || m_osFlavor > GenericDarwinFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case UnixOS:
        if (m_osFlavor < GenericUnixFlavor || m_osFlavor > SolarisUnixFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case WindowsOS:
        if (m_osFlavor < WindowsMsvc2005Flavor || m_osFlavor > WindowsCEFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case VxWorks:
        if (m_osFlavor != VxWorksFlavor)
            m_osFlavor = VxWorksFlavor;
        break;
    case QnxOS:
        if (m_osFlavor != GenericQnxFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case BareMetalOS:
        if (m_osFlavor != GenericBareMetalFlavor)
            m_osFlavor = GenericBareMetalFlavor;
        break;
    }
}

Abi::Abi(const QString &abiString) :
    m_architecture(UnknownArchitecture), m_os(UnknownOS),
    m_osFlavor(UnknownFlavor), m_binaryFormat(UnknownFormat), m_wordWidth(0)
{
    const QVector<QStringRef> abiParts = abiString.splitRef('-');
    if (abiParts.count() >= 1) {
        if (abiParts.at(0) == "unknown")
            m_architecture = UnknownArchitecture;
        else if (abiParts.at(0) == "arm")
            m_architecture = ArmArchitecture;
        else if (abiParts.at(0) == "aarch64")
            m_architecture = ArmArchitecture;
        else if (abiParts.at(0) == "avr")
            m_architecture = AvrArchitecture;
        else if (abiParts.at(0) == "x86")
            m_architecture = X86Architecture;
        else if (abiParts.at(0) == "mips")
            m_architecture = MipsArchitecture;
        else if (abiParts.at(0) == "ppc")
            m_architecture = PowerPCArchitecture;
        else if (abiParts.at(0) == "itanium")
            m_architecture = ItaniumArchitecture;
        else if (abiParts.at(0) == "sh")
            m_architecture = ShArchitecture;
        else
            return;
    }

    if (abiParts.count() >= 2) {
        if (abiParts.at(1) == "unknown")
            m_os = UnknownOS;
        else if (abiParts.at(1) == "linux")
            m_os = LinuxOS;
        else if (abiParts.at(1) == "bsd")
            m_os = BsdOS;
        else if (abiParts.at(1) == "darwin"
                || abiParts.at(1) == "macos")
            m_os = DarwinOS;
        else if (abiParts.at(1) == "unix")
            m_os = UnixOS;
        else if (abiParts.at(1) == "windows")
            m_os = WindowsOS;
        else if (abiParts.at(1) == "vxworks")
            m_os = VxWorks;
        else if (abiParts.at(1) == "qnx")
            m_os = QnxOS;
        else
            return;
    }

    if (abiParts.count() >= 3) {
        if (abiParts.at(2) == "unknown")
            m_osFlavor = UnknownFlavor;
        else if (abiParts.at(2) == "generic" && m_os == LinuxOS)
            m_osFlavor = GenericLinuxFlavor;
        else if (abiParts.at(2) == "android" && m_os == LinuxOS)
            m_osFlavor = AndroidLinuxFlavor;
        else if (abiParts.at(2) == "generic" && m_os == QnxOS)
            m_osFlavor = GenericQnxFlavor;
        else if (abiParts.at(2) == "freebsd" && m_os == BsdOS)
            m_osFlavor = FreeBsdFlavor;
        else if (abiParts.at(2) == "netbsd" && m_os == BsdOS)
            m_osFlavor = NetBsdFlavor;
        else if (abiParts.at(2) == "openbsd" && m_os == BsdOS)
            m_osFlavor = OpenBsdFlavor;
        else if (abiParts.at(2) == "generic" && m_os == DarwinOS)
            m_osFlavor = GenericDarwinFlavor;
        else if (abiParts.at(2) == "generic" && m_os == UnixOS)
            m_osFlavor = GenericUnixFlavor;
        else if (abiParts.at(2) == "solaris" && m_os == UnixOS)
            m_osFlavor = SolarisUnixFlavor;
        else if (abiParts.at(2) == "msvc2005" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2005Flavor;
        else if (abiParts.at(2) == "msvc2008" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2008Flavor;
        else if (abiParts.at(2) == "msvc2010" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2010Flavor;
        else if (abiParts.at(2) == "msvc2012" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2012Flavor;
        else if (abiParts.at(2) == "msvc2013" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2013Flavor;
        else if (abiParts.at(2) == "msvc2015" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2015Flavor;
        else if (abiParts.at(2) == "msvc2017" && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2017Flavor;
        else if (abiParts.at(2) == "msys" && m_os == WindowsOS)
            m_osFlavor = WindowsMSysFlavor;
        else if (abiParts.at(2) == "ce" && m_os == WindowsOS)
            m_osFlavor = WindowsCEFlavor;
        else if (abiParts.at(2) == "vxworks" && m_os == VxWorks)
            m_osFlavor = VxWorksFlavor;
        else
            return;
    }

    if (abiParts.count() >= 4) {
        if (abiParts.at(3) == "unknown")
            m_binaryFormat = UnknownFormat;
        else if (abiParts.at(3) == "elf")
            m_binaryFormat = ElfFormat;
        else if (abiParts.at(3) == "pe")
            m_binaryFormat = PEFormat;
        else if (abiParts.at(3) == "mach_o")
            m_binaryFormat = MachOFormat;
        else if (abiParts.at(3) == "qml_rt")
            m_binaryFormat = RuntimeQmlFormat;
        else
            return;
    }

    if (abiParts.count() >= 5) {
        const QStringRef &bits = abiParts.at(4);
        if (!bits.endsWith("bit"))
            return;

        bool ok = false;
        const QStringRef number =
            bits.string()->midRef(bits.position(), bits.count() - 3);
        const int bitCount = number.toInt(&ok);
        if (!ok)
            return;
        if (bitCount != 8 && bitCount != 16 && bitCount != 32 && bitCount != 64)
            return;
        m_wordWidth = bitCount;
    }
}

Abi Abi::abiFromTargetTriplet(const QString &triple)
{
    const QString machine = triple.toLower();
    if (machine.isEmpty())
        return Abi();

    const QVector<QStringRef> parts = machine.splitRef(QRegExp("[ /-]"));

    Architecture arch = UnknownArchitecture;
    OS os = UnknownOS;
    OSFlavor flavor = UnknownFlavor;
    BinaryFormat format = UnknownFormat;
    unsigned char width = 0;
    int unknownCount = 0;

    for (const QStringRef &p : parts) {
        if (p == "unknown" || p == "pc" || p == "none"
                || p == "gnu" || p == "uclibc"
                || p == "86_64" || p == "redhat"
                || p == "gnueabi" || p == "w64") {
            continue;
        } else if (p == "i386" || p == "i486" || p == "i586"
                   || p == "i686" || p == "x86") {
            arch = X86Architecture;
            width = 32;
        } else if (p.startsWith("arm")) {
            arch = ArmArchitecture;
            width = p.contains("64") ? 64 : 32;
        } else if (p.startsWith("aarch64")) {
            arch = ArmArchitecture;
            width = 64;
        } else if (p == "avr") {
            arch = AvrArchitecture;
            os = BareMetalOS;
            flavor = GenericBareMetalFlavor;
            format = ElfFormat;
            width = 16;
        } else if (p.startsWith("mips")) {
            arch = MipsArchitecture;
            width = p.contains("64") ? 64 : 32;
        } else if (p == "x86_64" || p == "amd64") {
            arch = X86Architecture;
            width = 64;
        } else if (p == "powerpc64") {
            arch = PowerPCArchitecture;
            width = 64;
        } else if (p == "powerpc") {
            arch = PowerPCArchitecture;
            width = 32;
        } else if (p == "linux" || p == "linux6e") {
            os = LinuxOS;
            if (flavor == UnknownFlavor)
                flavor = GenericLinuxFlavor;
            format = ElfFormat;
        } else if (p == "android") {
            flavor = AndroidLinuxFlavor;
        } else if (p == "androideabi") {
            flavor = AndroidLinuxFlavor;
        } else if (p.startsWith("freebsd")) {
            os = BsdOS;
            if (flavor == UnknownFlavor)
                flavor = FreeBsdFlavor;
            format = ElfFormat;
        } else if (p.startsWith("openbsd")) {
            os = BsdOS;
            if (flavor == UnknownFlavor)
                flavor = OpenBsdFlavor;
            format = ElfFormat;
        } else if (p == "mingw32" || p == "win32"
                   || p == "mingw32msvc" || p == "msys"
                   || p == "cygwin" || p == "windows") {
            arch = X86Architecture;
            os = WindowsOS;
            flavor = WindowsMSysFlavor;
            format = PEFormat;
        } else if (p == "apple") {
            os = DarwinOS;
            flavor = GenericDarwinFlavor;
            format = MachOFormat;
        } else if (p == "darwin10") {
            width = 64;
        } else if (p == "darwin9") {
            width = 32;
        } else if (p == "gnueabi") {
            format = ElfFormat;
        } else if (p == "wrs") {
            continue;
        } else if (p == "vxworks") {
            os = VxWorks;
            flavor = VxWorksFlavor;
            format = ElfFormat;
        } else if (p.startsWith("qnx")) {
            os = QnxOS;
            flavor = GenericQnxFlavor;
            format = ElfFormat;
        } else {
            ++unknownCount;
        }
    }

    return Abi(arch, os, flavor, format, width);
}

QString Abi::toString() const
{
    const QStringList dn = {toString(m_architecture), toString(m_os), toString(m_osFlavor),
                            toString(m_binaryFormat), toString(m_wordWidth)};
    return dn.join('-');
}

bool Abi::operator != (const Abi &other) const
{
    return !operator ==(other);
}

bool Abi::operator == (const Abi &other) const
{
    return m_architecture == other.m_architecture
            && m_os == other.m_os
            && m_osFlavor == other.m_osFlavor
            && m_binaryFormat == other.m_binaryFormat
            && m_wordWidth == other.m_wordWidth;
}

bool Abi::isCompatibleWith(const Abi &other) const
{
    // Generic match: If stuff is identical or the other side is unknown, then this is a match.
    bool isCompat = (architecture() == other.architecture() || other.architecture() == UnknownArchitecture)
                     && (os() == other.os() || other.os() == UnknownOS)
                     && (osFlavor() == other.osFlavor() || other.osFlavor() == UnknownFlavor)
                     && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == UnknownFormat)
                     && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0);

    // *-linux-generic-* is compatible with *-linux-* (both ways): This is for the benefit of
    // people building Qt themselves using e.g. a meego toolchain.
    //
    // We leave it to the specific targets to filter out the tool chains that do not
    // work for them.
    if (!isCompat && (architecture() == other.architecture() || other.architecture() == UnknownArchitecture)
                  && ((os() == other.os()) && (os() == LinuxOS))
                  && (osFlavor() == GenericLinuxFlavor || other.osFlavor() == GenericLinuxFlavor)
                  && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == UnknownFormat)
                  && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0)) {
        isCompat = true;
    }

    // Make Android matching more strict than the generic Linux matches so far:
    if (isCompat && (osFlavor() == AndroidLinuxFlavor || other.osFlavor() == AndroidLinuxFlavor))
        isCompat = (architecture() == other.architecture()) &&  (osFlavor() == other.osFlavor());

    // MSVC2017 is compatible with MSVC2015
    if (!isCompat
            && ((osFlavor() == WindowsMsvc2015Flavor && other.osFlavor() == WindowsMsvc2017Flavor)
                || (osFlavor() == WindowsMsvc2017Flavor && other.osFlavor() == WindowsMsvc2015Flavor))) {
        isCompat = true;
    }
    return isCompat;
}

bool Abi::isValid() const
{
    return m_architecture != UnknownArchitecture
            && m_os != UnknownOS
            && m_osFlavor != UnknownFlavor
            && m_binaryFormat != UnknownFormat
            && m_wordWidth != 0;
}

bool Abi::isNull() const
{
    return m_architecture == UnknownArchitecture
            && m_os == UnknownOS
            && m_osFlavor == UnknownFlavor
            && m_binaryFormat == UnknownFormat
            && m_wordWidth == 0;
}

QString Abi::toString(const Architecture &a)
{
    switch (a) {
    case ArmArchitecture:
        return QLatin1String("arm");
    case AvrArchitecture:
        return QLatin1String("avr");
    case X86Architecture:
        return QLatin1String("x86");
    case MipsArchitecture:
        return QLatin1String("mips");
    case PowerPCArchitecture:
        return QLatin1String("ppc");
    case ItaniumArchitecture:
        return QLatin1String("itanium");
    case ShArchitecture:
        return QLatin1String("sh");
    case UnknownArchitecture: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(const OS &o)
{
    switch (o) {
    case LinuxOS:
        return QLatin1String("linux");
    case BsdOS:
        return QLatin1String("bsd");
    case DarwinOS:
        return QLatin1String("darwin");
    case UnixOS:
        return QLatin1String("unix");
    case WindowsOS:
        return QLatin1String("windows");
    case VxWorks:
        return QLatin1String("vxworks");
    case QnxOS:
        return QLatin1String("qnx");
    case BareMetalOS:
        return QLatin1String("baremetal");
    case UnknownOS: // fall through!
    default:
        return QLatin1String("unknown");
    };
}

QString Abi::toString(const OSFlavor &of)
{
    switch (of) {
    case GenericLinuxFlavor:
        return QLatin1String("generic");
    case AndroidLinuxFlavor:
        return QLatin1String("android");
    case FreeBsdFlavor:
        return QLatin1String("freebsd");
    case NetBsdFlavor:
        return QLatin1String("netbsd");
    case OpenBsdFlavor:
        return QLatin1String("openbsd");
    case GenericDarwinFlavor:
        return QLatin1String("generic");
    case GenericUnixFlavor:
        return QLatin1String("generic");
    case SolarisUnixFlavor:
        return QLatin1String("solaris");
    case WindowsMsvc2005Flavor:
        return QLatin1String("msvc2005");
    case WindowsMsvc2008Flavor:
        return QLatin1String("msvc2008");
    case WindowsMsvc2010Flavor:
        return QLatin1String("msvc2010");
    case WindowsMsvc2012Flavor:
        return QLatin1String("msvc2012");
    case WindowsMsvc2013Flavor:
        return QLatin1String("msvc2013");
    case WindowsMsvc2015Flavor:
        return QLatin1String("msvc2015");
    case WindowsMsvc2017Flavor:
        return QLatin1String("msvc2017");
    case WindowsMSysFlavor:
        return QLatin1String("msys");
    case WindowsCEFlavor:
        return QLatin1String("ce");
    case VxWorksFlavor:
        return QLatin1String("vxworks");
    case GenericQnxFlavor:
    case GenericBareMetalFlavor:
        return QLatin1String("generic");
    case UnknownFlavor: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(const BinaryFormat &bf)
{
    switch (bf) {
    case ElfFormat:
        return QLatin1String("elf");
    case PEFormat:
        return QLatin1String("pe");
    case MachOFormat:
        return QLatin1String("mach_o");
    case RuntimeQmlFormat:
        return QLatin1String("qml_rt");
    case UnknownFormat: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(int w)
{
    if (w == 0)
        return QLatin1String("unknown");
    return QString::fromLatin1("%1bit").arg(w);
}

QList<Abi::OSFlavor> Abi::flavorsForOs(const Abi::OS &o)
{
    switch (o) {
    case BsdOS:
        return {FreeBsdFlavor, OpenBsdFlavor, NetBsdFlavor, UnknownFlavor};
    case LinuxOS:
        return {GenericLinuxFlavor, AndroidLinuxFlavor, UnknownFlavor};
    case DarwinOS:
        return {GenericDarwinFlavor, UnknownFlavor};
    case UnixOS:
        return {GenericUnixFlavor, SolarisUnixFlavor, UnknownFlavor};
    case WindowsOS:
        return {WindowsMsvc2005Flavor, WindowsMsvc2008Flavor, WindowsMsvc2010Flavor,
                WindowsMsvc2012Flavor, WindowsMsvc2013Flavor, WindowsMsvc2015Flavor,
                WindowsMsvc2017Flavor , WindowsMSysFlavor, WindowsCEFlavor, UnknownFlavor};
    case VxWorks:
        return {VxWorksFlavor, UnknownFlavor};
    case QnxOS:
        return {GenericQnxFlavor, UnknownFlavor};
    case BareMetalOS:
        return {GenericBareMetalFlavor, UnknownFlavor};
    case UnknownOS:
        return {UnknownFlavor};
    }
    return QList<OSFlavor>();
}

Abi::OSFlavor Abi::flavorForMsvcVersion(int version)
{
    if (version >= 1910)
        return WindowsMsvc2017Flavor;
    switch (version) {
    case 1900:
        return WindowsMsvc2015Flavor;
    case 1800:
        return WindowsMsvc2013Flavor;
    case 1700:
        return WindowsMsvc2012Flavor;
    case 1600:
        return WindowsMsvc2010Flavor;
    case 1500:
        return WindowsMsvc2008Flavor;
    case 1400:
        return WindowsMsvc2005Flavor;
    default:
        return WindowsMSysFlavor;
    }
}

Abi Abi::hostAbi()
{
    Architecture arch = QTC_CPU; // define set by qmake
    OS os = UnknownOS;
    OSFlavor subos = UnknownFlavor;
    BinaryFormat format = UnknownFormat;

#if defined (Q_OS_WIN)
    os = WindowsOS;
#ifdef _MSC_VER
    subos = flavorForMsvcVersion(_MSC_VER);
#elif defined (Q_CC_MINGW)
    subos = WindowsMSysFlavor;
#endif
    format = PEFormat;
#elif defined (Q_OS_LINUX)
    os = LinuxOS;
    subos = GenericLinuxFlavor;
    format = ElfFormat;
#elif defined (Q_OS_DARWIN)
    os = DarwinOS;
    subos = GenericDarwinFlavor;
    format = MachOFormat;
#elif defined (Q_OS_BSD4)
    os = BsdOS;
# if defined (Q_OS_FREEBSD)
    subos = FreeBsdFlavor;
# elif defined (Q_OS_NETBSD)
    subos = NetBsdFlavor;
# elif defined (Q_OS_OPENBSD)
    subos = OpenBsdFlavor;
# endif
    format = ElfFormat;
#endif

    const Abi result(arch, os, subos, format, QSysInfo::WordSize);
    if (!result.isValid())
        qWarning("Unable to completely determine the host ABI (%s).",
                 qPrintable(result.toString()));
    return result;
}

QList<Abi> Abi::abisOfBinary(const Utils::FileName &path)
{
    QList<Abi> tmp;
    if (path.isEmpty())
        return tmp;

    QFile f(path.toString());
    if (!f.exists())
        return tmp;

    if (!f.open(QFile::ReadOnly))
        return tmp;

    QByteArray data = f.read(1024);
    if (data.size() >= 67
            && getUint8(data, 0) == '!' && getUint8(data, 1) == '<' && getUint8(data, 2) == 'a'
            && getUint8(data, 3) == 'r' && getUint8(data, 4) == 'c' && getUint8(data, 5) == 'h'
            && getUint8(data, 6) == '>' && getUint8(data, 7) == 0x0a) {
        // We got an ar file: possibly a static lib for ELF, PE or Mach-O

        data = data.mid(8); // Cut of ar file magic
        quint64 offset = 8;

        while (!data.isEmpty()) {
            if ((getUint8(data, 58) != 0x60 || getUint8(data, 59) != 0x0a)) {
                qWarning() << path.toString() << ": Thought it was an ar-file, but it is not!";
                break;
            }

            const QString fileName = QString::fromLocal8Bit(data.mid(0, 16));
            quint64 fileNameOffset = 0;
            if (fileName.startsWith("#1/"))
                fileNameOffset = fileName.midRef(3).toInt();
            const QString fileLength = QString::fromLatin1(data.mid(48, 10));

            int toSkip = 60 + fileNameOffset;
            offset += fileLength.toInt() + 60 /* header */;

            tmp.append(abiOf(data.mid(toSkip)));
            if (tmp.isEmpty() && fileName == "/0              ")
                tmp = parseCoffHeader(data.mid(toSkip, 20)); // This might be windws...

            if (!tmp.isEmpty() && tmp.at(0).binaryFormat() != MachOFormat)
                break;

            offset += (offset % 2); // ar is 2 byte aligned
            f.seek(offset);
            data = f.read(1024);
        }
    } else {
        tmp = abiOf(data);
    }
    f.close();

    // Remove duplicates:
    QList<Abi> result;
    foreach (const Abi &a, tmp) {
        if (!result.contains(a))
            result.append(a);
    }

    return result;
}

} // namespace ProjectExplorer

// Unit tests:
#ifdef WITH_TESTS
#   include <QTest>
#   include <QFileInfo>

#   include "projectexplorer.h"

void ProjectExplorer::ProjectExplorerPlugin::testAbiOfBinary_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QStringList>("abis");

    QTest::newRow("no file")
            << QString()
            << (QStringList());
    QTest::newRow("non existing file")
            << QString::fromLatin1("/does/not/exist")
            << (QStringList());

    // Set up prefix for test data now that we can be sure to have some tests to run:
    QString prefix = QString::fromLocal8Bit(qgetenv("QTC_TEST_EXTRADATALOCATION"));
    if (prefix.isEmpty())
        return;
    prefix += "/projectexplorer/abi";

    QFileInfo fi(prefix);
    if (!fi.exists() || !fi.isDir())
        return;
    prefix = fi.absoluteFilePath();

    QTest::newRow("text file")
            << QString::fromLatin1("%1/broken/text.txt").arg(prefix)
            << (QStringList());

    QTest::newRow("static QtCore: win msvc2008")
            << QString::fromLatin1("%1/static/win-msvc2008-release.lib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-32bit"));
    QTest::newRow("static QtCore: win msvc2008 II")
            << QString::fromLatin1("%1/static/win-msvc2008-release2.lib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-64bit"));
    QTest::newRow("static QtCore: win msvc2008 (debug)")
            << QString::fromLatin1("%1/static/win-msvc2008-debug.lib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-32bit"));
    QTest::newRow("static QtCore: win mingw")
            << QString::fromLatin1("%1/static/win-mingw.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-unknown-pe-32bit"));
    QTest::newRow("static QtCore: mac (debug)")
            << QString::fromLatin1("%1/static/mac-32bit-debug.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-darwin-generic-mach_o-32bit"));
    QTest::newRow("static QtCore: linux 32bit")
            << QString::fromLatin1("%1/static/linux-32bit-release.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-linux-generic-elf-32bit"));
    QTest::newRow("static QtCore: linux 64bit")
            << QString::fromLatin1("%1/static/linux-64bit-release.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-linux-generic-elf-64bit"));

    QTest::newRow("static stdc++: mac fat")
            << QString::fromLatin1("%1/static/mac-fat.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-darwin-generic-mach_o-32bit")
                              << QString::fromLatin1("ppc-darwin-generic-mach_o-32bit")
                              << QString::fromLatin1("x86-darwin-generic-mach_o-64bit"));

    QTest::newRow("executable: win msvc2013 64bit")
            << QString::fromLatin1("%1/executables/x86-windows-mvsc2013-pe-64bit.exe").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2013-pe-64bit"));
    QTest::newRow("executable: win msvc2013 32bit")
            << QString::fromLatin1("%1/executables/x86-windows-mvsc2013-pe-32bit.exe").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2013-pe-32bit"));
    QTest::newRow("dynamic: win msvc2013 64bit")
            << QString::fromLatin1("%1/dynamic/x86-windows-mvsc2013-pe-64bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2013-pe-64bit"));
    QTest::newRow("dynamic: win msvc2013 32bit")
            << QString::fromLatin1("%1/dynamic/x86-windows-mvsc2013-pe-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2013-pe-32bit"));
    QTest::newRow("dynamic QtCore: win msvc2010 64bit")
            << QString::fromLatin1("%1/dynamic/win-msvc2010-64bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2010-pe-64bit"));
    QTest::newRow("dynamic QtCore: win msvc2008 32bit")
            << QString::fromLatin1("%1/dynamic/win-msvc2008-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2008-pe-32bit"));
    QTest::newRow("dynamic QtCore: win msvc2005 32bit")
            << QString::fromLatin1("%1/dynamic/win-msvc2005-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msvc2005-pe-32bit"));
    QTest::newRow("dynamic QtCore: win msys 32bit")
            << QString::fromLatin1("%1/dynamic/win-mingw-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msys-pe-32bit"));
    QTest::newRow("dynamic QtCore: win mingw 64bit")
            << QString::fromLatin1("%1/dynamic/win-mingw-64bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msys-pe-64bit"));
    QTest::newRow("dynamic QtCore: wince mips msvc2005 32bit")
            << QString::fromLatin1("%1/dynamic/wince-32bit.dll").arg(prefix)
            << (QStringList() << QString::fromLatin1("mips-windows-msvc2005-pe-32bit"));
    QTest::newRow("dynamic QtCore: wince arm msvc2008 32bit")
            << QString::fromLatin1("%1/dynamic/arm-win-ce-pe-32bit").arg(prefix)
            << (QStringList() << QString::fromLatin1("arm-windows-msvc2008-pe-32bit"));

    QTest::newRow("dynamic stdc++: mac fat")
            << QString::fromLatin1("%1/dynamic/mac-fat.dylib").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-darwin-generic-mach_o-32bit")
                              << QString::fromLatin1("ppc-darwin-generic-mach_o-32bit")
                              << QString::fromLatin1("x86-darwin-generic-mach_o-64bit"));
    QTest::newRow("dynamic QtCore: arm linux 32bit")
            << QString::fromLatin1("%1/dynamic/arm-linux.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("arm-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: arm linux 32bit, using ARM as OSABI")
            << QString::fromLatin1("%1/dynamic/arm-linux2.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("arm-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: arm linux 32bit (angstrom)")
            << QString::fromLatin1("%1/dynamic/arm-angstrom-linux.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("arm-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: sh4 linux 32bit")
            << QString::fromLatin1("%1/dynamic/sh4-linux.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("sh-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: mips linux 32bit")
            << QString::fromLatin1("%1/dynamic/mips-linux.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("mips-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: projectexplorer/abi/static/win-msvc2010-32bit.libppc be linux 32bit")
            << QString::fromLatin1("%1/dynamic/ppcbe-linux-32bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("ppc-linux-generic-elf-32bit"));
    QTest::newRow("dynamic QtCore: x86 freebsd 64bit")
            << QString::fromLatin1("%1/dynamic/freebsd-elf-64bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-bsd-freebsd-elf-64bit"));
    QTest::newRow("dynamic QtCore: x86 freebsd 64bit")
            << QString::fromLatin1("%1/dynamic/freebsd-elf-64bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-bsd-freebsd-elf-64bit"));
    QTest::newRow("dynamic QtCore: x86 freebsd 32bit")
            << QString::fromLatin1("%1/dynamic/freebsd-elf-32bit.so").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-bsd-freebsd-elf-32bit"));

    QTest::newRow("executable: x86 win 32bit cygwin executable")
            << QString::fromLatin1("%1/executable/cygwin-32bit.exe").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msys-pe-32bit"));
    QTest::newRow("executable: x86 win 32bit mingw executable")
            << QString::fromLatin1("%1/executable/mingw-32bit.exe").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-windows-msys-pe-32bit"));
}

void ProjectExplorer::ProjectExplorerPlugin::testAbiOfBinary()
{
    QFETCH(QString, file);
    QFETCH(QStringList, abis);

    QList<Abi> result = Abi::abisOfBinary(Utils::FileName::fromString(file));
    QCOMPARE(result.count(), abis.count());
    for (int i = 0; i < abis.count(); ++i)
        QCOMPARE(result.at(i).toString(), abis.at(i));
}

void ProjectExplorer::ProjectExplorerPlugin::testFlavorForOs()
{
    QList<QList<Abi::OSFlavor> > flavorLists;
    for (int i = 0; i != static_cast<int>(Abi::UnknownOS); ++i)
        flavorLists.append(Abi::flavorsForOs(static_cast<Abi::OS>(i)));

    int foundCounter = 0;
    for (int i = 0; i != Abi::UnknownFlavor; ++i) {
        foundCounter = 0;
        // make sure i is in exactly on of the flavor lists!
        foreach (const QList<Abi::OSFlavor> &l, flavorLists) {
            QVERIFY(l.contains(Abi::UnknownFlavor));
            if (l.contains(static_cast<Abi::OSFlavor>(i)))
                ++foundCounter;
        }
        QCOMPARE(foundCounter, 1);
    }
}

void ProjectExplorer::ProjectExplorerPlugin::testAbiFromTargetTriplet_data()
{
    QTest::addColumn<int>("architecture");
    QTest::addColumn<int>("os");
    QTest::addColumn<int>("osFlavor");
    QTest::addColumn<int>("binaryFormat");
    QTest::addColumn<int>("wordWidth");

    QTest::newRow("x86_64-apple-darwin") << int(Abi::X86Architecture)
                                         << int(Abi::DarwinOS) << int(Abi::GenericDarwinFlavor)
                                         << int(Abi::MachOFormat) << 64;

    QTest::newRow("x86_64-apple-darwin12.5.0") << int(Abi::X86Architecture)
                                               << int(Abi::DarwinOS) << int(Abi::GenericDarwinFlavor)
                                               << int(Abi::MachOFormat) << 64;

    QTest::newRow("x86_64-linux-gnu") << int(Abi::X86Architecture)
                                      << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                      << int(Abi::ElfFormat) << 64;

    QTest::newRow("x86_64-pc-mingw32msvc") << int(Abi::X86Architecture)
                                           << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                           << int(Abi::PEFormat) << 64;

    QTest::newRow("i586-pc-mingw32msvc") << int(Abi::X86Architecture)
                                         << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                         << int(Abi::PEFormat) << 32;

    QTest::newRow("i686-linux-gnu") << int(Abi::X86Architecture)
                                    << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                    << int(Abi::ElfFormat) << 32;

    QTest::newRow("i686-linux-android") << int(Abi::X86Architecture)
                                        << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                        << int(Abi::ElfFormat) << 32;

    QTest::newRow("i686-pc-linux-android") << int(Abi::X86Architecture)
                                           << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                           << int(Abi::ElfFormat) << 32;

    QTest::newRow("i686-pc-mingw32") << int(Abi::X86Architecture)
                                     << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                     << int(Abi::PEFormat) << 32;

    QTest::newRow("i686-w64-mingw32") << int(Abi::X86Architecture)
                                      << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                      << int(Abi::PEFormat) << 32;

    QTest::newRow("x86_64-pc-msys") << int(Abi::X86Architecture)
                                    << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                    << int(Abi::PEFormat) << 64;

    QTest::newRow("x86_64-pc-cygwin") << int(Abi::X86Architecture)
                                      << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                      << int(Abi::PEFormat) << 64;

    QTest::newRow("x86-pc-windows-msvc") << int(Abi::X86Architecture)
                                         << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                                         << int(Abi::PEFormat) << 32;

    QTest::newRow("mingw32") << int(Abi::X86Architecture)
                             << int(Abi::WindowsOS) << int(Abi::WindowsMSysFlavor)
                             << int(Abi::PEFormat) << 0;

    QTest::newRow("arm-linux-android") << int(Abi::ArmArchitecture)
                                       << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                       << int(Abi::ElfFormat) << 32;

    QTest::newRow("arm-linux-androideabi") << int(Abi::ArmArchitecture)
                                           << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                           << int(Abi::ElfFormat) << 32;

    QTest::newRow("arm-none-linux-gnueabi") << int(Abi::ArmArchitecture)
                                            << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                            << int(Abi::ElfFormat) << 32;

    QTest::newRow("mipsel-linux-android") << int(Abi::MipsArchitecture)
                                          << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                          << int(Abi::ElfFormat) << 32;

    QTest::newRow("mipsel-unknown-linux-android") << int(Abi::MipsArchitecture)
                                                  << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                                  << int(Abi::ElfFormat) << 32;

    QTest::newRow("mips-linux-gnu") << int(Abi::MipsArchitecture)
                                    << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                    << int(Abi::ElfFormat) << 32;

    QTest::newRow("mips64el-linux-android") << int(Abi::MipsArchitecture)
                                            << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                            << int(Abi::ElfFormat) << 64;

    QTest::newRow("mips64el-unknown-linux-android") << int(Abi::MipsArchitecture)
                                                    << int(Abi::LinuxOS) << int(Abi::AndroidLinuxFlavor)
                                                    << int(Abi::ElfFormat) << 64;

    QTest::newRow("mips64-linux-octeon-gnu") << int(Abi::MipsArchitecture)
                                             << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                             << int(Abi::ElfFormat) << 64;

    QTest::newRow("mips64el-linux-gnuabi") << int(Abi::MipsArchitecture)
                                    << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                    << int(Abi::ElfFormat) << 64;

    QTest::newRow("arm-wrs-vxworks") << int(Abi::ArmArchitecture)
                                     << int(Abi::VxWorks) << int(Abi::VxWorksFlavor)
                                     << int(Abi::ElfFormat) << 32;

    QTest::newRow("x86_64-unknown-openbsd") << int(Abi::X86Architecture)
                                            << int(Abi::BsdOS) << int(Abi::OpenBsdFlavor)
                                            << int(Abi::ElfFormat) << 64;

    QTest::newRow("aarch64-unknown-linux-gnu") << int(Abi::ArmArchitecture)
                                               << int(Abi::LinuxOS) << int(Abi::GenericLinuxFlavor)
                                               << int(Abi::ElfFormat) << 64;

    // Yes, that's the entire triplet
    QTest::newRow("avr") << int(Abi::AvrArchitecture)
                         << int(Abi::BareMetalOS) << int(Abi::GenericBareMetalFlavor)
                         << int(Abi::ElfFormat) << 16;
}

void ProjectExplorer::ProjectExplorerPlugin::testAbiFromTargetTriplet()
{
    QFETCH(int, architecture);
    QFETCH(int, os);
    QFETCH(int, osFlavor);
    QFETCH(int, binaryFormat);
    QFETCH(int, wordWidth);

    const Abi expectedAbi = Abi(Abi::Architecture(architecture),
                                Abi::OS(os), Abi::OSFlavor(osFlavor),
                                Abi::BinaryFormat(binaryFormat),
                                static_cast<unsigned char>(wordWidth));

    QCOMPARE(Abi::abiFromTargetTriplet(QLatin1String(QTest::currentDataTag())), expectedAbi);
}

#endif
