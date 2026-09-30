// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dwarflinetable.h"

#include <commontraceformat/util/leb128.h>

#include <algorithm>
#include <span>

namespace Profiler::Internal {

namespace {

// A cursor over one bounded byte range that turns "ran out of bytes" into a
// sticky failure flag instead of a crash: once any read fails, every further
// read is a no-op returning 0/empty, so callers can do a batch of reads and
// check ok() once instead of after every field -- exactly what a line-table
// header (a long flat sequence of fields) wants.
class Reader
{
public:
    Reader(const char *begin, const char *end) : m_pos(begin), m_end(end) {}

    bool ok() const { return m_ok; }
    const char *pos() const { return m_pos; }
    const char *end() const { return m_end; }
    void seek(const char *p) { m_pos = p; }
    bool atEnd() const { return m_pos >= m_end; }

    quint8 u8() { return takeInt<quint8>(1); }
    quint16 u16() { return takeInt<quint16>(2); }
    quint32 u32() { return takeInt<quint32>(4); }
    quint64 u64() { return takeInt<quint64>(8); }
    qint8 s8() { return qint8(u8()); }

    quint64 uleb()
    {
        if (!m_ok)
            return 0;
        const auto [value, n] = CommonTraceFormat::decodeULeb128(
            std::span<const char>(m_pos, std::size_t(m_end - m_pos)));
        if (n < 0) {
            m_ok = false;
            return 0;
        }
        m_pos += n;
        return value;
    }

    qint64 sleb()
    {
        if (!m_ok)
            return 0;
        const auto [value, n] = CommonTraceFormat::decodeSLeb128(
            std::span<const char>(m_pos, std::size_t(m_end - m_pos)));
        if (n < 0) {
            m_ok = false;
            return 0;
        }
        m_pos += n;
        return value;
    }

    // A NUL-terminated string; consumes through (and including) the NUL.
    QString cstring()
    {
        if (!m_ok)
            return {};
        const char *nul = m_pos;
        while (nul < m_end && *nul != '\0')
            ++nul;
        if (nul >= m_end) {
            m_ok = false;
            return {};
        }
        const QString s = QString::fromUtf8(m_pos, int(nul - m_pos));
        m_pos = nul + 1;
        return s;
    }

    void skip(qint64 n)
    {
        // n < 0 covers a huge, corrupt-input-controlled length that wrapped
        // negative when a caller cast a decoded quint64 down to qint64.
        if (!m_ok || n < 0 || m_end - m_pos < n) {
            m_ok = false;
            return;
        }
        m_pos += n;
    }

private:
    template<typename T> T takeInt(qsizetype n)
    {
        if (!m_ok || m_end - m_pos < n) {
            m_ok = false;
            return 0;
        }
        T v = 0;
        for (qsizetype i = 0; i < n; ++i)
            v |= T(uchar(m_pos[i])) << (8 * i);
        m_pos += n;
        return v;
    }

    const char *m_pos;
    const char *m_end;
    bool m_ok = true;
};

// DW_FORM_* codes this reader understands (DWARF5 spec, section 7.5.6).
enum : quint64 {
    FormData2 = 0x05,
    FormData4 = 0x06,
    FormData8 = 0x07,
    FormString = 0x08,
    FormBlock = 0x09,
    FormData1 = 0x0b,
    FormStrp = 0x0e,
    FormUdata = 0x0f,
    FormData16 = 0x1e,
    FormLineStrp = 0x1f,
};

// DW_LNCT_* content-type codes (DWARF5's line-header directory/file tables).
enum : quint64 {
    LnctPath = 0x1,
    LnctDirectoryIndex = 0x2,
};

QString readCStringAt(QByteArrayView section, quint32 offset)
{
    if (offset >= quint32(section.size()))
        return {};
    const char *start = section.data() + offset;
    const char *end = section.data() + section.size();
    const char *nul = start;
    while (nul < end && *nul != '\0')
        ++nul;
    return QString::fromUtf8(start, int(nul - start));
}

struct FormValue
{
    QString text;
    quint64 number = 0;
};

// Reads one entry-format value. Only DW_FORM_string/strp/line_strp (for
// DW_LNCT_path) and DW_FORM_udata/data1/2/4/8 (for DW_LNCT_directory_index)
// ever populate `out`; every other recognized form's bytes are still
// correctly consumed so the cursor stays in sync with the caller's format
// list. Returns false for a form outside this set, since the header can no
// longer be parsed reliably past that point.
bool readFormValue(Reader &r, quint64 form, QByteArrayView debugLineStr, QByteArrayView debugStr,
                   FormValue *out)
{
    switch (form) {
    case FormString:
        out->text = r.cstring();
        return r.ok();
    case FormStrp: {
        const quint32 offset = r.u32();
        if (!r.ok())
            return false;
        out->text = readCStringAt(debugStr, offset);
        return true;
    }
    case FormLineStrp: {
        const quint32 offset = r.u32();
        if (!r.ok())
            return false;
        out->text = readCStringAt(debugLineStr, offset);
        return true;
    }
    case FormUdata:
        out->number = r.uleb();
        return r.ok();
    case FormData1:
        out->number = r.u8();
        return r.ok();
    case FormData2:
        out->number = r.u16();
        return r.ok();
    case FormData4:
        out->number = r.u32();
        return r.ok();
    case FormData8:
        out->number = r.u64();
        return r.ok();
    case FormData16:
        r.skip(16); // an MD5 checksum: irrelevant here, but must be skipped
        return r.ok();
    case FormBlock: {
        const quint64 len = r.uleb();
        r.skip(qint64(len));
        return r.ok();
    }
    default:
        return false; // an unrecognized form: give up on this unit's header
    }
}

QString joinPath(const QString &dir, const QString &file)
{
    if (file.isEmpty() || file.startsWith(u'/') || dir.isEmpty())
        return file;
    return dir + u'/' + file;
}

struct EntryFormat
{
    quint64 contentType;
    quint64 form;
};

// Reads a DWARF5 directory_entry_format/file_name_entry_format table plus its
// entries (DWARF5 spec, 6.2.4.1). `dirIndices`, when non-null, collects each
// entry's DW_LNCT_directory_index value (meaningful for file entries only).
bool readDwarf5Table(Reader &r, QByteArrayView debugLineStr, QByteArrayView debugStr,
                     QList<QString> *paths, QList<quint64> *dirIndices)
{
    const quint8 formatCount = r.u8();
    QList<EntryFormat> formats;
    formats.reserve(formatCount);
    for (int i = 0; i < formatCount; ++i) {
        const quint64 contentType = r.uleb();
        const quint64 form = r.uleb();
        formats.append({contentType, form});
    }
    const quint64 count = r.uleb();
    if (!r.ok())
        return false;
    // Every form takes at least a byte, so an entry without any, or more
    // entries than bytes left, can only come from a corrupt table.
    if (count > 0 && (formats.isEmpty() || count > quint64(r.end() - r.pos())))
        return false;
    paths->reserve(qsizetype(count));
    if (dirIndices)
        dirIndices->reserve(qsizetype(count));
    for (quint64 i = 0; i < count; ++i) {
        QString path;
        quint64 dirIndex = 0;
        for (const EntryFormat &f : std::as_const(formats)) {
            FormValue value;
            if (!readFormValue(r, f.form, debugLineStr, debugStr, &value))
                return false;
            if (f.contentType == LnctPath)
                path = value.text;
            else if (f.contentType == LnctDirectoryIndex)
                dirIndex = value.number;
        }
        paths->append(path);
        if (dirIndices)
            dirIndices->append(dirIndex);
    }
    return r.ok();
}

// DWARF2-4 include_directories: a sequence of non-empty NUL-terminated
// strings, terminated by an empty one.
bool readLegacyDirectories(Reader &r, QList<QString> *dirs)
{
    for (;;) {
        const QString s = r.cstring();
        if (!r.ok())
            return false;
        if (s.isEmpty())
            return true;
        dirs->append(s);
    }
}

// DWARF2-4 file_names: {name, dir_index(uleb), mtime(uleb), length(uleb)},
// terminated by an entry with an empty name.
bool readLegacyFiles(Reader &r, QList<QString> *files, QList<quint64> *dirIndices)
{
    for (;;) {
        const QString name = r.cstring();
        if (!r.ok())
            return false;
        if (name.isEmpty())
            return true;
        const quint64 dirIndex = r.uleb();
        r.uleb(); // mtime
        r.uleb(); // length
        if (!r.ok())
            return false;
        files->append(name);
        dirIndices->append(dirIndex);
    }
}

// One decoded line-table row, before global file-id remapping and sorting
// (see DwarfLineTable::parse()). fileId == -1 marks a DW_LNE_end_sequence row.
struct LineRow
{
    quint64 address = 0;
    int fileId = -1;
    int line = 0;
};

// One compilation unit's line-number-program header and program (DWARF5
// spec, 6.2), decoded into `outRows` (appended) using file paths interned
// into `outFiles`. `r` is bounded to exactly this unit's content (the bytes
// after its length field(s)). Returns false only when the *header* couldn't
// be parsed (unsupported version/form); a malformed program body simply
// stops early via Reader::ok(), keeping whatever rows were already decoded.
bool parseUnitBody(Reader r, QByteArrayView debugLineStr, QByteArrayView debugStr,
                   QList<LineRow> *outRows, QList<QString> *outFiles)
{
    const quint16 version = r.u16();
    if (version >= 5) {
        r.u8(); // address_size: see DW_LNE_set_address below for why this
                // isn't needed -- its own instruction length is used instead.
        r.u8(); // segment_selector_size: segmented addressing isn't
                // supported, but the common case (0) needs no special
                // handling below, so just consume the byte.
    }
    if (!r.ok() || version < 2 || version > 5)
        return false;

    const quint32 headerLength = r.u32();
    if (!r.ok())
        return false;
    // Bounds-checked before forming the pointer (see the identical comment
    // in parse()): headerLength is as attacker/corruption-controlled as any
    // other field here, and this unit's own buffer may be far smaller than
    // a corrupt value could claim.
    if (quint64(headerLength) > quint64(r.end() - r.pos()))
        return false;
    const char *programStart = r.pos() + headerLength;

    const quint8 minInstructionLength = r.u8();
    if (version >= 4)
        r.u8(); // maximum_operations_per_instruction: VLIW targets aren't
                // supported (see the class doc comment), so this is unused.
    r.u8();               // default_is_stmt: unused, this reader ignores is_stmt entirely.
    const qint8 lineBase = r.s8();
    const quint8 lineRange = r.u8();
    const quint8 opcodeBase = r.u8();
    if (!r.ok() || lineRange == 0 || opcodeBase == 0)
        return false;
    QList<quint8> standardOpcodeLengths;
    standardOpcodeLengths.reserve(opcodeBase - 1);
    for (int i = 1; i < opcodeBase; ++i)
        standardOpcodeLengths.append(r.u8());
    if (!r.ok())
        return false;

    // File table: local (per-unit) index -> resolved, directory-joined path.
    // DWARF5's default `file` register value (1) is treated below as a
    // direct, unadjusted index into this table -- GCC and Clang duplicate
    // file_names[0] into [1] specifically so that works without an explicit
    // DW_LNS_set_file, which is what they actually emit in practice.
    QList<QString> localFiles;
    const bool isDwarf5 = version >= 5;
    if (isDwarf5) {
        QList<QString> dirs;
        if (!readDwarf5Table(r, debugLineStr, debugStr, &dirs, nullptr))
            return false;
        QList<QString> fileNames;
        QList<quint64> fileDirIndices;
        if (!readDwarf5Table(r, debugLineStr, debugStr, &fileNames, &fileDirIndices))
            return false;
        localFiles.reserve(fileNames.size());
        for (int i = 0; i < fileNames.size(); ++i) {
            const quint64 dirIndex = fileDirIndices.at(i);
            const QString dir = dirIndex < quint64(dirs.size()) ? dirs.at(dirIndex) : QString();
            localFiles.append(joinPath(dir, fileNames.at(i)));
        }
    } else {
        QList<QString> dirs;
        if (!readLegacyDirectories(r, &dirs))
            return false;
        QList<QString> fileNames;
        QList<quint64> fileDirIndices;
        if (!readLegacyFiles(r, &fileNames, &fileDirIndices))
            return false;
        localFiles.reserve(fileNames.size());
        for (int i = 0; i < fileNames.size(); ++i) {
            const quint64 dirIndex = fileDirIndices.at(i);
            // Index 0 means "the compilation directory", which is
            // DW_AT_comp_dir in .debug_info -- unavailable here (see the
            // class doc comment) -- so such files fall back to their bare
            // name rather than a guessed directory.
            const QString dir = (dirIndex >= 1 && dirIndex <= quint64(dirs.size()))
                                    ? dirs.at(dirIndex - 1)
                                    : QString();
            localFiles.append(joinPath(dir, fileNames.at(i)));
        }
    }
    if (!r.ok() || programStart < r.pos() || programStart > r.end())
        return false;

    // Intern this unit's files into the shared, global file list, remapping
    // local (per-unit) indices to global ones as rows are emitted below.
    const int fileIdBase = outFiles->size();
    outFiles->append(localFiles);
    const auto globalFileId = [&](quint64 localIndex) -> int {
        const quint64 idx = isDwarf5 ? localIndex : localIndex - 1; // see above
        return idx < quint64(localFiles.size()) ? fileIdBase + int(idx) : -1;
    };

    // The line-number program itself: the state machine from DWARF5 spec
    // section 6.2.5, executed over exactly the bytes the header declared.
    Reader pr(programStart, r.end());
    quint64 address = 0;
    quint64 fileRegister = 1;
    qint64 line = 1;

    const auto appendRow = [&] { outRows->append({address, globalFileId(fileRegister), int(line)}); };
    const auto resetRegisters = [&] {
        address = 0;
        fileRegister = 1;
        line = 1;
    };

    while (pr.ok() && !pr.atEnd()) {
        const quint8 opcode = pr.u8();
        if (!pr.ok())
            break;
        if (opcode == 0) {
            // Extended opcode: a ULEB128 length of everything that follows
            // (including the sub-opcode byte) makes every extended opcode
            // skippable even when its specific meaning isn't recognized.
            const quint64 length = pr.uleb();
            // Bounds-checked before forming the pointer (see the identical
            // comment in parse()): `length` is as corrupt-input-controlled
            // as any other field here.
            if (!pr.ok() || length == 0 || length > quint64(pr.end() - pr.pos()))
                break;
            const char *instrEnd = pr.pos() + length;
            const quint8 subOpcode = pr.u8();
            switch (subOpcode) {
            case 1: // DW_LNE_end_sequence
                outRows->append({address, -1, 0});
                resetRegisters();
                break;
            case 2: { // DW_LNE_set_address: sized by this instruction's own
                      // declared length, not a header address_size field --
                      // works the same whether or not that field exists.
                const qint64 n = instrEnd - pr.pos();
                address = n >= 8 ? pr.u64() : (n >= 4 ? pr.u32() : 0);
                break;
            }
            default:
                break; // DW_LNE_define_file, DW_LNE_set_discriminator, vendor
                       // extensions: nothing here needs their effect.
            }
            pr.seek(instrEnd); // skip anything the case above didn't consume
        } else if (opcode < opcodeBase) {
            switch (opcode) {
            case 1: // DW_LNS_copy
                appendRow();
                break;
            case 2: // DW_LNS_advance_pc
                address += pr.uleb() * minInstructionLength;
                break;
            case 3: // DW_LNS_advance_line
                line += pr.sleb();
                break;
            case 4: // DW_LNS_set_file
                fileRegister = pr.uleb();
                break;
            case 5: // DW_LNS_set_column
                pr.uleb();
                break;
            case 8: // DW_LNS_const_add_pc: same address advance as special
                    // opcode 255, with no line/row change.
                address += ((255 - opcodeBase) / lineRange) * minInstructionLength;
                break;
            case 9: // DW_LNS_fixed_advance_pc: a plain u16, deliberately not
                    // scaled by minInstructionLength (that's the point of
                    // this opcode existing separately from advance_pc).
                address += pr.u16();
                break;
            case 12: // DW_LNS_set_isa
                pr.uleb();
                break;
            case 6:  // DW_LNS_negate_stmt
            case 7:  // DW_LNS_set_basic_block
            case 10: // DW_LNS_set_prologue_end
            case 11: // DW_LNS_set_epilogue_begin
                break;
            default:
                // A standard opcode this reader doesn't special-case: its
                // ULEB128 operand count is declared in the header, so skip
                // exactly that many rather than guessing.
                for (int i = 0, n = standardOpcodeLengths.value(opcode - 1); i < n; ++i)
                    pr.uleb();
                break;
            }
        } else {
            // Special opcode: advances both address and line, then emits a row.
            const quint8 adjusted = opcode - opcodeBase;
            address += (adjusted / lineRange) * minInstructionLength;
            line += lineBase + (adjusted % lineRange);
            appendRow();
        }
    }
    return true;
}

} // namespace

DwarfLineTable DwarfLineTable::parse(QByteArrayView debugLine, QByteArrayView debugLineStr,
                                     QByteArrayView debugStr)
{
    QList<LineRow> rows;
    QList<QString> files;

    if (!debugLine.isEmpty()) {
        Reader reader(debugLine.data(), debugLine.data() + debugLine.size());
        while (!reader.atEnd()) {
            const quint32 rawLength = reader.u32();
            if (!reader.ok())
                break;
            const bool dwarf64 = rawLength == 0xffffffffu;
            const quint64 unitLength = dwarf64 ? reader.u64() : quint64(rawLength);
            if (!reader.ok())
                break;
            const char *unitContentStart = reader.pos();
            // Bounds-checked in the section's own byte-length domain, not by
            // forming the (possibly wildly out-of-range, for corrupt input)
            // pointer first and comparing after -- advancing a pointer past
            // its allocation is undefined behavior even if the result is
            // only ever compared, never dereferenced.
            const quint64 remaining = quint64(reader.end() - unitContentStart);
            if (unitLength == 0 || unitLength > remaining)
                break; // corrupt: the declared length overruns the section
            const char *unitContentEnd = unitContentStart + unitLength;

            // DWARF64 units use 8-byte offsets throughout (header_length,
            // and every DW_FORM_strp/line_strp) instead of the 4-byte ones
            // this parser assumes; skip such a unit rather than misparse it.
            // In practice this is essentially never seen for normal-sized
            // application binaries.
            if (!dwarf64) {
                Reader unitReader(unitContentStart, unitContentEnd);
                parseUnitBody(unitReader, debugLineStr, debugStr, &rows, &files);
            }
            reader.seek(unitContentEnd);
        }
    }

    // At an address where one sequence ends and another starts, the end goes
    // first: lookup() takes the last row at or before an address, and that has
    // to be the start, or the new sequence's first instructions lose their line.
    std::stable_sort(rows.begin(), rows.end(), [](const LineRow &a, const LineRow &b) {
        if (a.address != b.address)
            return a.address < b.address;
        return a.fileId == -1 && b.fileId != -1;
    });

    DwarfLineTable table;
    table.m_files = std::move(files);
    table.m_rows.reserve(rows.size());
    for (const LineRow &row : std::as_const(rows))
        table.m_rows.append({row.address, row.fileId, row.line});
    return table;
}

std::optional<SourceLocation> DwarfLineTable::lookup(quint64 address) const
{
    // The last row at or before `address`.
    auto it = std::upper_bound(m_rows.cbegin(), m_rows.cend(), address,
                               [](quint64 addr, const Row &row) { return addr < row.address; });
    if (it == m_rows.cbegin())
        return std::nullopt;
    --it;
    if (it->fileId < 0 || it->fileId >= m_files.size())
        return std::nullopt; // an end-of-sequence row, or an unresolved file index
    return SourceLocation{m_files.at(it->fileId), it->line};
}

} // namespace Profiler::Internal
