// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fieldclass.h"

#include "../binary/bitbuffer.h" // ByteOrder, BitOrder

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace CommonTraceFormat {

// Implementation limit: fixed-length bit-array-based fields (bit array, bit map,
// boolean, integer) are decoded into a 64-bit word, so `length` MUST be in
// 1..64. The spec only bounds the *integer* classes this way; longer bit arrays,
// bit maps, and booleans are valid per spec but unsupported here and rejected at
// parse time rather than silently truncated. Fixed-length floats are exempt
// (they are assembled in 32/64-bit words, see FixedLengthFloatFC).

// ── Fixed-length bit array (raw bits, no interpretation) ──────────────────
struct CMNTRCEFMT_EXPORT FixedLengthBitArrayFC : FieldClass
{
    int length = 8; // bits
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    int alignment = 1; // bits

    explicit FixedLengthBitArrayFC()
        : FieldClass(FieldClassKind::FixedLengthBitArray)
    {}
};

// ── Fixed-length bitmap (bit flags with named ranges) ────────────────────
struct BitFlagRange
{
    int start;
    int end;
}; // inclusive bit positions within the field

struct CMNTRCEFMT_EXPORT FixedLengthBitMapFC : FieldClass
{
    int length = 8;
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    int alignment = 1;
    // name → bit index range set (a flag may map to several bit ranges).
    QHash<QString, QList<BitFlagRange>> flags;

    explicit FixedLengthBitMapFC()
        : FieldClass(FieldClassKind::FixedLengthBitMap)
    {}
};

// Active flag names of a decoded bit map value (spec 5.3.5 / 6.4.4): a flag is
// active when at least one bit index of any of its ranges is set in `value`.
// The decoded value carries the raw bit array as an integer (element I is bit I,
// i.e. the least-significant-first interpretation of spec 6.4.6); this resolves
// it to the named flags on demand, without bloating the runtime value type.
inline QStringList activeBitMapFlags(const FixedLengthBitMapFC &fc, quint64 value)
{
    QStringList active;
    for (auto it = fc.flags.cbegin(); it != fc.flags.cend(); ++it) {
        for (const BitFlagRange &r : it.value()) {
            bool any = false;
            for (int i = r.start; i <= r.end && !any; ++i)
                any = i >= 0 && i < 64 && ((value >> i) & 1);
            if (any) {
                active.append(it.key());
                break;
            }
        }
    }
    return active;
}

// ── Fixed-length boolean ─────────────────────────────────────────────────
struct CMNTRCEFMT_EXPORT FixedLengthBoolFC : FieldClass
{
    int length = 8;
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    int alignment = 1;

    explicit FixedLengthBoolFC()
        : FieldClass(FieldClassKind::FixedLengthBool)
    {}
};

// ── Integer mappings: map value ranges to labels ─────────────────────────
struct IntMappingRange
{
    qint64 lower;
    qint64 upper;
}; // signed: bounds may be negative
using IntMappings = QHash<QString, QList<IntMappingRange>>;

enum class DisplayBase { Binary = 2, Octal = 8, Decimal = 10, Hexadecimal = 16 };

// ── Integer roles ────────────────────────────────────────────────────────
enum class UIntRole {
    None,
    PacketMagicNumber,
    EventRecordClassId,
    DataStreamId,
    DataStreamClassId,
    PacketTotalLength,
    PacketContentLength,
    DefaultClockTimestamp,
    PacketEndDefaultClockTimestamp,
    DiscardedEventRecordCounterSnapshot,
    PacketSequenceNumber,
};

// ── Fixed-length unsigned integer ────────────────────────────────────────
struct CMNTRCEFMT_EXPORT FixedLengthUIntFC : FieldClass
{
    int length = 32;
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    int alignment = 1;
    DisplayBase displayBase = DisplayBase::Decimal;
    IntMappings mappings;
    QList<UIntRole> roles;

    explicit FixedLengthUIntFC()
        : FieldClass(FieldClassKind::FixedLengthUInt)
    {}
};

// ── Fixed-length signed integer ──────────────────────────────────────────
struct CMNTRCEFMT_EXPORT FixedLengthSIntFC : FieldClass
{
    int length = 32;
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    int alignment = 1;
    DisplayBase displayBase = DisplayBase::Decimal;
    IntMappings mappings;

    explicit FixedLengthSIntFC()
        : FieldClass(FieldClassKind::FixedLengthSInt)
    {}
};

// ── Fixed-length float ───────────────────────────────────────────────────
// Implementation limit: for 128-bit and binaryK (K > 128, multiple of 32)
// floats, only the two natural byte/bit-order combinations are decoded
// (little-endian + first-to-last, big-endian + last-to-first); other
// combinations require cross-word bit assembly and are reported as an error
// rather than decoded. 16/32/64-bit lengths support all combinations.
struct CMNTRCEFMT_EXPORT FixedLengthFloatFC : FieldClass
{
    int length = 64; // 16/32/64/128, or a multiple of 32 greater than 128
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    BitOrder bitOrder = BitOrder::LeastSignificantFirst;
    int alignment = 1;

    explicit FixedLengthFloatFC()
        : FieldClass(FieldClassKind::FixedLengthFloat)
    {}
};

// ── Variable-length unsigned integer (ULEB128) ───────────────────────────
struct CMNTRCEFMT_EXPORT VariableLengthUIntFC : FieldClass
{
    DisplayBase displayBase = DisplayBase::Decimal;
    IntMappings mappings;
    QList<UIntRole> roles;

    explicit VariableLengthUIntFC()
        : FieldClass(FieldClassKind::VariableLengthUInt)
    {}
};

// ── Variable-length signed integer (SLEB128) ─────────────────────────────
struct CMNTRCEFMT_EXPORT VariableLengthSIntFC : FieldClass
{
    DisplayBase displayBase = DisplayBase::Decimal;
    IntMappings mappings;

    explicit VariableLengthSIntFC()
        : FieldClass(FieldClassKind::VariableLengthSInt)
    {}
};

} // namespace CommonTraceFormat
