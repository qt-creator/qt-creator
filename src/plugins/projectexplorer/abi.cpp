/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "abi.h"

#include <utils/fileutils.h>

#include <QDebug>
#include <QtEndian>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QSysInfo>

/*!
    \class ProjectExplorer::Abi

    \brief Represents the Application Binary Interface (ABI) of a target platform.

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
        return Abi(Abi::X86Architecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    case 0x01000000 +  7: // CPU_TYPE_X86_64
        return Abi(Abi::X86Architecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 64);
    case 18: // CPU_TYPE_POWERPC
        return Abi(Abi::PowerPCArchitecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    case 0x01000000 + 18: // CPU_TYPE_POWERPC64
        return Abi(Abi::PowerPCArchitecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
    case 12: // CPU_TYPE_ARM
        return Abi(Abi::ArmArchitecture, Abi::MacOS, Abi::GenericMacFlavor, Abi::MachOFormat, 32);
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
        case 2: // NetBSD:
            os = Abi::BsdOS;
            flavor = Abi::NetBsdFlavor;
            break;
        case 3: // Linux:
        case 0: // no extra info available: Default to Linux:
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
        case 12: // OpenBSD:
            os = Abi::BsdOS;
            flavor = Abi::OpenBsdFlavor;
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
            ;;
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
    case ProjectExplorer::Abi::UnknownOS:
        m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::LinuxOS:
        if (m_osFlavor < GenericLinuxFlavor || m_osFlavor > MaemoLinuxFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::BsdOS:
        m_osFlavor = FreeBsdFlavor;
        break;
    case ProjectExplorer::Abi::MacOS:
        if (m_osFlavor < GenericMacFlavor || m_osFlavor > GenericMacFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::UnixOS:
        if (m_osFlavor < GenericUnixFlavor || m_osFlavor > GenericUnixFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::WindowsOS:
        if (m_osFlavor < WindowsMsvc2005Flavor || m_osFlavor > WindowsCEFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    }
}

Abi::Abi(const QString &abiString) :
    m_architecture(UnknownArchitecture), m_os(UnknownOS),
    m_osFlavor(UnknownFlavor), m_binaryFormat(UnknownFormat), m_wordWidth(0)
{
    QStringList abiParts = abiString.split(QLatin1Char('-'));
    if (abiParts.count() >= 1) {
        if (abiParts.at(0) == QLatin1String("unknown"))
            m_architecture = UnknownArchitecture;
        else if (abiParts.at(0) == QLatin1String("arm"))
            m_architecture = ArmArchitecture;
        else if (abiParts.at(0) == QLatin1String("x86"))
            m_architecture = X86Architecture;
        else if (abiParts.at(0) == QLatin1String("mips"))
            m_architecture = MipsArchitecture;
        else if (abiParts.at(0) == QLatin1String("ppc"))
            m_architecture = PowerPCArchitecture;
        else if (abiParts.at(0) == QLatin1String("itanium"))
            m_architecture = ItaniumArchitecture;
        else if (abiParts.at(0) == QLatin1String("sh"))
            m_architecture = ShArchitecture;
        else
            return;
    }

    if (abiParts.count() >= 2) {
        if (abiParts.at(1) == QLatin1String("unknown"))
            m_os = UnknownOS;
        else if (abiParts.at(1) == QLatin1String("linux"))
            m_os = LinuxOS;
        else if (abiParts.at(1) == QLatin1String("bsd"))
            m_os = BsdOS;
        else if (abiParts.at(1) == QLatin1String("macos"))
            m_os = MacOS;
        else if (abiParts.at(1) == QLatin1String("unix"))
            m_os = UnixOS;
        else if (abiParts.at(1) == QLatin1String("windows"))
            m_os = WindowsOS;
        else

            return;
    }

    if (abiParts.count() >= 3) {
        if (abiParts.at(2) == QLatin1String("unknown"))
            m_osFlavor = UnknownFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == LinuxOS)
            m_osFlavor = GenericLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("android") && m_os == LinuxOS)
            m_osFlavor = AndroidLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("freebsd") && m_os == BsdOS)
            m_osFlavor = FreeBsdFlavor;
        else if (abiParts.at(2) == QLatin1String("netbsd") && m_os == BsdOS)
            m_osFlavor = NetBsdFlavor;
        else if (abiParts.at(2) == QLatin1String("openbsd") && m_os == BsdOS)
            m_osFlavor = OpenBsdFlavor;
        else if (abiParts.at(2) == QLatin1String("maemo") && m_os == LinuxOS)
            m_osFlavor = MaemoLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("harmattan") && m_os == LinuxOS)
            m_osFlavor = HarmattanLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == MacOS)
            m_osFlavor = GenericMacFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == UnixOS)
            m_osFlavor = GenericUnixFlavor;
        else if (abiParts.at(2) == QLatin1String("solaris") && m_os == UnixOS)
            m_osFlavor = SolarisUnixFlavor;
        else if (abiParts.at(2) == QLatin1String("msvc2005") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2005Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2008") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2008Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2010") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2010Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2012") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2012Flavor;
        else if (abiParts.at(2) == QLatin1String("msys") && m_os == WindowsOS)
            m_osFlavor = WindowsMSysFlavor;
        else if (abiParts.at(2) == QLatin1String("ce") && m_os == WindowsOS)
            m_osFlavor = WindowsCEFlavor;
        else
            return;
    }

    if (abiParts.count() >= 4) {
        if (abiParts.at(3) == QLatin1String("unknown"))
            m_binaryFormat = UnknownFormat;
        else if (abiParts.at(3) == QLatin1String("elf"))
            m_binaryFormat = ElfFormat;
        else if (abiParts.at(3) == QLatin1String("pe"))
            m_binaryFormat = PEFormat;
        else if (abiParts.at(3) == QLatin1String("mach_o"))
            m_binaryFormat = MachOFormat;
        else if (abiParts.at(3) == QLatin1String("qml_rt"))
            m_binaryFormat = RuntimeQmlFormat;
        else
            return;
    }

    if (abiParts.count() >= 5) {
        const QString &bits = abiParts.at(4);
        if (!bits.endsWith(QLatin1String("bit")))
            return;

        bool ok = false;
        int bitCount = bits.left(bits.count() - 3).toInt(&ok);
        if (!ok)
            return;
        if (bitCount != 8 && bitCount != 16 && bitCount != 32 && bitCount != 64)
            return;
        m_wordWidth = bitCount;
    }
}

QString Abi::toString() const
{
    QStringList dn;
    dn << toString(m_architecture);
    dn << toString(m_os);
    dn << toString(m_osFlavor);
    dn << toString(m_binaryFormat);
    dn << toString(m_wordWidth);

    return dn.join(QLatin1String("-"));
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
    bool isCompat = (architecture() == other.architecture() || other.architecture() == Abi::UnknownArchitecture)
                     && (os() == other.os() || other.os() == Abi::UnknownOS)
                     && (osFlavor() == other.osFlavor() || other.osFlavor() == Abi::UnknownFlavor)
                     && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == Abi::UnknownFormat)
                     && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0);
    // *-linux-generic-* is compatible with *-linux-* (both ways): This is for the benefit of
    // people building Qt themselves using e.g. a meego toolchain.
    //
    // We leave it to the specific targets to catch filter out the tool chains that do not
    // work for them.
    if (!isCompat && (architecture() == other.architecture() || other.architecture() == Abi::UnknownArchitecture)
                  && ((os() == other.os()) && (os() == LinuxOS))
                  && (osFlavor() == GenericLinuxFlavor || other.osFlavor() == GenericLinuxFlavor)
                  && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == Abi::UnknownFormat)
                  && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0))
        isCompat = true;
    if (osFlavor() == AndroidLinuxFlavor || other.osFlavor() == AndroidLinuxFlavor)
        isCompat = (osFlavor() == other.osFlavor() && architecture() == other.architecture());
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
    case MacOS:
        return QLatin1String("macos");
    case UnixOS:
        return QLatin1String("unix");
    case WindowsOS:
        return QLatin1String("windows");
    case UnknownOS: // fall through!
    default:
        return QLatin1String("unknown");
    };
}

QString Abi::toString(const OSFlavor &of)
{
    switch (of) {
    case ProjectExplorer::Abi::GenericLinuxFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::AndroidLinuxFlavor:
        return QLatin1String("android");
    case ProjectExplorer::Abi::FreeBsdFlavor:
        return QLatin1String("freebsd");
    case ProjectExplorer::Abi::NetBsdFlavor:
        return QLatin1String("netbsd");
    case ProjectExplorer::Abi::OpenBsdFlavor:
        return QLatin1String("openbsd");
    case ProjectExplorer::Abi::MaemoLinuxFlavor:
        return QLatin1String("maemo");
    case ProjectExplorer::Abi::HarmattanLinuxFlavor:
        return QLatin1String("harmattan");
    case ProjectExplorer::Abi::GenericMacFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::GenericUnixFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::SolarisUnixFlavor:
        return QLatin1String("solaris");
    case ProjectExplorer::Abi::WindowsMsvc2005Flavor:
        return QLatin1String("msvc2005");
    case ProjectExplorer::Abi::WindowsMsvc2008Flavor:
        return QLatin1String("msvc2008");
    case ProjectExplorer::Abi::WindowsMsvc2010Flavor:
        return QLatin1String("msvc2010");
    case ProjectExplorer::Abi::WindowsMsvc2012Flavor:
        return QLatin1String("msvc2012");
    case ProjectExplorer::Abi::WindowsMSysFlavor:
        return QLatin1String("msys");
    case ProjectExplorer::Abi::WindowsCEFlavor:
        return QLatin1String("ce");
    case ProjectExplorer::Abi::UnknownFlavor: // fall through!
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
    QList<OSFlavor> result;
    switch (o) {
    case BsdOS:
        return result << FreeBsdFlavor << OpenBsdFlavor << NetBsdFlavor << UnknownFlavor;
    case LinuxOS:
        return result << GenericLinuxFlavor << HarmattanLinuxFlavor << MaemoLinuxFlavor
                      << AndroidLinuxFlavor << UnknownFlavor;
    case MacOS:
        return result << GenericMacFlavor << UnknownFlavor;
    case UnixOS:
        return result << GenericUnixFlavor << SolarisUnixFlavor << UnknownFlavor;
    case WindowsOS:
        return result << WindowsMsvc2005Flavor << WindowsMsvc2008Flavor << WindowsMsvc2010Flavor
                      << WindowsMsvc2012Flavor << WindowsMSysFlavor << WindowsCEFlavor << UnknownFlavor;
    case UnknownOS:
        return result << UnknownFlavor;
    default:
        break;
    }
    return result;
}

Abi Abi::hostAbi()
{
    Architecture arch = QTC_CPU; // define set by qmake
    OS os = UnknownOS;
    OSFlavor subos = UnknownFlavor;
    BinaryFormat format = UnknownFormat;

#if defined (Q_OS_WIN)
    os = WindowsOS;
#if _MSC_VER == 1700
    subos = WindowsMsvc2012Flavor;
#elif _MSC_VER == 1600
    subos = WindowsMsvc2010Flavor;
#elif _MSC_VER == 1500
    subos = WindowsMsvc2008Flavor;
#elif _MSC_VER == 1400
    subos = WindowsMsvc2005Flavor;
#elif defined (Q_CC_MINGW)
    subos = WindowsMSysFlavor;
#endif
    format = PEFormat;
#elif defined (Q_OS_LINUX)
    os = LinuxOS;
    subos = GenericLinuxFlavor;
    format = ElfFormat;
#elif defined (Q_OS_MAC)
    os = MacOS;
    subos = GenericMacFlavor;
    format = MachOFormat;
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
            if (fileName.startsWith(QLatin1String("#1/")))
                fileNameOffset = fileName.mid(3).toInt();
            const QString fileLength = QString::fromLatin1(data.mid(48, 10));

            int toSkip = 60 + fileNameOffset;
            offset += fileLength.toInt() + 60 /* header */;

            tmp.append(abiOf(data.mid(toSkip)));
            if (tmp.isEmpty() && fileName == QLatin1String("/0              "))
                tmp = parseCoffHeader(data.mid(toSkip, 20)); // This might be windws...

            if (!tmp.isEmpty()
                    && tmp.at(0).binaryFormat() != Abi::MachOFormat)
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
    prefix += QLatin1String("/projectexplorer/abi");

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
            << (QStringList() << QString::fromLatin1("x86-macos-generic-mach_o-32bit"));
    QTest::newRow("static QtCore: linux 32bit")
            << QString::fromLatin1("%1/static/linux-32bit-release.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-linux-generic-elf-32bit"));
    QTest::newRow("static QtCore: linux 64bit")
            << QString::fromLatin1("%1/static/linux-64bit-release.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-linux-generic-elf-64bit"));

    QTest::newRow("static stdc++: mac fat")
            << QString::fromLatin1("%1/static/mac-fat.a").arg(prefix)
            << (QStringList() << QString::fromLatin1("x86-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("ppc-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("x86-macos-generic-mach_o-64bit"));

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
            << (QStringList() << QString::fromLatin1("x86-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("ppc-macos-generic-mach_o-32bit")
                              << QString::fromLatin1("x86-macos-generic-mach_o-64bit"));
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

    QList<ProjectExplorer::Abi> result = Abi::abisOfBinary(Utils::FileName::fromString(file));
    QCOMPARE(result.count(), abis.count());
    for (int i = 0; i < abis.count(); ++i)
        QCOMPARE(result.at(i).toString(), abis.at(i));
}

void ProjectExplorer::ProjectExplorerPlugin::testFlavorForOs()
{
    QList<QList<ProjectExplorer::Abi::OSFlavor> > flavorLists;
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


#endif
