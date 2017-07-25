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

#include <QList>
#include <QHash>

namespace Utils { class FileName; }

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ABI (documentation inside)
// --------------------------------------------------------------------------

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
        GenericLinuxFlavor,
        AndroidLinuxFlavor,

        // Darwin
        GenericDarwinFlavor,

        // Unix
        GenericUnixFlavor,
        SolarisUnixFlavor,

        // Windows
        WindowsMsvc2005Flavor,
        WindowsMsvc2008Flavor,
        WindowsMsvc2010Flavor,
        WindowsMsvc2012Flavor,
        WindowsMsvc2013Flavor,
        WindowsMsvc2015Flavor,
        WindowsMsvc2017Flavor,
        WindowsMSysFlavor,
        WindowsCEFlavor,

        // Embedded
        VxWorksFlavor,
        GenericQnxFlavor,
        GenericBareMetalFlavor,

        UnknownFlavor
    };

    enum BinaryFormat {
        ElfFormat,
        MachOFormat,
        PEFormat,
        RuntimeQmlFormat,
        UnknownFormat
    };

    Abi() :
        m_architecture(UnknownArchitecture), m_os(UnknownOS),
        m_osFlavor(UnknownFlavor), m_binaryFormat(UnknownFormat), m_wordWidth(0)
    { }

    Abi(const Architecture &a, const OS &o,
        const OSFlavor &so, const BinaryFormat &f, unsigned char w);
    Abi(const QString &abiString);

    static Abi abiFromTargetTriplet(const QString &machineTriple);

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

    static QString toString(const Architecture &a);
    static QString toString(const OS &o);
    static QString toString(const OSFlavor &of);
    static QString toString(const BinaryFormat &bf);
    static QString toString(int w);

    static QList<OSFlavor> flavorsForOs(const OS &o);
    static OSFlavor flavorForMsvcVersion(int version);

    static Abi hostAbi();
    static QList<Abi> abisOfBinary(const Utils::FileName &path);

private:
    Architecture m_architecture;
    OS m_os;
    OSFlavor m_osFlavor;
    BinaryFormat m_binaryFormat;
    unsigned char m_wordWidth;
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
