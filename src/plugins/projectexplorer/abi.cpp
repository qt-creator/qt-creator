/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "abi.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSysInfo>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

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
    quint16 machine = (data.at(1) << 8) + data.at(0);
    switch (machine) {
    case 0x8664: // x86_64
        arch = Abi::X86Architecture;
        width = 64;
        break;
    case 0x014c: // i386
        arch = Abi::X86Architecture;
        width = 32;
        break;
    case 0x0200: // ia64
        arch = Abi::ItaniumArchitecture;
        width = 64;
        break;
    }

    if (data.size() >= 68) {
        // Get Major and Minor Image Version from optional header fields
        quint32 image = (data.at(67) << 24) + (data.at(66) << 16) + (data.at(65) << 8) + data.at(64);
        if (image == 1) { // Image is 1 for mingw and higher for MSVC (4.something in some encoding)
            flavor = Abi::WindowsMSysFlavor;
        } else {
            switch (data.at(22)) {
            case 8:
                flavor = Abi::WindowsMsvc2005Flavor;
                break;
            case 9:
                flavor = Abi::WindowsMsvc2008Flavor;
                break;
            case 10:
                flavor = Abi::WindowsMsvc2010Flavor;
                break;
            default:
                // Keep unknown flavor
                break;
            }
        }
    }

    if (arch != Abi::UnknownArchitecture && width != 0)
        result.append(Abi(arch, Abi::WindowsOS, flavor, Abi::PEFormat, width));

    return result;
}

static QList<Abi> abiOf(const QByteArray &data)
{
    QList<Abi> result;

    if (data.size() >= 20
            && static_cast<unsigned char>(data.at(0)) == 0x7f && static_cast<unsigned char>(data.at(1)) == 'E'
            && static_cast<unsigned char>(data.at(2)) == 'L' && static_cast<unsigned char>(data.at(3)) == 'F') {
        // ELF format:
        quint16 machine = (data.at(19) << 8) + data.at(18);
        switch (machine) {
        case 3: // EM_386
            result.append(Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
            break;
        case 8: // EM_MIPS
            result.append(Abi(Abi::MipsArcitecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
            break;
        case 20: // EM_PPC
            result.append(Abi(Abi::PowerPCArchitecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
            break;
        case 21: // EM_PPC64
            result.append(Abi(Abi::PowerPCArchitecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 64));
            break;
        case 62: // EM_X86_64
            result.append(Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 64));
            break;
        case 50: // EM_IA_64
            result.append(Abi(Abi::ItaniumArchitecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 64));
            break;
        default:
            ;;
        }
    } else if (data.size() >= 8
               && (static_cast<unsigned char>(data.at(0)) == 0xce || static_cast<unsigned char>(data.at(0)) == 0xcf)
               && static_cast<unsigned char>(data.at(1)) == 0xfa
               && static_cast<unsigned char>(data.at(2)) == 0xed && static_cast<unsigned char>(data.at(3)) == 0xfe) {
        // Mach-O format (Mac non-fat binary, 32 and 64bit magic)
        quint32 type = (data.at(7) << 24) + (data.at(6) << 16) + (data.at(5) << 8) + data.at(4);
        result.append(macAbiForCpu(type));
    } else if (data.size() >= 8
               && static_cast<unsigned char>(data.at(0)) == 0xca && static_cast<unsigned char>(data.at(1)) == 0xfe
               && static_cast<unsigned char>(data.at(2)) == 0xba && static_cast<unsigned char>(data.at(3)) == 0xbe) {
        // Mac fat binary:
        quint32 count = (data.at(4) << 24) + (data.at(5) << 16) + (data.at(6) << 8) + data.at(7);
        int pos = 8;
        for (quint32 i = 0; i < count; ++i) {
            if (data.size() <= pos + 4)
                break;

            quint32 type = (data.at(pos) << 24) + (data.at(pos + 1) << 16) + (data.at(pos + 2) << 8) + data.at(pos + 3);
            result.append(macAbiForCpu(type));
            pos += 20;
        }
    } else {
        // Windows PE
        // Windows can have its magic bytes everywhere...
        int pePos = data.indexOf(QByteArray("PE\0\0", 4));
        if (pePos >= 0)
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
        if (m_osFlavor < GenericLinuxFlavor || m_osFlavor > MeegoLinuxFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::MacOS:
        if (m_osFlavor < GenericMacFlavor || m_osFlavor > GenericMacFlavor)
            m_osFlavor = UnknownFlavor;
        break;
    case ProjectExplorer::Abi::SymbianOS:
        if (m_osFlavor < SymbianDeviceFlavor || m_osFlavor > SymbianEmulatorFlavor)
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
            m_architecture = MipsArcitecture;
        else if (abiParts.at(0) == QLatin1String("ppc"))
            m_architecture = PowerPCArchitecture;
        else if (abiParts.at(0) == QLatin1String("itanium"))
            m_architecture = ItaniumArchitecture;
        else
            return;
    }

    if (abiParts.count() >= 2) {
        if (abiParts.at(1) == QLatin1String("unknown"))
            m_os = UnknownOS;
        else if (abiParts.at(1) == QLatin1String("linux"))
            m_os = LinuxOS;
        else if (abiParts.at(1) == QLatin1String("macos"))
            m_os = MacOS;
        else if (abiParts.at(1) == QLatin1String("symbian"))
            m_os = SymbianOS;
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
        else if (abiParts.at(2) == QLatin1String("maemo") && m_os == LinuxOS)
            m_osFlavor = MaemoLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("meego") && m_os == LinuxOS)
            m_osFlavor = MeegoLinuxFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == MacOS)
            m_osFlavor = GenericMacFlavor;
        else if (abiParts.at(2) == QLatin1String("device") && m_os == SymbianOS)
            m_osFlavor = SymbianDeviceFlavor;
        else if (abiParts.at(2) == QLatin1String("emulator") && m_os == SymbianOS)
            m_osFlavor = SymbianEmulatorFlavor;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == UnixOS)
            m_osFlavor = GenericUnixFlavor;
        else if (abiParts.at(2) == QLatin1String("msvc2005") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2005Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2008") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2008Flavor;
        else if (abiParts.at(2) == QLatin1String("msvc2010") && m_os == WindowsOS)
            m_osFlavor = WindowsMsvc2010Flavor;
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
    return (architecture() == other.architecture() || other.architecture() == Abi::UnknownArchitecture)
            && (os() == other.os() || other.os() == Abi::UnknownOS)
            && (osFlavor() == other.osFlavor() || other.osFlavor() == Abi::UnknownFlavor)
            && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == Abi::UnknownFormat)
            && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0);
}

bool Abi::isValid() const
{
    return m_architecture != UnknownArchitecture
            && m_os != UnknownOS
            && m_osFlavor != UnknownFlavor
            && m_binaryFormat != UnknownFormat
            && m_wordWidth != 0;
}

QString Abi::toString(const Architecture &a)
{
    switch (a) {
    case ArmArchitecture:
        return QLatin1String("arm");
    case X86Architecture:
        return QLatin1String("x86");
    case MipsArcitecture:
        return QLatin1String("mips");
    case PowerPCArchitecture:
        return QLatin1String("ppc");
    case ItaniumArchitecture:
        return QLatin1String("itanium");
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
    case MacOS:
        return QLatin1String("macos");
    case SymbianOS:
        return QLatin1String("symbian");
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
    case ProjectExplorer::Abi::MaemoLinuxFlavor:
        return QLatin1String("maemo");
    case ProjectExplorer::Abi::HarmattanLinuxFlavor:
        return QLatin1String("harmattan");
    case ProjectExplorer::Abi::MeegoLinuxFlavor:
        return QLatin1String("meego");
    case ProjectExplorer::Abi::GenericMacFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::SymbianDeviceFlavor:
        return QLatin1String("device");
    case ProjectExplorer::Abi::SymbianEmulatorFlavor:
        return QLatin1String("emulator");
    case ProjectExplorer::Abi::GenericUnixFlavor:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::WindowsMsvc2005Flavor:
        return QLatin1String("msvc2005");
    case ProjectExplorer::Abi::WindowsMsvc2008Flavor:
        return QLatin1String("msvc2008");
    case ProjectExplorer::Abi::WindowsMsvc2010Flavor:
        return QLatin1String("msvc2010");
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


Abi Abi::hostAbi()
{
    Architecture arch = QTC_CPU; // define set by qmake
    OS os = UnknownOS;
    OSFlavor subos = UnknownFlavor;
    BinaryFormat format = UnknownFormat;

#if defined (Q_OS_WIN)
    os = WindowsOS;
#if _MSC_VER == 1600
    subos = WindowsMsvc2010Flavor;
#elif _MSC_VER == 1500
    subos = WindowsMsvc2008Flavor;
#elif _MSC_VER == 1400
    subos = WindowsMsvc2005Flavor;
#elif defined (mingw32)
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

    return Abi(arch, os, subos, format, QSysInfo::WordSize);
}

QList<Abi> Abi::abisOfBinary(const QString &path)
{
    QList<Abi> result;
    if (path.isEmpty())
        return result;

    QFile f(path);
    if (!f.exists())
        return result;

    bool windowsStatic = path.endsWith(QLatin1String(".lib"));

    f.open(QFile::ReadOnly);
    QByteArray data = f.read(1024);
    if (data.size() >= 67
            && static_cast<unsigned char>(data.at(0)) == '!' && static_cast<unsigned char>(data.at(1)) == '<'
            && static_cast<unsigned char>(data.at(2)) == 'a' && static_cast<unsigned char>(data.at(3)) == 'r'
            && static_cast<unsigned char>(data.at(4)) == 'c' && static_cast<unsigned char>(data.at(5)) == 'h'
            && static_cast<unsigned char>(data.at(6)) == '>' && static_cast<unsigned char>(data.at(7)) == 0x0a) {
        // We got an ar file: possibly a static lib for ELF or Mach-O

        data = data.mid(8); // Cut of ar file magic
        quint64 offset = 8;

        while (!data.isEmpty()) {
            if ((data.at(58) != 0x60 || data.at(59) != 0x0a)) {
                qWarning() << path << ": Thought it was an ar-file, but it is not!";
                break;
            }

            const QString fileName = QString::fromLocal8Bit(data.mid(0, 16));
            quint64 fileNameOffset = 0;
            if (fileName.startsWith(QLatin1String("#1/")))
                fileNameOffset = fileName.mid(3).toInt();
            const QString fileLength = QString::fromAscii(data.mid(48, 10));

            int toSkip = 60 + fileNameOffset;
            offset += fileLength.toInt() + 60 /* header */;
            if (windowsStatic) {
                if (fileName == QLatin1String("/0              "))
                    result = parseCoffHeader(data.mid(toSkip, 20));
            } else {
                result = abiOf(data.mid(toSkip));
            }
            if (!result.isEmpty())
                break;

            f.seek(offset + (offset % 2)); // ar is 2 byte alligned
            data = f.read(1024);
        }
    } else {
        result = abiOf(data);
    }
    f.close();

    return result;
}

} // namespace ProjectExplorer
