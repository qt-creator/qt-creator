// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>

#include <cstdint>
#include <span>

namespace CommonTraceFormat {

// Encode unsigned value as ULEB128, appending bytes to dst.
inline void encodeULeb128(quint64 value, QByteArray &dst)
{
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0)
            byte |= 0x80;
        dst.append(static_cast<char>(byte));
    } while (value != 0);
}

// Encode signed value as SLEB128, appending bytes to dst.
inline void encodeSLeb128(qint64 value, QByteArray &dst)
{
    bool more = true;
    while (more) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        // Arithmetic shift: check sign extension
        if ((value == 0 && !(byte & 0x40)) || (value == -1 && (byte & 0x40)))
            more = false;
        else
            byte |= 0x80;
        dst.append(static_cast<char>(byte));
    }
}

struct LebResult
{
    quint64 value;
    int bytesConsumed; // -1 on error
};

struct SLebResult
{
    qint64 value;
    int bytesConsumed; // -1 on error
};

// Decode ULEB128 from src starting at offset. Returns {value, bytes} or {0,-1} on error.
inline LebResult decodeULeb128(std::span<const char> src, int offset = 0)
{
    quint64 result = 0;
    int shift = 0;
    int i = offset;
    while (i < static_cast<int>(src.size())) {
        if (shift >= 64)
            return {0, -1};
        auto byte = static_cast<uint8_t>(src[i++]);
        result |= static_cast<quint64>(byte & 0x7F) << shift;
        shift += 7;
        if (!(byte & 0x80))
            return {result, i - offset};
    }
    return {0, -1}; // truncated
}

// Decode SLEB128 from src starting at offset.
inline SLebResult decodeSLeb128(std::span<const char> src, int offset = 0)
{
    qint64 result = 0;
    int shift = 0;
    int i = offset;
    uint8_t byte = 0;
    while (i < static_cast<int>(src.size())) {
        if (shift >= 64)
            return {0, -1};
        byte = static_cast<uint8_t>(src[i++]);
        result |= static_cast<qint64>(byte & 0x7F) << shift;
        shift += 7;
        if (!(byte & 0x80))
            break;
    }
    if (byte & 0x80)
        return {0, -1}; // truncated
    if (shift < 64 && (byte & 0x40))
        result |= static_cast<qint64>(~0ULL << shift); // sign extend (unsigned shift avoids UB)
    return {result, i - offset};
}

} // namespace CommonTraceFormat
