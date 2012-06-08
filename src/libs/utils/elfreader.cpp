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

typedef quint16  qelfhalf_t;
typedef quint32  qelfword_t;
typedef quintptr qelfoff_t;
typedef quintptr qelfaddr_t;


template <typename T>
T get(const unsigned char *s, ElfEndian endian)
{
    if (endian == ElfBigEndian)
        return qFromBigEndian<T>(s);
    else
        return qFromLittleEndian<T>(s);
}

class RawElfSectionHeader
{
public:
    qelfword_t name;
    qelfword_t type;
    qelfoff_t  data;
    qelfoff_t  offset;
    qelfoff_t  size;
};

static void parseSectionHeader(const uchar *data, RawElfSectionHeader *sh, ElfEndian endian)
{
    sh->name = get<qelfword_t>(data, endian);
    data += sizeof(qelfword_t); // sh_name
    sh->type = get<qelfword_t>(data, endian);
    data += sizeof(qelfword_t); // sh_type
    data += sizeof(qelfaddr_t);  // sh_flags
    sh->data = get<qelfaddr_t>(data, endian);
    data += sizeof(qelfaddr_t); // sh_addr
    sh->offset = get<qelfoff_t>(data, endian);
    data += sizeof(qelfoff_t);  // sh_offset
    sh->size = get<qelfoff_t>(data, endian);
    data += sizeof(qelfoff_t);  // sh_size
}

class RawElfProgramHeader
{
public:
    qelfword_t type;
    qelfoff_t  offset;
    qelfword_t filesz;
    qelfword_t memsz;
};

static void parseProgramHeader(const uchar *data, RawElfProgramHeader *sh, ElfEndian endian)
{
    sh->type = get<qelfword_t>(data, endian);
    data += sizeof(qelfword_t); // p_type
    sh->offset = get<qelfoff_t>(data, endian);
    data += sizeof(qelfoff_t);  // p_offset
    data += sizeof(qelfaddr_t); // p_vaddr
    data += sizeof(qelfaddr_t); // p_paddr
    sh->filesz = get<qelfword_t>(data, endian);
    data += sizeof(qelfword_t); // p_filesz
    sh->memsz = get<qelfword_t>(data, endian);
    data += sizeof(qelfword_t); // p_memsz
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

    const uchar *data = mapper.ustart;
    if (strncmp((const char *)data, "\177ELF", 4) != 0) {
        m_errorString = QLibrary::tr("'%1' is not an ELF object")
                .arg(m_binary);
        return NotElf;
    }

    // 32 or 64 bit
    if (data[4] != 1 && data[4] != 2) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("odd cpu architecture"));
        return Corrupt;
    }
    m_elfData.elfclass = (data[4] == 1 ? Elf_ELFCLASS32 : Elf_ELFCLASS64);

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
    if (data[5] == 0) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("odd endianess"));
        return Corrupt;
    }
    m_elfData.endian = (data[5] == 1 ? ElfLittleEndian : ElfBigEndian);

    data += 16;                 // e_ident
    m_elfData.elftype = ElfType(get<qelfhalf_t>(data, m_elfData.endian));
    data += sizeof(qelfhalf_t); // e_type

    m_elfData.elfmachine = ElfMachine(get<qelfhalf_t>(data, m_elfData.endian));
    data += sizeof(qelfhalf_t); // e_machine

    data += sizeof(qelfword_t); // e_version
    data += sizeof(qelfaddr_t); // e_entry

    qelfoff_t e_phoff = get<qelfoff_t>(data, m_elfData.endian);
    data += sizeof(qelfoff_t);  // e_phoff

    qelfoff_t e_shoff = get<qelfoff_t>(data, m_elfData.endian);
    data += sizeof(qelfoff_t);  // e_shoff
    data += sizeof(qelfword_t); // e_flags

    qelfhalf_t e_shsize = get<qelfhalf_t>(data, m_elfData.endian);
    data += sizeof(qelfhalf_t); // e_ehsize

    if (e_shsize > fdlen) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("unexpected e_shsize"));
        return Corrupt;
    }

    qelfhalf_t e_phentsize = get<qelfhalf_t>(data, m_elfData.endian);
    data += sizeof(qelfhalf_t); // e_phentsize

    qelfhalf_t e_phnum = get<qelfhalf_t>(data, m_elfData.endian);
    data += sizeof(qelfhalf_t); // e_phnum

    qelfhalf_t e_shentsize = get<qelfhalf_t>(data, m_elfData.endian);
    data += sizeof(qelfhalf_t); // e_shentsize

    if (e_shentsize % 4) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("unexpected e_shentsize"));
        return Corrupt;
    }

    qelfhalf_t e_shnum     = get<qelfhalf_t>(data, m_elfData.endian);
    data += sizeof(qelfhalf_t); // e_shnum
    qelfhalf_t e_shtrndx   = get<qelfhalf_t>(data, m_elfData.endian);
    data += sizeof(qelfhalf_t); // e_shtrndx

    if (quint32(e_shnum * e_shentsize) > fdlen) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary)
            .arg(QLatin1String("announced %2 sections, each %3 bytes, exceed file size"))
            .arg(e_shnum).arg(e_shentsize);
        return Corrupt;
    }

    qulonglong soff = e_shoff + e_shentsize * (e_shtrndx);

//    if ((soff + e_shentsize) > fdlen || soff % 4 || soff == 0) {
//        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
//           .arg(m_binary)
//           .arg(QLatin1String("shstrtab section header seems to be at %1"))
//           .arg(QString::number(soff, 16));
//        return Corrupt;
//    }

    if (e_shoff) {
        RawElfSectionHeader strtab;
        parseSectionHeader(mapper.ustart + soff, &strtab, m_elfData.endian);
        const int stringTableFileOffset = strtab.offset;

        if (quint32(stringTableFileOffset + e_shentsize) >= fdlen
                || stringTableFileOffset == 0) {
            m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                .arg(m_binary)
                .arg(QLatin1String("string table seems to be at %1"))
                .arg(QString::number(soff, 16));
            return Corrupt;
        }

        const uchar *s = mapper.ustart + e_shoff;
        for (int i = 0; i < e_shnum; ++i) {
            RawElfSectionHeader sh;
            parseSectionHeader(s, &sh, m_elfData.endian);
            if (sh.name == 0) {
                s += e_shentsize;
                continue;
            }

            if (stringTableFileOffset + sh.name > fdlen) {
                m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                    .arg(m_binary)
                    .arg(QLatin1String("section name %2 of %3 behind end of file"))
                    .arg(i).arg(e_shnum);
                return Corrupt;
            }

            ElfSectionHeader header;
            header.name = mapper.start + stringTableFileOffset + sh.name;
            header.index = sh.name;
            header.offset = sh.offset;
            header.size = sh.size;
            header.data = sh.data;
            if (header.name == ".gdb_index") {
                m_elfData.symbolsType = FastSymbols;
            } else if (header.name == ".debug_info") {
                m_elfData.symbolsType = PlainSymbols;
            } else if (header.name == ".gnu_debuglink") {
                m_elfData.debugLink = QByteArray(mapper.start + header.offset);
                m_elfData.symbolsType = LinkedSymbols;
            } else if (header.name == ".note.gnu.build-id") {
                m_elfData.symbolsType = BuildIdSymbols;
                if (header.size > 16)
                    m_elfData.buildId = QByteArray(mapper.start + header.offset + 16,
                                                   header.size - 16).toHex();
            }
            m_elfData.sectionHeaders.append(header);

            s += e_shentsize;
        }
    }

    if (e_phoff) {
        const uchar *s = mapper.ustart + e_phoff;

        for (int i = 0; i < e_phnum; ++i) {
            RawElfProgramHeader ph;
            parseProgramHeader(s, &ph, m_elfData.endian);

            ElfProgramHeader header;
            header.type = ph.type;
            header.offset = ph.offset;
            header.filesz = ph.filesz;
            header.memsz = ph.memsz;

            m_elfData.programHeaders.append(header);
            s += e_phentsize;
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
    return QByteArray((const char *)mapper.ustart + section.offset, section.size);
}

QByteArray ElfReader::readCoreName(bool *isCore)
{
    *isCore = false;

    readIt();

    ElfMapper mapper(this);
    if (!mapper.map())
        return QByteArray();

    *isCore = (m_elfData.elftype == Elf_ET_CORE);
    if (!*isCore)
        return QByteArray();

    for (int i = 0, n = m_elfData.sectionHeaders.size(); i != n; ++i)
        if (m_elfData.sectionHeaders.at(i).type == Elf_PT_NOTE) {
            const ElfSectionHeader &header = m_elfData.sectionHeaders.at(i);
            const char *s = mapper.start + header.offset + 0x30; // short name
            const char *t = s + strlen(s) + 1; // command line
            int n = strlen(t);
            return QByteArray(t, n);
        }

    for (int i = 0, n = m_elfData.programHeaders.size(); i != n; ++i)
        if (m_elfData.programHeaders.at(i).type == Elf_PT_NOTE) {
            const ElfProgramHeader &header = m_elfData.programHeaders.at(i);
            const char *s = mapper.start + header.offset;
            QByteArray ba(s + 0xec);
            return ba;
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
