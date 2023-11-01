// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/osspecificaspects.h>

#include <QList>
#include <QHash>

#include <vector>

namespace Utils { class FilePath; }

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ABI (documentation inside)
// --------------------------------------------------------------------------

class Abi;
using Abis = QVector<Abi>;

class PROJECTEXPLORER_EXPORT Abi
{
public:
    enum Architecture {
        ArmArchitecture,
        X86Architecture,
        ItaniumArchitecture,
        MipsArchitecture,
        PowerPCArchitecture,
        ShArchitecture,
        AvrArchitecture,
        Avr32Architecture,
        XtensaArchitecture,
        Mcs51Architecture,
        Mcs251Architecture,
        AsmJsArchitecture,
        Stm8Architecture,
        Msp430Architecture,
        Rl78Architecture,
        C166Architecture,
        V850Architecture,
        Rh850Architecture,
        RxArchitecture,
        K78Architecture,
        M68KArchitecture,
        M32CArchitecture,
        M16CArchitecture,
        M32RArchitecture,
        R32CArchitecture,
        CR16Architecture,
        RiscVArchitecture,
        UnknownArchitecture
    };

    enum OS {
        BsdOS,
        LinuxOS,
        DarwinOS,
        UnixOS,
        WindowsOS,
        VxWorks,
        QnxOS,
        BareMetalOS,
        UnknownOS
    };

    enum OSFlavor {
        // BSDs
        FreeBsdFlavor,
        NetBsdFlavor,
        OpenBsdFlavor,

        // Linux
        AndroidLinuxFlavor,

        // Unix
        SolarisUnixFlavor,

        // Windows
        WindowsMsvc2005Flavor,
        WindowsMsvc2008Flavor,
        WindowsMsvc2010Flavor,
        WindowsMsvc2012Flavor,
        WindowsMsvc2013Flavor,
        WindowsMsvc2015Flavor,
        WindowsMsvc2017Flavor,
        WindowsMsvc2019Flavor,
        WindowsMsvc2022Flavor,
        WindowsLastMsvcFlavor = WindowsMsvc2022Flavor,
        WindowsMSysFlavor,
        WindowsCEFlavor,

        // Embedded
        VxWorksFlavor,

        // Generic:
        RtosFlavor,
        GenericFlavor,

        // Yocto
        PokyFlavor,

        UnknownFlavor // keep last in this enum!
    };

    enum BinaryFormat {
        ElfFormat,
        MachOFormat,
        PEFormat,
        RuntimeQmlFormat,
        UbrofFormat,
        OmfFormat,
        EmscriptenFormat,
        UnknownFormat
    };

    Abi(const Architecture &a = UnknownArchitecture, const OS &o = UnknownOS,
        const OSFlavor &so = UnknownFlavor, const BinaryFormat &f = UnknownFormat,
        unsigned char w = 0, const QString &p = {});

    static Abi abiFromTargetTriplet(const QString &machineTriple);

    static Utils::OsType abiOsToOsType(const OS os);

    bool operator != (const Abi &other) const;
    bool operator == (const Abi &other) const;
    bool isCompatibleWith(const Abi &other) const;

    bool isValid() const;
    bool isNull() const;

    Architecture architecture() const { return m_architecture; }
    OS os() const { return m_os; }
    OSFlavor osFlavor() const { return m_osFlavor; }
    BinaryFormat binaryFormat() const { return m_binaryFormat; }
    unsigned char wordWidth() const { return m_wordWidth; }

    QString toString() const;
    QString param() const;

    static QString toString(const Architecture &a);
    static QString toString(const OS &o);
    static QString toString(const OSFlavor &of);
    static QString toString(const BinaryFormat &bf);
    static QString toString(int w);

    static Architecture architectureFromString(const QString &a);
    static OS osFromString(const QString &o);
    static OSFlavor osFlavorFromString(const QString &of, const OS os);
    static BinaryFormat binaryFormatFromString(const QString &bf);
    static unsigned char wordWidthFromString(const QString &w);

    static OSFlavor registerOsFlavor(const std::vector<OS> &oses, const QString &flavorName);
    static QList<OSFlavor> flavorsForOs(const OS &o);
    static QList<OSFlavor> allOsFlavors();
    static bool osSupportsFlavor(const OS &os, const OSFlavor &flavor);
    static OSFlavor flavorForMsvcVersion(int version);

    static Abi fromString(const QString &abiString);
    static Abi hostAbi();
    static Abis abisOfBinary(const Utils::FilePath &path);

    friend auto qHash(const ProjectExplorer::Abi &abi)
    {
        int h = abi.architecture()
                + (abi.os() << 3)
                + (abi.osFlavor() << 6)
                + (abi.binaryFormat() << 10)
                + (abi.wordWidth() << 13);
        return QT_PREPEND_NAMESPACE(qHash)(h);
    }

private:
    Architecture m_architecture;
    OS m_os;
    OSFlavor m_osFlavor;
    BinaryFormat m_binaryFormat;
    unsigned char m_wordWidth;
    QString m_param;
};

} // namespace ProjectExplorer
