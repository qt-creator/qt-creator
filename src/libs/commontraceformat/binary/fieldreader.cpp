// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fieldreader.h"

#include "../schema/blobfieldclasses.h"
#include "../schema/compoundfieldclasses.h"
#include "../schema/scalarfieldclasses.h"
#include "../schema/stringfieldclasses.h"
#include "util/leb128.h"

#include <utils/result.h>

#include <QFloat16>
#include <QStringConverter>

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <vector>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

// A LEB128-encoded value that fits in 64 bits needs at most ceil(64/7) = 10
// bytes; anything longer is malformed. Capping the byte count keeps a packet
// full of continuation bytes (0x80..0xFF) from being buffered in its entirety
// for a single integer field.
static constexpr int kMaxLeb128Bytes = 10;

// Upper bound on the element count of a dynamic-length array whose element
// decodes to zero bits. Such an array consumes no packet data, so its declared
// length is otherwise unbounded by the buffer and a hostile length (e.g.
// UINT64_MAX) would loop almost forever. A million empty elements is already
// far beyond any plausible trace.
static constexpr quint64 kMaxZeroWidthArrayElements = 1u << 20;

// Upper bound on a fixed-length float's bit width. binaryK with K a large
// multiple of 32 would otherwise allocate K/8 bytes before reading anything;
// well above binary256 but bounded against crafted metadata. Mirrors the
// metadata reader's gate so the TSDL path (which derives length from
// exp + mantissa digits) is covered too.
static constexpr int kMaxFloatLength = 2048;

// Convert an IEEE 754-2008 binary128 value (1 sign, 15-bit exponent, 112-bit
// mantissa), given as its high and low 64-bit halves, to the nearest double.
// Precision beyond double's 52-bit mantissa is necessarily lost.
static double binary128ToDouble(quint64 hi, quint64 lo)
{
    const int sign = (hi >> 63) & 1;
    const int exponent = (hi >> 48) & 0x7FFF;      // bits 112..126
    const quint64 mantHi = hi & 0xFFFFFFFFFFFFULL; // mantissa bits 64..111
    const quint64 mantLo = lo;                     // mantissa bits 0..63

    double result;
    if (exponent == 0x7FFF) {
        result = (mantHi == 0 && mantLo == 0) ? std::numeric_limits<double>::infinity()
                                              : std::numeric_limits<double>::quiet_NaN();
    } else {
        // Mantissa as a fraction in [0, 1): mantissa / 2^112.
        const double mant = std::ldexp(static_cast<double>(mantHi), 64)
                            + static_cast<double>(mantLo);
        const double frac = std::ldexp(mant, -112);
        result = (exponent == 0) ? std::ldexp(frac, 1 - 16383) // subnormal / zero
                                 : std::ldexp(1.0 + frac, exponent - 16383);
    }
    return sign ? -result : result;
}

// Extract `count` (<= 64) bits from a big-endian bit array (be[0] is the most
// significant byte), starting at bit index `startFromMsb` counted from the MSB.
static quint64 bitsFromMsb(const std::vector<uint8_t> &be, int startFromMsb, int count)
{
    quint64 v = 0;
    for (int i = 0; i < count; ++i) {
        const int b = startFromMsb + i;
        const int byteIdx = b / 8;
        const int bitInByte = 7 - (b % 8);
        v = (v << 1) | ((be[byteIdx] >> bitInByte) & 1);
    }
    return v;
}

// Convert an IEEE 754-2008 binaryK interchange value (K a multiple of 32, K >=
// 128), given as a big-endian byte array, to the nearest double. The exponent
// field width follows the interchange format: w = round(4·log2(K)) − 13, the
// trailing significand is t = K − w − 1, and the bias is 2^(w−1) − 1. Precision
// beyond double's 52-bit significand is necessarily lost.
static double binaryKToDouble(const std::vector<uint8_t> &be, int k)
{
    const int w = static_cast<int>(std::lround(4.0 * std::log2(double(k)))) - 13;
    const int t = k - w - 1;
    const int sign = (be[0] >> 7) & 1;
    const quint64 exponent = bitsFromMsb(be, 1, w);

    // Top significand bits drive the double; scan the remainder only to tell a
    // NaN from an infinity when the exponent field is all ones.
    const int topBits = std::min(t, 53);
    const quint64 topMant = bitsFromMsb(be, 1 + w, topBits);
    bool mantNonZero = topMant != 0;
    for (int i = topBits; i < t && !mantNonZero; ++i)
        mantNonZero = bitsFromMsb(be, 1 + w + i, 1) != 0;

    const quint64 maxExp = (w >= 64) ? ~quint64(0) : ((quint64(1) << w) - 1);
    const double bias = std::ldexp(1.0, w - 1) - 1.0; // 2^(w-1) - 1

    double result;
    if (exponent == maxExp) {
        result = mantNonZero ? std::numeric_limits<double>::quiet_NaN()
                             : std::numeric_limits<double>::infinity();
    } else {
        const double frac = std::ldexp(double(topMant), -topBits); // [0, 1)
        result = (exponent == 0)
                     ? std::ldexp(frac, static_cast<int>(1 - bias)) // subnormal / zero
                     : std::ldexp(1.0 + frac, static_cast<int>(double(exponent) - bias));
    }
    return sign ? -result : result;
}

// Number of bytes per code unit for a string encoding (spec 6.4.11/12/14).
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

static QString decodeStringBytes(const QByteArray &bytes, StringEncoding enc)
{
    switch (enc) {
    case StringEncoding::Utf8:
        return QString::fromUtf8(bytes);
    case StringEncoding::Utf16Be:
        return QStringDecoder(QStringDecoder::Utf16BE)(bytes);
    case StringEncoding::Utf16Le:
        return QStringDecoder(QStringDecoder::Utf16LE)(bytes);
    case StringEncoding::Utf32Be:
        return QStringDecoder(QStringDecoder::Utf32BE)(bytes);
    case StringEncoding::Utf32Le:
        return QStringDecoder(QStringDecoder::Utf32LE)(bytes);
    }
    return QString::fromUtf8(bytes);
}

// Validate a located length field (spec 5.3.14/5.3.17/5.3.21: the length field
// MUST be an unsigned integer). A signed located field carrying a negative
// value would otherwise wrap to a huge unsigned length.
static Utils::Result<void> checkUnsignedLength(quint64 value, bool isSigned, QStringView what)
{
    if (isSigned && static_cast<qint64>(value) < 0)
        return Utils::ResultError(QStringLiteral("%1 length field is negative").arg(what));
    return Utils::ResultOk;
}

// Validate a located byte length before narrowing the unsigned value to the
// signed qsizetype used for buffer reads. A value past the bytes remaining in
// the packet content area (or one that would overflow qsizetype) is rejected
// rather than wrapped to a negative count, which would otherwise read nothing
// and silently yield an empty result.
static Utils::Result<qsizetype> checkedByteLength(
    const BitBuffer &buf, quint64 value, QStringView what)
{
    const qint64 remainingBits = buf.contentEndBit() - buf.readBitOffset();
    const quint64 remainingBytes = remainingBits > 0 ? quint64(remainingBits) / 8 : 0;
    if (value > remainingBytes)
        return Utils::ResultError(
            QStringLiteral("%1 length %2 exceeds the %3 bytes remaining in the packet content")
                .arg(what)
                .arg(value)
                .arg(remainingBytes));
    return static_cast<qsizetype>(value);
}

// True if any byte of the code unit [data+off, off+ul) is non-zero.
static bool codeUnitHasSetBit(const QByteArray &data, int off, int ul)
{
    for (int i = 0; i < ul; ++i) {
        if (static_cast<uint8_t>(data[off + i]) != 0)
            return true;
    }
    return false;
}

FieldReader::FieldReader(
    BitBuffer &buf, FieldLocationResolver resolver, FieldLocation::Origin currentOrigin)
    : m_buf(buf)
    , m_resolver(std::move(resolver))
    , m_currentOrigin(currentOrigin)
{}

Utils::Result<FieldValue> FieldReader::read(const FieldClass &fc)
{
    switch (fc.kind) {
    case FieldClassKind::FixedLengthBitArray:
    case FieldClassKind::FixedLengthBitMap: {
        const auto &f = static_cast<const FixedLengthBitArrayFC &>(fc);
        if (auto r = m_buf.alignReadTo(f.alignment); !r)
            return Utils::ResultError(r.error());
        // Spec 6.4.3: a read past the packet content boundary surfaces as an
        // error and aborts, rather than yielding a zero-filled value.
        const auto v = m_buf.readBits(f.length, f.byteOrder, f.bitOrder);
        if (!v)
            return Utils::ResultError(v.error());
        return *v;
    }
    case FieldClassKind::FixedLengthBool: {
        const auto &f = static_cast<const FixedLengthBoolFC &>(fc);
        if (auto r = m_buf.alignReadTo(f.alignment); !r)
            return Utils::ResultError(r.error());
        const auto v = m_buf.readBits(f.length, f.byteOrder, f.bitOrder);
        if (!v)
            return Utils::ResultError(v.error());
        return bool(*v != 0);
    }
    case FieldClassKind::FixedLengthUInt: {
        const auto &f = static_cast<const FixedLengthUIntFC &>(fc);
        if (auto r = m_buf.alignReadTo(f.alignment); !r)
            return Utils::ResultError(r.error());
        const auto v = m_buf.readBits(f.length, f.byteOrder, f.bitOrder);
        if (!v)
            return Utils::ResultError(v.error());
        return *v;
    }
    case FieldClassKind::FixedLengthSInt: {
        const auto &f = static_cast<const FixedLengthSIntFC &>(fc);
        if (auto r = m_buf.alignReadTo(f.alignment); !r)
            return Utils::ResultError(r.error());
        const auto rawResult = m_buf.readBits(f.length, f.byteOrder, f.bitOrder);
        if (!rawResult)
            return Utils::ResultError(rawResult.error());
        quint64 raw = *rawResult;
        int bits = f.length;
        if (bits < 64 && (raw & (quint64(1) << (bits - 1))))
            raw |= ~((quint64(1) << bits) - 1);
        return qint64(raw);
    }
    case FieldClassKind::FixedLengthFloat: {
        const auto &f = static_cast<const FixedLengthFloatFC &>(fc);
        if (auto r = m_buf.alignReadTo(f.alignment); !r)
            return Utils::ResultError(r.error());
        if (f.length == 16) {
            const auto bits = m_buf.readBits(16, f.byteOrder, f.bitOrder);
            if (!bits)
                return Utils::ResultError(bits.error());
            qfloat16 hv = std::bit_cast<qfloat16>(static_cast<quint16>(*bits));
            return double(float(hv));
        } else if (f.length == 32) {
            const auto bits = m_buf.readBits(32, f.byteOrder, f.bitOrder);
            if (!bits)
                return Utils::ResultError(bits.error());
            float fv = std::bit_cast<float>(static_cast<quint32>(*bits));
            return double(fv);
        } else if (f.length == 64) {
            const auto bits = m_buf.readBits(64, f.byteOrder, f.bitOrder);
            if (!bits)
                return Utils::ResultError(bits.error());
            double dv = std::bit_cast<double>(*bits);
            return dv;
        } else if (f.length == 128) {
            // Read two 64-bit words. Only the natural byte/bit-order combinations
            // (LE + first-to-last, BE + last-to-first) are supported, since
            // mixed-order assembly across the two words isn't handled here.
            const bool le = f.byteOrder == ByteOrder::LittleEndian
                            && f.bitOrder == BitOrder::LeastSignificantFirst;
            const bool be = f.byteOrder == ByteOrder::BigEndian
                            && f.bitOrder == BitOrder::MostSignificantFirst;
            if (!le && !be)
                return Utils::ResultError(u"Unsupported 128-bit float byte/bit-order combination"_s);
            const auto w0 = m_buf.readBits(64, f.byteOrder, f.bitOrder);
            if (!w0)
                return Utils::ResultError(w0.error());
            const auto w1 = m_buf.readBits(64, f.byteOrder, f.bitOrder);
            if (!w1)
                return Utils::ResultError(w1.error());
            // LE: first word holds bits 0..63 (low); BE: first word holds the
            // most significant 64 bits (high).
            const quint64 lo = le ? *w0 : *w1;
            const quint64 hi = le ? *w1 : *w0;
            return binary128ToDouble(hi, lo);
        } else if (f.length > 128 && f.length % 32 == 0) {
            // binaryK (K a multiple of 32, K > 128). Read 32-bit words and
            // assemble a big-endian byte array (be[0] is the most significant
            // byte). As with binary128, only the natural byte/bit-order combos
            // are handled, since mixed-order cross-word assembly isn't.
            const bool le = f.byteOrder == ByteOrder::LittleEndian
                            && f.bitOrder == BitOrder::LeastSignificantFirst;
            const bool be = f.byteOrder == ByteOrder::BigEndian
                            && f.bitOrder == BitOrder::MostSignificantFirst;
            if (!le && !be)
                return Utils::ResultError(
                    u"Unsupported %1-bit float byte/bit-order combination"_s.arg(f.length));
            if (f.length > kMaxFloatLength)
                return Utils::ResultError(
                    u"Floating point length %1 exceeds maximum of %2"_s.arg(f.length)
                        .arg(kMaxFloatLength));
            const int nWords = f.length / 32;
            std::vector<uint8_t> bytes(f.length / 8, 0);
            for (int i = 0; i < nWords; ++i) {
                const auto wordResult = m_buf.readBits(32, f.byteOrder, f.bitOrder);
                if (!wordResult)
                    return Utils::ResultError(wordResult.error());
                const quint32 word = static_cast<quint32>(*wordResult);
                // LE: word 0 is the least significant, so it lands last in the
                // big-endian layout; BE: word 0 is the most significant (first).
                const int wordPos = le ? (nWords - 1 - i) : i;
                bytes[wordPos * 4 + 0] = (word >> 24) & 0xFF;
                bytes[wordPos * 4 + 1] = (word >> 16) & 0xFF;
                bytes[wordPos * 4 + 2] = (word >> 8) & 0xFF;
                bytes[wordPos * 4 + 3] = word & 0xFF;
            }
            return binaryKToDouble(bytes, f.length);
        }
        return Utils::ResultError(u"Unsupported floating point length: %1"_s.arg(f.length));
    }
    case FieldClassKind::VariableLengthUInt: {
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        QByteArray encoded;
        while (true) {
            if (encoded.size() >= kMaxLeb128Bytes)
                return Utils::ResultError(u"ULEB128 exceeds 64 bits"_s);
            // readU8 enforces the packet content boundary (spec 6.4.9: error if
            // the next byte would extend past PKT_CONTENT_LEN).
            const auto byte = m_buf.readU8();
            if (!byte)
                return Utils::ResultError(u"Truncated ULEB128"_s);
            encoded.append(static_cast<char>(*byte));
            if (!(*byte & 0x80))
                break;
        }
        m_lastVlByteCount = static_cast<int>(encoded.size());
        auto [value, n] = decodeULeb128(std::span(encoded.constData(), encoded.size()));
        if (n < 0)
            return Utils::ResultError(u"Invalid ULEB128"_s);
        return quint64(value);
    }
    case FieldClassKind::VariableLengthSInt: {
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        QByteArray encoded;
        while (true) {
            if (encoded.size() >= kMaxLeb128Bytes)
                return Utils::ResultError(u"SLEB128 exceeds 64 bits"_s);
            const auto byte = m_buf.readU8();
            if (!byte)
                return Utils::ResultError(u"Truncated SLEB128"_s);
            encoded.append(static_cast<char>(*byte));
            if (!(*byte & 0x80))
                break;
        }
        m_lastVlByteCount = static_cast<int>(encoded.size());
        auto [value, n] = decodeSLeb128(std::span(encoded.constData(), encoded.size()));
        if (n < 0)
            return Utils::ResultError(u"Invalid SLEB128"_s);
        return qint64(value);
    }
    case FieldClassKind::NullTerminatedString: {
        const auto &f = static_cast<const NullTerminatedStringFC &>(fc);
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        return readNullTerminatedString(f.encoding);
    }
    case FieldClassKind::StaticLengthString: {
        const auto &f = static_cast<const StaticLengthStringFC &>(fc);
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        return readLengthPrefixedString(f.length, f.encoding);
    }
    case FieldClassKind::DynamicLengthString: {
        const auto &f = static_cast<const DynamicLengthStringFC &>(fc);
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        auto len = resolveLocation(f.lengthFieldLocation);
        if (!len)
            return Utils::ResultError(len.error());
        if (auto r = checkUnsignedLength(len->value, len->isSigned, u"dynamic-length string"); !r)
            return Utils::ResultError(r.error());
        auto byteLen = checkedByteLength(m_buf, len->value, u"dynamic-length string");
        if (!byteLen)
            return Utils::ResultError(byteLen.error());
        return readLengthPrefixedString(*byteLen, f.encoding);
    }
    case FieldClassKind::StaticLengthBlob: {
        const auto &f = static_cast<const StaticLengthBlobFC &>(fc);
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        auto bytes = m_buf.readBytes(f.length);
        if (!bytes)
            return Utils::ResultError(bytes.error());
        return *bytes;
    }
    case FieldClassKind::DynamicLengthBlob: {
        const auto &f = static_cast<const DynamicLengthBlobFC &>(fc);
        if (auto r = m_buf.alignReadTo(8); !r)
            return Utils::ResultError(r.error());
        auto len = resolveLocation(f.lengthFieldLocation);
        if (!len)
            return Utils::ResultError(len.error());
        if (auto r = checkUnsignedLength(len->value, len->isSigned, u"dynamic-length BLOB"); !r)
            return Utils::ResultError(r.error());
        auto byteLen = checkedByteLength(m_buf, len->value, u"dynamic-length BLOB");
        if (!byteLen)
            return Utils::ResultError(byteLen.error());
        auto bytes = m_buf.readBytes(*byteLen);
        if (!bytes)
            return Utils::ResultError(bytes.error());
        return *bytes;
    }
    case FieldClassKind::Structure:
        return readStructure(static_cast<const StructureFC &>(fc));
    case FieldClassKind::StaticLengthArray:
        return readStaticArray(static_cast<const StaticLengthArrayFC &>(fc));
    case FieldClassKind::DynamicLengthArray:
        return readDynamicArray(static_cast<const DynamicLengthArrayFC &>(fc));
    case FieldClassKind::Variant:
        return readVariant(static_cast<const VariantFC &>(fc));
    case FieldClassKind::Optional:
        return readOptional(static_cast<const OptionalFC &>(fc));
    }
    return Utils::ResultError(u"Unknown field class kind"_s);
}

Utils::Result<FieldValue> FieldReader::readStructure(const StructureFC &fc)
{
    if (auto r = m_buf.alignReadTo(fc.minimumAlignment); !r)
        return Utils::ResultError(r.error());
    auto sv = std::make_shared<StructureValue>();

    // Push a scope for this structure so the field location procedure can find
    // its members (prior siblings) and navigate to/from its parent.
    Scope scope;
    scope.sv = sv.get();
    scope.parent = m_scopes.isEmpty() ? -1 : static_cast<int>(m_scopes.size()) - 1;
    scope.memberInParent = m_pendingChildMember;
    m_pendingChildMember.clear();
    const int scopeIdx = static_cast<int>(m_scopes.size());
    const bool isRoot = scopeIdx == 0;
    m_scopes.append(scope);

    for (const auto &member : fc.members) {
        m_pendingChildMember = member.name;
        auto result = read(*member.fieldClass);
        m_pendingChildMember.clear();
        if (!result) {
            m_scopes.removeLast();
            return Utils::ResultError(result.error());
        }

        // Record the clock length L (spec 6.3) for top-level integer members.
        if (isRoot) {
            const FieldClass *mfc = member.fieldClass.get();
            if (mfc->kind == FieldClassKind::FixedLengthUInt)
                m_rootMemberClockBits[member.name]
                    = static_cast<const FixedLengthUIntFC &>(*mfc).length;
            else if (mfc->kind == FieldClassKind::VariableLengthUInt)
                m_rootMemberClockBits[member.name] = m_lastVlByteCount * 7;
        }

        sv->set(member.name, std::move(*result));
    }

    m_scopes.removeLast();
    return FieldValue{sv};
}

Utils::Result<FieldValue> FieldReader::readStaticArray(const StaticLengthArrayFC &fc)
{
    if (auto r = m_buf.alignReadTo(fc.minimumAlignment); !r)
        return Utils::ResultError(r.error());
    // Each element structure is labelled with the array's own member name, so a
    // field location can reach the element currently being decoded.
    const QString memberName = m_pendingChildMember;
    auto av = std::make_shared<ArrayValue>();
    for (qint64 i = 0; i < fc.length; ++i) {
        const qint64 posBefore = m_buf.readBitOffset();
        m_pendingChildMember = memberName;
        auto result = read(*fc.elementFieldClass);
        if (!result)
            return Utils::ResultError(result.error());
        av->elements.append(std::move(*result));
        // A zero-width element consumes no data, so the packet content boundary
        // never terminates the loop; a malformed schema with a huge static
        // length would otherwise spin (near-)forever building empty values.
        if (m_buf.readBitOffset() == posBefore && fc.length > qint64(kMaxZeroWidthArrayElements))
            return Utils::ResultError(
                u"Static-length array of zero-width elements has an implausible length %1"_s.arg(
                    fc.length));
    }
    m_pendingChildMember.clear();
    return FieldValue{av};
}

Utils::Result<FieldValue> FieldReader::readDynamicArray(const DynamicLengthArrayFC &fc)
{
    if (auto r = m_buf.alignReadTo(fc.minimumAlignment); !r)
        return Utils::ResultError(r.error());
    const QString memberName = m_pendingChildMember;
    auto len = resolveLocation(fc.lengthFieldLocation);
    if (!len)
        return Utils::ResultError(len.error());
    if (auto r = checkUnsignedLength(len->value, len->isSigned, u"dynamic-length array"); !r)
        return Utils::ResultError(r.error());
    // The spec leaves the element count unbounded, so a hostile length (e.g.
    // UINT64_MAX, far larger than the bits actually encoded) could drive
    // billions of FieldValue allocations. A non-zero-width element is at least
    // one bit, so the count can never exceed the remaining content bits; the
    // zero-width case is bounded by kMaxZeroWidthArrayElements (and the
    // per-iteration guard below). Reject anything past both ceilings up front.
    const qint64 remainingBits = m_buf.contentEndBit() - m_buf.readBitOffset();
    const quint64 maxByContent = remainingBits > 0 ? quint64(remainingBits) : 0;
    const quint64 maxElements = std::max(maxByContent, kMaxZeroWidthArrayElements);
    if (len->value > maxElements)
        return Utils::ResultError(
            u"Dynamic-length array length %1 exceeds the maximum decodable element count %2"_s
                .arg(len->value)
                .arg(maxElements));
    auto av = std::make_shared<ArrayValue>();
    for (quint64 i = 0; i < len->value; ++i) {
        const qint64 posBefore = m_buf.readBitOffset();
        m_pendingChildMember = memberName;
        auto result = read(*fc.elementFieldClass);
        if (!result)
            return Utils::ResultError(result.error());
        av->elements.append(std::move(*result));
        // A zero-width element consumes no data, so the packet content boundary
        // can never terminate the loop; refuse an implausibly large count rather
        // than spin (near-)forever building empty values.
        if (m_buf.readBitOffset() == posBefore && len->value > kMaxZeroWidthArrayElements)
            return Utils::ResultError(
                u"Dynamic-length array of zero-width elements has an implausible length %1"_s.arg(
                    len->value));
    }
    m_pendingChildMember.clear();
    return FieldValue{av};
}

Utils::Result<FieldValue> FieldReader::readVariant(const VariantFC &fc)
{
    // The selected option's structure (if any) is labelled with the variant's
    // own member name for the field location procedure.
    const QString memberName = m_pendingChildMember;
    auto sel = resolveLocation(fc.selectorFieldLocation);
    if (!sel)
        return Utils::ResultError(sel.error());
    // Spec 5.3.23 / 6.4.20: a variant selector field MUST be an integer; a
    // boolean selector is not allowed.
    if (sel->isBoolean)
        return Utils::ResultError(u"Variant selector field must be an integer, not boolean"_s);
    const quint64 selector = sel->value;
    // A signed selector is compared in the signed domain so that ranges spanning
    // negative values match correctly (spec 5.3.23); otherwise compare unsigned.
    const bool selSigned = sel->isSigned;
    const qint64 sSelector = static_cast<qint64>(selector);
    const auto eqVal = [&](qint64 v) {
        return selSigned ? (sSelector == v) : (selector == static_cast<quint64>(v));
    };
    const auto inRange = [&](qint64 lo, qint64 hi) {
        return selSigned
                   ? (sSelector >= lo && sSelector <= hi)
                   : (selector >= static_cast<quint64>(lo) && selector <= static_cast<quint64>(hi));
    };
    for (int oi = 0; oi < fc.options.size(); ++oi) {
        const auto &opt = fc.options[oi];
        bool matched = false;
        for (auto sv : opt.selectorValues) {
            if (eqVal(sv)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            for (const auto &r : opt.selectorRanges) {
                if (inRange(r.lo, r.hi)) {
                    matched = true;
                    break;
                }
            }
        }
        if (matched) {
            m_pendingChildMember = memberName;
            auto result = read(*opt.fieldClass);
            if (!result)
                return Utils::ResultError(result.error());
            auto vv = std::make_shared<VariantValue>();
            vv->selectedOption = opt.name;
            vv->selectedIndex = oi;
            vv->value = std::make_unique<FieldValue>(std::move(*result));
            return FieldValue{vv};
        }
    }
    return Utils::ResultError(u"Variant selector %1 matched no option"_s.arg(selector));
}

Utils::Result<FieldValue> FieldReader::readOptional(const OptionalFC &fc)
{
    const QString memberName = m_pendingChildMember;
    auto sel = resolveLocation(fc.selectorFieldLocation);
    if (!sel)
        return Utils::ResultError(sel.error());
    const quint64 selector = sel->value;
    bool present;
    // Spec 6.4.19: a boolean selector makes the field present iff true; an
    // integer selector iff its value is in the range set. Prefer the located
    // field's runtime type; fall back to schema shape (no ranges => boolean)
    // when the type is unknown (e.g. resolved across roots).
    const bool booleanSelector = sel->isBoolean
                                 || (fc.selectorValues.isEmpty() && fc.selectorRanges.isEmpty());
    if (booleanSelector) {
        present = selector != 0;
    } else {
        // A signed selector is compared in the signed domain so that ranges
        // spanning negative values match correctly (spec 5.3.22).
        const bool selSigned = sel->isSigned;
        const qint64 sSelector = static_cast<qint64>(selector);
        const auto eqVal = [&](qint64 v) {
            return selSigned ? (sSelector == v) : (selector == static_cast<quint64>(v));
        };
        const auto inRange = [&](qint64 lo, qint64 hi) {
            return selSigned ? (sSelector >= lo && sSelector <= hi)
                             : (selector >= static_cast<quint64>(lo)
                                && selector <= static_cast<quint64>(hi));
        };
        present = false;
        for (qint64 v : fc.selectorValues) {
            if (eqVal(v)) {
                present = true;
                break;
            }
        }
        if (!present) {
            for (const auto &r : fc.selectorRanges) {
                if (inRange(r.lo, r.hi)) {
                    present = true;
                    break;
                }
            }
        }
    }
    if (!present)
        return std::monostate{};
    m_pendingChildMember = memberName;
    return read(*fc.fieldClass);
}

// Field location procedure (spec 6.4.2). Picks the starting structure, then
// hands off to walkLocation to follow the path to the located scalar field.
Utils::Result<FieldReader::LocatedValue> FieldReader::resolveLocation(const FieldLocation &loc)
{
    if (loc.path.isEmpty())
        return Utils::ResultError(u"Field location has an empty path"_s);

    if (loc.hasOrigin && loc.origin != m_currentOrigin) {
        // The location names another root, which has already been fully decoded:
        // defer to the external resolver over those completed root structures.
        // The resolver yields only the integer value, not the field's type.
        if (m_resolver) {
            if (auto v = m_resolver(loc))
                return LocatedValue{*v, false};
        }
        return Utils::ResultError(u"Field location: cannot resolve cross-root origin"_s);
    }

    if (m_scopes.isEmpty())
        return Utils::ResultError(u"Field location: no enclosing structure"_s);

    // An explicit origin naming this very root starts at the root scope (0);
    // a location without an origin starts at the immediate parent (top scope).
    const int start = loc.hasOrigin ? 0 : static_cast<int>(m_scopes.size()) - 1;
    return walkLocation(start, loc.path);
}

Utils::Result<FieldReader::LocatedValue> FieldReader::walkLocation(
    int startScope, const QList<std::optional<QString>> &path)
{
    // The current position is either an in-progress scope (scopeIdx >= 0) or a
    // value inside an already-decoded subtree (completed != nullptr).
    int scopeIdx = startScope;
    const FieldValue *completed = nullptr;

    for (int i = 0; i < path.size(); ++i) {
        const bool isLast = i == path.size() - 1;

        if (!path[i].has_value()) { // null: ascend to the immediate parent structure
            if (completed)
                return Utils::ResultError(u"Field location: cannot ascend out of a decoded field"_s);
            const int parent = m_scopes[scopeIdx].parent;
            if (parent < 0)
                return Utils::ResultError(u"Field location: no parent structure"_s);
            scopeIdx = parent;
            continue;
        }

        const QString &name = *path[i];
        const FieldValue *found = nullptr;

        if (!completed) {
            // In-progress scope: a prior sibling is already in the structure;
            // the member currently being decoded lives in a child scope.
            found = m_scopes[scopeIdx].sv->get(name);
            if (!found) {
                int child = -1;
                for (int s = scopeIdx + 1; s < m_scopes.size(); ++s) {
                    if (m_scopes[s].parent == scopeIdx && m_scopes[s].memberInParent == name) {
                        child = s;
                        break;
                    }
                }
                if (child < 0)
                    return Utils::ResultError(
                        u"Field location: no decoded member named '%1'"_s.arg(name));
                if (isLast)
                    return Utils::ResultError(
                        u"Field location: '%1' is a structure, not a scalar"_s.arg(name));
                scopeIdx = child; // descend into the in-progress child structure
                continue;
            }
        } else {
            if (!isStructure(*completed))
                return Utils::ResultError(u"Field location: '%1' has no members"_s.arg(name));
            found = asStructure(*completed).get(name);
            if (!found)
                return Utils::ResultError(u"Field location: no member named '%1'"_s.arg(name));
        }

        // Unwrap variants to the value of their selected option.
        const FieldValue *v = found;
        while (std::holds_alternative<std::shared_ptr<VariantValue>>(*v)) {
            const auto &vv = *std::get<std::shared_ptr<VariantValue>>(*v);
            if (!vv.value)
                return Utils::ResultError(u"Field location: empty variant"_s);
            v = vv.value.get();
        }

        if (isLast)
            return scalarValue(*v);

        if (!isStructure(*v))
            return Utils::ResultError(u"Field location: '%1' is not a structure"_s.arg(name));
        completed = v;
    }

    return Utils::ResultError(u"Field location did not resolve to a scalar field"_s);
}

Utils::Result<FieldReader::LocatedValue> FieldReader::scalarValue(const FieldValue &fv)
{
    if (std::holds_alternative<quint64>(fv))
        return LocatedValue{std::get<quint64>(fv), /*isBoolean=*/false, /*isSigned=*/false};
    if (std::holds_alternative<qint64>(fv))
        return LocatedValue{
            static_cast<quint64>(std::get<qint64>(fv)),
            /*isBoolean=*/false,
            /*isSigned=*/true};
    if (std::holds_alternative<bool>(fv))
        return LocatedValue{
            std::get<bool>(fv) ? quint64(1) : quint64(0),
            /*isBoolean=*/true,
            /*isSigned=*/false};
    return Utils::ResultError(u"Field location: located field is not an integer or boolean"_s);
}

Utils::Result<QString> FieldReader::readNullTerminatedString(StringEncoding enc)
{
    const int ul = codeUnitLength(enc);
    QByteArray content;
    while (true) {
        auto unit = m_buf.readBytes(ul);
        if (!unit)
            return Utils::ResultError(unit.error());
        if (!codeUnitHasSetBit(*unit, 0, ul))
            break; // terminating (all-zero) code unit
        content.append(*unit);
    }
    return decodeStringBytes(content, enc);
}

// Read `length` bytes for a static- or dynamic-length string, taking the
// content as the prefix up to (but excluding) the first all-zero code unit
// (spec 6.4.12 / 6.4.14). The full `length` bytes are always consumed.
Utils::Result<QString> FieldReader::readLengthPrefixedString(qsizetype length, StringEncoding enc)
{
    if (length < 0)
        return Utils::ResultError(u"Negative string length"_s);

    const int ul = codeUnitLength(enc);
    // Spec 6.4.12 / 6.4.14: the length MUST be a whole number of code units.
    if (ul > 1 && (length % ul) != 0)
        return Utils::ResultError(u"String length is not a whole number of code units"_s);

    auto bytesResult = m_buf.readBytes(length);
    if (!bytesResult)
        return Utils::ResultError(bytesResult.error());
    const QByteArray bytes = *bytesResult;

    int contentLen = 0;
    while (contentLen + ul <= bytes.size()) {
        if (!codeUnitHasSetBit(bytes, contentLen, ul))
            break;
        contentLen += ul;
    }
    return decodeStringBytes(bytes.left(contentLen), enc);
}

} // namespace CommonTraceFormat
