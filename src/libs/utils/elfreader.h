// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QList>
#include <QSharedPointer>

namespace Utils {

class ElfMapper;

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

enum ElfSectionHeaderType
{
    Elf_SHT_NULL          = 0,
    Elf_SHT_PROGBITS      = 1,
    Elf_SHT_SYMTAB        = 2,
    Elf_SHT_STRTAB        = 3,
    Elf_SHT_RELA          = 4,
    Elf_SHT_HASH          = 5,
    Elf_SHT_DYNAMIC       = 6,
    Elf_SHT_NOTE          = 7,
    Elf_SHT_NOBITS        = 8,
    Elf_SHT_REL           = 9,
    Elf_SHT_SHLIB         = 10,
    Elf_SHT_DYNSYM        = 11,
    Elf_SHT_INIT_ARRAY    = 14,
    Elf_SHT_FINI_ARRAY    = 15,
    Elf_SHT_PREINIT_ARRAY = 16,
    Elf_SHT_GROUP         = 17,
    Elf_SHT_SYMTAB_SHNDX  = 18
};

enum ElfEndian
{
    Elf_ELFDATANONE = 0,
    Elf_ELFDATA2LSB = 1,
    Elf_ELFDATA2MSB = 2,
    Elf_ELFDATANUM  = 3
};

enum ElfClass
{
    Elf_ELFCLASSNONE = 0,
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
    Elf_EM_NONE   =  0,
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
    quint32 flags;
    quint64 offset;
    quint64 size;
    quint64 addr;
};

class ElfProgramHeader
{
public:
    quint32 name;
    quint32 type;
    quint64 offset;
    quint64 filesz;
    quint64 memsz;
};

class QTCREATOR_UTILS_EXPORT ElfData
{
public:
    ElfData() = default;
    int indexOf(const QByteArray &name) const;

public:
    ElfEndian  endian = Elf_ELFDATANONE;
    ElfType    elftype = Elf_ET_NONE;
    ElfMachine elfmachine = Elf_EM_NONE;
    ElfClass   elfclass = Elf_ELFCLASSNONE;
    quint64    entryPoint = 0;
    QByteArray debugLink;
    QByteArray buildId;
    DebugSymbolsType symbolsType = UnknownSymbols;
    QList<ElfSectionHeader> sectionHeaders;
    QList<ElfProgramHeader> programHeaders;
};

class QTCREATOR_UTILS_EXPORT ElfReader
{
public:
    explicit ElfReader(const FilePath &binary);
    enum Result { Ok, NotElf, Corrupt };

    ElfData readHeaders();
    QSharedPointer<ElfMapper> readSection(const QByteArray &sectionName);
    QString errorString() const { return m_errorString; }
    QByteArray readCoreName(bool *isCore);

private:
    friend class ElfMapper;
    Result readIt();

    FilePath m_binary;
    QString m_errorString;
    ElfData m_elfData;
};

class QTCREATOR_UTILS_EXPORT ElfMapper
{
public:
    ElfMapper(const ElfReader *reader);
    bool map();

public:
    QFile file;
    QByteArray raw;
    union { const char *start; const uchar *ustart; };
    quint64 fdlen = 0;
    FilePath binary;
};

} // namespace Utils
