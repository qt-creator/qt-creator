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
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSysInfo>

namespace ProjectExplorer {

Abi::Abi(const Architecture &a, const OS &o,
         const OSFlavour &of, const BinaryFormat &f, unsigned char w) :
    m_architecture(a), m_os(o), m_osFlavor(of), m_binaryFormat(f), m_wordWidth(w)
{
    switch (m_os) {
    case ProjectExplorer::Abi::UNKNOWN_OS:
        m_osFlavor = UNKNOWN_OSFLAVOUR;
        break;
    case ProjectExplorer::Abi::Linux:
        if (m_osFlavor < Linux_generic || m_osFlavor > Linux_meego)
            m_osFlavor = UNKNOWN_OSFLAVOUR;
        break;
    case ProjectExplorer::Abi::Mac:
        if (m_osFlavor < Mac_generic || m_osFlavor > Mac_generic)
            m_osFlavor = UNKNOWN_OSFLAVOUR;
        break;
    case ProjectExplorer::Abi::Symbian:
        if (m_osFlavor < Symbian_device || m_osFlavor > Symbian_emulator)
            m_osFlavor = UNKNOWN_OSFLAVOUR;
        break;
    case ProjectExplorer::Abi::UNIX:
        if (m_osFlavor < Unix_generic || m_osFlavor > Unix_generic)
            m_osFlavor = UNKNOWN_OSFLAVOUR;
        break;
    case ProjectExplorer::Abi::Windows:
        if (m_osFlavor < Windows_msvc || m_osFlavor > Windows_ce)
            m_osFlavor = UNKNOWN_OSFLAVOUR;
        break;
    }
}

Abi::Abi(const QString &abiString) :
    m_architecture(UNKNOWN_ARCHITECTURE), m_os(UNKNOWN_OS),
    m_osFlavor(UNKNOWN_OSFLAVOUR), m_binaryFormat(UNKNOWN_FORMAT), m_wordWidth(0)
{
    QStringList abiParts = abiString.split(QLatin1Char('-'));
    if (abiParts.count() >= 1) {
        if (abiParts.at(0) == QLatin1String("unknown"))
            m_architecture = UNKNOWN_ARCHITECTURE;
        else if (abiParts.at(0) == QLatin1String("arm"))
            m_architecture = ARM;
        else if (abiParts.at(0) == QLatin1String("x86"))
            m_architecture = x86;
        else if (abiParts.at(0) == QLatin1String("mips"))
            m_architecture = Mips;
        else if (abiParts.at(0) == QLatin1String("ppc"))
            m_architecture = PowerPC;
        else if (abiParts.at(0) == QLatin1String("itanium"))
            m_architecture = Itanium;
        else
            return;
    }

    if (abiParts.count() >= 2) {
        if (abiParts.at(1) == QLatin1String("unknown"))
            m_os = UNKNOWN_OS;
        else if (abiParts.at(1) == QLatin1String("linux"))
            m_os = Linux;
        else if (abiParts.at(1) == QLatin1String("macos"))
            m_os = Mac;
        else if (abiParts.at(1) == QLatin1String("symbian"))
            m_os = Symbian;
        else if (abiParts.at(1) == QLatin1String("unix"))
            m_os = UNIX;
        else if (abiParts.at(1) == QLatin1String("windows"))
            m_os = Windows;
        else
            return;
    }

    if (abiParts.count() >= 3) {
        if (abiParts.at(2) == QLatin1String("unknown"))
            m_osFlavor = UNKNOWN_OSFLAVOUR;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == Linux)
            m_osFlavor = Linux_generic;
        else if (abiParts.at(2) == QLatin1String("maemo") && m_os == Linux)
            m_osFlavor = Linux_maemo;
        else if (abiParts.at(2) == QLatin1String("meego") && m_os == Linux)
            m_osFlavor = Linux_meego;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == Mac)
            m_osFlavor = Mac_generic;
        else if (abiParts.at(2) == QLatin1String("device") && m_os == Symbian)
            m_osFlavor = Symbian_device;
        else if (abiParts.at(2) == QLatin1String("emulator") && m_os == Symbian)
            m_osFlavor = Symbian_emulator;
        else if (abiParts.at(2) == QLatin1String("generic") && m_os == UNIX)
            m_osFlavor = Unix_generic;
        else if (abiParts.at(2) == QLatin1String("msvc") && m_os == Windows)
            m_osFlavor = Windows_msvc;
        else if (abiParts.at(2) == QLatin1String("msys") && m_os == Windows)
            m_osFlavor = Windows_msys;
        else if (abiParts.at(2) == QLatin1String("ce") && m_os == Windows)
            m_osFlavor = Windows_ce;
        else
            return;
    }

    if (abiParts.count() >= 4) {
        if (abiParts.at(3) == QLatin1String("unknown"))
            m_binaryFormat = UNKNOWN_FORMAT;
        else if (abiParts.at(3) == QLatin1String("elf"))
            m_binaryFormat = Format_ELF;
        else if (abiParts.at(3) == QLatin1String("pe"))
            m_binaryFormat = Format_PE;
        else if (abiParts.at(3) == QLatin1String("mach_o"))
            m_binaryFormat = Format_Mach_O;
        else if (abiParts.at(3) == QLatin1String("qml_rt"))
            m_binaryFormat = Format_Runtime_QML;
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
    return (architecture() == other.architecture() || other.architecture() == Abi::UNKNOWN_ARCHITECTURE)
            && (os() == other.os() || other.os() == Abi::UNKNOWN_OS)
            && (osFlavor() == other.osFlavor() || other.osFlavor() == Abi::UNKNOWN_OSFLAVOUR)
            && (binaryFormat() == other.binaryFormat() || other.binaryFormat() == Abi::UNKNOWN_FORMAT)
            && ((wordWidth() == other.wordWidth() && wordWidth() != 0) || other.wordWidth() == 0);
}

bool Abi::isValid() const
{
    return m_architecture != UNKNOWN_ARCHITECTURE
            && m_os != UNKNOWN_OS
            && m_osFlavor != UNKNOWN_OSFLAVOUR
            && m_binaryFormat != UNKNOWN_FORMAT
            && m_wordWidth != 0;
}

QString Abi::toString(const Architecture &a)
{
    switch (a) {
    case ARM:
        return QLatin1String("arm");
    case x86:
        return QLatin1String("x86");
    case Mips:
        return QLatin1String("mips");
    case PowerPC:
        return QLatin1String("ppc");
    case Itanium:
        return QLatin1String("itanium");
    case UNKNOWN_ARCHITECTURE: // fall through!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(const OS &o)
{
    switch (o) {
    case Linux:
        return QLatin1String("linux");
    case Mac:
        return QLatin1String("macos");
    case Symbian:
        return QLatin1String("symbian");
    case UNIX:
        return QLatin1String("unix");
    case Windows:
        return QLatin1String("windows");
    case UNKNOWN_OS: // fall through!
    default:
        return QLatin1String("unknown");
    };
}

QString Abi::toString(const OSFlavour &of)
{
    switch (of) {
    case ProjectExplorer::Abi::Linux_generic:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::Linux_maemo:
        return QLatin1String("maemo");
    case ProjectExplorer::Abi::Linux_harmattan:
        return QLatin1String("harmattan");
    case ProjectExplorer::Abi::Linux_meego:
        return QLatin1String("meego");
    case ProjectExplorer::Abi::Mac_generic:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::Symbian_device:
        return QLatin1String("device");
    case ProjectExplorer::Abi::Symbian_emulator:
        return QLatin1String("emulator");
    case ProjectExplorer::Abi::Unix_generic:
        return QLatin1String("generic");
    case ProjectExplorer::Abi::Windows_msvc:
        return QLatin1String("msvc");
    case ProjectExplorer::Abi::Windows_msys:
        return QLatin1String("msys");
    case ProjectExplorer::Abi::Windows_ce:
        return QLatin1String("ce");
    case ProjectExplorer::Abi::UNKNOWN_OSFLAVOUR: // fall throught!
    default:
        return QLatin1String("unknown");
    }
}

QString Abi::toString(const BinaryFormat &bf)
{
    switch (bf) {
    case Format_ELF:
        return QLatin1String("elf");
    case Format_PE:
        return QLatin1String("pe");
    case Format_Mach_O:
        return QLatin1String("mach_o");
    case Format_Runtime_QML:
        return QLatin1String("qml_rt");
    case UNKNOWN_FORMAT: // fall through!
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
    OS os = UNKNOWN_OS;
    OSFlavour subos = UNKNOWN_OSFLAVOUR;
    BinaryFormat format = UNKNOWN_FORMAT;

#if defined (Q_OS_WIN)
    os = Windows;
    subos = Windows_msvc;
    format = Format_PE;
#elif defined (Q_OS_LINUX)
    os = Linux;
    subos = Linux_generic;
    format = Format_ELF;
#elif defined (Q_OS_MAC)
    os = Mac;
    subos = Mac_generic;
    format = Format_Mach_O;
#endif

    return Abi(arch, os, subos, format, QSysInfo::WordSize);
}

static Abi macAbiForCpu(quint32 type) {
    switch (type) {
    case 7: // CPU_TYPE_X86, CPU_TYPE_I386
        return Abi(Abi::x86, Abi::Mac, Abi::Mac_generic, Abi::Format_Mach_O, 32);
    case 0x01000000 +  7: // CPU_TYPE_X86_64
        return Abi(Abi::x86, Abi::Mac, Abi::Mac_generic, Abi::Format_Mach_O, 64);
    case 18: // CPU_TYPE_POWERPC
        return Abi(Abi::PowerPC, Abi::Mac, Abi::Mac_generic, Abi::Format_Mach_O, 32);
    case 0x01000000 + 18: // CPU_TYPE_POWERPC64
        return Abi(Abi::PowerPC, Abi::Mac, Abi::Mac_generic, Abi::Format_Mach_O, 32);
    case 12: // CPU_TYPE_ARM
        return Abi(Abi::ARM, Abi::Mac, Abi::Mac_generic, Abi::Format_Mach_O, 32);
    default:
        return Abi();
    }
}

QList<Abi> Abi::abisOfBinary(const QString &path)
{
    QList<Abi> result;
    if (path.isEmpty())
        return result;

    QFile f(path);
    if (!f.exists())
        return result;

    f.open(QFile::ReadOnly);
    QByteArray data = f.read(1024);
    f.close();

    if (data.size() >= 20
            && static_cast<unsigned char>(data.at(0)) == 0x7f && static_cast<unsigned char>(data.at(1)) == 'E'
            && static_cast<unsigned char>(data.at(2)) == 'L' && static_cast<unsigned char>(data.at(3)) == 'F') {
        // ELF format:
        quint16 machine = (data.at(19) << 8) + data.at(18);
        switch (machine) {
        case 3: // EM_386
            result.append(Abi(Abi::x86, Abi::Linux, Abi::Linux_generic, Abi::Format_ELF, 32));
            break;
        case 8: // EM_MIPS
            result.append(Abi(Abi::Mips, Abi::Linux, Abi::Linux_generic, Abi::Format_ELF, 32));
            break;
        case 20: // EM_PPC
            result.append(Abi(Abi::PowerPC, Abi::Linux, Abi::Linux_generic, Abi::Format_ELF, 32));
            break;
        case 21: // EM_PPC64
            result.append(Abi(Abi::PowerPC, Abi::Linux, Abi::Linux_generic, Abi::Format_ELF, 64));
            break;
        case 62: // EM_X86_64
            result.append(Abi(Abi::x86, Abi::Linux, Abi::Linux_generic, Abi::Format_ELF, 64));
            break;
        case 50: // EM_IA_64
            result.append(Abi(Abi::Itanium, Abi::Linux, Abi::Linux_generic, Abi::Format_ELF, 64));
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
        int pePos = data.indexOf("PE\0\0");
        if (pePos >= 0 && pePos + 5 < data.size()) {
            quint16 machine = (data.at(pePos + 5) << 8) + data.at(pePos + 4);
            switch (machine) {
            case 0x8664: // x86_64
                result.append(Abi(Abi::x86, Abi::Windows, Abi::Windows_msvc, Abi::Format_PE, 64));
                break;
            case 0x014c: // i386
                result.append(Abi(Abi::x86, Abi::Windows, Abi::Windows_msvc, Abi::Format_PE, 32));
                break;
            case 0x0200: // ia64
                result.append(Abi(Abi::Itanium, Abi::Windows, Abi::Windows_msvc, Abi::Format_PE, 64));
                break;
            }
        }
    }
    return result;
}

} // namespace ProjectExplorer
