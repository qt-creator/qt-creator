// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include <QJsonObject>

#include <memory>

namespace CommonTraceFormat {

enum class FieldClassKind {
    FixedLengthBitArray,
    FixedLengthBitMap,
    FixedLengthBool,
    FixedLengthUInt,
    FixedLengthSInt,
    FixedLengthFloat,
    VariableLengthUInt,
    VariableLengthSInt,
    NullTerminatedString,
    StaticLengthString,
    DynamicLengthString,
    StaticLengthBlob,
    DynamicLengthBlob,
    Structure,
    StaticLengthArray,
    DynamicLengthArray,
    Variant,
    Optional,
};

struct CMNTRCEFMT_EXPORT FieldClass
{
    FieldClassKind kind;

    // Non-semantic attributes (spec 5.2), preserved for round-tripping. A
    // consumer MUST NOT need these to decode a field.
    QJsonObject attributes;

    explicit FieldClass(FieldClassKind k)
        : kind(k)
    {}
    virtual ~FieldClass() = default;

    FieldClass(const FieldClass &) = delete;
    FieldClass &operator=(const FieldClass &) = delete;
};

using FieldClassPtr = std::shared_ptr<FieldClass>;

} // namespace CommonTraceFormat
