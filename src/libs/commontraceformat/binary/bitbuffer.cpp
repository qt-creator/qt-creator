// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bitbuffer.h"

#include <QIODevice>

#include <cstring>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

BitBuffer::BitBuffer(QIODevice *dev)
    : m_dev(dev)
    , m_writeMode(true)
{}

BitBuffer::BitBuffer(QByteArray data)
    : m_buf(std::move(data))
    , m_writeMode(false)
{}

// ──────────────────────────────────────────────
// Internal bit manipulation helpers
// ──────────────────────────────────────────────

// Spec 6.4.3: decode/encode an L-bit fixed-length bit array field as a single
// pass, handling every byte-order/bit-order combination directly (no separate
// byte-reversal step). Two independent directions are at play:
//
//   * The reading direction within a data-stream byte — which physical bit
//     BIT_I a decode position X selects — depends on BYTE_ORDER:
//       big-endian    → BIT_I = 7 − (X mod 8)   (MSb of the byte first)
//       little-endian → BIT_I = X mod 8         (LSb of the byte first)
//
//   * The order in which decoded bits fill the result (element index DI)
//     depends on BIT_ORDER:
//       first-to-last (LeastSignificantFirst) → DI = I
//       last-to-first (MostSignificantFirst)  → DI = L − 1 − I
//
// Element 0 of the bit array is the least significant bit of the integer
// interpretation (spec 6.4.6), so DI doubles as the result bit position.

static quint64 readField(
    const uint8_t *buf, qint64 bitOffset, qint64 bitCount, ByteOrder byteOrder, BitOrder bitOrder)
{
    quint64 raw = 0;
    for (qint64 i = 0; i < bitCount; ++i) {
        const qint64 x = bitOffset + i;
        const qint64 byteIdx = x / 8;
        const int bitInByte = (byteOrder == ByteOrder::BigEndian) ? 7 - (x % 8) : (x % 8);
        const quint64 bit = (buf[byteIdx] >> bitInByte) & 1;
        const qint64 di = (bitOrder == BitOrder::LeastSignificantFirst) ? i : (bitCount - 1 - i);
        raw |= bit << di;
    }
    return raw;
}

static void storeField(
    uint8_t *buf,
    qint64 bitOffset,
    qint64 bitCount,
    quint64 value,
    ByteOrder byteOrder,
    BitOrder bitOrder)
{
    for (qint64 i = 0; i < bitCount; ++i) {
        const qint64 di = (bitOrder == BitOrder::LeastSignificantFirst) ? i : (bitCount - 1 - i);
        const uint8_t bit = (value >> di) & 1;
        const qint64 x = bitOffset + i;
        const qint64 byteIdx = x / 8;
        const int bitInByte = (byteOrder == ByteOrder::BigEndian) ? 7 - (x % 8) : (x % 8);
        buf[byteIdx] = (buf[byteIdx] & ~(1u << bitInByte)) | (bit << bitInByte);
    }
}

// ──────────────────────────────────────────────
// Write mode
// ──────────────────────────────────────────────

void BitBuffer::writeBits(quint64 value, int bitCount, ByteOrder byteOrder, BitOrder bitOrder)
{
    if (bitCount <= 0 || bitCount > 64)
        return;

    // Ensure buffer is large enough.
    qint64 lastBit = m_writeBitOffset + bitCount - 1;
    qint64 lastByte = lastBit / 8;
    while (m_buf.size() <= lastByte)
        m_buf.append('\0');

    auto *buf = reinterpret_cast<uint8_t *>(m_buf.data());
    storeField(buf, m_writeBitOffset, bitCount, value, byteOrder, bitOrder);
    m_writeBitOffset += bitCount;
}

void BitBuffer::writeBytes(const QByteArray &bytes)
{
    writeBytes(bytes.constData(), bytes.size());
}

void BitBuffer::writeBytes(const char *data, qsizetype len)
{
    if (len == 0)
        return;
    Q_ASSERT(m_writeBitOffset % 8 == 0); // must be byte-aligned
    qsizetype bytePos = m_writeBitOffset / 8;
    m_buf.resize(bytePos + len);
    memcpy(m_buf.data() + bytePos, data, len);
    m_writeBitOffset += len * 8;
}

void BitBuffer::alignWriteTo(int alignBits)
{
    if (alignBits <= 1)
        return;
    qint64 rem = m_writeBitOffset % alignBits;
    if (rem == 0)
        return;
    // The padding can exceed 64 bits (e.g. a 128-bit-aligned field), but
    // writeBits rejects counts > 64, so emit the zero padding in <=64-bit
    // chunks to guarantee the cursor actually reaches the alignment boundary.
    qint64 pad = alignBits - rem;
    while (pad > 0) {
        const int chunk = static_cast<int>(qMin<qint64>(pad, 64));
        writeBits(0, chunk, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
        pad -= chunk;
    }
}

void BitBuffer::patchBits(
    qint64 bitOffset, quint64 value, int bitCount, ByteOrder byteOrder, BitOrder bitOrder)
{
    if (bitCount <= 0 || bitCount > 64)
        return;

    // Back-patching must stay within the already-written buffer; an offset/length
    // past the end (e.g. a stale bookmark) would otherwise write out of bounds and
    // corrupt the heap. Such a patch is a no-op. Compare with subtraction so a
    // hostile/corrupt bitOffset can't overflow the signed addition before the test.
    if (bitOffset < 0 || bitCount > m_buf.size() * 8LL - bitOffset)
        return;

    auto *buf = reinterpret_cast<uint8_t *>(m_buf.data());
    storeField(buf, bitOffset, bitCount, value, byteOrder, bitOrder);
}

Utils::Result<> BitBuffer::flush()
{
    if (!m_dev)
        return Utils::ResultError(u"BitBuffer: no device to flush to"_s);
    if (m_buf.isEmpty())
        return Utils::ResultOk;
    qint64 written = m_dev->write(m_buf);
    if (written != m_buf.size())
        return Utils::ResultError(u"BitBuffer: short write while flushing"_s);
    m_buf.clear();
    m_writeBitOffset = 0;
    return Utils::ResultOk;
}

// ──────────────────────────────────────────────
// Read mode
// ──────────────────────────────────────────────

Utils::Result<quint64> BitBuffer::readBits(int bitCount, ByteOrder byteOrder, BitOrder bitOrder)
{
    if (bitCount <= 0 || bitCount > 64)
        return Utils::ResultError(u"BitBuffer: invalid bit count"_s);
    // Spec 6.4.3: a fixed-length bit array field must not end past the packet
    // content boundary (strict ">", so a field may end exactly at it), nor past
    // the physical buffer.
    if (m_readBitOffset + bitCount > m_contentEndBit
        || m_readBitOffset + bitCount > m_buf.size() * 8LL) {
        return Utils::ResultError(u"BitBuffer: read past packet content"_s);
    }
    // Spec 6.4.3: within a single byte, two fixed-length bit array fields must
    // share the same byte order.
    if (m_readBitOffset % 8 != 0 && m_hasLastByteOrder && byteOrder != m_lastByteOrder)
        return Utils::ResultError(u"BitBuffer: conflicting byte orders within a byte"_s);
    m_hasLastByteOrder = true;
    m_lastByteOrder = byteOrder;

    const auto *buf = reinterpret_cast<const uint8_t *>(m_buf.constData());
    const quint64 raw = readField(buf, m_readBitOffset, bitCount, byteOrder, bitOrder);
    m_readBitOffset += bitCount;
    return raw;
}

Utils::Result<quint8> BitBuffer::readU8()
{
    auto v = readBits(8, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
    if (!v)
        return Utils::ResultError(v.error());
    return static_cast<quint8>(*v);
}

Utils::Result<QByteArray> BitBuffer::readBytes(qsizetype byteCount)
{
    if (byteCount <= 0)
        return QByteArray{};
    Q_ASSERT(m_readBitOffset % 8 == 0);
    qsizetype bytePos = m_readBitOffset / 8;
    // Compare with subtraction so a hostile byteCount can't overflow the signed
    // additions (byteCount * 8, bytePos + byteCount) before the bound test.
    const qint64 contentBytesLeft = (m_contentEndBit - m_readBitOffset) / 8;
    if (byteCount > m_buf.size() - bytePos || byteCount > contentBytesLeft) {
        return Utils::ResultError(u"BitBuffer: read past packet content"_s);
    }
    QByteArray result = m_buf.mid(bytePos, byteCount);
    m_readBitOffset += byteCount * 8;
    return result;
}

Utils::Result<> BitBuffer::alignReadTo(int alignBits)
{
    if (alignBits <= 1)
        return Utils::ResultOk;
    qint64 rem = m_readBitOffset % alignBits;
    if (rem != 0)
        m_readBitOffset += alignBits - rem;
    // Spec 6.4.1: after aligning, a position past the packet content boundary
    // (strict ">", so landing exactly at it is allowed) is an error.
    if (m_readBitOffset > m_contentEndBit)
        return Utils::ResultError(u"BitBuffer: alignment past packet content"_s);
    return Utils::ResultOk;
}

bool BitBuffer::atEnd() const
{
    return m_readBitOffset >= m_buf.size() * 8LL;
}

} // namespace CommonTraceFormat
