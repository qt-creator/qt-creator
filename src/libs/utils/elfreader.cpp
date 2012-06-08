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

#include "elfreader.h"
#include "qtcassert.h"

#include <QFile>
#include <QLibrary>
#include <QDebug>

namespace Utils {

quint16 getHalfWord(const unsigned char *&s, const ElfData &context)
{
    quint16 res;
    if (context.endian == Elf_ELFDATA2MSB)
        res = qFromBigEndian<quint16>(s);
    else
        res = qFromLittleEndian<quint16>(s);
    s += 2;
    return res;
}

quint32 getWord(const unsigned char *&s, const ElfData &context)
{
    quint32 res;
    if (context.endian == Elf_ELFDATA2MSB)
        res = qFromBigEndian<quint32>(s);
    else
        res = qFromLittleEndian<quint32>(s);
    s += 4;
    return res;
}

quint64 getAddress(const unsigned char *&s, const ElfData &context)
{
    quint64 res;
    if (context.elfclass == Elf_ELFCLASS32) {
        if (context.endian == Elf_ELFDATA2MSB)
            res = qFromBigEndian<quint32>(s);
        else
            res = qFromLittleEndian<quint32>(s);
        s += 4;
    } else {
        if (context.endian == Elf_ELFDATA2MSB)
            res = qFromBigEndian<quint64>(s);
        else
            res = qFromLittleEndian<quint64>(s);
        s += 8;
    }
    return res;
}

quint64 getOffset(const unsigned char *&s, const ElfData &context)
{
    return getAddress(s, context);
}

static void parseSectionHeader(const uchar *s, ElfSectionHeader *sh, const ElfData &context)
{
    sh->index = getWord(s, context);
    sh->type = getWord(s, context);
    sh->flags = getOffset(s, context);
    sh->addr = getAddress(s, context);
    sh->offset = getOffset(s, context);
    sh->size = getOffset(s, context);
}

static void parseProgramHeader(const uchar *s, ElfProgramHeader *sh, const ElfData &context)
{
    sh->type = getWord(s, context);
    sh->offset = getOffset(s, context);
    /* p_vaddr = */ getAddress(s, context);
    /* p_paddr = */ getAddress(s, context);
    sh->filesz = getWord(s, context);
    sh->memsz = getWord(s, context);
}

class ElfMapper
{
public:
    ElfMapper(const ElfReader *reader) : file(reader->m_binary) {}

    bool map()
    {
        if (!file.open(QIODevice::ReadOnly))
            return false;

        fdlen = file.size();
        ustart = file.map(0, fdlen);
        if (ustart == 0) {
            // Try reading the data into memory instead.
            raw = file.readAll();
            start = raw.constData();
            fdlen = raw.size();
        }
        return true;
    }

public:
    QFile file;
    QByteArray raw;
    union { const char *start; const uchar *ustart; };
    quint64 fdlen;
};

ElfReader::ElfReader(const QString &binary)
    : m_binary(binary)
{
}

ElfData ElfReader::readHeaders()
{
    readIt();
    return m_elfData;
}

ElfReader::Result ElfReader::readIt()
{
    if (!m_elfData.sectionHeaders.isEmpty())
        return Ok;
    if (!m_elfData.programHeaders.isEmpty())
        return Ok;

    ElfMapper mapper(this);
    if (!mapper.map())
        return Corrupt;

    const quint64 fdlen = mapper.fdlen;

    if (fdlen < 64) {
        m_errorString = QLibrary::tr("'%1' is not an ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("file too small"));
        return NotElf;
    }

    if (strncmp(mapper.start, "\177ELF", 4) != 0) {
        m_errorString = QLibrary::tr("'%1' is not an ELF object")
                .arg(m_binary);
        return NotElf;
    }

    // 32 or 64 bit
    m_elfData.elfclass = ElfClass(mapper.start[4]);
    const bool is64Bit = m_elfData.elfclass == Elf_ELFCLASS64;
    if (m_elfData.elfclass != Elf_ELFCLASS32 && m_elfData.elfclass != Elf_ELFCLASS64) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("odd cpu architecture"));
        return Corrupt;
    }

    // int bits = (data[4] << 5);
    // If you remove this check to read ELF objects of a different arch,
    // please make sure you modify the typedefs
    // to match the _plugin_ architecture.
    // if ((sizeof(void*) == 4 && bits != 32)
    //     || (sizeof(void*) == 8 && bits != 64)) {
    //     if (errorString)
    //         *errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
    //         .arg(m_binary).arg(QLatin1String("wrong cpu architecture"));
    //     return Corrupt;
    // }

    // Read Endianess.
    m_elfData.endian = ElfEndian(mapper.ustart[5]);
    if (m_elfData.endian != Elf_ELFDATA2LSB && m_elfData.endian != Elf_ELFDATA2MSB) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("odd endianess"));
        return Corrupt;
    }

    const uchar *data = mapper.ustart + 16; // e_ident
    m_elfData.elftype    = ElfType(getHalfWord(data, m_elfData));
    m_elfData.elfmachine = ElfMachine(getHalfWord(data, m_elfData));
    /* e_version = */   getWord(data, m_elfData);
    m_elfData.entryPoint = getAddress(data, m_elfData);

    quint64 e_phoff   = getOffset(data, m_elfData);
    quint64 e_shoff   = getOffset(data, m_elfData);
    /* e_flags = */     getWord(data, m_elfData);

    quint32 e_shsize  = getHalfWord(data, m_elfData);

    if (e_shsize > fdlen) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("unexpected e_shsize"));
        return Corrupt;
    }

    quint32 e_phentsize = getHalfWord(data, m_elfData);
    QTC_CHECK(e_phentsize == (is64Bit ? 44 : 32));
    quint32 e_phnum     = getHalfWord(data, m_elfData);

    quint32 e_shentsize = getHalfWord(data, m_elfData);

    if (e_shentsize % 4) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("unexpected e_shentsize"));
        return Corrupt;
    }

    quint32 e_shnum     = getHalfWord(data, m_elfData);
    quint32 e_shtrndx   = getHalfWord(data, m_elfData);
    QTC_CHECK(data == mapper.ustart + (is64Bit ? 58 : 52));

    if (quint64(e_shnum) * e_shentsize > fdlen) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary)
            .arg(QLatin1String("announced %2 sections, each %3 bytes, exceed file size"))
            .arg(e_shnum).arg(e_shentsize);
        return Corrupt;
    }

    quint64 soff = e_shoff + e_shentsize * e_shtrndx;

//    if ((soff + e_shentsize) > fdlen || soff % 4 || soff == 0) {
//        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
//           .arg(m_binary)
//           .arg(QLatin1String("shstrtab section header seems to be at %1"))
//           .arg(QString::number(soff, 16));
//        return Corrupt;
//    }

    if (e_shoff) {
        ElfSectionHeader strtab;
        parseSectionHeader(mapper.ustart + soff, &strtab, m_elfData);
        const quint64 stringTableFileOffset = strtab.offset;

        if (quint32(stringTableFileOffset + e_shentsize) >= fdlen
                || stringTableFileOffset == 0) {
            m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                .arg(m_binary)
                .arg(QLatin1String("string table seems to be at %1"))
                .arg(QString::number(soff, 16));
            return Corrupt;
        }

        for (int i = 0; i < e_shnum; ++i) {
            const uchar *s = mapper.ustart + e_shoff + i * e_shentsize;
            ElfSectionHeader sh;
            parseSectionHeader(s, &sh, m_elfData);

            if (stringTableFileOffset + sh.index > fdlen) {
                m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                    .arg(m_binary)
                    .arg(QLatin1String("section name %2 of %3 behind end of file"))
                    .arg(i).arg(e_shnum);
                return Corrupt;
            }

            sh.name = mapper.start + stringTableFileOffset + sh.index;
            if (sh.name == ".gdb_index") {
                m_elfData.symbolsType = FastSymbols;
            } else if (sh.name == ".debug_info") {
                m_elfData.symbolsType = PlainSymbols;
            } else if (sh.name == ".gnu_debuglink") {
                m_elfData.debugLink = QByteArray(mapper.start + sh.offset);
                m_elfData.symbolsType = LinkedSymbols;
            } else if (sh.name == ".note.gnu.build-id") {
                m_elfData.symbolsType = BuildIdSymbols;
                if (sh.size > 16)
                    m_elfData.buildId = QByteArray(mapper.start + sh.offset + 16,
                                                   sh.size - 16).toHex();
            }
            m_elfData.sectionHeaders.append(sh);
        }
    }

    if (e_phoff) {
        for (int i = 0; i < e_phnum; ++i) {
            const uchar *s = mapper.ustart + e_phoff + i * e_phentsize;
            ElfProgramHeader ph;
            parseProgramHeader(s, &ph, m_elfData);
            m_elfData.programHeaders.append(ph);
        }
    }
    return Ok;
}

QByteArray ElfReader::readSection(const QByteArray &name)
{
    readIt();
    int i = m_elfData.indexOf(name);
    if (i == -1)
        return QByteArray();

    ElfMapper mapper(this);
    if (!mapper.map())
        return QByteArray();

    const ElfSectionHeader &section = m_elfData.sectionHeaders.at(i);
    return QByteArray(mapper.start + section.offset, section.size);
}

QByteArray ElfReader::readCoreName(bool *isCore)
{
    *isCore = false;

    readIt();

    ElfMapper mapper(this);
    if (!mapper.map())
        return QByteArray();

    if (m_elfData.elftype != Elf_ET_CORE)
        return QByteArray();

    *isCore = true;

    for (int i = 0, n = m_elfData.sectionHeaders.size(); i != n; ++i)
        if (m_elfData.sectionHeaders.at(i).type == Elf_SHT_NOTE) {
            const ElfSectionHeader &header = m_elfData.sectionHeaders.at(i);
            const char *s = mapper.start + header.offset + 0x40;
            return QByteArray(s);
        }

    for (int i = 0, n = m_elfData.programHeaders.size(); i != n; ++i)
        if (m_elfData.programHeaders.at(i).type == Elf_PT_NOTE) {
            const ElfProgramHeader &header = m_elfData.programHeaders.at(i);
            const char *s = mapper.start + header.offset + 0xec;
            return QByteArray(s);
        }

    return QByteArray();
}

int ElfData::indexOf(const QByteArray &name) const
{
    for (int i = 0, n = sectionHeaders.size(); i != n; ++i)
        if (sectionHeaders.at(i).name == name)
            return i;
    return -1;
}

} // namespace Utils
