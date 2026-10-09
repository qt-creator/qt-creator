// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perftracepointformats.h"

#include <QtEndian>

#include <cstring>

namespace Profiler::Internal {

namespace {

// Reads perf's tracing data: a sequence of NUL-terminated strings and
// integers of either byte order, as its header says.
class Reader
{
public:
    Reader(const QByteArray &data) : m_data(data) {}

    bool ok() const { return m_ok; }
    void setBigEndian(bool bigEndian) { m_bigEndian = bigEndian; }

    QByteArray bytes(qsizetype size)
    {
        if (!m_ok || size < 0 || size > m_data.size() - m_pos) {
            m_ok = false;
            return {};
        }
        const QByteArray result = m_data.mid(m_pos, size);
        m_pos += size;
        return result;
    }

    QByteArray cstring()
    {
        const qsizetype nul = m_ok ? m_data.indexOf('\0', m_pos) : -1;
        if (nul < 0) {
            m_ok = false;
            return {};
        }
        const QByteArray result = m_data.mid(m_pos, nul - m_pos);
        m_pos = nul + 1;
        return result;
    }

    template<typename T> T integer()
    {
        const QByteArray raw = bytes(sizeof(T));
        if (!m_ok)
            return 0;
        return m_bigEndian ? qFromBigEndian<T>(raw.constData())
                           : qFromLittleEndian<T>(raw.constData());
    }

    // A u64 size, and that many bytes.
    QByteArray sizedBlock() { return bytes(qsizetype(integer<quint64>())); }

private:
    const QByteArray &m_data;
    qsizetype m_pos = 0;
    bool m_ok = true;
    bool m_bigEndian = false;
};

// "field:<declaration>;\toffset:<n>;\tsize:<n>;\tsigned:<0|1>;"
std::optional<TracepointField> parseField(const QByteArray &line)
{
    TracepointField field;
    QByteArray declaration;
    bool haveOffset = false;
    bool haveSize = false;
    for (const QByteArray &part : line.split(';')) {
        const QByteArray item = part.trimmed();
        const qsizetype colon = item.indexOf(':');
        if (colon < 0)
            continue;
        const QByteArray key = item.left(colon).trimmed();
        const QByteArray value = item.mid(colon + 1).trimmed();
        if (key == "field") {
            declaration = value;
        } else if (key == "offset") {
            field.offset = value.toInt(&haveOffset);
        } else if (key == "size") {
            field.size = value.toInt(&haveSize);
        } else if (key == "signed") {
            field.isSigned = value.toInt() != 0;
        }
    }
    if (declaration.isEmpty() || !haveOffset || !haveSize || field.offset < 0 || field.size < 0)
        return std::nullopt;

    // The name is the declaration's last identifier, before any "[n]".
    QByteArray name = declaration;
    const qsizetype bracket = name.indexOf('[');
    const bool isArray = bracket >= 0;
    if (isArray)
        name.truncate(bracket);
    name = name.trimmed();
    const qsizetype space = std::max(name.lastIndexOf(' '), name.lastIndexOf('*'));
    field.name = name.mid(space + 1);
    if (field.name.isEmpty())
        return std::nullopt;

    if (declaration.startsWith("__data_loc"))
        field.kind = TracepointField::DataLoc;
    else if (isArray && (declaration.startsWith("char ") || declaration.startsWith("const char ")))
        field.kind = TracepointField::String;
    else if (isArray)
        field.kind = TracepointField::Bytes;
    else
        field.kind = TracepointField::Number;
    return field;
}

QByteArray untilNul(const QByteArray &bytes)
{
    const qsizetype nul = bytes.indexOf('\0');
    return nul < 0 ? bytes : bytes.left(nul);
}

} // namespace

QList<QPair<QByteArray, QVariant>> TracepointFormat::decode(const QByteArray &record) const
{
    QList<QPair<QByteArray, QVariant>> values;
    const char *data = record.constData();
    const qsizetype size = record.size();
    for (const TracepointField &field : fields) {
        if (field.offset + field.size > size)
            continue;
        const char *p = data + field.offset;
        QVariant value;
        switch (field.kind) {
        case TracepointField::Number:
            switch (field.size) {
            case 1:
                value = field.isSigned ? QVariant(qint64(qint8(*p))) : QVariant(quint64(quint8(*p)));
                break;
            case 2:
                value = field.isSigned ? QVariant(qint64(qFromLittleEndian<qint16>(p)))
                                       : QVariant(quint64(qFromLittleEndian<quint16>(p)));
                break;
            case 4:
                value = field.isSigned ? QVariant(qint64(qFromLittleEndian<qint32>(p)))
                                       : QVariant(quint64(qFromLittleEndian<quint32>(p)));
                break;
            case 8:
                value = field.isSigned ? QVariant(qFromLittleEndian<qint64>(p))
                                       : QVariant(qFromLittleEndian<quint64>(p));
                break;
            default:
                value = QByteArray(p, field.size);
                break;
            }
            break;
        case TracepointField::String:
            value = untilNul(QByteArray(p, field.size));
            break;
        case TracepointField::DataLoc: {
            // The low 16 bits are the offset of the data in the record, the
            // high 16 bits its length.
            if (field.size < 4)
                continue;
            const quint32 loc = qFromLittleEndian<quint32>(p);
            const qsizetype offset = loc & 0xffff;
            const qsizetype length = loc >> 16;
            if (offset + length > size)
                continue;
            value = untilNul(QByteArray(data + offset, length));
            break;
        }
        case TracepointField::Bytes:
            value = QByteArray(p, field.size);
            break;
        }
        values.append({field.name, value});
    }
    return values;
}

std::optional<QPair<quint64, TracepointFormat>> PerfTracepointFormats::parseFormat(
    const QByteArray &system, const QByteArray &text)
{
    TracepointFormat format;
    format.system = system;
    bool haveId = false;
    quint64 id = 0;
    for (const QByteArray &rawLine : text.split('\n')) {
        const QByteArray line = rawLine.trimmed();
        if (line.startsWith("name:")) {
            format.name = line.mid(5).trimmed();
        } else if (line.startsWith("ID:")) {
            id = line.mid(3).trimmed().toULongLong(&haveId);
        } else if (line.startsWith("field:")) {
            if (const std::optional<TracepointField> field = parseField(line)) {
                if (!field->name.startsWith("common_"))
                    format.fields.append(*field);
            }
        }
    }
    if (!haveId || format.name.isEmpty())
        return std::nullopt;
    return QPair<quint64, TracepointFormat>(id, format);
}

void PerfTracepointFormats::parse(const QByteArray &tracingData)
{
    // "\x17\x08\x44tracing"; version; u8 big_endian; u8 long_size; u32
    // page_size; "header_page" and its block; "header_event" and its block;
    // the ftrace formats; the event formats, by system; kallsyms; printk
    // formats; and, from version 0.6, saved_cmdlines.
    Reader r(tracingData);
    static const char magic[] = "\x17\x08\x44tracing";
    if (r.bytes(10) != QByteArray(magic, 10))
        return;
    r.cstring(); // version
    const QByteArray endianness = r.bytes(1);
    if (!r.ok())
        return;
    r.setBigEndian(endianness.at(0) != 0);
    r.bytes(1); // long_size
    r.integer<quint32>(); // page_size

    if (r.cstring() != "header_page")
        return;
    r.sizedBlock();
    if (r.cstring() != "header_event")
        return;
    r.sizedBlock();

    const quint32 ftraceCount = r.integer<quint32>();
    for (quint32 i = 0; i < ftraceCount && r.ok(); ++i) {
        const QByteArray text = r.sizedBlock();
        if (const auto format = parseFormat("ftrace", text))
            m_formats.insert(format->first, format->second);
    }

    const quint32 systems = r.integer<quint32>();
    for (quint32 i = 0; i < systems && r.ok(); ++i) {
        const QByteArray system = r.cstring();
        const quint32 count = r.integer<quint32>();
        for (quint32 j = 0; j < count && r.ok(); ++j) {
            const QByteArray text = r.sizedBlock();
            if (const auto format = parseFormat(system, text))
                m_formats.insert(format->first, format->second);
        }
    }
}

const TracepointFormat *PerfTracepointFormats::format(quint64 id) const
{
    const auto it = m_formats.constFind(id);
    return it == m_formats.constEnd() ? nullptr : &it.value();
}

} // namespace Profiler::Internal
