/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef UTILS_ELFREADER_H
#define UTILS_ELFREADER_H

#include "utils_global.h"

#include <qendian.h>
#include <qstring.h>
#include <qvector.h>

namespace Utils {

enum ElfProgramHeaderType
{
    Elf_PT_NULL    = 0,
    Elf_PT_LOAD    = 1,
    Elf_PT_DYNAMIC = 2,
    Elf_PT_INTERP  = 3,
    Elf_PT_NOTE    = 4,
    Elf_PT_SHLIB   = 5,
    Elf_PT_PHDR    = 6,
    Elf_PT_TLS     = 7,
    Elf_PT_NUM     = 8
};

enum ElfEndian
{
    ElfLittleEndian = 0,
    ElfBigEndian = 1
};

enum ElfClass
{
    Elf_ELFCLASS32 = 1,
    Elf_ELFCLASS64 = 2
};

enum ElfType
{
    Elf_ET_NONE = 0,
    Elf_ET_REL  = 1,
    Elf_ET_EXEC = 2,
    Elf_ET_DYN  = 3,
    Elf_ET_CORE = 4
};

enum ElfMachine
{
    Elf_EM_386    =  3,
    Elf_EM_ARM    = 40,
    Elf_EM_X86_64 = 62
};

enum DebugSymbolsType
{
    UnknownSymbols   = 0,    // Unknown.
    NoSymbols        = 1,    // No usable symbols.
    LinkedSymbols    = 2,    // Link to symols available.
    BuildIdSymbols   = 4,    // BuildId available.
    PlainSymbols     = 8,    // Ordinary symbols available.
    FastSymbols      = 16    // Dwarf index available.
};

class ElfSectionHeader
{
public:
    QByteArray name;
    quint32 index;
    quint32 type;
    quint64 offset;
    quint64 size;
    quint64 data;
};

class ElfProgramHeader
{
public:
    quint32 type;
    quint64 offset;
    quint64 filesz;
    quint64 memsz;
};

class QTCREATOR_UTILS_EXPORT ElfData
{
public:
    ElfData() : symbolsType(UnknownSymbols) {}
    int indexOf(const QByteArray &name) const;

public:
    ElfEndian  endian;
    ElfType    elftype;
    ElfMachine elfmachine;
    ElfClass   elfclass;
    QByteArray debugLink;
    QByteArray buildId;
    DebugSymbolsType symbolsType;
    QVector<ElfSectionHeader> sectionHeaders;
    QVector<ElfProgramHeader> programHeaders;
};

class QTCREATOR_UTILS_EXPORT ElfReader
{
public:
    explicit ElfReader(const QString &binary);
    enum Result { Ok, NotElf, Corrupt };

    ElfData readHeaders();
    QByteArray readSection(const QByteArray &sectionName);
    QString errorString() const { return m_errorString; }
    QByteArray readCoreName(bool *isCore);

private:
    friend class ElfMapper;
    Result readIt();

    QString m_binary;
    QString m_errorString;
    ElfData m_elfData;
};

} // namespace Utils

#endif // UTILS_ELFREADER_H
