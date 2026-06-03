// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "bitbuffer.h"
#include "fieldvalue.h"

#include "../schema/compoundfieldclasses.h"
#include "../schema/fieldclass.h"
#include "../schema/fieldlocation.h"

namespace CommonTraceFormat {

// Resolves an anterior field value by its FieldLocation.
// Returned as quint64 for numeric selectors/lengths.
// Recursively encodes FieldValue trees into a BitBuffer driven by the schema.
class CMNTRCEFMT_EXPORT FieldWriter
{
public:
    explicit FieldWriter(BitBuffer &buf, FieldLocationResolver resolver = {});

    void write(const FieldClass &fc, const FieldValue &value);

private:
    void writeStructure(const StructureFC &fc, const StructureValue &value);
    void writeStaticArray(const StaticLengthArrayFC &fc, const ArrayValue &value);
    void writeDynamicArray(const DynamicLengthArrayFC &fc, const ArrayValue &value);
    void writeVariant(const VariantFC &fc, const VariantValue &value);
    void writeOptional(const OptionalFC &fc, const FieldValue &value);

    BitBuffer &m_buf;
    FieldLocationResolver m_resolver;
};

} // namespace CommonTraceFormat
