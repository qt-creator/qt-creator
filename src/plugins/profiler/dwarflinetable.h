// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <QByteArrayView>
#include <QList>
#include <QString>

#include <optional>

namespace Profiler::Internal {

struct SourceLocation
{
    QString file;
    int line = 0;
};

// A merged, address-sorted view of every line-number program in one module's
// ".debug_line" section (DWARF versions 2-5), resolving an address (using
// whatever convention the caller's own symbol table already uses -- see
// perfrecordreader.cpp's ELF file-offset addresses) to the source file/line
// the compiler attributed it to.
//
// Deliberately narrow: no column, no is_stmt/basic-block/ISA state, and no
// inline-frame expansion (that needs DW_TAG_inlined_subroutine from
// .debug_info, which this class never reads -- a real unwinder concern, out
// of scope for a frame-pointer-only sampler; see perfrecordreader.cpp). A
// compilation unit whose line-table header uses a form or version this
// parser doesn't recognize is simply skipped (contributes no rows) rather
// than aborting the whole section: this is a best-effort enrichment on top
// of already-working ELF symbol resolution, and a partial miss must never
// fail a capture.
class PROFILER_EXPORT DwarfLineTable
{
public:
    // `debugLine` is the raw ".debug_line" section contents. `debugLineStr`
    // and `debugStr` back DW_FORM_line_strp (DWARF5) and DW_FORM_strp
    // (DWARF2-4) file-name references respectively, and may be empty if the
    // module has neither section.
    static DwarfLineTable parse(QByteArrayView debugLine, QByteArrayView debugLineStr,
                                QByteArrayView debugStr);

    bool isEmpty() const { return m_rows.isEmpty(); }

    // The source location covering `address`, or nullopt if no compilation
    // unit's line program covers it (including: address falls in a gap
    // between two DW_LNE_end_sequence-terminated ranges).
    std::optional<SourceLocation> lookup(quint64 address) const;

private:
    struct Row
    {
        quint64 address = 0;
        int fileId = -1; // index into m_files; -1 marks an end-of-sequence row
        int line = 0;
    };

    QList<Row> m_rows; // sorted by address
    QList<QString> m_files;
};

} // namespace Profiler::Internal
