// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fieldclass.h"
#include "fieldlocation.h"

#include <QJsonObject>
#include <QList>
#include <QString>

#include <memory>
#include <optional>

namespace CommonTraceFormat {

// ── Structure ─────────────────────────────────────────────────────────────
struct StructureMember
{
    QString name;
    FieldClassPtr fieldClass;
    // Non-semantic attributes (spec 5.2) of the member class.
    QJsonObject attributes;
};

struct CMNTRCEFMT_EXPORT StructureFC : FieldClass
{
    QList<StructureMember> members;
    int minimumAlignment = 1; // bits

    explicit StructureFC()
        : FieldClass(FieldClassKind::Structure)
    {}
};

// ── Static-length array ───────────────────────────────────────────────────
struct CMNTRCEFMT_EXPORT StaticLengthArrayFC : FieldClass
{
    FieldClassPtr elementFieldClass;
    // Spec 5.3.20: `length` is a JSON integer >= 0, so use a 64-bit type rather
    // than int to represent the full permitted range.
    qint64 length = 0;
    int minimumAlignment = 1;

    explicit StaticLengthArrayFC()
        : FieldClass(FieldClassKind::StaticLengthArray)
    {}
};

// ── Dynamic-length array ──────────────────────────────────────────────────
struct CMNTRCEFMT_EXPORT DynamicLengthArrayFC : FieldClass
{
    FieldClassPtr elementFieldClass;
    FieldLocation lengthFieldLocation;
    int minimumAlignment = 1;

    explicit DynamicLengthArrayFC()
        : FieldClass(FieldClassKind::DynamicLengthArray)
    {}
};

// ── Variant ───────────────────────────────────────────────────────────────
// Each option covers a set of integer selector values or ranges.
struct VariantSelectorRange
{
    qint64 lo;
    qint64 hi;
};

struct VariantOption
{
    QList<qint64> selectorValues;               // matched against selector field
    QList<VariantSelectorRange> selectorRanges; // range-matched (lo..hi inclusive)
    FieldClassPtr fieldClass;
    QString name; // optional label
    // Non-semantic attributes (spec 5.2) of the option.
    QJsonObject attributes;
};

struct CMNTRCEFMT_EXPORT VariantFC : FieldClass
{
    QList<VariantOption> options;
    FieldLocation selectorFieldLocation;

    explicit VariantFC()
        : FieldClass(FieldClassKind::Variant)
    {}
};

// ── Optional ──────────────────────────────────────────────────────────────
struct CMNTRCEFMT_EXPORT OptionalFC : FieldClass
{
    FieldClassPtr fieldClass;
    FieldLocation selectorFieldLocation;
    // If the selector is a boolean field, presence follows its value and the
    // ranges below are empty. If the selector is an integer field, the field is
    // present when the selector value is contained in selectorValues or any of
    // selectorRanges (spec 5.3.22 field class; 6.4.19 decoding procedure).
    QList<qint64> selectorValues;
    QList<VariantSelectorRange> selectorRanges;

    explicit OptionalFC()
        : FieldClass(FieldClassKind::Optional)
    {}
};

} // namespace CommonTraceFormat
