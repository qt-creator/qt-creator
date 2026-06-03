// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include <utils/result.h>

#include <QByteArray>

#include <cstdint>
#include <limits>

class QIODevice;

namespace CommonTraceFormat {

enum class ByteOrder { BigEndian, LittleEndian };
enum class BitOrder { MostSignificantFirst, LeastSignificantFirst };

// Bit-level read/write buffer over a QByteArray.
//
// Write mode: grow internal buffer, flush to QIODevice.
// Read mode:  wrap a QByteArray, track read position.
//
// Alignment and bit positions are counted from bit 0 of the buffer
// (= bit 0 of the first byte), which corresponds to packet start in CTF2.
class CMNTRCEFMT_EXPORT BitBuffer
{
public:
    // Write-mode constructor. Call flush() to push accumulated data to dev.
    explicit BitBuffer(QIODevice *dev);

    // Read-mode constructor. Takes ownership of a copy of data.
    explicit BitBuffer(QByteArray data);

    // --- Write API ---

    // Write low `bitCount` bits of `value`.
    // byteOrder: which end of a multi-byte integer is stored first in memory.
    // bitOrder:  which end of each byte the first bit occupies.
    void writeBits(quint64 value, int bitCount, ByteOrder byteOrder, BitOrder bitOrder);

    // Convenience: write all 64 bits, LE byte order, LSB first.
    void writeU64Le(quint64 value)
    {
        writeBits(value, 64, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
    }
    void writeU32Le(quint32 value)
    {
        writeBits(value, 32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
    }
    void writeU8(quint8 value)
    {
        writeBits(value, 8, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
    }

    // Append raw bytes (alignment must already be on a byte boundary).
    void writeBytes(const QByteArray &bytes);
    void writeBytes(const char *data, qsizetype len);

    // Advance write cursor to next multiple of alignBits (must be power of 2).
    void alignWriteTo(int alignBits);

    // Overwrite bits at a previously recorded bit offset (for back-patching lengths).
    void patchBits(
        qint64 bitOffset, quint64 value, int bitCount, ByteOrder byteOrder, BitOrder bitOrder);

    // Flush accumulated bytes to the underlying QIODevice. Resets buffer.
    Utils::Result<> flush();

    // --- Read API ---

    // Each read returns an error (rather than a zero-filled value) when it would
    // cross the packet content boundary or the physical buffer (spec 6.4.*).
    Utils::Result<quint64> readBits(int bitCount, ByteOrder byteOrder, BitOrder bitOrder);
    Utils::Result<quint64> readU64Le()
    {
        return readBits(64, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
    }
    Utils::Result<quint64> readU32Le()
    {
        return readBits(32, ByteOrder::LittleEndian, BitOrder::LeastSignificantFirst);
    }
    Utils::Result<quint8> readU8();

    // Read raw bytes (cursor must be byte-aligned).
    Utils::Result<QByteArray> readBytes(qsizetype byteCount);

    // Advance read cursor to next multiple of alignBits. Errors if the new
    // position lands past the packet content boundary (spec 6.4.1).
    Utils::Result<> alignReadTo(int alignBits);

    // --- Common ---

    qint64 writeBitOffset() const { return m_writeBitOffset; }
    qint64 readBitOffset() const { return m_readBitOffset; }

    bool atEnd() const;

    // Allow PacketReader to save/restore read position.
    void setReadBitOffset(qint64 offset) { m_readBitOffset = offset; }

    // Limit reads to the packet content area (bits). A field that would end
    // past this boundary sets the error flag (spec 6.4.*: report an error if
    // X + len > PKT_CONTENT_LEN). Defaults to "no limit".
    void setContentEndBit(qint64 bit) { m_contentEndBit = bit; }
    qint64 contentEndBit() const { return m_contentEndBit; }

    // Reset the LAST_BYTE_ORDER tracking (spec 6.4.3). Call when starting a new
    // packet so cross-packet contiguity isn't enforced.
    void resetLastByteOrder() { m_hasLastByteOrder = false; }

private:
    // Shared helpers
    static quint64 extractBits(quint64 value, int bitCount, ByteOrder byteOrder, BitOrder bitOrder);
    static quint64 assembleBits(
        const uint8_t *bytes, int bitOffset, int bitCount, ByteOrder byteOrder, BitOrder bitOrder);
    static void storeBits(
        uint8_t *bytes,
        int bitOffset,
        int bitCount,
        quint64 value,
        ByteOrder byteOrder,
        BitOrder bitOrder);

    QIODevice *m_dev = nullptr; // write mode
    QByteArray m_buf;           // write: accumulation buffer; read: source data
    qint64 m_writeBitOffset = 0;
    qint64 m_readBitOffset = 0;
    qint64 m_contentEndBit = std::numeric_limits<qint64>::max();
    bool m_writeMode;
    // LAST_BYTE_ORDER tracking (spec 6.4.3): byte order of the last decoded
    // fixed-length bit array field, used to forbid mixing byte orders within a
    // single byte.
    bool m_hasLastByteOrder = false;
    ByteOrder m_lastByteOrder = ByteOrder::LittleEndian;
};

} // namespace CommonTraceFormat
