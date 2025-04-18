// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "elfreader.h"

#include "expected.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QDir>
#include <QtEndian>

#include <new> // std::bad_alloc

namespace Utils {

static quint16 getHalfWord(const unsigned char *&s, const ElfData &context)
{
    quint16 res;
    if (context.endian == Elf_ELFDATA2MSB)
        res = qFromBigEndian<quint16>(s);
    else
        res = qFromLittleEndian<quint16>(s);
    s += 2;
    return res;
}

static quint32 getWord(const unsigned char *&s, const ElfData &context)
{
    quint32 res;
    if (context.endian == Elf_ELFDATA2MSB)
        res = qFromBigEndian<quint32>(s);
    else
        res = qFromLittleEndian<quint32>(s);
    s += 4;
    return res;
}

static quint64 getAddress(const unsigned char *&s, const ElfData &context)
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

static quint64 getOffset(const unsigned char *&s, const ElfData &context)
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

// static void parseProgramHeader(const uchar *s, ElfProgramHeader *sh, const ElfData &context)
// {
//     sh->type = getWord(s, context);
//     sh->offset = getOffset(s, context);
//     /* p_vaddr = */ getAddress(s, context);
//     /* p_paddr = */ getAddress(s, context);
//     sh->filesz = getWord(s, context);
//     sh->memsz = getWord(s, context);
// }

ElfMapper::ElfMapper(const ElfReader *reader)
    : binary(reader->m_binary)
{}

bool ElfMapper::map()
{
    if (!binary.isLocal()) {
        const Result<QByteArray> contents = binary.fileContents();
        QTC_CHECK(contents);
        raw = contents.value_or(QByteArray());
        start = raw.constData();
        fdlen = raw.size();
        return fdlen > 0;
    }

    file.setFileName(binary.path());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    fdlen = file.size();
    ustart = file.map(0, fdlen);
    if (ustart == nullptr) {
        // Try reading the data into memory instead.
        try {
            raw = file.readAll();
        } catch (const std::bad_alloc &) {
            return false;
        }
        start = raw.constData();
        fdlen = raw.size();
    }
    return true;
}

ElfReader::ElfReader(const FilePath &binary)
    : m_binary(binary)
{
}

ElfData ElfReader::readHeaders()
{
    readIt();
    return m_elfData;
}

static QString msgInvalidElfObject(const FilePath &binary, const QString &why)
{
    return Tr::tr("\"%1\" is an invalid ELF object (%2)")
           .arg(binary.toUserOutput(), why);
}

ElfReader::Result ElfReader::readIt()
{
    if (!m_elfData.sectionHeaders.isEmpty())
        return Ok;
    // if (!m_elfData.programHeaders.isEmpty())
    //     return Ok;

    ElfMapper mapper(this);
    if (!mapper.map())
        return Corrupt;

    const quint64 fdlen = mapper.fdlen;

    if (fdlen < 64) {
        m_errorString = Tr::tr("\"%1\" is not an ELF object (file too small)").arg(m_binary.toUserOutput());
        return NotElf;
    }

    if (strncmp(mapper.start, "\177ELF", 4) != 0) {
        m_errorString = Tr::tr("\"%1\" is not an ELF object").arg(m_binary.toUserOutput());
        return NotElf;
    }

    // 32 or 64 bit
    m_elfData.elfclass = ElfClass(mapper.start[4]);
    const bool is64Bit = m_elfData.elfclass == Elf_ELFCLASS64;
    if (m_elfData.elfclass != Elf_ELFCLASS32 && m_elfData.elfclass != Elf_ELFCLASS64) {
        m_errorString = msgInvalidElfObject(m_binary, Tr::tr("odd cpu architecture"));
        return Corrupt;
    }

    // int bits = (data[4] << 5);
    // If you remove this check to read ELF objects of a different arch,
    // please make sure you modify the typedefs
    // to match the _plugin_ architecture.
    // if ((sizeof(void*) == 4 && bits != 32)
    //     || (sizeof(void*) == 8 && bits != 64)) {
    //     if (errorString)
    //         *errorString = QLibrary::Tr::tr("\"%1\" is an invalid ELF object (%2)")
    //         .arg(m_binary).arg(QLatin1String("wrong cpu architecture"));
    //     return Corrupt;
    // }

    // Read Endianess.
    m_elfData.endian = ElfEndian(mapper.ustart[5]);
    if (m_elfData.endian != Elf_ELFDATA2LSB && m_elfData.endian != Elf_ELFDATA2MSB) {
        m_errorString = msgInvalidElfObject(m_binary, Tr::tr("odd endianness"));
        return Corrupt;
    }

    const uchar *data = mapper.ustart + 16; // e_ident
    m_elfData.elftype    = ElfType(getHalfWord(data, m_elfData));
    m_elfData.elfmachine = ElfMachine(getHalfWord(data, m_elfData));
    /* e_version = */   getWord(data, m_elfData);
    m_elfData.entryPoint = getAddress(data, m_elfData);

    /* quint64 e_phoff = */ getOffset(data, m_elfData);
    quint64 e_shoff   = getOffset(data, m_elfData);
    /* e_flags = */     getWord(data, m_elfData);

    quint32 e_shsize  = getHalfWord(data, m_elfData);

    if (e_shsize > fdlen) {
        m_errorString = msgInvalidElfObject(m_binary, Tr::tr("unexpected e_shsize"));
        return Corrupt;
    }

    quint32 e_phentsize = getHalfWord(data, m_elfData);
    QTC_CHECK(e_phentsize == (is64Bit ? 56 : 32));
    /* quint32 e_phnum =  */ getHalfWord(data, m_elfData);

    quint32 e_shentsize = getHalfWord(data, m_elfData);

    if (e_shentsize % 4) {
        m_errorString = msgInvalidElfObject(m_binary, Tr::tr("unexpected e_shentsize"));
        return Corrupt;
    }

    quint32 e_shnum     = getHalfWord(data, m_elfData);
    quint32 e_shtrndx   = getHalfWord(data, m_elfData);
    QTC_CHECK(data == mapper.ustart + (is64Bit ? 64 : 52));

    if (quint64(e_shnum) * e_shentsize > fdlen) {
        const QString reason = Tr::tr("announced %n sections, each %1 bytes, exceed file size", nullptr, e_shnum)
                               .arg(e_shentsize);
        m_errorString = msgInvalidElfObject(m_binary, reason);
        return Corrupt;
    }

    quint64 soff = e_shoff + e_shentsize * e_shtrndx;

//    if ((soff + e_shentsize) > fdlen || soff % 4 || soff == 0) {
//        m_errorString = QLibrary::Tr::tr("\"%1\" is an invalid ELF object (%2)")
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
            const QString reason = Tr::tr("string table seems to be at 0x%1").arg(soff, 0, 16);
            m_errorString = msgInvalidElfObject(m_binary, reason);
            return Corrupt;
        }

        for (quint32 i = 0; i < e_shnum; ++i) {
            const uchar *s = mapper.ustart + e_shoff + i * e_shentsize;
            ElfSectionHeader sh;
            parseSectionHeader(s, &sh, m_elfData);

            if (stringTableFileOffset + sh.index > fdlen) {
                const QString reason = Tr::tr("section name %1 of %2 behind end of file")
                                       .arg(i).arg(e_shnum);
                m_errorString = msgInvalidElfObject(m_binary, reason);
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

    // if (e_phoff) {
    //     for (quint32 i = 0; i < e_phnum; ++i) {
    //         const uchar *s = mapper.ustart + e_phoff + i * e_phentsize;
    //         ElfProgramHeader ph;
    //         parseProgramHeader(s, &ph, m_elfData);
    //         m_elfData.programHeaders.append(ph);
    //     }
    // }
    return Ok;
}

std::unique_ptr<ElfMapper> ElfReader::readSection(const QByteArray &name)
{
    std::unique_ptr<ElfMapper> mapper;
    readIt();
    int i = m_elfData.indexOf(name);
    if (i == -1)
        return mapper;

    mapper.reset(new ElfMapper(this));
    if (!mapper->map())
        return mapper;

    const ElfSectionHeader &section = m_elfData.sectionHeaders.at(i);
    mapper->start += section.offset;
    mapper->fdlen = section.size;
    return mapper;
}

// static QByteArray cutout(const char *s)
// {
//     QByteArray res(s, 80);
//     const int pos = res.indexOf('\0');
//     if (pos != -1)
//         res.resize(pos - 1);
//     return res;
// }

// QByteArray ElfReader::readCoreName(bool *isCore)
// {
//     *isCore = false;

//     readIt();

//     ElfMapper mapper(this);
//     if (!mapper.map())
//         return QByteArray();

//     if (m_elfData.elftype != Elf_ET_CORE)
//         return QByteArray();

//     *isCore = true;

//     for (int i = 0, n = m_elfData.sectionHeaders.size(); i != n; ++i)
//         if (m_elfData.sectionHeaders.at(i).type == Elf_SHT_NOTE) {
//             const ElfSectionHeader &header = m_elfData.sectionHeaders.at(i);
//             return cutout(mapper.start + header.offset + 0x40);
//         }

//     for (int i = 0, n = m_elfData.programHeaders.size(); i != n; ++i)
//         if (m_elfData.programHeaders.at(i).type == Elf_PT_NOTE) {
//             const ElfProgramHeader &header = m_elfData.programHeaders.at(i);
//             return cutout(mapper.start + header.offset + 0xec);
//         }

//     return QByteArray();
// }

int ElfData::indexOf(const QByteArray &name) const
{
    for (int i = 0, n = sectionHeaders.size(); i != n; ++i)
        if (sectionHeaders.at(i).name == name)
            return i;
    return -1;
}

} // namespace Utils
