// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "bitbuffer.h"
#include "fieldvalue.h"

#include "../schema/compoundfieldclasses.h"
#include "../schema/fieldclass.h"
#include "../schema/fieldlocation.h"
#include "../schema/stringfieldclasses.h"

#include <utils/result.h>

namespace CommonTraceFormat {

// Recursively decodes BitBuffer data into FieldValue trees, driven by schema.
class CMNTRCEFMT_EXPORT FieldReader
{
public:
    // `currentOrigin` identifies which root structure this reader decodes, so a
    // field location whose `origin` names that same root is resolved against the
    // in-progress scope stack rather than the (not-yet-complete) external roots.
    explicit FieldReader(
        BitBuffer &buf,
        FieldLocationResolver resolver = {},
        FieldLocation::Origin currentOrigin = FieldLocation::Origin::EventRecordPayload);

    Utils::Result<FieldValue> read(const FieldClass &fc);

    // On-wire clock length L (spec 6.3) for each top-level member of the root
    // structure: the `length` property for fixed-length unsigned integers, or
    // S×7 for variable-length unsigned integers where S is the actual on-wire
    // byte count. Populated as the root structure is read.
    const QHash<QString, int> &rootMemberClockBits() const { return m_rootMemberClockBits; }

private:
    Utils::Result<FieldValue> readStructure(const StructureFC &fc);
    Utils::Result<FieldValue> readStaticArray(const StaticLengthArrayFC &fc);
    Utils::Result<FieldValue> readDynamicArray(const DynamicLengthArrayFC &fc);
    Utils::Result<FieldValue> readVariant(const VariantFC &fc);
    Utils::Result<FieldValue> readOptional(const OptionalFC &fc);
    Utils::Result<QString> readNullTerminatedString(StringEncoding enc);
    Utils::Result<QString> readLengthPrefixedString(qsizetype length, StringEncoding enc);

    // A located length/selector field's value, plus how to interpret it: whether
    // the field was a boolean (so optional fields can distinguish a boolean
    // selector from an integer selector at runtime, per spec 6.4.19) and whether
    // it was a signed integer (so selector ranges spanning negative values are
    // compared in the signed domain, per spec 5.3.22/5.3.23). The raw bits are
    // always carried as quint64; `isSigned` says to reinterpret them as qint64.
    struct LocatedValue
    {
        quint64 value = 0;
        bool isBoolean = false;
        bool isSigned = false;
    };

    // Field location procedure (spec 6.4.2). Resolves a length/selector field,
    // or returns an error (which aborts decoding).
    Utils::Result<LocatedValue> resolveLocation(const FieldLocation &loc);
    Utils::Result<LocatedValue> walkLocation(
        int startScope, const QList<std::optional<QString>> &path);
    static Utils::Result<LocatedValue> scalarValue(const FieldValue &fv);

    BitBuffer &m_buf;
    FieldLocationResolver m_resolver;
    FieldLocation::Origin m_currentOrigin;

    // Stack of structures currently being decoded. Each scope points at its
    // in-progress StructureValue, its parent scope, and the member name that
    // labels it within that parent (for the field location procedure's path
    // traversal and `null` parent navigation).
    struct Scope
    {
        StructureValue *sv = nullptr;
        int parent = -1;
        QString memberInParent;
    };
    QList<Scope> m_scopes;
    // Member name to attach to the next structure scope pushed (set by the
    // enclosing structure/array/variant/optional before decoding that member).
    QString m_pendingChildMember;
    // On-wire byte count of the most recently decoded variable-length integer.
    int m_lastVlByteCount = 0;

    QHash<QString, int> m_rootMemberClockBits;
};

} // namespace CommonTraceFormat
