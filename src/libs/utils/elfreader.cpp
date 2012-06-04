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

class ElfSectionHeader
{
public:
    qelfword_t name;
    qelfword_t type;
    qelfoff_t  offset;
    qelfoff_t  size;
};

template <typename T>
T read(const char *s, ElfReader::ElfEndian endian)
{
    if (endian == ElfReader::ElfBigEndian)
        return qFromBigEndian<T>(reinterpret_cast<const uchar *>(s));
    else
        return qFromLittleEndian<T>(reinterpret_cast<const uchar *>(s));
}

ElfReader::ElfReader(const QString &binary)
    : m_binary(binary)
{
}

const char *ElfReader::parseSectionHeader(const char *data, ElfSectionHeader *sh)
{
    sh->name = read<qelfword_t>(data, m_endian);
    data += sizeof(qelfword_t); // sh_name
    sh->type = read<qelfword_t>(data, m_endian);
    data += sizeof(qelfword_t)  // sh_type
         + sizeof(qelfaddr_t)   // sh_flags
         + sizeof(qelfaddr_t);  // sh_addr
    sh->offset = read<qelfoff_t>(data, m_endian);
    data += sizeof(qelfoff_t);  // sh_offset
    sh->size = read<qelfoff_t>(data, m_endian);
    data += sizeof(qelfoff_t);  // sh_size
    return data;
}

ElfReader::Result ElfReader::parse(const char *dataStart, quint64 fdlen,
    ElfSections *sections)
{
    if (fdlen < 64) {
        m_errorString = QLibrary::tr("'%1' is not an ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("file too small"));
        return NotElf;
    }
    const char *data = dataStart;
    if (strncmp(data, "\177ELF", 4) != 0) {
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
    m_endian = (data[5] == 1 ? ElfLittleEndian : ElfBigEndian);

    data += 16                  // e_ident
         +  sizeof(qelfhalf_t)  // e_type
         +  sizeof(qelfhalf_t)  // e_machine
         +  sizeof(qelfword_t)  // e_version
         +  sizeof(qelfaddr_t)  // e_entry
         +  sizeof(qelfoff_t);  // e_phoff

    qelfoff_t e_shoff = read<qelfoff_t>(data, m_endian);
    data += sizeof(qelfoff_t)    // e_shoff
         +  sizeof(qelfword_t);  // e_flags

    qelfhalf_t e_shsize = read<qelfhalf_t>(data, m_endian);

    if (e_shsize > fdlen) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("unexpected e_shsize"));
        return Corrupt;
    }

    data += sizeof(qelfhalf_t)  // e_ehsize
         +  sizeof(qelfhalf_t)  // e_phentsize
         +  sizeof(qelfhalf_t); // e_phnum

    qelfhalf_t e_shentsize = read<qelfhalf_t>(data, m_endian);

    if (e_shentsize % 4) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary).arg(QLatin1String("unexpected e_shentsize"));
        return Corrupt;
    }
    data += sizeof(qelfhalf_t); // e_shentsize
    qelfhalf_t e_shnum     = read<qelfhalf_t>(data, m_endian);
    data += sizeof(qelfhalf_t); // e_shnum
    qelfhalf_t e_shtrndx   = read<qelfhalf_t>(data, m_endian);
    data += sizeof(qelfhalf_t); // e_shtrndx

    if (quint32(e_shnum * e_shentsize) > fdlen) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary)
            .arg(QLatin1String("announced %2 sections, each %3 bytes, exceed file size"))
            .arg(e_shnum).arg(e_shentsize);
        return Corrupt;
    }

    ElfSectionHeader strtab;
    qulonglong soff = e_shoff + e_shentsize * (e_shtrndx);

    if ((soff + e_shentsize) > fdlen || soff % 4 || soff == 0) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
           .arg(m_binary)
           .arg(QLatin1String("shstrtab section header seems to be at %1"))
           .arg(QString::number(soff, 16));
        return Corrupt;
    }

    parseSectionHeader(dataStart + soff, &strtab);
    const int stringTableFileOffset = strtab.offset;

    if (quint32(stringTableFileOffset + e_shentsize) >= fdlen
            || stringTableFileOffset == 0) {
        m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
            .arg(m_binary)
            .arg(QLatin1String("string table seems to be at %1"))
            .arg(QString::number(soff, 16));
        return Corrupt;
    }

    const char *s = dataStart + e_shoff;
    for (int i = 0; i < e_shnum; ++i) {
        ElfSectionHeader sh;
        parseSectionHeader(s, &sh);
        if (sh.name == 0) {
            s += e_shentsize;
            continue;
        }
        const char *shnam = dataStart + stringTableFileOffset + sh.name;

        if (stringTableFileOffset + sh.name > fdlen) {
            m_errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                .arg(m_binary)
                .arg(QLatin1String("section name %2 of %3 behind end of file"))
                .arg(i).arg(e_shnum);
            return Corrupt;
        }

        if (sections) {
            ElfSection section;
            section.name = shnam;
            section.index = strtab.name;
            section.offset = strtab.offset;
            section.size = strtab.size;
        }

        s += e_shentsize;
    }
    return Ok;
}

ElfSections ElfReader::sections()
{
    ElfSections names;

    QFile file(m_binary);
    if (!file.open(QIODevice::ReadOnly)) {
        m_errorString = file.errorString();
        return names;
    }

    QByteArray data;
    const char *filedata = 0;
    quint64 fdlen = file.size();
    filedata = (char *) file.map(0, fdlen);
    if (filedata == 0) {
        // Try reading the data into memory instead.
        data = file.readAll();
        filedata = data.constData();
        fdlen = data.size();
    }

    parse(filedata, fdlen, &names);
    return names;
}

} // namespace Utils
