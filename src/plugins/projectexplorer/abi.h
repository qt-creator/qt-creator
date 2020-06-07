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
        WindowsLastMsvcFlavor = WindowsMsvc2019Flavor,
        WindowsMSysFlavor,
        WindowsCEFlavor,

        // Embedded
        VxWorksFlavor,

        // Generic:
        RtosFlavor,
        GenericFlavor,

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
    bool isFullyCompatibleWith(const Abi &other) const;

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

    static Architecture architectureFromString(const QStringRef &a);
    static OS osFromString(const QStringRef &o);
    static OSFlavor osFlavorFromString(const QStringRef &of, const OS os);
    static BinaryFormat binaryFormatFromString(const QStringRef &bf);
    static unsigned char wordWidthFromString(const QStringRef &w);

    static OSFlavor registerOsFlavor(const std::vector<OS> &oses, const QString &flavorName);
    static QList<OSFlavor> flavorsForOs(const OS &o);
    static QList<OSFlavor> allOsFlavors();
    static bool osSupportsFlavor(const OS &os, const OSFlavor &flavor);
    static OSFlavor flavorForMsvcVersion(int version);

    static Abi fromString(const QString &abiString);
    static Abi hostAbi();
    static Abis abisOfBinary(const Utils::FilePath &path);


private:
    Architecture m_architecture;
    OS m_os;
    OSFlavor m_osFlavor;
    BinaryFormat m_binaryFormat;
    unsigned char m_wordWidth;
    QString m_param;
};

inline int qHash(const ProjectExplorer::Abi &abi)
{
    int h = abi.architecture()
            + (abi.os() << 3)
            + (abi.osFlavor() << 6)
            + (abi.binaryFormat() << 10)
            + (abi.wordWidth() << 13);
    return QT_PREPEND_NAMESPACE(qHash)(h);
}

} // namespace ProjectExplorer
