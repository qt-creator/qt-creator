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

#ifndef PROJECTEXPLORER_ABI_H
#define PROJECTEXPLORER_ABI_H

#include "projectexplorer_export.h"

#include <QtCore/QList>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ABI
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT Abi
{
public:
    enum Architecture {
        UNKNOWN_ARCHITECTURE,
        ARM,
        x86,
        Itanium,
        Mips,
        PowerPC
    };

    enum OS {
        UNKNOWN_OS,
        Linux,
        Mac,
        Symbian,
        UNIX,
        Windows
    };

    enum OSFlavour {
        UNKNOWN_OSFLAVOUR,

        // Linux
        Linux_generic,
        Linux_harmattan,
        Linux_maemo,
        Linux_meego,

        // Mac
        Mac_generic,

        // Symbian
        Symbian_device,
        Symbian_emulator,

        // Unix
        Unix_generic,

        // Windows
        Windows_msvc,
        Windows_msys,
        Windows_ce
    };

    enum BinaryFormat {
        UNKNOWN_FORMAT,
        Format_ELF,
        Format_Mach_O,
        Format_PE,
        Format_Runtime_QML
    };

    Abi() :
        m_architecture(UNKNOWN_ARCHITECTURE), m_os(UNKNOWN_OS),
        m_osFlavor(UNKNOWN_OSFLAVOUR), m_binaryFormat(UNKNOWN_FORMAT), m_wordWidth(0)
    { }

    Abi(const Architecture &a, const OS &o,
        const OSFlavour &so, const BinaryFormat &f, unsigned char w);
    Abi(const QString &abiString);

    bool operator == (const Abi &other) const;
    bool isCompatibleWith(const Abi &other) const;

    bool isValid() const;

    Architecture architecture() const { return m_architecture; }
    OS os() const { return m_os; }
    OSFlavour osFlavor() const { return m_osFlavor; }
    BinaryFormat binaryFormat() const { return m_binaryFormat; }
    unsigned char wordWidth() const { return m_wordWidth; }

    QString toString() const;

    static QString toString(const Architecture &a);
    static QString toString(const OS &o);
    static QString toString(const OSFlavour &of);
    static QString toString(const BinaryFormat &bf);
    static QString toString(int w);

    static Abi hostAbi();
    static QList<Abi> abisOfBinary(const QString &path);

private:
    Architecture m_architecture;
    OS m_os;
    OSFlavour m_osFlavor;
    BinaryFormat m_binaryFormat;
    unsigned char m_wordWidth;
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_ABI_H
