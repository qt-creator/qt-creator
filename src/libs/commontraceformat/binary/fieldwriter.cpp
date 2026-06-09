// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fieldwriter.h"

#include "../schema/blobfieldclasses.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"
#include "../schema/stringfieldclasses.h"
#include "../util/leb128.h"

#include <utils/qtcassert.h>

#include <QFloat16>
#include <QStringConverter>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace CommonTraceFormat {

// Bytes per code unit for a string encoding (mirrors FieldReader); the
// null-terminator of a null-terminated string is one all-zero code unit wide.
static int codeUnitLength(StringEncoding enc)
{
    switch (enc) {
    case StringEncoding::Utf8:
        return 1;
    case StringEncoding::Utf16Be:
    case StringEncoding::Utf16Le:
        return 2;
    case StringEncoding::Utf32Be:
    case StringEncoding::Utf32Le:
        return 4;
    }
    return 1;
}

// Encode a string in the field's declared encoding (the inverse of
// FieldReader's decodeStringBytes); writing always-UTF-8 would otherwise fail
// to round-trip a UTF-16/32 field.
static QByteArray encodeStringBytes(const QString &s, StringEncoding enc)
{
    switch (enc) {
    case StringEncoding::Utf8:
        return s.toUtf8();
    case StringEncoding::Utf16Be:
        return QStringEncoder(QStringEncoder::Utf16BE)(s);
    case StringEncoding::Utf16Le:
        return QStringEncoder(QStringEncoder::Utf16LE)(s);
    case StringEncoding::Utf32Be:
        return QStringEncoder(QStringEncoder::Utf32BE)(s);
    case StringEncoding::Utf32Le:
        return QStringEncoder(QStringEncoder::Utf32LE)(s);
    }
    return s.toUtf8();
}

// Encode a double into a big-endian IEEE 754-2008 interchange byte array of K
// bits (K == 128, or a multiple of 32 greater than 128). Bit layout, MSB first
// (be[0] bit 7 is the sign): [sign(1)][exponent(w)][trailing significand(t)],
// where w = round(4·log2(K)) − 13 and t = K − w − 1 (binary128 ⇒ w=15, t=112).
// This is the inverse of FieldReader's binary128/binaryK decoders; precision
// beyond double's 52-bit significand is necessarily lost.
static std::vector<uint8_t> doubleToBeFloatBytes(double dv, int k)
{
    const int w = (k == 128) ? 15
                             : (static_cast<int>(std::lround(4.0 * std::log2(double(k)))) - 13);
    const int t = k - w - 1;
    std::vector<uint8_t> be(k / 8, 0);
    // Metadata validation caps k at 2048 (w == 31), so the 1u << w shifts below
    // stay defined. Guard anyway: a w >= 64 would be undefined behaviour. Bail
    // with a zeroed buffer rather than emit garbage.
    QTC_ASSERT(w > 0 && w < 64, return be);
    const auto setBit = [&](int idx) { be[idx / 8] |= static_cast<uint8_t>(1u << (7 - (idx % 8))); };

    quint64 db;
    std::memcpy(&db, &dv, 8);
    const int sign = static_cast<int>((db >> 63) & 1);
    const int de = static_cast<int>((db >> 52) & 0x7FF);
    const quint64 dm = db & ((quint64(1) << 52) - 1);

    const quint64 maxExp = (quint64(1) << w) - 1;     // w < 64, asserted above
    const double bias = std::ldexp(1.0, w - 1) - 1.0; // 2^(w-1) - 1

    quint64 expField = 0;
    quint64 frac52 = 0; // double fraction bits, copied into the top of the mantissa
    bool nan = false;

    if (de == 0x7FF) {
        expField = maxExp; // infinity or NaN
        nan = (dm != 0);
    } else if (de == 0 && dm == 0) {
        expField = 0; // zero
    } else {
        int e2; // unbiased base-2 exponent of the value (1.frac × 2^e2)
        if (de == 0) {
            // Subnormal double: value = dm × 2^(1-1023-52); normalize it (the
            // huge binary128/binaryK exponent range always represents it normally).
            int p = 51;
            while (p >= 0 && !((dm >> p) & 1))
                --p;
            e2 = (1 - 1023) - (52 - p);
            frac52 = (dm & ((quint64(1) << p) - 1)) << (52 - p);
        } else {
            e2 = de - 1023;
            frac52 = dm;
        }
        const long targetExp = static_cast<long>(e2) + static_cast<long>(bias);
        if (targetExp <= 0)
            expField = 0; // underflow (not for K>=128)
        else if (static_cast<quint64>(targetExp) >= maxExp)
            expField = maxExp; // overflow (not for K>=128)
        else
            expField = static_cast<quint64>(targetExp);
    }

    if (sign)
        setBit(0);
    for (int e = 0; e < w; ++e)
        if ((expField >> (w - 1 - e)) & 1)
            setBit(1 + e);
    const int topBits = std::min(t, 52);
    for (int i = 0; i < topBits; ++i)
        if ((frac52 >> (51 - i)) & 1)
            setBit(1 + w + i);
    if (nan)
        setBit(1 + w); // ensure a non-zero (quiet) NaN significand
    return be;
}

FieldWriter::FieldWriter(BitBuffer &buf, FieldLocationResolver resolver)
    : m_buf(buf)
    , m_resolver(std::move(resolver))
{}

void FieldWriter::write(const FieldClass &fc, const FieldValue &value)
{
    switch (fc.kind) {
    case FieldClassKind::FixedLengthBitArray:
    case FieldClassKind::FixedLengthBitMap: {
        const auto &f = static_cast<const FixedLengthBitArrayFC &>(fc);
        quint64 v = std::holds_alternative<quint64>(value) ? std::get<quint64>(value) : 0;
        m_buf.alignWriteTo(f.alignment);
        m_buf.writeBits(v, f.length, f.byteOrder, f.bitOrder);
        break;
    }
    case FieldClassKind::FixedLengthBool: {
        const auto &f = static_cast<const FixedLengthBoolFC &>(fc);
        bool v = std::holds_alternative<bool>(value) ? std::get<bool>(value) : false;
        m_buf.alignWriteTo(f.alignment);
        m_buf.writeBits(v ? 1 : 0, f.length, f.byteOrder, f.bitOrder);
        break;
    }
    case FieldClassKind::FixedLengthUInt: {
        const auto &f = static_cast<const FixedLengthUIntFC &>(fc);
        quint64 v = std::holds_alternative<quint64>(value) ? std::get<quint64>(value) : 0;
        m_buf.alignWriteTo(f.alignment);
        m_buf.writeBits(v, f.length, f.byteOrder, f.bitOrder);
        break;
    }
    case FieldClassKind::FixedLengthSInt: {
        const auto &f = static_cast<const FixedLengthSIntFC &>(fc);
        qint64 sv = std::holds_alternative<qint64>(value) ? std::get<qint64>(value) : 0;
        m_buf.alignWriteTo(f.alignment);
        m_buf.writeBits(static_cast<quint64>(sv), f.length, f.byteOrder, f.bitOrder);
        break;
    }
    case FieldClassKind::FixedLengthFloat: {
        const auto &f = static_cast<const FixedLengthFloatFC &>(fc);
        double dv = std::holds_alternative<double>(value) ? std::get<double>(value) : 0.0;
        m_buf.alignWriteTo(f.alignment);
        if (f.length == 16) {
            qfloat16 hv = qfloat16(static_cast<float>(dv));
            quint16 bits;
            memcpy(&bits, &hv, 2);
            m_buf.writeBits(bits, 16, f.byteOrder, f.bitOrder);
        } else if (f.length == 32) {
            float fv = static_cast<float>(dv);
            quint32 bits;
            memcpy(&bits, &fv, 4);
            m_buf.writeBits(bits, 32, f.byteOrder, f.bitOrder);
        } else if (f.length == 64) {
            quint64 bits;
            memcpy(&bits, &dv, 8);
            m_buf.writeBits(bits, 64, f.byteOrder, f.bitOrder);
        } else {
            // binary128 / binaryK (length a multiple of 32, > 64): assemble a
            // big-endian byte array, then emit 32-bit words in the order the
            // reader reconstructs them (mirrors FieldReader). Only the natural
            // byte/bit-order combinations are supported, as in the reader.
            const std::vector<uint8_t> beBytes = doubleToBeFloatBytes(dv, f.length);
            const bool le = f.byteOrder == ByteOrder::LittleEndian
                            && f.bitOrder == BitOrder::LeastSignificantFirst;
            const int nWords = f.length / 32;
            for (int i = 0; i < nWords; ++i) {
                // LE: word 0 is the least significant (it sits last in the
                // big-endian layout); BE: word 0 is the most significant.
                const int wp = le ? (nWords - 1 - i) : i;
                const quint32 word = (quint32(beBytes[wp * 4 + 0]) << 24)
                                     | (quint32(beBytes[wp * 4 + 1]) << 16)
                                     | (quint32(beBytes[wp * 4 + 2]) << 8)
                                     | quint32(beBytes[wp * 4 + 3]);
                m_buf.writeBits(word, 32, f.byteOrder, f.bitOrder);
            }
        }
        break;
    }
    case FieldClassKind::VariableLengthUInt: {
        quint64 v = std::holds_alternative<quint64>(value) ? std::get<quint64>(value) : 0;
        QByteArray encoded;
        encodeULeb128(v, encoded);
        m_buf.alignWriteTo(8);
        m_buf.writeBytes(encoded);
        break;
    }
    case FieldClassKind::VariableLengthSInt: {
        qint64 sv = std::holds_alternative<qint64>(value) ? std::get<qint64>(value) : 0;
        QByteArray encoded;
        encodeSLeb128(sv, encoded);
        m_buf.alignWriteTo(8);
        m_buf.writeBytes(encoded);
        break;
    }
    case FieldClassKind::NullTerminatedString: {
        const auto &f = static_cast<const NullTerminatedStringFC &>(fc);
        const QString &s = std::holds_alternative<QString>(value) ? std::get<QString>(value)
                                                                  : QString{};
        m_buf.alignWriteTo(8);
        QByteArray bytes = encodeStringBytes(s, f.encoding);
        // Terminate with one all-zero code unit (1/2/4 bytes for UTF-8/16/32).
        bytes.append(QByteArray(codeUnitLength(f.encoding), '\0'));
        m_buf.writeBytes(bytes);
        break;
    }
    case FieldClassKind::StaticLengthString: {
        const auto &f = static_cast<const StaticLengthStringFC &>(fc);
        const QString &s = std::holds_alternative<QString>(value) ? std::get<QString>(value)
                                                                  : QString{};
        m_buf.alignWriteTo(8);
        QByteArray bytes = encodeStringBytes(s, f.encoding).left(f.length);
        bytes.resize(f.length, '\0');
        m_buf.writeBytes(bytes);
        break;
    }
    case FieldClassKind::DynamicLengthString: {
        const auto &f = static_cast<const DynamicLengthStringFC &>(fc);
        const QString &s = std::holds_alternative<QString>(value) ? std::get<QString>(value)
                                                                  : QString{};
        m_buf.alignWriteTo(8);
        m_buf.writeBytes(encodeStringBytes(s, f.encoding));
        break;
    }
    case FieldClassKind::StaticLengthBlob: {
        const auto &f = static_cast<const StaticLengthBlobFC &>(fc);
        const QByteArray &b = std::holds_alternative<QByteArray>(value)
                                  ? std::get<QByteArray>(value)
                                  : QByteArray{};
        m_buf.alignWriteTo(8);
        QByteArray padded = b.left(f.length);
        padded.resize(f.length, '\0');
        m_buf.writeBytes(padded);
        break;
    }
    case FieldClassKind::DynamicLengthBlob: {
        const QByteArray &b = std::holds_alternative<QByteArray>(value)
                                  ? std::get<QByteArray>(value)
                                  : QByteArray{};
        m_buf.alignWriteTo(8);
        m_buf.writeBytes(b);
        break;
    }
    case FieldClassKind::Structure: {
        const auto &f = static_cast<const StructureFC &>(fc);
        if (isStructure(value))
            writeStructure(f, asStructure(value));
        else
            writeStructure(f, StructureValue{}); // empty fallback
        break;
    }
    case FieldClassKind::StaticLengthArray: {
        const auto &f = static_cast<const StaticLengthArrayFC &>(fc);
        if (isArray(value))
            writeStaticArray(f, asArray(value));
        else
            writeStaticArray(f, ArrayValue{});
        break;
    }
    case FieldClassKind::DynamicLengthArray: {
        const auto &f = static_cast<const DynamicLengthArrayFC &>(fc);
        if (isArray(value))
            writeDynamicArray(f, asArray(value));
        else
            writeDynamicArray(f, ArrayValue{});
        break;
    }
    case FieldClassKind::Variant: {
        const auto &f = static_cast<const VariantFC &>(fc);
        if (std::holds_alternative<std::shared_ptr<VariantValue>>(value)) {
            if (const auto &vv = std::get<std::shared_ptr<VariantValue>>(value))
                writeVariant(f, *vv);
        }
        break;
    }
    case FieldClassKind::Optional: {
        const auto &f = static_cast<const OptionalFC &>(fc);
        writeOptional(f, value);
        break;
    }
    }
}

void FieldWriter::writeStructure(const StructureFC &fc, const StructureValue &sv)
{
    m_buf.alignWriteTo(fc.minimumAlignment);
    for (const auto &member : fc.members) {
        const FieldValue *fv = sv.get(member.name);
        write(*member.fieldClass, fv ? *fv : FieldValue{});
    }
}

void FieldWriter::writeStaticArray(const StaticLengthArrayFC &fc, const ArrayValue &av)
{
    m_buf.alignWriteTo(fc.minimumAlignment);
    for (qint64 i = 0; i < fc.length; ++i) {
        const FieldValue &elem = (i < av.elements.size()) ? av.elements[i] : FieldValue{};
        write(*fc.elementFieldClass, elem);
    }
}

void FieldWriter::writeDynamicArray(const DynamicLengthArrayFC &fc, const ArrayValue &av)
{
    m_buf.alignWriteTo(fc.minimumAlignment);
    for (const auto &elem : av.elements)
        write(*fc.elementFieldClass, elem);
}

void FieldWriter::writeVariant(const VariantFC &fc, const VariantValue &vv)
{
    // Prefer the unambiguous option index (option names are optional and may be
    // empty or duplicated, spec 5.3.23.1). Fall back to matching by name only
    // when no index was recorded.
    if (vv.selectedIndex >= 0 && vv.selectedIndex < fc.options.size()) {
        const auto &opt = fc.options[vv.selectedIndex];
        write(*opt.fieldClass, vv.value ? *vv.value : FieldValue{});
        return;
    }
    for (const auto &opt : fc.options) {
        if (opt.name == vv.selectedOption) {
            write(*opt.fieldClass, vv.value ? *vv.value : FieldValue{});
            return;
        }
    }
}

void FieldWriter::writeOptional(const OptionalFC &fc, const FieldValue &value)
{
    if (std::holds_alternative<std::monostate>(value))
        return;
    write(*fc.fieldClass, value);
}

} // namespace CommonTraceFormat
